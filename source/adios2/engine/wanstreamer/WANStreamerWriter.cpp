/* SPDX-License-Identifier: Apache-2.0 */
#include "WANStreamerWriter.h"
#include "adios2/helper/adiosFunctions.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <type_traits>

namespace adios2::core::engine
{
namespace
{
void ReplaceAll(std::string &value, const std::string &from, const std::string &to)
{
    size_t offset = 0;
    while ((offset = value.find(from, offset)) != std::string::npos)
    {
        value.replace(offset, from.size(), to);
        offset += to.size();
    }
}
}

WANStreamerWriter::WANStreamerWriter(IO &io, const std::string &name, const Mode mode,
                                     helper::Comm comm)
: Engine("WANStreamerWriter", io, name, mode, std::move(comm)), m_StreamID(wanstreamer::RandomID())
{
    ReadParameters();
    LaunchConsumer(false);
    m_SenderThread = std::thread(&WANStreamerWriter::SenderLoop, this);
    m_IsOpen = true;
}

WANStreamerWriter::~WANStreamerWriter()
{
    if (!m_IsClosed)
    {
        try
        {
            DoClose();
        }
        catch (...)
        {
            {
                std::lock_guard<std::mutex> lock(m_ReplayMutex);
                m_CloseRequested = true;
                m_Closing = true;
            }
            m_ReplayChanged.notify_all();
            Disconnect();
            if (m_SenderThread.joinable())
                m_SenderThread.join();
        }
    }
}

void WANStreamerWriter::ReadParameters()
{
    m_ConnectionFile = helper::GetParameter("ConnectionFile", m_IO.m_Parameters, false, "");
    m_PrivateKey = helper::GetParameter("PrivateKey", m_IO.m_Parameters, false, "");
    helper::GetParameter(m_IO.m_Parameters, "LaunchMode", m_LaunchMode);
    m_LaunchCommand = helper::GetParameter("LaunchCommand", m_IO.m_Parameters, false, "");
    helper::GetParameter(m_IO.m_Parameters, "RetryIntervalSeconds", m_RetryIntervalSeconds);
    helper::GetParameter(m_IO.m_Parameters, "ReconnectTimeoutSeconds", m_ReconnectTimeoutSeconds);
    helper::GetParameter(m_IO.m_Parameters, "ReplayBufferSteps", m_ReplayBufferSteps);
    helper::GetParameter(m_IO.m_Parameters, "ReplayBufferBytes", m_ReplayBufferBytes);
    m_ConnectionFile = wanstreamer::RankPath(m_ConnectionFile, m_Comm.Rank(), m_Comm.Size());
    if (m_ConnectionFile.empty() || m_PrivateKey.empty())
        throw std::invalid_argument("WANStreamer writer requires ConnectionFile and PrivateKey");
    if (m_LaunchMode != "none" && m_LaunchMode != "system")
        throw std::invalid_argument("WANStreamer LaunchMode must be none or system");
    if (m_LaunchMode == "system" && m_LaunchCommand.empty())
        throw std::invalid_argument("WANStreamer LaunchMode=system requires LaunchCommand");
    if (m_RetryIntervalSeconds <= 0.0)
        throw std::invalid_argument("WANStreamer RetryIntervalSeconds must be positive");
    if (m_ReplayBufferSteps == 0)
        throw std::invalid_argument("WANStreamer ReplayBufferSteps must be positive");
}

void WANStreamerWriter::LaunchConsumer(const bool relaunch)
{
    if (m_LaunchMode != "system" || (!relaunch && m_Launched))
        return;
    std::string command = m_LaunchCommand;
    ReplaceAll(command, "{connection_file}", m_ConnectionFile);
    ReplaceAll(command, "{rank}", std::to_string(m_Comm.Rank()));
    ReplaceAll(command, "{stream_id}", m_StreamID);
    const int status = std::system((command + " &").c_str());
    if (status != 0)
        std::cerr << "WANStreamer: consumer launch command returned " << status << std::endl;
    m_Launched = true;
}

void WANStreamerWriter::Connect()
{
    const auto connection = wanstreamer::DecryptConnectionFile(m_ConnectionFile, m_PrivateKey);
    if (connection.value("id", "") != "wanstreamer" ||
        connection.value("protocol_version", 0) != wanstreamer::ProtocolVersion)
        throw std::runtime_error("WANStreamer rendezvous protocol mismatch");
    int socket = wanstreamer::ConnectTcp(connection.at("host").get<std::string>(),
                                         connection.at("port").get<int>());
    try
    {
        wanstreamer::AnswerChallenge(socket, m_PrivateKey);
        wanstreamer::SendJson(socket, {{"type", "hello"},
                                       {"protocol_version", wanstreamer::ProtocolVersion},
                                       {"stream_id", m_StreamID},
                                       {"rank", m_Comm.Rank()},
                                       {"consumer_id", connection.at("consumer_id")}});
        const auto resume = wanstreamer::ReceiveJson(socket);
        if (resume.value("type", "") != "resume" || resume.value("stream_id", "") != m_StreamID)
            throw std::runtime_error("WANStreamer consumer sent an invalid resume response");
        const int64_t committed = resume.value("committed_step", int64_t{-1});
        std::lock_guard<std::mutex> lock(m_ReplayMutex);
        while (!m_Replay.empty() && static_cast<int64_t>(m_Replay.front()->Step) <= committed)
        {
            m_ReplayBytes -= StepBytes(*m_Replay.front());
            m_Replay.pop_front();
        }
        m_ReplayChanged.notify_all();
    }
    catch (...)
    {
        wanstreamer::CloseSocket(socket);
        throw;
    }
    m_Socket = socket;
}

void WANStreamerWriter::Disconnect() noexcept { wanstreamer::CloseSocket(m_Socket); }

void WANStreamerWriter::SendStep(const wanstreamer::StepBuffer &step)
{
    nlohmann::json variables = nlohmann::json::array();
    for (const auto &variable : step.Variables)
        variables.push_back({{"name", variable.Name},
                             {"dtype", wanstreamer::TypeToDtype(variable.Type)},
                             {"adios_type", static_cast<int>(variable.Type)},
                             {"shape", variable.Shape},
                             {"nbytes", variable.Data.size()},
                             {"payload_nbytes", variable.Data.size()}});
    wanstreamer::SendJson(m_Socket, {{"type", "data"},
                                     {"stream_id", step.StreamID},
                                     {"step", step.Step},
                                     {"variables", std::move(variables)}});
    for (const auto &variable : step.Variables)
        wanstreamer::SendAll(m_Socket, variable.Data.data(), variable.Data.size());
}

bool WANStreamerWriter::RetryExpired(const std::chrono::steady_clock::time_point &started) const
{
    return m_ReconnectTimeoutSeconds > 0.0 &&
           std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count() >=
               m_ReconnectTimeoutSeconds;
}

void WANStreamerWriter::SendUntilAcknowledged()
{
    auto started = std::chrono::steady_clock::now();
    bool reported = false;
    size_t failedConnections = 0;
    while (true)
    {
        std::shared_ptr<wanstreamer::StepBuffer> step;
        try
        {
            {
                std::lock_guard<std::mutex> lock(m_ReplayMutex);
                if (m_Replay.empty())
                    return;
                step = m_Replay.front();
            }
            if (m_Socket < 0)
                Connect();
            // Resume may report the front (or every queued step) already
            // committed when the previous acknowledgement was lost.
            {
                std::lock_guard<std::mutex> lock(m_ReplayMutex);
                if (m_Replay.empty())
                    return;
                step = m_Replay.front();
            }
            SendStep(*step);
            const auto acknowledgement = wanstreamer::ReceiveJson(m_Socket);
            if (acknowledgement.value("type", "") != "ack" ||
                acknowledgement.value("stream_id", "") != m_StreamID ||
                acknowledgement.value("step", uint64_t(-1)) != step->Step)
                throw std::runtime_error("WANStreamer received an invalid acknowledgement");
            {
                std::lock_guard<std::mutex> lock(m_ReplayMutex);
                if (!m_Replay.empty() && m_Replay.front()->Step == step->Step)
                {
                    m_ReplayBytes -= StepBytes(*m_Replay.front());
                    m_Replay.pop_front();
                }
            }
            m_ReplayChanged.notify_all();
            reported = false;
            failedConnections = 0;
            started = std::chrono::steady_clock::now();
        }
        catch (const std::exception &error)
        {
            Disconnect();
            if (m_Closing)
                return;
            if (RetryExpired(started))
                throw std::runtime_error(std::string("WANStreamer reconnect timeout: ") +
                                         error.what());
            if (!reported)
            {
                std::cerr << "WANStreamer: connection lost; retaining step "
                          << (step ? std::to_string(step->Step) : "unknown") << " and reconnecting"
                          << std::endl;
                reported = true;
            }
            ++failedConnections;
            // First give the existing reader's listener time to accept a new
            // connection.  Relaunch only after repeated failures, and then at
            // a bounded cadence so a stale rendezvous cannot create a process
            // storm.
            if (failedConnections == 3 || failedConnections % 30 == 0)
                LaunchConsumer(true);
            std::this_thread::sleep_for(std::chrono::duration<double>(m_RetryIntervalSeconds));
        }
    }
}

uint64_t WANStreamerWriter::StepBytes(const wanstreamer::StepBuffer &step)
{
    uint64_t size = 0;
    for (const auto &variable : step.Variables)
        size += variable.Data.size();
    return size;
}

void WANStreamerWriter::RaiseSenderError() const
{
    if (m_SenderError)
        std::rethrow_exception(m_SenderError);
}

void WANStreamerWriter::SenderLoop() noexcept
{
    try
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(m_ReplayMutex);
            m_ReplayChanged.wait(lock, [&] { return m_CloseRequested || !m_Replay.empty(); });
            if (m_CloseRequested && m_Replay.empty())
                return;
            lock.unlock();
            SendUntilAcknowledged();
        }
    }
    catch (...)
    {
        std::lock_guard<std::mutex> lock(m_ReplayMutex);
        m_SenderError = std::current_exception();
        m_ReplayChanged.notify_all();
    }
}

