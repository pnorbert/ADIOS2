/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOSPy.h
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#ifndef ADIOSPY_H_
#define ADIOSPY_H_

#include <map>
#include <memory> //std::shared_ptr
#include <string>

#ifdef HAVE_BOOSTPYTHON
#include "boost/python.hpp"
#endif

#ifdef HAVE_PYBIND11
#include "pybind11/pybind11.h"
#endif

#include "ADIOS.h"
#include "MethodPy.h"
#include "VariablePy.h"
#include "adiosPyFunctions.h" //ListToVector, VectorToList

namespace adios
{

#ifdef HAVE_BOOSTPYTHON
using pyList = boost::python::list;
using pyObject = boost::python::object;
using pyNone = pyObject();
#endif

#ifdef HAVE_PYBIND11
using pyList = pybind11::list;
using pyObject = pybind11::object;
#endif

class EnginePy;

class ADIOSPy : public ADIOS
{

public:
    ADIOSPy(MPI_Comm mpiComm, const bool debug);
    ~ADIOSPy();

    void HelloMPI(); ///< says hello from rank/size for testing

    VariablePy DefineVariablePy(const std::string name,
                                const pyList localDimensionsPy = pyList(),
                                const pyList globalDimensionsPy = pyList(),
                                const pyList globalOffsetsPy = pyList());

    MethodPy &DeclareMethodPy(const std::string methodName);

    EnginePy OpenPy(const std::string name, const std::string accessMode,
                    const MethodPy &method, pyObject py_comm = pyObject());

    void DefineVariableType(VariablePy &variablePy);

private:
    std::set<std::string> m_VariablesPyNames;
};

} // end namespace

#endif /* ADIOSPY_H_ */
