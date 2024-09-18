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

/* This function should only be called for float and double type variables */
template <class T>
void RefactorWriter::PutRefactored(Variable<T> &variable, const T *data)
{
    if (variable.m_SingleValue)
    {
        m_DataEngine->Put(variable, data);
        return;
    }

    m_DataEngine->Put(variable, data, Mode::Deferred);

    auto *var0 = m_MDRIO->InquireVariable<uint8_t>(variable.m_Name);
    if (!var0)
    {
        var0 = &m_MDRIO->DefineVariable<uint8_t>(variable.m_Name, {}, {}, {UnknownDim});
    }

    size_t ElemCount = helper::GetTotalSize(variable.m_Shape);
    size_t ElemSize = (variable.m_Type == DataType::Double ? sizeof(double) : sizeof(float));
    size_t DimCount = variable.m_Shape.size();
    size_t *Count = variable.m_Shape.data();
    size_t AllocSize = m_RefactorOperator->GetEstimatedSize(ElemCount, ElemSize, DimCount, Count);
    MemorySpace MemSpace = variable.GetMemorySpace(data);

    m_RefData.Reset();
    m_RefData.Allocate(AllocSize, ElemSize);
    char *RefactoredData = (char *)m_RefData.GetPtr(0);

    size_t RefactoredSize = m_RefactorOperator->Operate(
        (const char *)data, variable.m_Start, variable.m_Count, variable.m_Type, RefactoredData);
    size_t HeaderSize = m_RefactorOperator->GetHeaderSize();

    std::cout << "=== RefactorWriter::Put Refactored size = " << RefactoredSize
              << " HeaderSize = " << HeaderSize << std::endl;

    // if the operator was not applied
    if (RefactoredSize == 0)
    {
        RefactoredSize =
            helper::CopyMemoryWithOpHeader((const char *)data, variable.m_Count, variable.m_Type,
                                           RefactoredData, HeaderSize, MemSpace);
    }

    var0->SetSelection({{}, {HeaderSize}});
    m_MDREngine->Put(*var0, (uint8_t *)RefactoredData, Mode::Sync);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_REFACTORWRITER_TCC_ */
