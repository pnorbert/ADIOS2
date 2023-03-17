#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindSZ
# -----------
#
# Try to find the cuSZ library
#
# This module defines the following variables:
#
#   SZ_FOUND        - System has cuSZ
#   SZ_INCLUDE_DIRS - The cuSZ include directory
#   SZ_LIBRARIES    - Link these to use cuSZ
#
# and the following imported targets:
#   SZ::SZ - The SZ compression library target
#
# You can also set the following variable to help guide the search:
#   SZ_ROOT - The install prefix for SZ containing the
#              include and lib folders
#              Note: this can be set as a CMake variable or an
#                    environment variable.  If specified as a CMake
#                    variable, it will override any setting specified
#                    as an environment variable.

message(" ====  CUSZ 1 ======")
if(NOT CUSZ_FOUND)
  message(" ====  CUSZ 2 ======")
  if((NOT SZ_ROOT) AND (NOT (ENV{SZ_ROOT} STREQUAL "")))
    set(SZ_ROOT "$ENV{SZ_ROOT}")
  endif()
  message(" ====  CUSZ 3 ======")
  if(SZ_ROOT)
    set(SZ_INCLUDE_OPTS HINTS ${SZ_ROOT}/include NO_DEFAULT_PATHS)
    set(SZ_LIBRARY_OPTS
      HINTS ${SZ_ROOT}/lib ${SZ_ROOT}/lib64
      NO_DEFAULT_PATHS
    )
  endif()

  message(" ====  CUSZ 4 ======")
  find_path(SZ_INCLUDE_DIR cusz.h PATH_SUFFIXES cusz ${SZ_INCLUDE_OPTS})
  find_library(SZ_LIBRARY NAMES cusz ${SZ_LIBRARY_OPTS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CUSZ
    FOUND_VAR CUSZ_FOUND
    REQUIRED_VARS SZ_LIBRARY SZ_INCLUDE_DIR
  )
  message(" ====  CUSZ 5 ======")
  if(CUSZ_FOUND)
  message(" ====  CUSZ 6 ======")
    set(SZ_INCLUDE_DIRS ${SZ_INCLUDE_DIR})
    set(SZ_LIBRARIES ${SZ_LIBRARY})
    if(CUSZ_FOUND AND NOT TARGET SZ::SZ)
    message(" ====  CUSZ 7 ======")
      add_library(SZ::SZ UNKNOWN IMPORTED)
      set_target_properties(SZ::SZ PROPERTIES
        IMPORTED_LOCATION             "${SZ_LIBRARY}"
        INTERFACE_LINK_LIBRARIES      "${SZ_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${SZ_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
