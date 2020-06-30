/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * AsyncTaskManager.h
 *
 *  Created on: June 29, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_ASYNCTASKMANAGER_H_
#define ADIOS2_TOOLKIT_TRANSPORT_ASYNCTASKMANAGER_H_

#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <streambuf>
#include <string>

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace transportman
{

enum class AsyncOp
{
    MkdirSingle,  // Create a directory, single process operation
    MkdirBarrier, // Create a directory by rank 0, Barrier on all processes
    Open,         // Open file(s)
    Write,        // Write data from memory
    Read,         // Read data into memory
    GetSize,      // Get the current size
    Flush,        // Flush memory
    Close,        // Close file(s)
    SeekEnd,      // Seek to the end of target file toFileName (for future
    SeekBegin,    // Seek to beginning of file(s)
    Delete        // Remove a file (must be opened for this operation)
};

struct AsyncTaskOperation
{
    AsyncOp op;
    Transport &transport;
    std::string fromFileName;
    std::string toFileName;
    size_t countBytes;
    size_t fromOffset;
    size_t toOffset;
    std::vector<char> dataToWrite; // memory to write with Write operation

    AsyncTaskOperation(AsyncOp op, const std::string &fromFileName,
                       const std::string &toFileName, size_t countBytes,
                       size_t fromOffset, size_t toOffset, const void *data);
};

typedef std::map<std::string, std::shared_ptr<std::ifstream>> InputFileMap;
typedef std::map<std::string, std::shared_ptr<std::ofstream>> OutputFileMap;
typedef std::shared_ptr<std::ifstream> InputFile;
typedef std::shared_ptr<std::ofstream> OutputFile;

class AsyncTaskManager
{
public:
    AsyncTaskManager(helper::Comm &comm);

    virtual ~AsyncTaskManager() = default;

    void AddOperation(AsyncTaskOperation &operation);
    void AddOperation(AsyncOp op, const std::string &fromFileName,
                      const std::string &toFileName, size_t fromOffset,
                      size_t toOffset, size_t countBytes,
                      const void *data = nullptr);

    void AddOperationSeekEnd(const std::string &toFileName);
    void AddOperationCopyAt(const std::string &fromFileName,
                            const std::string &toFileName, size_t fromOffset,
                            size_t toOffset, size_t countBytes);
    void AddOperationCopy(const std::string &fromFileName,
                          const std::string &toFileName, size_t countBytes);
    void AddOperationWriteAt(const std::string &toFileName, size_t toOffset,
                             size_t countBytes, const void *data);
    void AddOperationWrite(const std::string &toFileName, size_t countBytes,
                           const void *data);
    void AddOperationOpen(const std::string &toFileName, Mode mode);

    void AddOperationDelete(const std::string &toFileName);

    /** Create thread */
    virtual void Start() = 0;

    /** Tell thread to terminate when all draining has finished. */
    virtual void Finish() = 0;

    /** Join the thread. Main thread will block until thread terminates */
    virtual void Join() = 0;

    /** turn on verbosity. set rank to differentiate between the output of
     * processes */
    void SetVerbose(int verboseLevel, int rank);

protected:
    helper::Comm const &m_Comm;
    std::queue<AsyncTaskOperation> operations;
    std::mutex operationsMutex;

    /** rank of process just for stdout/stderr messages */
    int m_Rank = 0;
    int m_Verbose = 0;
    static const int errorState = -1;

    /** instead for Open, use this function */
    InputFile GetFileForRead(const std::string &path);
    OutputFile GetFileForWrite(const std::string &path, bool append = false);

    /** return true if the File is usable (no previous errors) */
    bool Good(InputFile &f);
    bool Good(OutputFile &f);

    void CloseAll();

    void Seek(InputFile &f, size_t offset, const std::string &path);
    void Seek(OutputFile &f, size_t offset, const std::string &path);
    void SeekEnd(OutputFile &f);

    /** Read from file. Return a pair of
     *  - number of bytes written
     *  - time spent in waiting for file to be actually written to disk for this
     * read to succeed.
     */
    std::pair<size_t, double> Read(InputFile &f, size_t count, char *buffer,
                                   const std::string &path);
    size_t Write(OutputFile &f, size_t count, const char *buffer,
                 const std::string &path);

    void Delete(OutputFile &f, const std::string &path);

private:
    InputFileMap m_InputFileMap;
    OutputFileMap m_OutputFileMap;
    void Open(InputFile &f, const std::string &path);
    void Close(InputFile &f);
    void Open(OutputFile &f, const std::string &path, bool append);
    void Close(OutputFile &f);
    size_t GetFileSize(InputFile &f);
};

} // end namespace transportman
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORT_ASYNCTASKMANAGER_H_ */