StepStatus WANStreamerWriter::BeginStep(StepMode, float)
{
    if (m_BuildingStep)
        throw std::logic_error("WANStreamer BeginStep called before EndStep");
    ++m_CurrentStep;
    m_BuildingStep = std::make_shared<wanstreamer::StepBuffer>();
    m_BuildingStep->StreamID = m_StreamID;
    m_BuildingStep->Step = static_cast<uint64_t>(m_CurrentStep);
    return StepStatus::OK;
}

size_t WANStreamerWriter::CurrentStep() const { return static_cast<size_t>(m_CurrentStep); }

template <class T>
void WANStreamerWriter::Put(Variable<T> &variable, const T *values)
{
    if constexpr (std::is_same_v<T, std::string>)
    {
        throw std::invalid_argument("WANStreamer does not support string variables");
    }
    else
    {
        if (!m_BuildingStep)
            throw std::logic_error("WANStreamer Put called outside a step");
        wanstreamer::VariableBuffer output;
        output.Name = variable.m_Name;
        output.Type = variable.m_Type;
        // Validate before EndStep enters its reconnect loop. Unsupported local
        // data is an application error, not a network failure to retry.
        (void)wanstreamer::TypeToDtype(output.Type);
        output.Shape = variable.m_Count;
        const size_t size = GetTotalSize(variable.m_Count, sizeof(T));
        output.Data.resize(size);
        std::memcpy(output.Data.data(), values, size);
        m_BuildingStep->Variables.emplace_back(std::move(output));
    }
}

