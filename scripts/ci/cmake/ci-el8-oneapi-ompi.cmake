include(ProcessorCount)
ProcessorCount(NCPUS)
math(EXPR N2CPUS "${NCPUS}*2")

set(ENV{CC}  icx)
set(ENV{CXX} icpx)
set(ENV{FC}  ifort) # oneapi fortran compiler currently has issues

set(dashboard_cache "
ADIOS2_USE_BZip2:BOOL=ON
ADIOS2_USE_Blosc:BOOL=OFF
ADIOS2_USE_DataMan:BOOL=ON
ADIOS2_USE_DataSpaces:BOOL=OFF
ADIOS2_USE_Fortran:BOOL=OFF
ADIOS2_USE_HDF5:BOOL=ON
ADIOS2_USE_MPI:BOOL=ON
ADIOS2_USE_Python:BOOL=ON
ADIOS2_USE_SZ:BOOL=ON
ADIOS2_USE_ZeroMQ:STRING=ON
ADIOS2_USE_ZFP:BOOL=ON

CMAKE_C_COMPILER_LAUNCHER=ccache
CMAKE_CXX_COMPILER_LAUNCHER=ccache
CMAKE_C_FLAGS:STRING=-Wall
CMAKE_C_FLAGS_DEBUG:STRING=-g -O0
CMAKE_CXX_FLAGS:STRING=-Wall
CMAKE_CXX_FLAGS_DEBUG:STRING=-g -O0
CMAKE_Fortran_FLAGS:STRING=-W1

MPIEXEC_EXTRA_FLAGS:STRING=--allow-run-as-root --oversubscribe
MPIEXEC_MAX_NUMPROCS:STRING=${N2CPUS}
")

set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
