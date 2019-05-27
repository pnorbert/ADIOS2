/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
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

const int AMPI_COMM_NULL = 0;
const int AMPI_COMM_WORLD = 1;
const int AMPI_COMM_SELF = 2;

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

const int AMPI_Sizeof_Datatype[AMPI_Datatype_size] = {
    0,
    sizeof(char),
    sizeof(char),
    sizeof(short int),
    sizeof(int),
    sizeof(long int),
    sizeof(float),
    sizeof(double),
    sizeof(long double),
    sizeof(unsigned char),
    sizeof(signed char),
    sizeof(unsigned short int),
    sizeof(unsigned long int),
    sizeof(unsigned int),
    sizeof(float) + sizeof(int),
    sizeof(double) + sizeof(int),
    sizeof(long double) + sizeof(int),
    sizeof(long int) + sizeof(int),
    sizeof(short int) + sizeof(int),
    2 * sizeof(int),
    sizeof(long long int) + sizeof(int),
    sizeof(long long int),
    sizeof(unsigned long long int),
    sizeof(size_t)};

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
