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

#include <iostream>

namespace adios2
{

#ifdef ADIOS2_HAVE_MPI

AMPI_Comm::AMPI_Comm(MPI_Comm comm)
: m_Type(CommType::MPI), m_FreeOnDestruct(true),
  driver(std::make_shared<RealMPI>())
{
    MPI_Comm_dup(comm, &this->comm);

    int rank;
    this->Rank(&rank);
    std::cout << "AMPI_Comm(" << static_cast<void *>(comm)
              << ") --> acomm: " << static_cast<void *>(this)
              << " rank = " << rank
              << " comm: " << static_cast<void *>(this->comm) << std::endl;
};

AMPI_Comm::AMPI_Comm(MPI_Comm comm, std::shared_ptr<AMPI> driver, bool flag)
: comm(comm), m_Type(CommType::MPI), m_FreeOnDestruct(flag), driver(driver)
{
    int rank;
    this->Rank(&rank);
    std::cout << "AMPI_Comm(" << static_cast<void *>(comm)
              << ") --> acomm: " << static_cast<void *>(this)
              << " rank = " << rank
              << " comm: " << static_cast<void *>(this->comm) << std::endl;
};

#endif

#if 0
AMPI_Comm::AMPI_Comm(const AMPI_Comm &acomm)
: m_Type(acomm.Type()), m_FreeOnDestruct(false), driver(acomm.MPI())
#ifdef ADIOS2_HAVE_MPI
  ,
  comm(acomm.comm)
#endif
{

    int rank;
    this->Rank(&rank);
    std::cout << "AMPI_Comm(acomm" << static_cast<const void *>(&acomm)
              << ") --> new acomm: " << static_cast<void *>(this)
              << " rank = " << rank
#ifdef ADIOS2_HAVE_MPI
              << " comm: " << static_cast<void *>(this->comm)
#endif
              << std::endl;
};
#endif

AMPI_Comm::AMPI_Comm()
: comm(MPI_COMM_NULL), m_Type(CommType::Dummy), m_FreeOnDestruct(false),
  driver(std::make_shared<DummyMPI>())
{
    int rank;
    this->Rank(&rank);
    std::cout << "AMPI_Comm() --> acomm: " << static_cast<void *>(this)
              << " rank = " << rank
              << " comm: " << static_cast<void *>(this->comm) << std::endl;
};

AMPI_Comm::AMPI_Comm(std::shared_ptr<AMPI> driver, bool flag)
: comm(MPI_COMM_NULL), m_Type(CommType::Dummy), m_FreeOnDestruct(flag),
  driver(driver)
{
    int rank;
    this->Rank(&rank);
    std::cout << "AMPI_Comm() --> acomm: " << static_cast<void *>(this)
              << " rank = " << rank
              << " comm: " << static_cast<void *>(this->comm) << std::endl;
};

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
    std::cout << "AMPI_Comm destructor called. acomm: "
              << static_cast<void *>(this)
              << " comm: " << static_cast<void *>(this->comm)
              << "  Remove driver "
              << (m_Type == CommType::MPI ? "RealMPI" : "DummyMPI")
              << std::endl;
}

CommType AMPI_Comm::Type() const { return m_Type; }

AMPI_Comm AMPI_Comm::Duplicate() const
{
#ifdef ADIOS2_HAVE_MPI
    if (m_Type == CommType::MPI)
    {
        if (comm == MPI_COMM_NULL || comm == MPI_COMM_SELF)
        {
            return AMPI_Comm(MPI_COMM_SELF, driver);
        }
        else
        {
            MPI_Comm dupcomm;
            MPI_Comm_dup(comm, &dupcomm);
            return AMPI_Comm(dupcomm, driver, true);
        }
    }
    else
#endif
    {
        return AMPI_Comm(driver, false);
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
                comm = MPI_COMM_NULL;
            }
        }
#endif
    }
    return retval;
}

int AMPI_Comm::Rank(int *rank) const
{
#ifdef ADIOS2_HAVE_MPI
    if (m_Type == CommType::MPI)
    {
        std::cout << "AMPI_Comm().Rank --> acomm: "
                  << static_cast<const void *>(this)
                  << " comm: " << static_cast<void *>(this->comm) << std::endl;
        return MPI_Comm_rank(comm, rank);
    }
    else
#endif
    {
        *rank = 0;
        return MPI_SUCCESS;
    }
}

int AMPI_Comm::Size(int *size) const
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

int AMPI_Comm::Split(int color, int key, AMPI_Comm *newcomm) const
{
#ifdef ADIOS2_HAVE_MPI
    if (m_Type == CommType::MPI)
    {
        MPI_Comm ncomm;
        int retval = MPI_Comm_split(comm, color, key, &ncomm);
        *newcomm = AMPI_Comm(ncomm, driver, true);
        return retval;
    }
    else
#endif
    {
        *newcomm = AMPI_Comm(driver);
        return MPI_SUCCESS;
    }
}

std::shared_ptr<AMPI> AMPI_Comm::MPI() const { return driver; }
}
