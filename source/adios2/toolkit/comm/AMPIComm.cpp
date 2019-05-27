/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include "AMPIComm.h"
#include "DummyMPI.h"
#include "RealMPI.h"
#include "adios2/helper/adiosMPIFunctions.h"

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

namespace adios2
{

#ifdef ADIOS2_HAVE_MPI

AMPI_Comm::AMPI_Comm(MPI_Comm comm)
: comm(comm), m_Type(CommType::MPI), driver(new RealMPI()){};
AMPI_Comm::AMPI_Comm()
: comm(MPI_COMM_NULL), m_Type(CommType::Dummy), driver(new RealMPI()){};
AMPI_Comm::AMPI_Comm(MPI_Comm comm, bool flag)
: comm(comm), m_FreeOnDestruct(flag), m_Type(CommType::MPI),
  driver(new RealMPI()){};

#else

AMPI_Comm::AMPI_Comm() : m_Type(CommType::Dummy), driver(new DummyMPI()){};

#endif

AMPI_Comm::~AMPI_Comm()
{
    if (m_FreeOnDestruct)
    {
#ifdef ADIOS2_HAVE_MPI
        if (m_Type == CommType::MPI)
        {
            if (comm == MPI_COMM_NULL || comm == MPI_COMM_SELF)
            {
                MPI_Comm_free(&comm);
            }
        }
#endif
    }
    delete (driver);
}

CommType AMPI_Comm::Type() { return m_Type; }

AMPI_Comm AMPI_Comm::Duplicate()
{
#ifdef ADIOS2_HAVE_MPI
    if (m_Type == CommType::MPI)
    {
        if (comm == MPI_COMM_NULL || comm == MPI_COMM_SELF)
        {
            return AMPI_Comm(MPI_COMM_SELF);
        }
        else
        {
            MPI_Comm dupcomm;
            MPI_Comm_dup(comm, &dupcomm);
            return AMPI_Comm(dupcomm, true);
        }
    }
    else
#endif
    {
        return AMPI_Comm();
    }
}

int AMPI_Comm::Free()
{
    int retval = MPI_SUCCESS;
    if (m_FreeOnDestruct)
    {
#ifdef ADIOS2_HAVE_MPI
        if (m_Type == CommType::MPI)
        {
            if (comm == MPI_COMM_NULL || comm == MPI_COMM_SELF)
            {
                retval = MPI_Comm_free(&comm);
                m_FreeOnDestruct = false;
            }
        }
#endif
    }
    return retval;
}

int AMPI_Comm::Rank(int *rank)
{
#ifdef ADIOS2_HAVE_MPI
    if (m_Type == CommType::MPI)
    {
        return MPI_Comm_rank(comm, rank);
    }
    else
#endif
    {
        *rank = 0;
        return MPI_SUCCESS;
    }
}

int AMPI_Comm::Size(int *size)
{
#ifdef ADIOS2_HAVE_MPI
    if (m_Type == CommType::MPI)
    {
        return MPI_Comm_size(comm, size);
    }
    else
#endif
    {
        *size = 1;
        return MPI_SUCCESS;
    }
}

int AMPI_Comm::Split(int color, int key, AMPI_Comm *newcomm)
{
#ifdef ADIOS2_HAVE_MPI
    if (m_Type == CommType::MPI)
    {
        MPI_Comm ncomm;
        int retval = MPI_Comm_split(comm, color, key, &ncomm);
        *newcomm = AMPI_Comm(ncomm);
        return retval;
    }
    else
#endif
    {
        *newcomm = AMPI_Comm();
        return MPI_SUCCESS;
    }
}

AMPI *AMPI_Comm::MPI() { return driver; }

}
