/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */

#ifndef ADIOS2_AMPI_DEFS_H_
#define ADIOS2_AMPI_DEFS_H_

#include <cstdint>
#include <cstdio>

namespace adios2
{

#ifndef ADIOS2_HAVE_MPI

/*
 * Miscellaneous constants
 */
#define MPI_ANY_SOURCE -1          /* match any source rank */
#define MPI_PROC_NULL -2           /* rank of null process */
#define MPI_ROOT -4                /* special value for intercomms */
#define MPI_ANY_TAG -1             /* match any message tag */
#define MPI_MAX_PROCESSOR_NAME 256 /* max proc. name length */
#define MPI_MAX_ERROR_STRING 256   /* max error message length */
#define MPI_MAX_OBJECT_NAME 256    /* max object name length */
#define MPI_MAX_LIBRARY_VERSION_STRING                                         \
    256                       /* max length of library version string */
#define MPI_UNDEFINED -32766  /* undefined stuff */
#define MPI_DIST_GRAPH 3      /* dist graph topology */
#define MPI_CART 1            /* cartesian topology */
#define MPI_GRAPH 2           /* graph topology */
#define MPI_KEYVAL_INVALID -1 /* invalid key value */

/*
 * More constants
 */
#define MPI_UNWEIGHTED ((void *)2)     /* unweighted graph */
#define MPI_WEIGHTS_EMPTY ((void *)3)  /* empty weights */
#define MPI_BOTTOM ((void *)0)         /* base reference address */
#define MPI_IN_PLACE ((void *)1)       /* in place buffer */
#define MPI_BSEND_OVERHEAD 128         /* size of bsend header + ptr */
#define MPI_MAX_INFO_KEY 256           /* max info key length */
#define MPI_MAX_INFO_VAL 256           /* max info value length */
#define MPI_ARGV_NULL ((char **)0)     /* NULL argument vector */
#define MPI_ARGVS_NULL ((char ***)0)   /* NULL argument vectors */
#define MPI_ERRCODES_IGNORE ((int *)0) /* don't return error codes */
#define MPI_MAX_PORT_NAME 256          /* max port name length */
#define MPI_ORDER_C 0                  /* C row major order */
#define MPI_ORDER_FORTRAN 1            /* Fortran column major order */
#define MPI_DISTRIBUTE_BLOCK 0         /* block distribution */
#define MPI_DISTRIBUTE_CYCLIC 1        /* cyclic distribution */
#define MPI_DISTRIBUTE_NONE 2          /* not distributed */
#define MPI_DISTRIBUTE_DFLT_DARG (-1)  /* default distribution arg */

#if OMPI_PROVIDE_MPI_FILE_INTERFACE
/*
 * Since these values are arbitrary to Open MPI, we might as well make
 * them the same as ROMIO for ease of mapping.  These values taken
 * from ROMIO's mpio.h file.
 */
#define MPI_MODE_CREATE 1           /* ADIO_CREATE */
#define MPI_MODE_RDONLY 2           /* ADIO_RDONLY */
#define MPI_MODE_WRONLY 4           /* ADIO_WRONLY  */
#define MPI_MODE_RDWR 8             /* ADIO_RDWR  */
#define MPI_MODE_DELETE_ON_CLOSE 16 /* ADIO_DELETE_ON_CLOSE */
#define MPI_MODE_UNIQUE_OPEN 32     /* ADIO_UNIQUE_OPEN */
#define MPI_MODE_EXCL 64            /* ADIO_EXCL */
#define MPI_MODE_APPEND 128         /* ADIO_APPEND */
#define MPI_MODE_SEQUENTIAL 256     /* ADIO_SEQUENTIAL */

#define MPI_DISPLACEMENT_CURRENT -54278278

#define MPI_SEEK_SET 600
#define MPI_SEEK_CUR 602
#define MPI_SEEK_END 604

/* Max data representation length */
#define MPI_MAX_DATAREP_STRING OPAL_MAX_DATAREP_STRING

#endif /* #if OMPI_PROVIDE_MPI_FILE_INTERFACE */

/*
 * MPI-2 One-Sided Communications asserts
 */
#define MPI_MODE_NOCHECK 1
#define MPI_MODE_NOPRECEDE 2
#define MPI_MODE_NOPUT 4
#define MPI_MODE_NOSTORE 8
#define MPI_MODE_NOSUCCEED 16

#define MPI_LOCK_EXCLUSIVE 1
#define MPI_LOCK_SHARED 2

#define MPI_WIN_FLAVOR_CREATE 1
#define MPI_WIN_FLAVOR_ALLOCATE 2
#define MPI_WIN_FLAVOR_DYNAMIC 3
#define MPI_WIN_FLAVOR_SHARED 4

#define MPI_WIN_UNIFIED 0
#define MPI_WIN_SEPARATE 1

/*
 * Predefined attribute keyvals
 *
 * DO NOT CHANGE THE ORDER WITHOUT ALSO CHANGING THE ORDER IN
 * src/attribute/attribute_predefined.c and mpif.h.in.
 */
enum
{
    /* MPI-1 */
    MPI_TAG_UB,
    MPI_HOST,
    MPI_IO,
    MPI_WTIME_IS_GLOBAL,