#define declare_type(T)                                                                            \
    void WANStreamerWriter::DoPutSync(Variable<T> &variable, const T *values)                      \
    {                                                                                              \
        Put(variable, values);                                                                     \
    }                                                                                              \
    void WANStreamerWriter::DoPutDeferred(Variable<T> &variable, const T *values)                  \
    {                                                                                              \
        Put(variable, values);                                                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void WANStreamerWriter::EndStep()
{
    if (!m_BuildingStep)
        throw std::logic_error("WANStreamer EndStep called without BeginStep");
    const uint64_t bytes = StepBytes(*m_BuildingStep);
    std::unique_lock<std::mutex> lock(m_ReplayMutex);
    m_ReplayChanged.wait(lock, [&] {
        return m_SenderError || (m_Replay.size() < m_ReplayBufferSteps &&
                                 (m_ReplayBufferBytes == 0 || m_Replay.empty() ||
                                  m_ReplayBytes + bytes <= m_ReplayBufferBytes));
    });
    RaiseSenderError();
    m_ReplayBytes += bytes;
    m_Replay.emplace_back(std::move(m_BuildingStep));
    lock.unlock();
    m_ReplayChanged.notify_all();
}

void WANStreamerWriter::DoClose(int)
{
    {
        std::lock_guard<std::mutex> lock(m_ReplayMutex);
        if (m_BuildingStep)
            throw std::logic_error("WANStreamer Close called with an open step");
        m_CloseRequested = true;
    }
    m_ReplayChanged.notify_all();
    if (m_SenderThread.joinable())
        m_SenderThread.join();
    RaiseSenderError();
    m_Closing = true;
    if (m_Socket >= 0)
    {
        try
        {
            wanstreamer::SendJson(
                m_Socket,
                {{"type", "end"}, {"stream_id", m_StreamID}, {"final_step", m_CurrentStep}});
        }
        catch (...)
        {
        }
    }
    Disconnect();
    m_IsClosed = true;
}

} // namespace adios2::core::engine
