/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * A dummy MPI 'implementation' for the BP READ API, to have an MPI-free version
 * of the API
 *
 */

#include "AMPIRequest.h"

#include <cstdlib>

namespace adios2
{

AMPI_Request::AMPI_Request() : m_Request(nullptr){};
AMPI_Request::~AMPI_Request()
{
    if (m_Request)
    {
        free(m_Request);
    }
};
const void *AMPI_Request::Get() { return m_Request; };
void AMPI_Request::Set(void *ptr) { m_Request = ptr; };

} // end namespace adios2
