/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include "DummyMPI.h"

namespace adios2
{

DummyMPI::DummyMPI(){};
DummyMPI::~DummyMPI(){};

int DummyMPI::Barrier(AMPI_Comm comm) { return 0; }

}
