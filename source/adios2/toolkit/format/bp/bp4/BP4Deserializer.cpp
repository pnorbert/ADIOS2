/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Deserializer.cpp
 *
 *  Created on: Aug 1, 2018
 *  Author: William F Godoy godoywf@ornl.gov
 *          Lipeng Wan wanl@ornl.gov
 *          Norbert Podhorszki pnb@ornl.gov
 */

#include "BP4Deserializer.h"
#include "BP4Deserializer.tcc"

#include <future>
#include <unordered_set>
#include <vector>

#include <iostream>

#include "adios2/helper/adiosFunctions.h" //helper::ReadValue<T>

#ifdef _WIN32
#pragma warning(disable : 4503) // Windows complains about SubFileInfoMap levels
#endif

namespace adios2
{
namespace format
{

std::mutex BP4Deserializer::m_Mutex;

BP4Deserializer::BP4Deserializer(helper::Comm const &comm)
: BP4Base(comm), BPBase(comm), m_Minifooter(4)
{
}

size_t BP4Deserializer::ParseMetadata(const BufferSTL &bufferSTL,
                                      core::Engine &engine,
                                      const bool firstStep)
{
    const size_t oldSteps = (firstStep ? 0 : m_MetadataSet.StepsCount);
    size_t allSteps = m_MetadataIndexTable[0].size();
    m_MetadataSet.StepsCount = allSteps;
    m_MetadataSet.CurrentStep = allSteps - 1;
    size_t lastposition = 0;
    /* parse the metadata step by step using the pointers saved in the metadata
    index table */
    for (size_t i = oldSteps; i < allSteps; i++)
    {
        if (m_Minifooter.IsMetadataUnsorted)
        {
            ParseUnsortedIndexPerStep(bufferSTL, engine, 0, i + 1);
        }
        else
        {
            ParsePGIndexPerStep(bufferSTL, engine.m_IO.m_HostLanguage, 0,
                                i + 1);
            ParseVariablesIndexPerStep(bufferSTL, engine, 0, i + 1);
            ParseAttributesIndexPerStep(bufferSTL, engine, 0, i + 1);
        }
        lastposition = m_MetadataIndexTable[0][i + 1][3];
    }
    return lastposition;
}

void BP4Deserializer::ParseMetadataIndex(BufferSTL &bufferSTL,
                                         const size_t absoluteStartPos,
                                         const bool hasHeader,
                                         const bool oneStepOnly)
{
    const auto &buffer = bufferSTL.m_Buffer;
    size_t &position = bufferSTL.m_Position;

    if (hasHeader)
    {
        // Read header (64 bytes)
        // long version string
        position = m_VersionTagPosition;
        m_Minifooter.VersionTag.assign(&buffer[position], m_VersionTagLength);

        position = m_EndianFlagPosition;
        const uint8_t endianness = helper::ReadValue<uint8_t>(buffer, position);
        m_Minifooter.IsLittleEndian = (endianness == 0) ? true : false;
#ifndef ADIOS2_HAVE_ENDIAN_REVERSE
        if (helper::IsLittleEndian() != m_Minifooter.IsLittleEndian)
        {
            throw std::runtime_error(
                "ERROR: reader found BigEndian bp file, "
                "this version of ADIOS2 wasn't compiled "
                "with the cmake flag -DADIOS2_USE_Endian_Reverse=ON "
                "explicitly, in call to Open\n");
        }
#endif

        // This has no flag in BP4 header. Always true
        m_Minifooter.HasSubFiles = true;

        // BP version
        position = m_BPVersionPosition;
        m_Minifooter.Version = helper::ReadValue<uint8_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        if (m_Minifooter.Version != 4)
        {
            throw std::runtime_error(
                "ERROR: ADIOS2 BP4 Engine only supports bp format "
                "version 4, found " +
                std::to_string(m_Minifooter.Version) + " version \n");
        }

        // Writer active flag
        position = m_ActiveFlagPosition;
        const char activeChar = helper::ReadValue<uint8_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        m_WriterIsActive = (activeChar == '\1' ? true : false);

        /* Metadata sorted flag:
         * 0 means original sorted and
         * 1 means the new unsorted metadata
         */

        position = m_UnsortedFlagPosition;
        const char unsortedChar = helper::ReadValue<uint8_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        m_Minifooter.IsMetadataUnsorted = (unsortedChar == '\1' ? true : false);

        // move position to first row
        position = 64;
    }

    // Read each record now
    do
    {
        std::vector<uint64_t> ptrs;
        const uint64_t currentStep = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        const uint64_t mpiRank = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        const uint64_t pgIndexStart = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        ptrs.push_back(pgIndexStart - absoluteStartPos);
        const uint64_t variablesIndexStart = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        ptrs.push_back(variablesIndexStart - absoluteStartPos);
        const uint64_t attributesIndexStart = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        ptrs.push_back(attributesIndexStart - absoluteStartPos);
        const uint64_t currentStepEndPos = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        ptrs.push_back(currentStepEndPos - absoluteStartPos);
        const uint64_t currentTimeStamp = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        ptrs.push_back(currentTimeStamp);
        m_MetadataIndexTable[mpiRank][currentStep] = ptrs;
        position += 8;
    } while (!oneStepOnly && position < buffer.size());
}

const helper::BlockOperationInfo &BP4Deserializer::InitPostOperatorBlockData(
    const std::vector<helper::BlockOperationInfo> &blockOperationsInfo) const
{
    size_t index = 0;
    for (const helper::BlockOperationInfo &blockOperationInfo :
         blockOperationsInfo)
    {
        const std::string type = blockOperationInfo.Info.at("Type");
        if (m_TransformTypes.count(type) == 1)
        {
            break;
        }
        ++index;
    }
    return blockOperationsInfo.at(index);
}

void BP4Deserializer::ParseUnsortedIndexPerStep(const BufferSTL &bufferSTL,
                                                core::Engine &engine,
                                                size_t submetadatafileId,
                                                size_t step)
{
    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_MetadataIndexTable[submetadatafileId][step][0];
    const std::string &hostLanguage = engine.m_IO.m_HostLanguage;

    // read length of entire index for the step
    uint64_t indexLen = helper::ReadValue<uint64_t>(
        buffer, position, m_Minifooter.IsLittleEndian);

    // read Step of index
    uint32_t indexStep = helper::ReadValue<uint32_t>(
        buffer, position, m_Minifooter.IsLittleEndian);

    if (indexStep != step)
    {
        throw std::runtime_error(
            "BP4Deserializer::ParseUnsortedIndexPerStep() processing step " +
            std::to_string(step) + " encountered step " +
            std::to_string(indexStep) + " at position " +
            std::to_string(m_MetadataIndexTable[submetadatafileId][step][0]));
    }

    size_t indexEndPosition = position + indexLen;

    while (position < indexEndPosition)
    {
        // we have one PG to process at a time
        m_MetadataSet.DataPGCount += 1;

        // read rank of PG
        uint32_t pgRank = helper::ReadValue<uint32_t>(
            buffer, position, m_Minifooter.IsLittleEndian);

        // read length of index block which is not used, only for moving the
        // pointer
        size_t idxBlockLen = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);

        // length of PG, Var and Attr index in the block
        size_t pgIdxLen = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        size_t varIdxLen = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        size_t attrIdxLen = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);

