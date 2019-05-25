/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include "RealMPI.h"

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

int RealMPI::Barrier(AMPI_Comm comm) { return MPI_Barrier(comm.comm); }
int RealMPI::Bcast(void *buffer, int count, AMPI_Datatype datatype, int root,
                   AMPI_Comm comm)
{
    return MPI_Bcast(buffer, count, dataTypes[datatype], root, comm.comm);
}

}
