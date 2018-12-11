/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StagingReader.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#include "StagingReader.h"
#include "StagingReader.tcc"

#include "adios2/helper/adiosFunctions.h" // CSVToVector
#include "adios2/toolkit/transport/file/FileFStream.h"

#include <iostream>

#include <zmq.h>

namespace adios2
{
namespace core
{
namespace engine
{

StagingReader::StagingReader(IO &io, const std::string &name, const Mode mode,
                             MPI_Comm mpiComm)
: Engine("StagingReader", io, name, mode, mpiComm),
  m_DataManSerializer(helper::IsRowMajor(io.m_HostLanguage), true,
                      helper::IsLittleEndian())
{
    m_DataTransport = std::make_shared<transportman::StagingMan>(
        mpiComm, Mode::Read, m_Timeout, 1e9);
    m_MetadataTransport = std::make_shared<transportman::StagingMan>(
        mpiComm, Mode::Read, m_Timeout, 1e6);
    m_EndMessage = " in call to IO Open StagingReader " + m_Name + "\n";
    MPI_Comm_rank(mpiComm, &m_MpiRank);
    Init();
    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " Open(" << m_Name
                  << ") in constructor." << std::endl;
    }
}

StagingReader::~StagingReader()
{
    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " deconstructor on "
                  << m_Name << "\n";
    }
}

StepStatus StagingReader::BeginStep(const StepMode stepMode,
                                    const float timeoutSeconds)
{

    ++m_CurrentStep;

    if (m_CurrentStep == 0)
    {
        MetadataReqThread();
        m_DataManSerializer.GetAttributes(m_IO);
    }

    if (m_MetadataReqThread != nullptr)
    {
        if (m_MetadataReqThread->joinable())
        {
            m_MetadataReqThread->join();
        }
    }

    m_MetaDataMap = m_DataManSerializer.GetMetaData();

    size_t maxStep = std::numeric_limits<size_t>::min();
    size_t minStep = std::numeric_limits<size_t>::max();

    for (auto &i : m_MetaDataMap)
    {
        if (i.first > maxStep)
        {
            maxStep = i.first;
        }
        if (i.first < minStep)
        {
            minStep = i.first;
        }
    }

    if (stepMode == StepMode::NextAvailable)
    {
        m_CurrentStep = minStep;
    }
    else if (stepMode == StepMode::LatestAvailable)
    {
        m_CurrentStep = maxStep;
    }
    else
    {
        throw(std::invalid_argument(
            "[StagingReader::BeginStep] Step mode is not supported!"));
    }

    std::shared_ptr<std::vector<format::DataManSerializer::DataManVar>> vars =
        nullptr;
    auto currentStepIt = m_MetaDataMap.find(m_CurrentStep);
    if (currentStepIt != m_MetaDataMap.end())
    {
        vars = currentStepIt->second;
    }

    if (vars != nullptr)
    {

        for (const auto &i : *vars)
        {
            if (i.step == m_CurrentStep)
            {
                if (i.type == "compound")
                {
                    throw("Compound type is not supported yet.");
                }
#define declare_type(T)                                                        \
    else if (i.type == helper::GetType<T>())                                   \
    {                                                                          \
        CheckIOVariable<T>(i.name, i.shape, i.start, i.count);                 \
    }
                ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
                else
                {
                    throw("Unknown type caught in "
                          "DataManReader::BeginStepSubscribe.");
                }
            }
        }
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Reader " << m_MpiRank
                  << "   BeginStep() new step " << m_CurrentStep
                  << ". The minimum step is " << minStep
                  << ". The maximum step is " << maxStep << "\n";
    }

    return StepStatus::OK;
}

void StagingReader::PerformGets()
{

    auto requests = m_DataManSerializer.GetDeferredRequest();
    if (m_Verbosity >= 10)
    {
        std::cout << "Staging Reader " << m_MpiRank
                  << " PerformGets() DeferredRequest from serializer, size = "
                  << requests->size() << std::endl;
    }
    for (const auto &i : *requests)
    {
        auto reply = m_DataTransport->Request(i.second, i.first);
        if (reply == nullptr)
        {
            std::cout << "Step " << m_CurrentStep
                      << " received empty data package from writer " << i.first
                      << ". This may be caused by a network failure. Data for "
                         "this step may not be correct but application will "
                         "continue running."
                      << std::endl;
        }
        else
        {
            m_DataManSerializer.PutPack(reply);
        }
    }

    auto varsVec = m_MetaDataMap.find(m_CurrentStep);
    if (varsVec == m_MetaDataMap.end())
    {
        return;
    }
    if (varsVec->second == nullptr)
    {
        return;
    }

    for (const auto &req : m_DeferredRequests)
    {

        if (req.type == "compound")
        {
            throw("Compound type is not supported yet.");
        }
#define declare_type(T)                                                        \
    else if (req.type == helper::GetType<T>())                                 \
    {                                                                          \
        m_DataManSerializer.GetVar(reinterpret_cast<T *>(req.data),            \
                                   req.variable, req.start, req.count,         \
                                   req.step);                                  \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " PerformGets()\n";
    }
}

size_t StagingReader::CurrentStep() const { return m_CurrentStep; }

void StagingReader::EndStep()
{
    PerformGets();
    m_DataManSerializer.Erase(CurrentStep());
    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " EndStep()\n";
    }
    m_MetadataReqThread =
        std::make_shared<std::thread>(&StagingReader::MetadataReqThread, this);
}

// PRIVATE

#define declare_type(T)                                                        \
    void StagingReader::DoGetSync(Variable<T> &variable, T *data)              \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void StagingReader::DoGetDeferred(Variable<T> &variable, T *data)          \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void StagingReader::Init()
{
    InitParameters();
    Handshake();
    InitTransports();
}

void StagingReader::InitParameters()
{
    for (const auto &pair : m_IO.m_Parameters)
    {
        std::string key(pair.first);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        std::string value(pair.second);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (key == "verbose")
        {
            m_Verbosity = std::stoi(value);
            if (m_DebugMode)
            {
                if (m_Verbosity < 0 || m_Verbosity > 5)
                    throw std::invalid_argument(
                        "ERROR: Method verbose argument must be an "
                        "integer in the range [0,5], in call to "
                        "Open or Engine constructor\n");
            }
        }
    }
}

void StagingReader::InitTransports()
{
    m_MetadataTransport->OpenTransport(m_FullMetadataAddress);
}

void StagingReader::Handshake()
{
    transport::FileFStream ipstream(m_MPIComm, m_DebugMode);
    ipstream.Open(".StagingHandshake", Mode::Read);
    auto size = ipstream.GetSize();
    std::vector<char> address(size);
    ipstream.Read(address.data(), size);
    ipstream.Close();
    m_FullMetadataAddress = std::string(address.begin(), address.end());
}

void StagingReader::MetadataReqThread()
{

    std::shared_ptr<std::vector<char>> reply = nullptr;
    if (m_MpiRank == 0)
    {
        std::vector<char> request(1, 'M');

        auto start_time = std::chrono::system_clock::now();
        while (reply == nullptr)
        {
            auto now_time = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                now_time - start_time);
            if (duration.count() > m_Timeout)
            {
                break;
            }
            reply = m_MetadataTransport->Request(request);
        }
    }
    else
    {
        reply = std::make_shared<std::vector<char>>();
    }

    if (m_Verbosity >= 100)
    {
        if (m_MpiRank == 0)
        {
            std::cout << "StagingReader::MetadataReqThread Cbor data, size =  "
                      << reply->size() << std::endl;
            std::cout << "========================" << std::endl;
            for (auto i : *reply)
            {
                std::cout << i;
            }
            std::cout << std::endl << "========================" << std::endl;
        }
    }

    m_DataManSerializer.PutAggregatedMetadata(m_MPIComm, reply);
}

void StagingReader::DoClose(const int transportIndex)
{
    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " Close(" << m_Name
                  << ")\n";
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