        // read PG count, which is always 1 here
        helper::ReadValue<uint64_t>(buffer, position,
                                    m_Minifooter.IsLittleEndian);

        /* Sanity check of lengths */
        if (pgIdxLen + varIdxLen + attrIdxLen + 44 != idxBlockLen)
        {
            throw std::runtime_error(
                "BP4Deserializer::ParseUnsortedIndexPerStep() processing "
                "step " +
                std::to_string(step) + " at position " +
                std::to_string(position - 44) +
                +" processing index block has invalid lengths ");
        }

        ProcessGroupIndex index = ReadProcessGroupIndexHeader(
            buffer, position, m_Minifooter.IsLittleEndian);
        if (index.IsColumnMajor == 'y')
        {
            m_IsRowMajor = false;
        }
        if (m_IsRowMajor != helper::IsRowMajor(hostLanguage))
        {
            m_ReverseDimensions = true;
        }

        /* Parse Variables */

        auto lf_ReadElementIndexPerStep = [&](core::Engine &engine,
                                              const std::vector<char> &buffer,
                                              size_t position, size_t step) {
            const ElementIndexHeader header = ReadElementIndexHeader(
                buffer, position, m_Minifooter.IsLittleEndian);

            switch (header.DataType)
            {

#define make_case(T)                                                           \
    case (TypeTraits<T>::type_enum):                                           \
    {                                                                          \
        DefineVariableInEngineIOPerStep<T>(header, engine, buffer, position,   \
                                           step);                              \
        break;                                                                 \
    }
                ADIOS2_FOREACH_STDTYPE_1ARG(make_case)
#undef make_case

            } // end switch
        };

