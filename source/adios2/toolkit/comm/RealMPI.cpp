/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include "RealMPI.h"

#include <cstdlib>
#include <mpi.h>

namespace adios2
{

static MPI_Datatype dataTypes[AMPI_Datatype_size];
static MPI_Op ops[AMPI_Op_size];
static void MPIConstantsInit()
{
    dataTypes[AMPI_DATATYPE_NULL] = MPI_DATATYPE_NULL;
    dataTypes[AMPI_BYTE] = MPI_BYTE;
    dataTypes[AMPI_CHAR] = MPI_CHAR;
    dataTypes[AMPI_SHORT] = MPI_SHORT;
    dataTypes[AMPI_INT] = MPI_INT;
    dataTypes[AMPI_LONG] = MPI_LONG;

    dataTypes[AMPI_FLOAT] = MPI_FLOAT;
    dataTypes[AMPI_DOUBLE] = MPI_DOUBLE;
    dataTypes[AMPI_LONG_DOUBLE] = MPI_LONG_DOUBLE;

    dataTypes[AMPI_UNSIGNED_CHAR] = MPI_UNSIGNED_CHAR;
    dataTypes[AMPI_SIGNED_CHAR] = MPI_SIGNED_CHAR;
    dataTypes[AMPI_UNSIGNED_SHORT] = MPI_UNSIGNED_SHORT;
    dataTypes[AMPI_UNSIGNED_LONG] = MPI_UNSIGNED_LONG;
    dataTypes[AMPI_UNSIGNED] = MPI_UNSIGNED;

    dataTypes[AMPI_FLOAT_INT] = MPI_FLOAT_INT;
    dataTypes[AMPI_DOUBLE_INT] = MPI_DOUBLE_INT;
    dataTypes[AMPI_LONG_DOUBLE_INT] = MPI_LONG_DOUBLE_INT;
    dataTypes[AMPI_LONG_INT] = MPI_LONG_INT;
    dataTypes[AMPI_SHORT_INT] = MPI_SHORT_INT;

    dataTypes[AMPI_2INT] = MPI_2INT;

#ifdef MPI_UNSIGNED_LONG_LONG
    dataTypes[AMPI_LONG_LONG_INT] = MPI_LONG_LONG_INT;
    dataTypes[AMPI_LONG_LONG] = MPI_LONG_LONG;
    dataTypes[AMPI_UNSIGNED_LONG_LONG] = MPI_UNSIGNED_LONG_LONG;
#endif

#if SIZE_MAX == UINT_MAX
    dataTypes[AMPI_SIZE_T] = MPI_UNSIGNED;
#elif SIZE_MAX == ULONG_MAX
    dataTypes[AMPI_SIZE_T] = MPI_UNSIGNED_LONG;
#elif SIZE_MAX == ULLONG_MAX
    dataTypes[AMPI_SIZE_T] = MPI_UNSIGNED_LONG_LONG;
#else
#error "AMPI_SIZE_T could not be mapped to a real MPI Type"
#endif

    ops[AMPI_MAX] = MPI_MAX;
    ops[AMPI_MIN] = MPI_MIN;
    ops[AMPI_SUM] = MPI_SUM;
}

RealMPI::RealMPI() { MPIConstantsInit(); };
RealMPI::~RealMPI(){};

int RealMPI::RealMPI::Barrier(AMPI_Comm comm) { return MPI_Barrier(comm.comm); }
int RealMPI::RealMPI::Bcast(void *buffer, int count, AMPI_Datatype datatype,
                            int root, AMPI_Comm comm)
{
    return MPI_Bcast(buffer, count, dataTypes[datatype], root, comm.comm);
}

int RealMPI::Gather(const void *sendbuf, int sendcount, AMPI_Datatype sendtype,
                    void *recvbuf, int recvcount, AMPI_Datatype recvtype,
                    int root, AMPI_Comm comm)
{
    return MPI_Gather(sendbuf, sendcount, dataTypes[sendtype], recvbuf,
                      recvcount, dataTypes[recvtype], root, comm.comm);
}

int RealMPI::Gatherv(const void *sendbuf, int sendcount, AMPI_Datatype sendtype,
                     void *recvbuf, const int *recvcounts, const int *displs,
                     AMPI_Datatype recvtype, int root, AMPI_Comm comm)
{
    return MPI_Gatherv(sendbuf, sendcount, dataTypes[sendtype], recvbuf,
                       recvcounts, displs, dataTypes[recvtype], root,
                       comm.comm);
}

int RealMPI::Allgather(const void *sendbuf, int sendcount,
                       AMPI_Datatype sendtype, void *recvbuf, int recvcount,
                       AMPI_Datatype recvtype, AMPI_Comm comm)
{
    return MPI_Allgather(sendbuf, sendcount, dataTypes[sendtype], recvbuf,
                         recvcount, dataTypes[recvtype], comm.comm);
}

int RealMPI::Scatter(const void *sendbuf, int sendcount, AMPI_Datatype sendtype,
                     void *recvbuf, int recvcount, AMPI_Datatype recvtype,
                     int root, AMPI_Comm comm)
{
    return MPI_Scatter(sendbuf, sendcount, dataTypes[sendtype], recvbuf,
                       recvcount, dataTypes[recvtype], root, comm.comm);
}

int RealMPI::Scatterv(const void *sendbuf, const int *sendcounts,
                      const int *displs, AMPI_Datatype sendtype, void *recvbuf,
                      int recvcount, AMPI_Datatype recvtype, int root,
                      AMPI_Comm comm)
{
    return MPI_Scatterv(sendbuf, sendcounts, displs, dataTypes[sendtype],
                        recvbuf, recvcount, dataTypes[recvtype], root,
                        comm.comm);
}

int RealMPI::Recv(void *buf, int count, AMPI_Datatype datatype, int source,
                  int tag, AMPI_Comm comm, AMPI_Status *status)
{
    MPI_Status *stat = (MPI_Status *)malloc(sizeof(MPI_Status));
    status->Set(stat);
    return MPI_Recv(buf, count, dataTypes[datatype], source, tag, comm.comm,
                    stat);
}
int RealMPI::Irecv(void *buf, int count, AMPI_Datatype datatype, int source,
                   int tag, AMPI_Comm comm, AMPI_Request *request)
{
    MPI_Request *req = (MPI_Request *)malloc(sizeof(MPI_Request));
    request->Set(req);
    return MPI_Irecv(buf, count, dataTypes[datatype], source, tag, comm.comm,
                     req);
}

int RealMPI::Send(const void *buf, int count, AMPI_Datatype datatype, int dest,
                  int tag, AMPI_Comm comm)
{
    return MPI_Send(buf, count, dataTypes[datatype], dest, tag, comm.comm);
}
int RealMPI::Isend(const void *buf, int count, AMPI_Datatype datatype, int dest,
                   int tag, AMPI_Comm comm, AMPI_Request *request)
{
    MPI_Request *req = (MPI_Request *)malloc(sizeof(MPI_Request));
    request->Set(req);
    return MPI_Isend(buf, count, dataTypes[datatype], dest, tag, comm.comm,
                     req);
}

int RealMPI::Wait(AMPI_Request *request, AMPI_Status *status)
{
    MPI_Request *req = (MPI_Request *)request->Get();
    MPI_Status *stat = (MPI_Status *)status->Get();
    return MPI_Wait(req, stat);
}

/*
int RealMPI::File_open(AMPI_Comm comm, const char *filename, int amode,
                   AMPI_Info info, AMPI_File *fh){}
int RealMPI::File_close(AMPI_File *fh){}
int RealMPI::File_get_size(AMPI_File fh, AMPI_Offset *size){}
int RealMPI::File_read(AMPI_File fh, void *buf, int count,
                   AMPI_Datatype datatype, AMPI_Status *status){}
int RealMPI::File_seek(AMPI_File fh, AMPI_Offset offset, int whence){}
*/

int RealMPI::Get_count(const AMPI_Status *status, AMPI_Datatype datatype,
                       int *count)
{
    const MPI_Status *stat = (const MPI_Status *)status->Get();
    return MPI_Get_count(stat, dataTypes[datatype], count);
}

int RealMPI::Error_string(int errorcode, char *string, int *resultlen)
{
    return MPI_Error_string(errorcode, string, resultlen);
}

int RealMPI::Comm_split(AMPI_Comm comm, int color, int key, AMPI_Comm *comm_out)
{
    MPI_Comm newcomm;
    int ret = MPI_Comm_split(comm.comm, color, key, &newcomm);
    *comm_out = AMPI_Comm(newcomm);
    return ret;
}

int RealMPI::Get_processor_name(char *name, int *resultlen) {}

double RealMPI::Wtime() { return MPI_Wtime(); }

int RealMPI::Reduce(const void *sendbuf, void *recvbuf, int count,
                    AMPI_Datatype datatype, AMPI_Op op, int root,
                    AMPI_Comm comm)
{
    return MPI_Reduce(sendbuf, recvbuf, count, dataTypes[datatype], ops[op],
                      root, comm.comm);
}

int RealMPI::Allreduce(const void *sendbuf, void *recvbuf, int count,
                       AMPI_Datatype datatype, AMPI_Op op, AMPI_Comm comm)
{
    return MPI_Allreduce(sendbuf, recvbuf, count, dataTypes[datatype], ops[op],
                         comm.comm);
}

}
