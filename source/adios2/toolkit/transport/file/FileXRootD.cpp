/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileXRootD.cpp wrapper of XRootD client library functions for remote file I/O
 *
 *  Created on: Sep 27, 2023
 *      Author: Norbert Podhorszki, pnorbert@ornl.gov
 */
#include "FileXRootD.h"
#include "adios2/helper/adiosLog.h"

#include <cstdio>  // remove
#include <cstring> // strerror
#include <errno.h> // errno
#include <fstream>

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
/// \endcond

namespace adios2
{
namespace transport
{

FileXRootD::FileXRootD(helper::Comm const &comm) : Transport("File", "XRootD", comm) {}

FileXRootD::~FileXRootD()
{
    if (m_IsOpen)
    {
        Close();
    }
}

void FileXRootD::WaitForOpen()
{
    if (m_IsOpening)
    {
        if (m_OpenFuture.valid())
        {
            m_FileStatus = m_OpenFuture.get();
        }
        m_IsOpening = false;
        CheckFile("couldn't open file " + m_Name + ", in call to XRootD open");
        m_IsOpen = true;
    }
}

XrdCl::OpenFlags::Flags FileXRootD::ModeToOpenFlags(const Mode mode)
{

    switch (mode)
    {
    case Mode::Write:
        return XrdCl::OpenFlags::Flags::Write | XrdCl::OpenFlags::Flags::Delete |
               XrdCl::OpenFlags::Flags::Force;
        break;

    case Mode::Append:
        return XrdCl::OpenFlags::Flags::Write | XrdCl::OpenFlags::Flags::Force;
        break;

    case Mode::Read:
    case Mode::ReadRandomAccess:
        return XrdCl::OpenFlags::Flags::Read | XrdCl::OpenFlags::Flags::Force;
        break;

    default:
        break;
    }
    return XrdCl::OpenFlags::Flags::None;
}

void FileXRootD::Open(const std::string &name, const Mode openMode, const bool async,
                      const bool directio)
{
    auto lf_AsyncOpenWrite = [&](const std::string &name,
                                 XrdCl::OpenFlags::Flags flags) -> XrdCl::XRootDStatus {
        ProfilerStart("open");
        XrdCl::Access::Mode mode = XrdCl::Access::None;
        uint16_t timeout = 0;
        XrdCl::XRootDStatus status;
        status = m_File.Open(name, flags, mode, timeout);
        ProfilerStop("open");
        std::cout << "FileXRootD::Open::AsyncOpenWrite " << name << " status = " << status.ToStr()
                  << std::endl;
        return status;
    };

    m_Name = name;
    CheckName();
    m_OpenMode = openMode;
    XrdCl::OpenFlags::Flags flags = ModeToOpenFlags(m_OpenMode);
    switch (m_OpenMode)
    {

    case Mode::Write:
        if (async)
        {
            m_IsOpening = true;
            m_OpenFuture = std::async(std::launch::async, lf_AsyncOpenWrite, name, flags);
        }
        else
        {
            ProfilerStart("open");
            m_FileStatus = m_File.Open(name, flags, XrdCl::Access::None, (uint16_t)0U);
            std::cout << "FileXRootD::Open for Write" << name
                      << " status = " << m_FileStatus.ToStr() << std::endl;
            ProfilerStop("open");
        }
        break;

    case Mode::Append:
        ProfilerStart("open");
        m_FileStatus = m_File.Open(name, flags, XrdCl::Access::None, (uint16_t)0U);
        std::cout << "FileXRootD::Open for Append" << name << " status = " << m_FileStatus.ToStr()
                  << std::endl;
        GetSize();
        Seek(m_Size); // append at end of file
        ProfilerStop("open");
        break;

    case Mode::Read:
        ProfilerStart("open");
        m_FileStatus = m_File.Open(name, flags, XrdCl::Access::None, (uint16_t)0U);
        std::cout << "FileXRootD::Open for Read" << name << " status = " << m_FileStatus.ToStr()
                  << std::endl;
        GetSize();
        ProfilerStop("open");
        break;

    default:
        CheckFile("unknown open mode for file " + m_Name + ", in call to XRootD open");
    }

    if (!m_IsOpening)
    {
        CheckFile("couldn't open file " + m_Name + ", in call to XRootD open");
        m_IsOpen = true;
    }
}

void FileXRootD::OpenChain(const std::string &name, Mode openMode, const helper::Comm &chainComm,
                           const bool async, const bool directio)
{
    // only when process is a single writer, can create the file
    // asynchronously, otherwise other processes are waiting on it
    bool doAsync = (async && chainComm.Size() == 1);

    int token = 0;
    if (chainComm.Rank() > 0)
    {
        chainComm.Recv(&token, 1, chainComm.Rank() - 1, 0, "Chain token in FileXRootD::OpenChain");
    }

    Open(name, openMode, doAsync, directio);

    if (chainComm.Rank() < chainComm.Size() - 1)
    {
        chainComm.Isend(&token, 1, chainComm.Rank() + 1, 0,
                        "Sending Chain token in FileXRootD::OpenChain");
    }
}

void FileXRootD::Write(const char *buffer, size_t size, size_t start)
{
    auto lf_Write = [&](const char *buffer, size_t size, size_t start) {
        ProfilerStart("write");
        ProfilerWriteBytes(size);
        uint32_t size32 = static_cast<uint32_t>(size);
        XrdCl::XRootDStatus status;
        status = m_File.Write(start, size32, buffer, (uint16_t)0U);
        std::cout << "FileXRootD::Write " << m_Name << " start = " << start << " size = " << size32
                  << " status = " << status.ToStr() << std::endl;
        ProfilerStop("write");

        if (status.IsError())
        {
            helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileXRootD", "Write",
                                                  "couldn't write to file " + m_Name + " " +
                                                      status.ToStr());
        }
    };

