#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(bpWriter)
add_subdirectory(timeBP)

if(ADIOS_USE_ADIOS1)
  add_subdirectory(adios1Writer)
endif()

if(ADIOS_USE_DataMan)
  add_subdirectory(datamanReader)
  add_subdirectory(datamanWriter)
endif()
