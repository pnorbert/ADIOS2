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

    int Barrier(AMPI_Comm comm);
    /*int DummyMPI_Bcast(void *buffer, int count, DummyMPI_Datatype datatype,
       int root, AMPI_Comm comm);*/
};

}

#endif /* ADIOS2_DummyMPI_H_ */
