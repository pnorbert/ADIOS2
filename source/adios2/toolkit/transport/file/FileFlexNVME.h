/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SmaertNVME.h wrapper of FlexNVME library functions for file I/O
 *
 *  Created on: Oct 3, 2022
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_FILE_SMNVME_H_
#define ADIOS2_TOOLKIT_TRANSPORT_FILE_SMNVME_H_

#include <future> //std::async, std::future

#include "adios2/common/ADIOSConfig.h"
#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace helper
{
class Comm;
}
namespace transport
{

/** File descriptor transport using the FlexNVME IO library */
class FileFlexNVME : public Transport
{

public:
    FileFlexNVME(helper::Comm const &comm);

    ~FileFlexNVME();

    void Open(const std::string &name, const Mode openMode,
              const bool async = false, const bool directio = false) final;

    void OpenChain(const std::string &name, Mode openMode,
                   const helper::Comm &chainComm, const bool async = false,
                   const bool directio = false) final;

    void Write(const char *buffer, size_t size, size_t start = MaxSizeT) final;

    /* Actual writev() function overriding Transport::WriteV().
       BP5 is using this for data.N files */
    void WriteV(const core::iovec *iov, const int iovcnt,
                size_t start = MaxSizeT) final;

    void Read(char *buffer, size_t size, size_t start = MaxSizeT) final;

    size_t GetSize() final;

    /** Does nothing, each write is supposed to flush */
    void Flush() final;

    void Close() final;

    void Delete() final;

    void SeekToEnd() final;

    void SeekToBegin() final;

    void Seek(const size_t start = MaxSizeT) final;

    void Truncate(const size_t length) final;

    void MkDir(const std::string &fileName) final;

private:
    /** FlexNVME file handle returned by Open */
    int m_FileDescriptor = -1;
    int m_Errno = 0;
    bool m_IsOpening = false;
    std::future<int> m_OpenFuture;
    bool m_DirectIO = false;

    /**
     * Check if m_FileDescriptor is -1 after an operation
     * @param hint exception message
     */
    void CheckFile(const std::string hint) const;
    void WaitForOpen();
    std::string SysErrMsg() const;
};

} // end namespace transport
} // end namespace adios2

#endif /* ADIOS2_TRANSPORT_FILE_SMNVME_H_ */
