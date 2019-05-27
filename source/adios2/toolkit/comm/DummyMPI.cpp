/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include "DummyMPI.h"

#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <string>

namespace adios2
{

DummyMPI::DummyMPI(){};
DummyMPI::~DummyMPI(){};

int DummyMPI::Barrier(const AMPI_Comm & /*comm*/) { return MPI_SUCCESS; }
int DummyMPI::Bcast(void * /*buffer*/, int /*count*/,
                    AMPI_Datatype /*datatype*/, int /*root*/,
                    const AMPI_Comm & /*comm*/)
{
    return MPI_SUCCESS;
}

int DummyMPI::Gather(const void *sendbuf, int sendcnt, AMPI_Datatype sendtype,
                     void *recvbuf, int recvcnt, AMPI_Datatype recvtype,
                     int root, const AMPI_Comm &comm)
{
    int ier = MPI_SUCCESS;
    size_t n = 0, nsent = 0, nrecv = 0;
    if (!sendbuf && !recvbuf)
    {
        return ier;
    }
    if (root)
    {
        ier = MPI_ERR_COMM;
    }

    n = AMPI_Sizeof_Datatype[sendtype];
    nsent = n * sendcnt;

    n = AMPI_Sizeof_Datatype[recvtype];
    nrecv = n * recvcnt;

    if (nrecv != nsent)
    {
        ier = MPI_ERR_COUNT;
    }

    if (ier == MPI_SUCCESS)
    {
        std::memcpy(recvbuf, sendbuf, nsent);
    }
    else
    {
        std::snprintf(mpierrmsg, MPI_MAX_ERROR_STRING,
                      "could not gather data\n");
    }

    return ier;
}

int DummyMPI::Gatherv(const void *sendbuf, int sendcnt, AMPI_Datatype sendtype,
                      void *recvbuf, const int *recvcnts,
                      const int * /*displs */, AMPI_Datatype recvtype, int root,
                      const AMPI_Comm &comm)
{
    int ier = MPI_SUCCESS;
    if (*recvcnts != sendcnt)
    {
        ier = MPI_ERR_BUFFER;
        return ier;
    }

    ier = Gather(sendbuf, sendcnt, sendtype, recvbuf, *recvcnts, recvtype, root,
                 comm);
    return ier;
}

int DummyMPI::Allgather(const void *sendbuf, int sendcount,
                        AMPI_Datatype sendtype, void *recvbuf, int recvcount,
                        AMPI_Datatype recvtype, const AMPI_Comm &comm)
{
    return Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, 0,
                  comm);
}

int DummyMPI::Scatter(const void *sendbuf, int sendcnt, AMPI_Datatype sendtype,
                      void *recvbuf, int recvcnt, AMPI_Datatype recvtype,
                      int root, const AMPI_Comm &comm)
{
    int ier = MPI_SUCCESS;
    size_t n = 0, nsent = 0, nrecv = 0;
    if (!sendbuf || !recvbuf)
    {
        ier = MPI_ERR_BUFFER;
    }

    if (root)
    {
        ier = MPI_ERR_COMM;
    }

    n = AMPI_Sizeof_Datatype[sendtype];
    nsent = n * sendcnt;

    n = AMPI_Sizeof_Datatype[recvtype];
    nrecv = n * recvcnt;

    if (nrecv != nsent)
    {
        ier = MPI_ERR_COUNT;
    }

    if (ier == MPI_SUCCESS)
    {
        std::memcpy(recvbuf, sendbuf, nsent);
    }
    else
    {
        std::snprintf(mpierrmsg, MPI_MAX_ERROR_STRING,
                      "could not scatter data\n");
    }

    return ier;
}

