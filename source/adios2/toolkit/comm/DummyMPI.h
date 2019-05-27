/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_DummyMPI_H_
#define ADIOS2_DummyMPI_H_

#include "AMPIComm.h"
#include "adios2/ADIOSConfig.h"

namespace adios2
{
/** @brief Point of entry class for an application.
 *         Serves as factory of IO class objects and Transforms */

class DummyMPI : public AMPI
{
public:
    DummyMPI();
    ~DummyMPI();

    int Barrier(const AMPI_Comm &comm);
    int Bcast(void *buffer, int count, AMPI_Datatype datatype, int root,
              const AMPI_Comm &comm);

    int Gather(const void *sendbuf, int sendcount, AMPI_Datatype sendtype,
               void *recvbuf, int recvcount, AMPI_Datatype recvtype, int root,
               const AMPI_Comm &comm);
    int Gatherv(const void *sendbuf, int sendcount, AMPI_Datatype sendtype,
                void *recvbuf, const int *recvcounts, const int *displs,
                AMPI_Datatype recvtype, int root, const AMPI_Comm &comm);
    int Allgather(const void *sendbuf, int sendcount, AMPI_Datatype sendtype,
                  void *recvbuf, int recvcount, AMPI_Datatype recvtype,
                  const AMPI_Comm &comm);

    int Scatter(const void *sendbuf, int sendcount, AMPI_Datatype sendtype,
                void *recvbuf, int recvcount, AMPI_Datatype recvtype, int root,
                const AMPI_Comm &comm);
    int Scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                 AMPI_Datatype sendtype, void *recvbuf, int recvcount,
                 AMPI_Datatype recvtype, int root, const AMPI_Comm &comm);

    int Recv(void *buf, int count, AMPI_Datatype datatype, int source, int tag,
             const AMPI_Comm &comm, AMPI_Status *status);
    int Irecv(void *buf, int count, AMPI_Datatype datatype, int source, int tag,
              const AMPI_Comm &comm, AMPI_Request *request);
    int Send(const void *buf, int count, AMPI_Datatype datatype, int dest,
             int tag, const AMPI_Comm &comm);
    int Isend(const void *buf, int count, AMPI_Datatype datatype, int dest,
              int tag, const AMPI_Comm &comm, AMPI_Request *request);

    int Wait(AMPI_Request *request, AMPI_Status *status);

    /*
    int File_open(const AMPI_Comm &comm, const char *filename, int amode,
                       AMPI_Info info, AMPI_File *fh);
    int File_close(AMPI_File *fh);
    int File_get_size(AMPI_File fh, AMPI_Offset *size);
    int File_read(AMPI_File fh, void *buf, int count,
                       AMPI_Datatype datatype, AMPI_Status *status);
    int File_seek(AMPI_File fh, AMPI_Offset offset, int whence);
    */

    int Get_count(const AMPI_Status *status, AMPI_Datatype datatype,
                  int *count);
    int Error_string(int errorcode, char *string, int *resultlen);

    int Get_processor_name(char *name, int *resultlen);

    double Wtime();

    int Reduce(const void *sendbuf, void *recvbuf, int count,
               AMPI_Datatype datatype, AMPI_Op op, int root, const AMPI_Comm &comm);

    int Allreduce(const void *sendbuf, void *recvbuf, int count,
                  AMPI_Datatype datatype, AMPI_Op op, const AMPI_Comm &comm);

private:
    char mpierrmsg[MPI_MAX_ERROR_STRING];
    static void SetStatus(AMPI_Status *status, const int count);
    static int TypeSize(AMPI_Datatype);
};

}

#endif /* ADIOS2_DummyMPI_H_ */
