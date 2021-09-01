/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Transport.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: wfg
 */

#include "Transport.h"

#include "adios2/helper/adiosFunctions.h" //CreateDirectory

namespace adios2
{

Transport::Transport(const std::string type, const std::string library,
                     helper::Comm const &comm)
: m_Type(type), m_Library(library), m_Comm(comm)
{
}

void Transport::IWrite(const char *buffer, size_t size, Status &status,
                       size_t start)
{
    throw std::invalid_argument("ERROR: this class doesn't implement IWrite\n");
}

void Transport::WriteV(const core::iovec *iov, const int iovcnt, size_t start)
{
    if (iovcnt > 0)
    {
        std::cout << "WriteV 0:\tlength = " << iov[0].iov_len
                  << " offset = " << start << "\n";
        Write(static_cast<const char *>(iov[0].iov_base), iov[0].iov_len,
              start);
        for (int c = 1; c < iovcnt; ++c)
        {
            std::cout << "       " << c << ":\tlength = " << iov[c].iov_len
                      << "\n";
            Write(static_cast<const char *>(iov[c].iov_base), iov[c].iov_len);
        }
    }
}

void Transport::IRead(char *buffer, size_t size, Status &status, size_t start)
{
    throw std::invalid_argument("ERROR: this class doesn't implement IRead\n");
}

void Transport::InitProfiler(const Mode openMode, const TimeUnit timeUnit)
{
    m_Profiler.m_IsActive = true;

    m_Profiler.m_Timers.emplace(std::make_pair(
        "open", profiling::Timer("open", TimeUnit::Microseconds)));

    if (openMode == Mode::Write)
    {
        m_Profiler.m_Timers.emplace("write",
                                    profiling::Timer("write", timeUnit));

        m_Profiler.m_Bytes.emplace("write", 0);
    }
    else if (openMode == Mode::Append)
    {
        /*
        m_Profiler.Timers.emplace(
            "append", profiling::Timer("append", timeUnit));
        m_Profiler.Bytes.emplace("append", 0);
        */
        m_Profiler.m_Timers.emplace("write",
                                    profiling::Timer("write", timeUnit));

        m_Profiler.m_Bytes.emplace("write", 0);

        m_Profiler.m_Timers.emplace("read", profiling::Timer("read", timeUnit));

        m_Profiler.m_Bytes.emplace("read", 0);
    }
    else if (openMode == Mode::Read)
    {
        m_Profiler.m_Timers.emplace("read", profiling::Timer("read", timeUnit));
        m_Profiler.m_Bytes.emplace("read", 0);
    }

    m_Profiler.m_Timers.emplace(
        "close", profiling::Timer("close", TimeUnit::Microseconds));
}

void Transport::OpenChain(const std::string &name, const Mode openMode,
                          const helper::Comm &chainComm, const bool async)
{
    std::invalid_argument("ERROR: " + m_Name + " transport type " + m_Type +
                          " using library " + m_Library +
                          " doesn't implement the OpenChain function\n");
}

void Transport::SetParameters(const Params &parameters) {}

void Transport::SetBuffer(char * /*buffer*/, size_t /*size*/)
{
    std::invalid_argument("ERROR: " + m_Name + " transport type " + m_Type +
                          " using library " + m_Library +
                          " doesn't implement the SetBuffer function\n");
}

void Transport::Flush()
{
    std::invalid_argument("ERROR: " + m_Name + " transport type " + m_Type +
                          " using library " + m_Library +
                          " doesn't implement the Flush function\n");
}

size_t Transport::GetSize() { return 0; }

void Transport::ProfilerStart(const std::string process) noexcept
{
    if (m_Profiler.m_IsActive)
    {
        m_Profiler.m_Timers.at(process).Resume();
    }
}

void Transport::ProfilerStop(const std::string process) noexcept
{
    if (m_Profiler.m_IsActive)
    {
        m_Profiler.m_Timers.at(process).Pause();
    }
}

void Transport::CheckName() const
{
    if (m_Name.empty())
    {
        throw std::invalid_argument("ERROR: name can't be empty for " +
                                    m_Library + " transport \n");
    }
}

void Transport::MkDir(const std::string &fileName)
{
    const auto lastPathSeparator(fileName.find_last_of(PathSeparator));
    if (lastPathSeparator == std::string::npos)
    {
        return;
    }

    const std::string path(fileName.substr(0, lastPathSeparator));
    helper::CreateDirectory(path);
}

} // end namespace adios2
