/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * A dummy MPI 'implementation' for the BP READ API, to have an MPI-free version
 * of the API
 *
 */

#ifndef ADIOS2_AMPI_STATUS_H_
#define ADIOS2_AMPI_STATUS_H_

namespace adios2
{

class AMPI_Status
{
public:
    AMPI_Status();
    virtual ~AMPI_Status();
    const void *Get();
    void Set(void *ptr);

private:
    // the actual status object of the driver
    void *m_Status;
};

} // end namespace adios2

#endif /* ADIOS2_AMPI_STATUS_H_ */