int DummyMPI::Scatterv(const void *sendbuf, const int *sendcnts,
                       const int *displs, AMPI_Datatype sendtype, void *recvbuf,
                       int recvcnt, AMPI_Datatype recvtype, int root,
                       const AMPI_Comm &comm)
{
    int ier = MPI_SUCCESS;
    if (!sendcnts || !displs)
    {
        ier = MPI_ERR_BUFFER;
    }

    if (ier == MPI_SUCCESS)
    {
        ier = Scatter(sendbuf, *sendcnts, sendtype, recvbuf, recvcnt, recvtype,
                      root, comm);
    }

    return ier;
}

void DummyMPI::SetStatus(AMPI_Status *status, const int count)
{
    int *stat = (int *)malloc(sizeof(int));
    // stat will be freed in AMPI_Status destructor
    stat[0] = count;
    status->Set(stat);
}

int DummyMPI::Recv(void * /*recvbuffer*/, int count, AMPI_Datatype /*type*/,
                   int /*source*/, int /*tag*/, const AMPI_Comm & /*comm*/,
                   AMPI_Status *status)
{
    SetStatus(status, count);
    return 0;
}

int DummyMPI::Irecv(void * /*recvbuffer*/, int /*count*/,
                    AMPI_Datatype /*type*/, int /*source*/, int /*tag*/,
                    const AMPI_Comm & /*comm*/, AMPI_Request * /*request*/)

{
    return 0;
}

int DummyMPI::Send(const void * /*sendbuffer*/, int /*count*/,
                   AMPI_Datatype /*type*/, int /*destination*/, int /*tag*/,
                   const AMPI_Comm & /*comm*/)
{
    return 0;
}

int DummyMPI::Isend(const void * /*recvbuffer*/, int /*count*/,
                    AMPI_Datatype /*type*/, int /*source*/, int /*tag*/,
                    const AMPI_Comm & /*comm*/, AMPI_Request * /*request*/)
{
    return 0;
}

int DummyMPI::Wait(AMPI_Request * /*request*/, AMPI_Status * /*status*/)
{
    return 0;
}

#if 0
int DummyMPI::File_open(const AMPI_Comm &/*comm*/, const char *filename, int amode,
                        AMPI_Info /*info*/, AMPI_File *fh)
{
    std::string mode;
    if (amode | MPI_MODE_RDONLY)
    {
        mode += "r";
    }
    if (amode | MPI_MODE_WRONLY)
    {
        mode += "w";
    }
    if (amode | MPI_MODE_APPEND)
    {
        mode += "a";
    }
    mode += "b";

    *fh = std::fopen(filename, mode.c_str());
    if (!*fh)
    {
        std::snprintf(mpierrmsg, MPI_MAX_ERROR_STRING, "File not found: %s",
                      filename);
        return -1;
    }
    return MPI_SUCCESS;
}

int DummyMPI::File_close(AMPI_File *fh) { return fclose(*fh); }

int DummyMPI::File_get_size(AMPI_File fh, AMPI_Offset *size)
{
    long curpos = std::ftell(fh);
    fseek(fh, 0, SEEK_END); // go to end, returned is the size in bytes
    long endpos = std::ftell(fh);
    std::fseek(fh, curpos, SEEK_SET); // go back where we were
    *size = static_cast<MPI_Offset>(endpos);
    // printf("MPI_File_get_size: fh=%d, size=%lld\n", fh, *size);
    return MPI_SUCCESS;
}

int DummyMPI::File_read(AMPI_File fh, void *buf, int count,
                        AMPI_Datatype datatype, AMPI_Status *status)
{
    // FIXME: int count can read only 2GB (*datatype size) array at max
    size_t bytes_to_read = static_cast<size_t>(count) * datatype;
    size_t bytes_read;
    bytes_read = std::fread(buf, 1, bytes_to_read, fh);
    if (bytes_read != bytes_to_read)
    {
        std::snprintf(mpierrmsg, MPI_MAX_ERROR_STRING,
                      "could not read %llu bytes. read only: %llu"
                      "\n",
                      (unsigned long long)bytes_to_read,
                      (unsigned long long)bytes_read);
        return -2;
    }
    SetStatus(status, bytes_read);
    // printf("MPI_File_read: fh=%d, count=%d, typesize=%d, bytes read=%lld\n",
    // fh, count, datatype, *status);
    return MPI_SUCCESS;
}