    /* MPI-2 */
    MPI_APPNUM,
    MPI_LASTUSEDCODE,
    MPI_UNIVERSE_SIZE,
    MPI_WIN_BASE,
    MPI_WIN_SIZE,
    MPI_WIN_DISP_UNIT,
    MPI_WIN_CREATE_FLAVOR,
    MPI_WIN_MODEL,

    /* Even though these four are IMPI attributes, they need to be there
       for all MPI jobs */
    IMPI_CLIENT_SIZE,
    IMPI_CLIENT_COLOR,
    IMPI_HOST_SIZE,
    IMPI_HOST_COLOR
};

/*
 * Error classes and codes
 * Do not change the values of these without also modifying mpif.h.in.
 */
#define MPI_SUCCESS 0
#define MPI_ERR_BUFFER 1
#define MPI_ERR_COUNT 2
#define MPI_ERR_TYPE 3
#define MPI_ERR_TAG 4
#define MPI_ERR_COMM 5
#define MPI_ERR_RANK 6
#define MPI_ERR_REQUEST 7
#define MPI_ERR_ROOT 8
#define MPI_ERR_GROUP 9
#define MPI_ERR_OP 10
#define MPI_ERR_TOPOLOGY 11
#define MPI_ERR_DIMS 12
#define MPI_ERR_ARG 13
#define MPI_ERR_UNKNOWN 14
#define MPI_ERR_TRUNCATE 15
#define MPI_ERR_OTHER 16
#define MPI_ERR_INTERN 17
#define MPI_ERR_IN_STATUS 18
#define MPI_ERR_PENDING 19
#define MPI_ERR_ACCESS 20
#define MPI_ERR_AMODE 21
#define MPI_ERR_ASSERT 22
#define MPI_ERR_BAD_FILE 23
#define MPI_ERR_BASE 24
#define MPI_ERR_CONVERSION 25
#define MPI_ERR_DISP 26
#define MPI_ERR_DUP_DATAREP 27
#define MPI_ERR_FILE_EXISTS 28
#define MPI_ERR_FILE_IN_USE 29
#define MPI_ERR_FILE 30
#define MPI_ERR_INFO_KEY 31
#define MPI_ERR_INFO_NOKEY 32
#define MPI_ERR_INFO_VALUE 33
#define MPI_ERR_INFO 34
#define MPI_ERR_IO 35
#define MPI_ERR_KEYVAL 36
#define MPI_ERR_LOCKTYPE 37
#define MPI_ERR_NAME 38
#define MPI_ERR_NO_MEM 39
#define MPI_ERR_NOT_SAME 40
#define MPI_ERR_NO_SPACE 41
#define MPI_ERR_NO_SUCH_FILE 42
#define MPI_ERR_PORT 43
#define MPI_ERR_QUOTA 44
#define MPI_ERR_READ_ONLY 45
#define MPI_ERR_RMA_CONFLICT 46
#define MPI_ERR_RMA_SYNC 47
#define MPI_ERR_SERVICE 48
#define MPI_ERR_SIZE 49
#define MPI_ERR_SPAWN 50
#define MPI_ERR_UNSUPPORTED_DATAREP 51
#define MPI_ERR_UNSUPPORTED_OPERATION 52
#define MPI_ERR_WIN 53
#define MPI_T_ERR_MEMORY 54
#define MPI_T_ERR_NOT_INITIALIZED 55
#define MPI_T_ERR_CANNOT_INIT 56
#define MPI_T_ERR_INVALID_INDEX 57
#define MPI_T_ERR_INVALID_ITEM 58
#define MPI_T_ERR_INVALID_HANDLE 59
#define MPI_T_ERR_OUT_OF_HANDLES 60
#define MPI_T_ERR_OUT_OF_SESSIONS 61
#define MPI_T_ERR_INVALID_SESSION 62
#define MPI_T_ERR_CVAR_SET_NOT_NOW 63
#define MPI_T_ERR_CVAR_SET_NEVER 64
#define MPI_T_ERR_PVAR_NO_STARTSTOP 65
#define MPI_T_ERR_PVAR_NO_WRITE 66
#define MPI_T_ERR_PVAR_NO_ATOMIC 67
#define MPI_ERR_RMA_RANGE 68
#define MPI_ERR_RMA_ATTACH 69
#define MPI_ERR_RMA_FLAVOR 70
#define MPI_ERR_RMA_SHARED 71
#define MPI_T_ERR_INVALID 72
#define MPI_T_ERR_INVALID_NAME 73

/* Per MPI-3 p349 47, MPI_ERR_LASTCODE must be >= the last predefined
   MPI_ERR_<foo> code. Set the last code to allow some room for adding
   error codes without breaking ABI. */
#define MPI_ERR_LASTCODE 92

#define MPI_ERR_SYSRESOURCE -2

#endif /* not have MPI */

} // end namespace adios2

#endif /* ADIOS2_AMPI_DEFS_H_ */