        const uint32_t varCount = helper::ReadValue<uint32_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        const uint64_t varLength = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);

        const size_t varStartPosition = position;
        size_t varLocalPosition = 0;

        /* FIXME: multi-threaded processing does not work here
         * because DefineVariable may be called from several threads
         */
        while (varLocalPosition < varLength)
        {
            lf_ReadElementIndexPerStep(engine, buffer, position, step);

            const size_t elementIndexSize =
                static_cast<size_t>(helper::ReadValue<uint32_t>(
                    buffer, position, m_Minifooter.IsLittleEndian));
            position += elementIndexSize;
            varLocalPosition = position - varStartPosition;
        }

        /* Parse the attributes index at each step */

        auto lf_ReadElementIndex = [&](core::Engine &engine,
                                       const std::vector<char> &buffer,
                                       size_t position) {
            const ElementIndexHeader header = ReadElementIndexHeader(
                buffer, position, m_Minifooter.IsLittleEndian);

            switch (header.DataType)
            {

#define make_case(T)                                                           \
    case (TypeTraits<T>::type_enum):                                           \
    {                                                                          \
        DefineAttributeInEngineIO<T>(header, engine, buffer, position);        \
        break;                                                                 \
    }
                ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(make_case)
#undef make_case
            case (type_string_array):
            {
                DefineAttributeInEngineIO<std::string>(header, engine, buffer,
                                                       position);
                break;
            }

            } // end switch
        };

        const uint32_t attrCount = helper::ReadValue<uint32_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        const uint64_t attrLength = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);

        const size_t attrStartPosition = position;
        size_t attrLocalPosition = 0;

        // Read sequentially
        while (attrLocalPosition < attrLength)
        {
            lf_ReadElementIndex(engine, buffer, position);
            const size_t elementIndexSize =
                static_cast<size_t>(helper::ReadValue<uint32_t>(
                    buffer, position, m_Minifooter.IsLittleEndian));
            position += elementIndexSize;
            attrLocalPosition = position - attrStartPosition;
        }
    } /* finished processing one PG */
}

/*
 * ====================== END OF Unsorted Metadata Parsing =================
 */

void BP4Deserializer::ParsePGIndexPerStep(const BufferSTL &bufferSTL,
                                          const std::string hostLanguage,
                                          size_t submetadatafileId, size_t step)
{
    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_MetadataIndexTable[submetadatafileId][step][0];
    // std::cout << step << ", " << position << std::endl;
    m_MetadataSet.DataPGCount =
        m_MetadataSet.DataPGCount +
        helper::ReadValue<uint64_t>(buffer, position,
                                    m_Minifooter.IsLittleEndian);

    // read length of pg index which is not used, only for moving the pointer
    helper::ReadValue<uint64_t>(buffer, position, m_Minifooter.IsLittleEndian);

    ProcessGroupIndex index = ReadProcessGroupIndexHeader(
        buffer, position, m_Minifooter.IsLittleEndian);
    if (index.IsColumnMajor == 'y')
    {
        m_IsRowMajor = false;
    }
    if (m_IsRowMajor != helper::IsRowMajor(hostLanguage))
    {
        m_ReverseDimensions = true;
    }
}

