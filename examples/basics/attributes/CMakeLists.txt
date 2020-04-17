#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(attributes_write attributes_write.cpp)
target_link_libraries(attributes_write adios2)

add_executable(attributes_read_as_stream attributes_read_as_stream.cpp)
target_link_libraries(attributes_read_as_stream adios2)

add_executable(attributes_read_as_file attributes_read_as_file.cpp)
target_link_libraries(attributes_read_as_file adios2)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(attributes_write MPI::MPI_C)
  target_link_libraries(attributes_read_as_stream MPI::MPI_C)
  target_link_libraries(attributes_read_as_file MPI::MPI_C)
endif()