    if (size == 0)
    {
        return;
    }

    WaitForOpen();
    if (start == MaxSizeT)
    {
        // use current position to write
        start = m_SeekPos;
    }
    else
    {
        Seek(start);
    }

    if (size > DefaultMaxFileBatchSize)
    {
        const size_t batches = size / DefaultMaxFileBatchSize;
        const size_t remainder = size % DefaultMaxFileBatchSize;

        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            lf_Write(&buffer[position], DefaultMaxFileBatchSize, start + position);
            position += DefaultMaxFileBatchSize;
        }
        lf_Write(&buffer[position], remainder, start + position);
    }
    else
    {
        lf_Write(buffer, size, start);
    }

    m_SeekPos += size;
    if (m_SeekPos > m_Size)
    {
        m_Size = m_SeekPos;
    }
}

void FileXRootD::Read(char *buffer, size_t size, size_t start)
{
    auto lf_Read = [&](char *buffer, size_t size) {
        ProfilerStart("read");
        errno = 0;
        XrdCl::XRootDStatus status;
        uint32_t size32 = static_cast<uint32_t>(size);
        uint32_t bytesRead;
        status = m_File.Read(start, size32, buffer, bytesRead, (uint16_t)0U);
        std::cout << "FileXRootD::Read " << m_Name << " start = " << start << " size = " << size32
                  << " status = " << status.ToStr() << " bytes read = " << bytesRead << std::endl;
        ProfilerStop("read");

        if (status.IsError())
        {

            helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileXRootD", "Read",
                                                  "couldn't read from file " + m_Name + " " +
                                                      status.ToStr());
        }
    };

    WaitForOpen();
    std::memset(buffer, 1, size);

    if (size > DefaultMaxFileBatchSize)
    {
        const size_t batches = size / DefaultMaxFileBatchSize;
        const size_t remainder = size % DefaultMaxFileBatchSize;

        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            lf_Read(&buffer[position], DefaultMaxFileBatchSize);
            position += DefaultMaxFileBatchSize;
        }
        lf_Read(&buffer[position], remainder);
    }
    else
    {
        lf_Read(buffer, size);
    }
}

size_t FileXRootD::GetSize()
{
    WaitForOpen();
    XrdCl::StatInfo *fileStat = nullptr;
    XrdCl::XRootDStatus status;
    status = m_File.Stat(true, fileStat, (uint16_t)0U);

    if (status.IsError())
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileXRootD", "GetSize",
                                              "couldn't get size of file " + m_Name +
                                                  status.ToStr());
    }
    if (fileStat)
    {
        m_Size = static_cast<size_t>(fileStat->GetSize());
        std::cout << "FileXRootD::GetSize " << m_Name << " size = " << m_Size << std::endl;
        delete fileStat;
    }
    return m_Size;
}

void FileXRootD::Flush()
{
    /* Turn this off now because BP3/BP4 calls manager Flush and this syncing
     * slows down IO performance */
#if 0
    m_FileStatus = m_File.Sync();
#endif
}

void FileXRootD::Close()
{
    WaitForOpen();
    ProfilerStart("close");
    if (m_OpenMode == Mode::Write || m_OpenMode == Mode::Append)
    {
        m_FileStatus = m_File.Sync();
        if (m_FileStatus.IsError())
        {
            helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileXRootD", "Close",
                                                  "couldn't sync before closing file" + m_Name +
                                                      ": " + m_FileStatus.ToStr());
        }
    }
    m_FileStatus = m_File.Close();
    m_IsOpen = false;
    ProfilerStop("close");

    if (m_FileStatus.IsError())
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileXRootD", "Close",
                                              "couldn't close file " + m_Name + ": " +
                                                  m_FileStatus.ToStr());
    }

    m_IsOpen = false;
}

void FileXRootD::Delete()
{
    WaitForOpen();
    if (m_IsOpen)
    {
        Close();
    }
    helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileXRootD", "Delete",
                                          "does not support deleting " + m_Name);
}

void FileXRootD::CheckFile(const std::string hint) const
{
    if (m_FileStatus.IsError())
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileXRootD", "CheckFile",
                                              hint + m_FileStatus.ToStr());
    }
}

void FileXRootD::SeekToEnd() { m_SeekPos = m_Size; }

void FileXRootD::SeekToBegin() { m_SeekPos = 0; }

void FileXRootD::Seek(const size_t start)
{
    if (start != MaxSizeT)
    {
        m_SeekPos = start; // this can point beyond current m_Size
    }
    else
    {
        SeekToEnd();
    }
}

void FileXRootD::Truncate(const size_t length)
{
    WaitForOpen();
    m_FileStatus = m_File.Truncate(length);
    if (m_FileStatus.IsError())
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileXRootD", "Truncate",
                                              "couldn't truncate to " + std::to_string(length) +
                                                  " bytes of file " + m_Name + " " +
                                                  m_FileStatus.ToStr());
    }
    GetSize();
    Seek(m_Size);
}

void FileXRootD::MkDir(const std::string &fileName) {}

} // end namespace transport
} // end namespace adios2
