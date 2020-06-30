/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * AsyncTaskManager.cpp
 *
 *  Created on: April 1, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#include "AsyncTaskManager.h"

#include <chrono>
#include <cstdio>
#include <cstring> // std::memcpy
#include <thread>  // std::this_thread::sleep_for

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
/// \endcond

namespace adios2
{
namespace transportman
{

AsyncTaskOperation::AsyncTaskOperation(AsyncOp op,
                                       const std::string &fromFileName,
                                       const std::string &toFileName,
                                       size_t countBytes, size_t fromOffset,
                                       size_t toOffset, const void *data)
: op(op), fromFileName(fromFileName), toFileName(toFileName),
  countBytes(countBytes), fromOffset(fromOffset), toOffset(toOffset)
{
    if (data)
    {
        dataToWrite.resize(countBytes);
        std::memcpy(dataToWrite.data(), data, countBytes);
    };
}

AsyncTaskManager::AsyncTaskManager(helper::Comm &comm) : m_Comm(comm) {}

void AsyncTaskManager::AddOperation(AsyncTaskOperation &operation)
{
    std::lock_guard<std::mutex> lockGuard(operationsMutex);
    operations.push(operation);
}

void AsyncTaskManager::AddOperation(AsyncOp op, const std::string &fromFileName,
                                    const std::string &toFileName,
                                    size_t fromOffset, size_t toOffset,
                                    size_t countBytes, const void *data)
{
    AsyncTaskOperation operation(op, fromFileName, toFileName, countBytes,
                                 fromOffset, toOffset, data);
    std::lock_guard<std::mutex> lockGuard(operationsMutex);
    operations.push(operation);
}

void AsyncTaskManager::AddOperationSeekEnd(const std::string &toFileName)
{
    std::string emptyStr;
    AddOperation(AsyncOp::SeekEnd, emptyStr, toFileName, 0, 0, 0);
}
void AsyncTaskManager::AddOperationCopyAt(const std::string &fromFileName,
                                          const std::string &toFileName,
                                          size_t fromOffset, size_t toOffset,
                                          size_t countBytes)
{
    AddOperation(AsyncOp::CopyAt, fromFileName, toFileName, fromOffset,
                 toOffset, countBytes);
}
void AsyncTaskManager::AddOperationCopy(const std::string &fromFileName,
                                        const std::string &toFileName,
                                        size_t countBytes)
{
    AddOperation(AsyncOp::Copy, fromFileName, toFileName, 0, 0, countBytes);
}

void AsyncTaskManager::AddOperationWriteAt(const std::string &toFileName,
                                           size_t toOffset, size_t countBytes,
                                           const void *data)
{
    std::string emptyStr;
    AddOperation(AsyncOp::WriteAt, emptyStr, toFileName, 0, toOffset,
                 countBytes, data);
}

void AsyncTaskManager::AddOperationWrite(const std::string &toFileName,
                                         size_t countBytes, const void *data)
{
    std::string emptyStr;
    AddOperation(AsyncOp::Write, emptyStr, toFileName, 0, 0, countBytes, data);
}

void AsyncTaskManager::AddOperationOpen(const std::string &toFileName,
                                        Mode mode)
{
    std::string emptyStr;
    if (mode == Mode::Write)
    {
        AddOperation(AsyncOp::Create, emptyStr, toFileName, 0, 0, 0);
    }
    else if (mode == Mode::Append)
    {
        AddOperation(AsyncOp::Open, emptyStr, toFileName, 0, 0, 0);
    }
    else
    {
        throw std::runtime_error(
            "ADIOS Coding ERROR: AsyncTaskManager::AddOperationOpen() only "
            "supports "
            "Write and Append modes\n");
    }
}

void AsyncTaskManager::AddOperationDelete(const std::string &toFileName)
{
    std::string emptyStr;
    AddOperation(AsyncOp::Delete, emptyStr, toFileName, 0, 0, 0);
}

InputFile AsyncTaskManager::GetFileForRead(const std::string &path)
{
    auto it = m_InputFileMap.find(path);
    if (it != m_InputFileMap.end())
    {
        return it->second;
    }
    else
    {
        InputFile f = std::make_shared<std::ifstream>();
        m_InputFileMap.emplace(path, f);
        Open(f, path);
        return f;
    }
}

OutputFile AsyncTaskManager::GetFileForWrite(const std::string &path,
                                             bool append)
{
    auto it = m_OutputFileMap.find(path);
    if (it != m_OutputFileMap.end())
    {
        return it->second;
    }
    else
    {
        OutputFile f = std::make_shared<std::ofstream>();
        m_OutputFileMap.emplace(path, f);
        Open(f, path, append);
        return f;
    }
}

void AsyncTaskManager::Open(InputFile &f, const std::string &path)
{

    f->rdbuf()->pubsetbuf(0, 0);
    f->open(path, std::ios::in | std::ios::binary);
}

void AsyncTaskManager::Open(OutputFile &f, const std::string &path, bool append)
{

    if (append)
    {
        f->rdbuf()->pubsetbuf(0, 0);
        f->open(path, std::ios::out | std::ios::app | std::ios::binary);
    }
    else
    {
        f->rdbuf()->pubsetbuf(0, 0);
        f->open(path, std::ios::out | std::ios::trunc | std::ios::binary);
    }
}

void AsyncTaskManager::Close(InputFile &f) { f->close(); }
void AsyncTaskManager::Close(OutputFile &f) { f->close(); }

bool AsyncTaskManager::Good(InputFile &f) { return (f->good()); }
bool AsyncTaskManager::Good(OutputFile &f) { return (f->good()); }

void AsyncTaskManager::CloseAll()
{
    for (auto it = m_OutputFileMap.begin(); it != m_OutputFileMap.end(); ++it)
    {
        // if (it->second->good())
        //{
        Close(it->second);
        //}
    }
    m_OutputFileMap.clear();
    for (auto it = m_InputFileMap.begin(); it != m_InputFileMap.end(); ++it)
    {
        // if (it->second->good())
        //{
        Close(it->second);
        //}
    }
    m_InputFileMap.clear();
}

void AsyncTaskManager::Seek(InputFile &f, size_t offset,
                            const std::string &path)
{
    f->seekg(offset, std::ios_base::beg);
}

void AsyncTaskManager::Seek(OutputFile &f, size_t offset,
                            const std::string &path)
{
    f->seekp(offset, std::ios_base::beg);
}

void AsyncTaskManager::SeekEnd(OutputFile &f)
{
    f->seekp(0, std::ios_base::end);
}

size_t AsyncTaskManager::GetFileSize(InputFile &f)
{
    const auto currentOffset = f->tellg();
    f->seekg(0, std::ios_base::end);
    auto fileSize = f->tellg();
    f->seekg(currentOffset, std::ios_base::beg);
    return static_cast<size_t>(fileSize);
}

std::pair<size_t, double> AsyncTaskManager::Read(InputFile &f, size_t count,
                                                 char *buffer,
                                                 const std::string &path)
{
    size_t totalRead = 0;
    double totalSlept = 0.0;
    const double sleepUnit = 0.01; // seconds
    while (count > 0)
    {
        const auto currentOffset = f->tellg();
        f->read(buffer, static_cast<std::streamsize>(count));
        const auto readSize = f->gcount();

        if (readSize < static_cast<std::streamsize>(count))
        {
            if (f->eof())
            {
                std::chrono::duration<double> d(sleepUnit);
                std::this_thread::sleep_for(d);
                f->clear(f->rdstate() & ~std::fstream::eofbit);
                totalSlept += sleepUnit;
            }
            else
            {
                throw std::ios_base::failure(
                    "AsyncTaskManager couldn't read from file " + path +
                    " offset = " + std::to_string(currentOffset) +
                    " count = " + std::to_string(count) + " bytes but only " +
                    std::to_string(totalRead + readSize) + ".\n");
            }
        }
        buffer += readSize;
        count -= readSize;
        totalRead += readSize;
    }
    return std::pair<size_t, double>(totalRead, totalSlept);
}

size_t AsyncTaskManager::Write(OutputFile &f, size_t count, const char *buffer,
                               const std::string &path)
{
    f->write(buffer, static_cast<std::streamsize>(count));

    if (f->bad())
    {
        throw std::ios_base::failure(
            "AsyncTaskManager couldn't write to file " + path +
            " count = " + std::to_string(count) + " bytes\n");
    }

    return count;
}

void AsyncTaskManager::Delete(OutputFile &f, const std::string &path)
{
    Close(f);
    std::remove(path.c_str());
}

void AsyncTaskManager::SetVerbose(int verboseLevel, int rank)
{
    m_Verbose = verboseLevel;
    m_Rank = rank;
}

} // end namespace transportman
} // end namespace adios2
