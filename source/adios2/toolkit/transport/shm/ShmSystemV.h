/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ShmSystemV.h
 *
 *  Created on: Sep 26, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_SHMEM_SHMEMSYSTEMV_H_
#define ADIOS2_TOOLKIT_TRANSPORT_SHMEM_SHMEMSYSTEMV_H_

#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace transport
{

class ShmSystemV : public Transport
{

public:
    /**
     * Unique constructor
     * @param pathName ftok input to create unique shared-memory key
     * @param projectID  ftok input to create unique shared-memory key. Must be
     * greater than zero.
     * @param size shared-memory pre-allocated data size
     * @param debugMode true: extra checks
     */
    ShmSystemV(const unsigned int projectID, const size_t size,
               const AMPI_Comm &acomm, const bool debugMode = false,
               const bool removeAtClose = false);

    ~ShmSystemV();

    void Open(const std::string &name, const Mode openMode) final;

    void Write(const char *buffer, size_t size, size_t start = MaxSizeT) final;

    void Read(char *buffer, size_t size, size_t start = MaxSizeT) final;

    void Close() final;

private:
    /** 1st argument of ftok to create shared memory segment key, from Open */
    std::string m_PathName;

    /** 2nd argument of ftok to create shared memory segment key, must be
     * greater than zero, from constructor */
    const unsigned int m_ProjectID;

    /** shared memory segment ID from shmget */
    int m_ShmID = -1;

    /** reference to a shared memory data buffer */
    char *m_Buffer = nullptr;

    /** size of the pre-allocated shared memory segment, non-const due to
     * SystemV API */
    size_t m_Size;

    /** remove shared-memory segment at close (will be manually cleaned later)*/
    const bool m_RemoveAtClose = false;

    void CheckShmID(const std::string hint) const;

    void CheckBuffer(const std::string hint) const;

    void CheckSizes(const size_t start, const size_t size,
                    const std::string hint) const;
};

} // end namespace transport
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORT_SHMEM_SHMEMSYSTEMV_H_ */
