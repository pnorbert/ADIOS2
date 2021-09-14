#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  set(maybe_adios2_core_mpi adios2_core_mpi)
else()
  set(maybe_adios2_core_mpi)
endif()

# stt_conn_tool
add_executable(sst_conn_tool sst_conn_tool.c sst_conn_tool.cxx)
target_link_libraries(sst_conn_tool ${maybe_adios2_core_mpi} adios2_core)
set_property(TARGET sst_conn_tool PROPERTY OUTPUT_NAME sst_conn_tool${ADIOS2_EXECUTABLE_SUFFIX})

if(WIN32)
  target_link_libraries(sst_conn_tool getopt)
endif()

target_link_libraries(sst_conn_tool sst adios2::thirdparty::EVPath)

target_include_directories(sst_conn_tool PRIVATE .. ../cp )

install(TARGETS sst_conn_tool EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_tools-runtime
)

# sst_mpi_test  and  libfabric_mpi_test and adios2_sst_mpi_test
if(ADIOS2_HAVE_MPI AND ADIOS2_SST_HAVE_LIBFABRIC)
  add_executable(sst_mpi_test sst_mpi_test.cpp)
  target_link_libraries(sst_mpi_test adios2_core_mpi adios2_core sst adios2::thirdparty::EVPath)
  target_include_directories(sst_mpi_test PRIVATE .. ../cp ../dp )
  set_property(TARGET sst_mpi_test PROPERTY OUTPUT_NAME sst_mpi_test${ADIOS2_EXECUTABLE_SUFFIX})

  add_executable(adios2_sst_mpi_test adios2_sst_mpi_test.cpp)
  target_link_libraries(adios2_sst_mpi_test adios2::cxx11_mpi MPI::MPI_C)
  #target_include_directories(adios2_sst_mpi_test)
  set_property(TARGET adios2_sst_mpi_test PROPERTY OUTPUT_NAME adios2_sst_mpi_test${ADIOS2_EXECUTABLE_SUFFIX})

  add_executable(libfabric_mpi_test libfabric_mpi_test.cpp)
  target_link_libraries(libfabric_mpi_test libfabric::libfabric MPI::MPI_C)
  target_include_directories(libfabric_mpi_test PRIVATE .. )
  set_property(TARGET libfabric_mpi_test PROPERTY OUTPUT_NAME libfabric_mpi_test${ADIOS2_EXECUTABLE_SUFFIX})

  if(WIN32)
    target_link_libraries(sst_mpi_test getopt)
    target_link_libraries(adios2_sst_mpi_test getopt)
    target_link_libraries(libfabric_mpi_test getopt)
  endif()

  install(TARGETS sst_mpi_test adios2_sst_mpi_test libfabric_mpi_test EXPORT adios2
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_tools-runtime
  )
endif()
