/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_AMPI_H_
#define ADIOS2_AMPI_H_

#include "AMPIRequest.h"
#include "AMPIStatus.h"
#include "AMPITypes.h"
#include "adios2/ADIOSConfig.h"

#include <climits> //UXXX_MAX
#include <cstdint> //SIZE_MAX

namespace adios2
{
/** @brief Point of entry class for an application.
 *         Serves as factory of IO class objects and Transforms */

class AMPI_Comm;

class AMPI
{
public:
    AMPI();
    virtual ~AMPI();

    virtual int Barrier(AMPI_Comm comm);
    int Bcast(void *buffer, int count, AMPI_Datatype datatype, int root,
              AMPI_Comm comm);

    /*
    virtual int Comm_dup(AMPI_Comm comm, AMPI_Comm *newcomm);
    virtual int Comm_rank(AMPI_Comm comm, int *rank);
    virtual int Comm_size(AMPI_Comm comm, int *size);
    virtual int Comm_free(AMPI_Comm *comm);
    */
    /*
    AMPI_Comm AMPI_Comm_f2c(AMPI_Fint comm);
    */
    int Gather(const void *sendbuf, int sendcount, AMPI_Datatype sendtype,
               void *recvbuf, int recvcount, AMPI_Datatype recvtype, int root,
               AMPI_Comm comm);
    int Gatherv(const void *sendbuf, int sendcount, AMPI_Datatype sendtype,
                void *recvbuf, const int *recvcounts, const int *displs,
                AMPI_Datatype recvtype, int root, AMPI_Comm comm);
    int Allgather(const void *sendbuf, int sendcount, AMPI_Datatype sendtype,
                  void *recvbuf, int recvcount, AMPI_Datatype recvtype,
                  AMPI_Comm comm);

    int Scatter(const void *sendbuf, int sendcount, AMPI_Datatype sendtype,
                void *recvbuf, int recvcount, AMPI_Datatype recvtype, int root,
                AMPI_Comm comm);
    int Scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                 AMPI_Datatype sendtype, void *recvbuf, int recvcount,
                 AMPI_Datatype recvtype, int root, AMPI_Comm comm);

    int Recv(void *buf, int count, AMPI_Datatype datatype, int source, int tag,
             AMPI_Comm comm, AMPI_Status *status);
    int Irecv(void *buf, int count, AMPI_Datatype datatype, int source, int tag,
              AMPI_Comm comm, AMPI_Request *request);
    int Send(const void *buf, int count, AMPI_Datatype datatype, int dest,
             int tag, AMPI_Comm comm);
    int Isend(const void *buf, int count, AMPI_Datatype datatype, int dest,
              int tag, AMPI_Comm comm, AMPI_Request *request);

    int Wait(AMPI_Request *request, AMPI_Status *status);

    /*
    int File_open(AMPI_Comm comm, const char *filename, int amode,
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
    int Comm_split(AMPI_Comm comm, int color, int key, AMPI_Comm *comm_out);

    int Get_processor_name(char *name, int *resultlen);

    double Wtime();

    int Reduce(const void *sendbuf, void *recvbuf, int count,
               AMPI_Datatype datatype, AMPI_Op op, int root, AMPI_Comm comm);

    int Allreduce(const void *sendbuf, void *recvbuf, int count,
                  AMPI_Datatype datatype, AMPI_Op op, AMPI_Comm comm);
};

}

#endif /* ADIOS2_AMPI_H_ */
