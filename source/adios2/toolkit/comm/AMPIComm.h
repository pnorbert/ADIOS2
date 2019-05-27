/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_AMPICOMM_H_
#define ADIOS2_AMPICOMM_H_

#include "AMPI.h"
#include "AMPIDefs.h"
#include "AMPITypes.h"
#include "adios2/ADIOSConfig.h"

#include <climits> //UXXX_MAX
#include <cstdint> //SIZE_MAX
#include <memory>  //std:shared_ptr

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

/* Define ADIOS2_MPI_SIZE_T which depends on the system size_t */
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

namespace adios2
{

/** @brief: Distinguish between using actual MPI or the dummy library for serial
 * codes
 */
enum class CommType
{
    MPI,
    Dummy
};

/** @brief Point of entry class for an application.
 *         Serves as factory of IO class objects and Transforms */

class AMPI_Comm
{
public:
#ifdef ADIOS2_HAVE_MPI
    AMPI_Comm(MPI_Comm comm);
    MPI_Comm comm = MPI_COMM_NULL;
#endif
    AMPI_Comm();
    ~AMPI_Comm();
    int Free(); /* aka MPI_Comm_free. non-const function */

    CommType Type() const;
    AMPI_Comm Duplicate() const; /* aka MPI_Comm_dup */
    int Rank(int *rank) const;   /* aka MPI_Comm_rank */
    int Size(int *size) const;   /* aka MPI_Comm_size */
    int Split(int color, int key, AMPI_Comm *newcomm) const;
    /* Return the actual MPI driver for this comm */
    std::shared_ptr<AMPI> MPI() const;

private:
#ifdef ADIOS2_HAVE_MPI
    AMPI_Comm(MPI_Comm comm, std::shared_ptr<AMPI> driver,
              bool shouldFree = false);
#endif
    AMPI_Comm(std::shared_ptr<AMPI> driver, bool shouldFree = false);
    CommType m_Type;
    bool m_FreeOnDestruct = false;
    std::shared_ptr<AMPI> driver;
};

}

#endif /* ADIOS2_AMPICOMM_H_ */
