/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * A dummy MPI 'implementation' for the BP READ API, to have an MPI-free version
 * of the API
 *
 */

#ifndef ADIOS2_AMPI_TYPES_H_
#define ADIOS2_AMPI_TYPES_H_

#include <cstdint>
#include <cstdio>

namespace adios2
{

// using MPI_Comm = int;
// using MPI_Datatype = int;
// using MPI_Status = std::uint64_t;
// using MPI_Request = std::uint64_t;
// using MPI_File = std::FILE *;
// using MPI_Info = int;
// using MPI_Fint = int;
// using MPI_Op = int;

const int AMPI_SUCCESS = 0;
const int AMPI_ERR_BUFFER = 1; /* Invalid buffer pointer */
const int AMPI_ERR_COUNT = 2;  /* Invalid count argument */
const int AMPI_ERR_TYPE = 3;   /* Invalid datatype argument */
const int AMPI_ERR_TAG = 4;    /* Invalid tag argument */
const int AMPI_ERR_COMM = 5;   /* Invalid communicator */
const int AMPI_ERR_INTERN = 6; /* Invalid memory */

const int AMPI_MAX_ERROR_STRING = 512;
const int AMPI_MODE_RDONLY = 1;
const int AMPI_MODE_WRONLY = 2;
const int AMPI_MODE_RDWR = (AMPI_MODE_RDONLY | AMPI_MODE_RDONLY);
const int AMPI_MODE_CREATE = AMPI_MODE_WRONLY;
const int AMPI_MODE_EXCL = 0;
const int AMPI_MODE_DELETE_ON_CLOSE = 0;
const int AMPI_MODE_UNIQUE_OPEN = 0;
const int AMPI_MODE_SEQUENTIAL = 0;
const int AMPI_MODE_APPEND = 4;
const int AMPI_SEEK_SET = SEEK_SET;
const int AMPI_SEEK_CUR = SEEK_CUR;
const int AMPI_SEEK_END = SEEK_END;
const int AMPI_BYTE_SIZE = 1; /* I need the size of the type here */
// const int AMPI_INFO_NULL = 0;

const int AMPI_COMM_NULL = 0;
const int AMPI_COMM_WORLD = 1;
const int AMPI_COMM_SELF = 2;

const int AMPI_ANY_SOURCE = 0;
const int AMPI_ANY_TAG = 0;
const int AMPI_MAX_PROCESSOR_NAME = 32;

enum AMPI_Datatype
{
    AMPI_DATATYPE_NULL = 0,
    AMPI_BYTE,
    AMPI_CHAR,
    AMPI_SHORT,
    AMPI_INT,
    AMPI_LONG,
    AMPI_FLOAT,
    AMPI_DOUBLE,
    AMPI_LONG_DOUBLE,
    AMPI_UNSIGNED_CHAR,
    AMPI_SIGNED_CHAR,
    AMPI_UNSIGNED_SHORT,
    AMPI_UNSIGNED_LONG,
    AMPI_UNSIGNED,
    AMPI_FLOAT_INT,
    AMPI_DOUBLE_INT,
    AMPI_LONG_DOUBLE_INT,
    AMPI_LONG_INT,
    AMPI_SHORT_INT,
    AMPI_2INT,
    AMPI_LONG_LONG_INT,
    AMPI_LONG_LONG,
    AMPI_UNSIGNED_LONG_LONG,
    AMPI_SIZE_T,

    /* Should be last to denote how many types we have */
    AMPI_Datatype_size
};

enum AMPI_Op
{
    AMPI_MAX,
    AMPI_MIN,
    AMPI_SUM,

    /* Should be last to denote how many types we have */
    AMPI_Op_size
};

} // end namespace adios2

#endif /* ADIOS2_AMPI_TYPES_H_ */