void BP4Deserializer::ParseVariablesIndexPerStep(const BufferSTL &bufferSTL,
                                                 core::Engine &engine,
                                                 size_t submetadatafileId,
                                                 size_t step)
{
    auto lf_ReadElementIndexPerStep = [&](core::Engine &engine,
                                          const std::vector<char> &buffer,
                                          size_t position, size_t step) {
        const ElementIndexHeader header = ReadElementIndexHeader(
            buffer, position, m_Minifooter.IsLittleEndian);

        switch (header.DataType)
        {

#define make_case(T)                                                           \
    case (TypeTraits<T>::type_enum):                                           \
    {                                                                          \
        DefineVariableInEngineIOPerStep<T>(header, engine, buffer, position,   \
                                           step);                              \
        break;                                                                 \
    }
            ADIOS2_FOREACH_STDTYPE_1ARG(make_case)
#undef make_case

        } // end switch
    };

    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_MetadataIndexTable[submetadatafileId][step][1];

    const uint32_t count = helper::ReadValue<uint32_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    const uint64_t length = helper::ReadValue<uint64_t>(
        buffer, position, m_Minifooter.IsLittleEndian);

    const size_t startPosition = position;
    size_t localPosition = 0;

    /* FIXME: multi-threaded processing does not work here
     * because DefineVariable may be called from several threads
     */
    /*if (m_Threads == 1)*/
    {
        while (localPosition < length)
        {
            lf_ReadElementIndexPerStep(engine, buffer, position, step);

            const size_t elementIndexSize =
                static_cast<size_t>(helper::ReadValue<uint32_t>(
                    buffer, position, m_Minifooter.IsLittleEndian));
            position += elementIndexSize;
            localPosition = position - startPosition;
        }
        return;
    }
    /*
        // threads for reading Variables
        std::vector<std::future<void>> asyncs(m_Threads);
        std::vector<size_t> asyncPositions(m_Threads);

        bool launched = false;

        while (localPosition < length)
        {
            // extract async positions
            for (unsigned int t = 0; t < m_Threads; ++t)
            {
                asyncPositions[t] = position;
                const size_t elementIndexSize =
                    static_cast<size_t>(helper::ReadValue<uint32_t>(
                        buffer, position, m_Minifooter.IsLittleEndian));
                position += elementIndexSize;
                localPosition = position - startPosition;

                if (launched)
                {
                    asyncs[t].get();
                }

                if (localPosition <= length)
                {
                    asyncs[t] =
                        std::async(std::launch::async,
       lf_ReadElementIndexPerStep, std::ref(engine), std::ref(buffer),
                                   asyncPositions[t], step);
                }
            }
            launched = true;
        }

        for (auto &async : asyncs)
        {
            if (async.valid())
            {
                async.wait();
            }
        }
    */
}

/* Parse the attributes index at each step */
void BP4Deserializer::ParseAttributesIndexPerStep(const BufferSTL &bufferSTL,
                                                  core::Engine &engine,
                                                  size_t submetadatafileId,
                                                  size_t step)
{
    auto lf_ReadElementIndex = [&](core::Engine &engine,
                                   const std::vector<char> &buffer,
                                   size_t position) {
        const ElementIndexHeader header = ReadElementIndexHeader(
            buffer, position, m_Minifooter.IsLittleEndian);

        switch (header.DataType)
        {

#define make_case(T)                                                           \
    case (TypeTraits<T>::type_enum):                                           \
    {                                                                          \
        DefineAttributeInEngineIO<T>(header, engine, buffer, position);        \
        break;                                                                 \
    }
            ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(make_case)
#undef make_case
        case (type_string_array):
        {
            DefineAttributeInEngineIO<std::string>(header, engine, buffer,
                                                   position);
            break;
        }

        } // end switch
    };

    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_MetadataIndexTable[submetadatafileId][step][2];

    const uint32_t count = helper::ReadValue<uint32_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    const uint64_t length = helper::ReadValue<uint64_t>(
        buffer, position, m_Minifooter.IsLittleEndian);

    const size_t startPosition = position;
    size_t localPosition = 0;

    // Read sequentially
    while (localPosition < length)
    {
        lf_ReadElementIndex(engine, buffer, position);
        const size_t elementIndexSize =
            static_cast<size_t>(helper::ReadValue<uint32_t>(
                buffer, position, m_Minifooter.IsLittleEndian));
        position += elementIndexSize;
        localPosition = position - startPosition;
    }
}

