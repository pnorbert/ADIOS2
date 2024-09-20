/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * RefactorWriter.tcc
 *
 *  Created on: Apr 6, 2019
 *      Author: Jason Wang w4g@ornl.gov
 */

#ifndef ADIOS2_ENGINE_REFACTORWRITER_TCC_
#define ADIOS2_ENGINE_REFACTORWRITER_TCC_

#include "RefactorWriter.h"
#include "adios2/helper/adiosMath.h"
#include "adios2/helper/adiosMemory.h"

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void RefactorWriter::PutSyncCommon(Variable<T> &variable, const T *data)
{
    PutDeferredCommon(variable, data);
    PerformPuts();
}

template <>
void RefactorWriter::PutDeferredCommon<std::string>(Variable<std::string> &variable,
                                                    const std::string *data)
{
    m_DataEngine->Put(variable, data, Mode::Sync);
}

template <>
void RefactorWriter::PutDeferredCommon<double>(Variable<double> &variable, const double *data)
{
    PutRefactored(variable, data);
}

template <>
void RefactorWriter::PutDeferredCommon<float>(Variable<float> &variable, const float *data)
{
    PutRefactored(variable, data);
}

template <class T>
void RefactorWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    m_DataEngine->Put(variable, data, Mode::Deferred);
}

void RefactorWriter::CallbackFromRefactorOperator(const char *dataIn, const char *bufferOut,
                                                  const size_t headerSize, const size_t bufferSize)
{
    /* This is a static function */
    auto it = m_DataPtrToMDRVariable.find(dataIn);
    if (it == m_DataPtrToMDRVariable.end())
    {
        helper::Throw<std::invalid_argument>(
            "Core", "RefactorWriter", "CallbackFromRefactorOperator",
            "unrecognized data pointer was returned from RefactorMDR operator");
    }

    RefactorWriter *object = it->second.first;
    std::string varName = it->second.second;

    auto *var0 = object->m_MDRIO->InquireVariable<uint8_t>(varName);
    if (!var0)
    {
        var0 = &object->m_MDRIO->DefineVariable<uint8_t>(varName, {}, {}, {UnknownDim});
    }

    std::cout << "=== RefactorWriter::Put Refactored size = " << bufferSize
              << " HeaderSize = " << headerSize << std::endl;

    var0->SetSelection({{}, {headerSize}});
    object->m_MDREngine->Put(*var0, (uint8_t *)bufferOut, Mode::Sync);
    m_DataPtrToMDRVariable.erase(it);
}

/* This function should only be called for float and double type variables */
template <class T>
void RefactorWriter::PutRefactored(Variable<T> &variable, const T *data)
{
    if (variable.m_SingleValue)
    {
        m_DataEngine->Put(variable, data);
        return;
    }

    m_DataPtrToMDRVariable.emplace((char *)data, std::make_pair(this, variable.m_Name));

    variable.RemoveOperations();
    variable.AddOperation(m_RefactorOperator);
    m_DataEngine->Put(variable, data, Mode::Deferred);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_REFACTORWRITER_TCC_ */
