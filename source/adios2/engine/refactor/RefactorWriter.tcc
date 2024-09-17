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

/* This function should only be called for float and double type variables */
template <class T>
void RefactorWriter::PutRefactored(Variable<T> &variable, const T *data)
{
    if (variable.m_SingleValue)
    {
        m_DataEngine->Put(variable, data);
    }

    m_DataEngine->Put(variable, data, Mode::Deferred);

    // std::shared_ptr<adios2::core::Operator> tr = nullptr;

    core::Variable<std::string> *var0 = m_MDRIO->InquireVariable<std::string>(variable.m_Name);
    if (!var0)
    {
        var0 = &m_MDRIO->DefineVariable<std::string>(variable.m_Name);
    }
    m_MDREngine->Put(*var0, ToString(variable.m_Type), Mode::Sync);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_REFACTORWRITER_TCC_ */