int DummyMPI::File_seek(AMPI_File fh, AMPI_Offset offset, int whence)
{
    return std::fseek(fh, offset, whence) == MPI_SUCCESS;
}

#endif

int DummyMPI::Get_count(const AMPI_Status *status, AMPI_Datatype, int *count)
{
    *count = *static_cast<const int *>(status->Get());
    return MPI_SUCCESS;
}

int DummyMPI::Error_string(int /*errorcode*/, char *string, int *resultlen)
{
    // std::sprintf(string, "Dummy lib does not know error strings.
    // Code=%d\n",errorcode);
    std::strcpy(string, mpierrmsg);
    *resultlen = static_cast<int>(std::strlen(string));
    return MPI_SUCCESS;
}

double DummyMPI::Wtime()
{
    std::chrono::duration<double> now =
        std::chrono::high_resolution_clock::now().time_since_epoch();
    return now.count();
}

int DummyMPI::Get_processor_name(char *name, int *resultlen)
{
    std::sprintf(name, "processor 0");
    *resultlen = 1;
    return 0;
}

int DummyMPI::Reduce(const void *sendbuf, void *recvbuf, int count,
                     AMPI_Datatype datatype, AMPI_Op op, int root,
                     const AMPI_Comm &comm)
{
    switch (datatype)
    {
    case AMPI_CHAR:
        if (op == AMPI_SUM)
        {
            char *recvBuffer = reinterpret_cast<char *>(recvbuf);
            const char *sendBuffer = reinterpret_cast<const char *>(sendbuf);
            *recvBuffer = std::accumulate(sendBuffer, sendBuffer + count, 0);
        }
        break;
    case AMPI_INT:
        if (op == AMPI_SUM)
        {
            int *recvBuffer = reinterpret_cast<int *>(recvbuf);
            const int *sendBuffer = reinterpret_cast<const int *>(sendbuf);
            *recvBuffer = std::accumulate(sendBuffer, sendBuffer + count, 0);
        }
        break;
    case AMPI_UNSIGNED:
        if (op == AMPI_SUM)
        {
            unsigned int *recvBuffer =
                reinterpret_cast<unsigned int *>(recvbuf);
            const unsigned int *sendBuffer =
                reinterpret_cast<const unsigned int *>(sendbuf);
            *recvBuffer = std::accumulate(sendBuffer, sendBuffer + count, 0);
        }
        break;
    case AMPI_UNSIGNED_LONG:
        if (op == AMPI_SUM)
        {
            unsigned long int *recvBuffer =
                reinterpret_cast<unsigned long int *>(recvbuf);
            const unsigned long int *sendBuffer =
                reinterpret_cast<const unsigned long int *>(sendbuf);
            *recvBuffer = std::accumulate(sendBuffer, sendBuffer + count, 0);
        }
        break;
    case AMPI_UNSIGNED_LONG_LONG:
        if (op == AMPI_SUM)
        {
            unsigned long long int *recvBuffer =
                reinterpret_cast<unsigned long long int *>(recvbuf);
            const unsigned long long int *sendBuffer =
                reinterpret_cast<const unsigned long long int *>(sendbuf);
            *recvBuffer =
                std::accumulate(sendBuffer, sendBuffer + count,
                                static_cast<unsigned long long int>(0));
        }
        break;
    default:
        return MPI_ERR_TYPE;
    }

    return 0;
}

int DummyMPI::Allreduce(const void *sendbuf, void *recvbuf, int count,
                        AMPI_Datatype datatype, AMPI_Op op, const AMPI_Comm &comm)
{
    return Reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
}

}