std::map<std::string, helper::SubFileInfoMap>
BP4Deserializer::PerformGetsVariablesSubFileInfo(core::IO &io)
{
    if (m_DeferredVariablesMap.empty())
    {
        return m_DeferredVariablesMap;
    }

    for (auto &subFileInfoPair : m_DeferredVariablesMap)
    {
        const std::string variableName(subFileInfoPair.first);
        const DataType type(io.InquireVariableType(variableName));

        if (type == DataType::Compound)
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        subFileInfoPair.second =                                               \
            GetSubFileInfo(*io.InquireVariable<T>(variableName));              \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
    return m_DeferredVariablesMap;
}

void BP4Deserializer::ClipMemory(const std::string &variableName, core::IO &io,
                                 const std::vector<char> &contiguousMemory,
                                 const Box<Dims> &blockBox,
                                 const Box<Dims> &intersectionBox) const
{
    const DataType type(io.InquireVariableType(variableName));

    if (type == DataType::Compound)
    {
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        core::Variable<T> *variable = io.InquireVariable<T>(variableName);     \
        if (variable != nullptr)                                               \
        {                                                                      \
            helper::ClipContiguousMemory(variable->m_Data, variable->m_Start,  \
                                         variable->m_Count, contiguousMemory,  \
                                         blockBox, intersectionBox,            \
                                         m_IsRowMajor, m_ReverseDimensions);   \
        }                                                                      \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
}

bool BP4Deserializer::ReadActiveFlag(std::vector<char> &buffer)
{
    if (buffer.size() < m_ActiveFlagPosition)
    {
        throw std::runtime_error("BP4Deserializer::ReadActiveFlag() is called "
                                 "with a buffer smaller than required");
    }
    // Writer active flag
    size_t position = m_ActiveFlagPosition;
    const char activeChar = helper::ReadValue<uint8_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    m_WriterIsActive = (activeChar == '\1' ? true : false);
    return m_WriterIsActive;
}

#define declare_template_instantiation(T)                                      \
    template void BP4Deserializer::GetSyncVariableDataFromStream(              \
        core::Variable<T> &, BufferSTL &) const;                               \
                                                                               \
    template typename core::Variable<T>::Info &                                \
    BP4Deserializer::InitVariableBlockInfo(core::Variable<T> &, T *) const;    \
                                                                               \
    template void BP4Deserializer::SetVariableBlockInfo(                       \
        core::Variable<T> &, typename core::Variable<T>::Info &) const;        \
                                                                               \
    template void BP4Deserializer::ClipContiguousMemory<T>(                    \
        typename core::Variable<T>::Info &, const std::vector<char> &,         \
        const Box<Dims> &, const Box<Dims> &) const;                           \
                                                                               \
    template void BP4Deserializer::GetValueFromMetadata(                       \
        core::Variable<T> &variable, T *) const;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
                                                                               \
    template std::map<std::string, helper::SubFileInfoMap>                     \
    BP4Deserializer::GetSyncVariableSubFileInfo(const core::Variable<T> &)     \
        const;                                                                 \
                                                                               \
    template void BP4Deserializer::GetDeferredVariable(core::Variable<T> &,    \
                                                       T *);                   \
                                                                               \
    template helper::SubFileInfoMap BP4Deserializer::GetSubFileInfo(           \
        const core::Variable<T> &) const;                                      \
                                                                               \
    template std::map<size_t, std::vector<typename core::Variable<T>::Info>>   \
    BP4Deserializer::AllStepsBlocksInfo(const core::Variable<T> &) const;      \
                                                                               \
    template std::vector<std::vector<typename core::Variable<T>::Info>>        \
    BP4Deserializer::AllRelativeStepsBlocksInfo(const core::Variable<T> &)     \
        const;                                                                 \
                                                                               \
    template std::vector<typename core::Variable<T>::Info>                     \
    BP4Deserializer::BlocksInfo(const core::Variable<T> &, const size_t)       \
        const;                                                                 \
                                                                               \
    template void BP4Deserializer::PreDataRead(                                \
        core::Variable<T> &, typename core::Variable<T>::Info &,               \
        const helper::SubStreamBoxInfo &, char *&, size_t &, size_t &,         \
        const size_t);                                                         \
                                                                               \
    template void BP4Deserializer::PostDataRead(                               \
        core::Variable<T> &, typename core::Variable<T>::Info &,               \
        const helper::SubStreamBoxInfo &, const bool, const size_t);

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2
