/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_RealMPI_H_
#define ADIOS2_RealMPI_H_

#include "AMPIComm.h"
#include "adios2/ADIOSConfig.h"

namespace adios2
{
/** @brief Point of entry class for an application.
 *         Serves as factory of IO class objects and Transforms */

class RealMPI : public AMPI
{
public:
    RealMPI();
    ~RealMPI();

    int Barrier(AMPI_Comm comm);
    int Bcast(void *buffer, int count, AMPI_Datatype datatype, int root,
              AMPI_Comm comm);
};

}

#endif /* ADIOS2_RealMPI_H_ */
