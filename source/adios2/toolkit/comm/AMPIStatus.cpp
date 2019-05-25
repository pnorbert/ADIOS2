/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * A dummy MPI 'implementation' for the BP READ API, to have an MPI-free version
 * of the API
 *
 */

#include "AMPIStatus.h"

#include <cstdlib>

namespace adios2
{

AMPI_Status::AMPI_Status() : m_Status(nullptr){};
AMPI_Status::~AMPI_Status()
{
    if (m_Status)
    {
        free(m_Status);
    }
};
const void *AMPI_Status::Get() { return m_Status; };
void AMPI_Status::Set(void *ptr) { m_Status = ptr; };

} // end namespace adios2
