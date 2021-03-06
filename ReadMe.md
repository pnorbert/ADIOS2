# Adaptable Input / Output System (ADIOS) v2.0
This is v2.0 of the ADIOS I/O system, developed as part of the
U.S. Department of Energy Exascale Computing Program.

## License
ADIOS >= 2.0 is licensed under the Apache License v2.0.  See the accompanying
Copyright.txt for more details.

## Directory layout

* cmake - Project specific CMake modules
* examples - ADIOS Examples
* include - Public header files 
* scripts - Project maintenance and development scripts
* source - Main ADIOS source
* testing - Tests

## Getting Started

ADIOS 2.0 uses CMake for it's build environment.  CMake expects projects to use "out-of-source" builds, which means keeping a separate build and source directory (different from autotools, which usually uses an in-source build).  To build ADIOS:
1. Clone the repository:
```
$ mkdir adios
$ cd adios
$ git clone https://github.com/ornladios/adios2.git source
```
2. Create a separate build directory:
```
$ mkdir build
```
3. Configure the project with CMake.  The following options can be specified as `ON` or `OFF` with cmake's `-DVAR=VALUE` syntax:
  * `ADIOS_BUILD_SHARED_LIBS` - Build shared libraries (`OFF` for static)
  * `ADIOS_BUILD_EXAMPLES   ` - Build examples
  * `ADIOS_BUILD_TESTING    ` - Build test code
  * `ADIOS_USE_MPI          ` - Enable MPI
  * `ADIOS_USE_BZip2        ` - Enable BZip2 compression
  * `ADIOS_USE_ADIOS1       ` - Enable the ADIOS 1.x engine
  * `ADIOS_USE_DataMan      ` - Enable the DataMan engine
```
$ cd build
$ cmake -DADIOS_USE_MPI=OFF ../source
-- The C compiler identification is GNU 6.3.1
-- The CXX compiler identification is GNU 6.3.1
-- Check for working C compiler: /usr/bin/cc
-- Check for working C compiler: /usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Check for working CXX compiler: /usr/bin/c++
-- Check for working CXX compiler: /usr/bin/c++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Found BZip2: /usr/lib64/libbz2.so (found version "1.0.6") 
-- Looking for BZ2_bzCompressInit
-- Looking for BZ2_bzCompressInit - found

ADIOS2 build configuration:
  C++ Compiler: GNU 6.3.1 
  /usr/bin/c++

  Installation prefix: /usr/local
  Features:
    Library Type: shared
    Build Type:   Debug
    Testing: ON
    MPI:     OFF
    BZip2:   ON
    ADIOS1:  OFF
    DataMan: OFF

-- Configuring done
-- Generating done
-- Build files have been written to: /path/to/adios/build
$
```
You can also use CMake's curses-base UI with `ccmake ../source`.

3. Compile:
```
$ make -j8
```
4. Run tests:
```
$ make test
```

## Developers

Please see the [Contributors Guide](Contributing.md) for how to submit changes
to ADIOS.
