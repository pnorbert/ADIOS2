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
#include "adios2/engine/bp5/BP5Reader.h"
#include "adios2/operator/refactor/RefactorMDR.h"
#include "adios2/toolkit/format/bp5/BP5Deserializer.h"

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

    if ((variable.m_SelectionType == adios2::SelectionType::BoundingBox) &&
        ((variable.m_ShapeID == ShapeID::GlobalArray) ||
         (variable.m_ShapeID == ShapeID::JoinedArray)))
    {
        m_DataEngine->Get(variable, data, Mode::Sync);
        return;
    }
    else if ((variable.m_SelectionType == adios2::SelectionType::WriteBlock) ||
             (variable.m_ShapeID == ShapeID::LocalArray))
    {
        if (variable.m_SelectionType == adios2::SelectionType::BoundingBox)
        {
            // Req.Start = variable.m_Start;
            // Req.Count = variable.m_Count;
            throw std::invalid_argument(
                "Refactor Engine Get does not support Local Array combined with selection");
        }

        auto e = reinterpret_cast<BP5Reader *>(m_DataEngine);

        Accuracy a{0.01, Linf_norm, false};
        variable.SetAccuracy(a);

        /* Prepare a BP5ArrayRequest like in BP5Deserializer::QueueGetSingle() */
        // format::BP5Deserializer::BP5VarRec *VarRec =
        // e->m_BP5Deserializer->VarByKey[&variable]; MemorySpace MemSpace =
        // variable.GetMemorySpace(data); format::BP5Deserializer::BP5ArrayRequest Req;
        var0->SetBlockSelection(variable.m_BlockID);
        std::vector<uint8_t> mdrblock;
        m_MDREngine->Get(*var0, mdrblock, Mode::Sync);
        m_RefactorOperator->SetAccuracy(variable.GetAccuracyRequested());
        auto *mdr = reinterpret_cast<refactor::RefactorMDR *>(m_RefactorOperator.get());
        refactor::RefactorMDR::RMD_V1 rmd;
        rmd = mdr->Reconstruct_ProcessMetadata_V1((char *)mdrblock.data(), mdrblock.size());

        if (!rmd.isRefactored)
        {
            /* TODO: Need to get the raw data as is from the raw data block, from offset
             * rmd.metadataSize */
            return;
        }

        std::cout << "RefactorReader::Get " << variable.m_Name
                  << " bytes needed for reconstruction = " << rmd.requiredDataSize << std::endl;

        /* Determine how many bytes of the data block we need to read in to reconstruct to given
         * accuracy */
        size_t bytesNeeded = 0;
        // size_t blockSize = variable.m_BlocksInfo;

        // format::BP5Deserializer::BP5VarRec *VarRec = e->m_BP5Deserializer->VarByKey[&variable];
    }
    else
    {
        std::cout << "Missed get type " << variable.m_SelectionType << " shape "
                  << variable.m_ShapeID << std::endl;
    }

    /*
        size_t ElemCount = helper::GetTotalSize(variable.m_Shape);
        size_t ElemSize = (variable.m_Type == DataType::Double ? sizeof(double) : sizeof(float));
        size_t DimCount = variable.m_Shape.size();
        size_t *Count = variable.m_Shape.data();
        size_t AllocSize = m_RefactorOperator->GetEstimatedSize(ElemCount, ElemSize, DimCount,
       Count); MemorySpace MemSpace = variable.GetMemorySpace(data);

        m_RefData.Reset();
        m_RefData.Allocate(AllocSize, ElemSize);
        char *RefactoredData = (char *)m_RefData.GetPtr(0);

        size_t RefactoredSize = m_RefactorOperator->Operate(
            (const char *)data, variable.m_Start, variable.m_Count, variable.m_Type,
       RefactoredData); size_t HeaderSize = m_RefactorOperator->GetHeaderSize();

        std::cout << "=== RefactorWriter::Put Refactored size = " << RefactoredSize
                  << " HeaderSize = " << HeaderSize << std::endl;

        // if the operator was not applied
        if (RefactoredSize == 0)
        {
            RefactoredSize =
                helper::CopyMemoryWithOpHeader((const char *)data, variable.m_Count,
       variable.m_Type, RefactoredData, HeaderSize, MemSpace);
        }

        var0->SetSelection({{}, {HeaderSize}});
        m_MDREngine->Put(*var0, (uint8_t *)RefactoredData, Mode::Sync);

        m_DataEngine->Get(variable, data, Mode::Sync);
    */
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_REFACTORREADER_TCC_
