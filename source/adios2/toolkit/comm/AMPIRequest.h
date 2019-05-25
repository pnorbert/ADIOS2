/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * A dummy MPI 'implementation' for the BP READ API, to have an MPI-free version
 * of the API
 *
 */

#ifndef ADIOS2_AMPI_REQUEST_H_
#define ADIOS2_AMPI_REQUEST_H_

namespace adios2
{

class AMPI_Request
{
public:
    AMPI_Request();
    virtual ~AMPI_Request();
    const void *Get();
    void Set(void *ptr);

private:
    // the actual Request object of the driver
    void *m_Request;
};

} // end namespace adios2

#endif /* ADIOS2_AMPI_REQUEST_H_ */
