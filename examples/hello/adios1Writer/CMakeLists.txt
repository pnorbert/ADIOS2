#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)
  add_executable(hello_adios1Writer helloADIOS1Writer.cpp)
  target_include_directories(hello_adios1Writer PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_adios1Writer adios2 ${MPI_C_LIBRARIES})
endif()

add_executable(hello_adios1Writer_nompi helloADIOS1Writer_nompi.cpp)
target_link_libraries(hello_adios1Writer_nompi adios2)
