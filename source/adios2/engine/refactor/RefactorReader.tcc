/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * RefactorReader.tcc
 *
 *  Created on: Aug 04, 2021
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#ifndef ADIOS2_ENGINE_REFACTORREADER_TCC_
#define ADIOS2_ENGINE_REFACTORREADER_TCC_

#include "RefactorReader.h"

namespace adios2
{
namespace core
{
namespace engine
{
/*
   Get Deferred
*/

template <>
inline void RefactorReader::GetDeferredCommon(Variable<std::string> &variable, std::string *data)
{
    m_DataEngine->Get(variable, data, Mode::Sync);
}

template <>
void RefactorReader::GetDeferredCommon<double>(Variable<double> &variable, double *data)
{
    GetRefactored(variable, data);
}

template <>
void RefactorReader::GetDeferredCommon<float>(Variable<float> &variable, float *data)
{
    GetRefactored(variable, data);
}

template <class T>
void RefactorReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    m_DataEngine->Get(variable, data, Mode::Deferred);
}

/*
   Get Sync
*/
template <>
inline void RefactorReader::GetSyncCommon(Variable<std::string> &variable, std::string *data)
{
    m_DataEngine->Get(variable, data, Mode::Sync);
}

template <>
void RefactorReader::GetSyncCommon<double>(Variable<double> &variable, double *data)
{
    GetDeferredCommon(variable, data);
}

template <>
void RefactorReader::GetSyncCommon<float>(Variable<float> &variable, float *data)
{
    GetDeferredCommon(variable, data);
}

template <class T>
inline void RefactorReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    GetDeferredCommon(variable, data);
    PerformGets();
}

/*
   Get Refactored
*/

/* This function should only be called for float and double type variables */
template <class T>
void RefactorReader::GetRefactored(Variable<T> &variable, T *data)
{
    if (variable.m_SingleValue)
    {
        m_DataEngine->Get(variable, data, Mode::Sync);
        return;
    }

    auto *var0 = m_MDRIO->InquireVariable<uint8_t>(variable.m_Name);
    if (!var0)
    {
        m_DataEngine->Get(variable, data, Mode::Sync);
        return;
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

    m_DataEngine->Get(variable, data, Mode::Sync);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_REFACTORREADER_TCC_
