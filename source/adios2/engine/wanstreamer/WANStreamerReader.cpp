/* SPDX-License-Identifier: Apache-2.0 */
#include "WANStreamerReader.h"
#include "adios2/helper/adiosFunctions.h"

#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <type_traits>

namespace adios2::core::engine
{

WANStreamerReader::WANStreamerReader(IO &io, const std::string &name, const Mode mode,
                                     helper::Comm comm)
: Engine("WANStreamerReader", io, name, mode, std::move(comm)),
  m_ConsumerID(wanstreamer::RandomID())
{
    ReadParameters();
    LoadCheckpoint();
    m_Listener = wanstreamer::CreateListener(m_BindAddress, m_Port, m_Port);
    PublishConnectionInfo();
    m_IsOpen = true;
}

WANStreamerReader::~WANStreamerReader()
{
    if (!m_IsClosed)
    {
        try
        {
            DoClose();
        }
        catch (...)
        {
            Disconnect();
            wanstreamer::CloseSocket(m_Listener);
        }
    }
}

void WANStreamerReader::ReadParameters()
{
    m_ConnectionFile = helper::GetParameter("ConnectionFile", m_IO.m_Parameters, false, "");
    m_PublicKey = helper::GetParameter("PublicKey", m_IO.m_Parameters, false, "");
    const std::string bindAddress =
        helper::GetParameter("BindAddress", m_IO.m_Parameters, false, "");
    const std::string advertiseAddress =
        helper::GetParameter("AdvertiseAddress", m_IO.m_Parameters, false, "");
    if (!bindAddress.empty())
        m_BindAddress = bindAddress;
    if (!advertiseAddress.empty())
        m_AdvertiseAddress = advertiseAddress;
    helper::GetParameter(m_IO.m_Parameters, "Port", m_Port);
    m_CheckpointFile = helper::GetParameter("CheckpointFile", m_IO.m_Parameters, false, "");
    m_ConnectionFile = wanstreamer::RankPath(m_ConnectionFile, m_Comm.Rank(), m_Comm.Size());
    if (m_CheckpointFile.empty())
        m_CheckpointFile = m_ConnectionFile + ".checkpoint";
    else
        m_CheckpointFile = wanstreamer::RankPath(m_CheckpointFile, m_Comm.Rank(), m_Comm.Size());
    if (m_ConnectionFile.empty() || m_PublicKey.empty())
        throw std::invalid_argument("WANStreamer reader requires ConnectionFile and PublicKey");
}

void WANStreamerReader::PublishConnectionInfo()
{
    nlohmann::json connection = {{"id", "wanstreamer"},
                                 {"protocol_version", wanstreamer::ProtocolVersion},
                                 {"consumer_id", m_ConsumerID},
                                 {"host", m_AdvertiseAddress},
                                 {"port", m_Port},
                                 {"rank", m_Comm.Rank()}};
    m_ConnectionContents = wanstreamer::EncryptConnectionInfo(connection, m_PublicKey);
    wanstreamer::WriteFileAtomically(m_ConnectionFile, m_ConnectionContents);
    std::cout << "WANStreamer encrypted connection information (" << m_ConnectionFile << "):\n"
              << m_ConnectionContents << std::flush;
}

void WANStreamerReader::LoadCheckpoint()
{
    std::ifstream input(m_CheckpointFile);
    if (!input)
        return;
    try
    {
        nlohmann::json checkpoint;
        input >> checkpoint;
        m_StreamID = checkpoint.value("stream_id", "");
        m_CommittedStep = checkpoint.value("committed_step", int64_t{-1});
    }
    catch (...)
    {
        m_StreamID.clear();
        m_CommittedStep = -1;
    }
}

void WANStreamerReader::SaveCheckpoint()
{
    wanstreamer::WriteFileAtomically(
        m_CheckpointFile,
        nlohmann::json({{"stream_id", m_StreamID}, {"committed_step", m_CommittedStep}}).dump(2) +
            "\n");
}

void WANStreamerReader::AcceptConnection()
{
    Disconnect();
    int socket = wanstreamer::Accept(m_Listener);
    try
    {
        wanstreamer::VerifyPrivateKey(socket, m_PublicKey);
        const auto hello = wanstreamer::ReceiveJson(socket);
        if (hello.value("type", "") != "hello" ||
            hello.value("protocol_version", 0) != wanstreamer::ProtocolVersion ||
            hello.value("consumer_id", "") != m_ConsumerID)
            throw std::runtime_error("WANStreamer received an invalid hello message");
        const std::string stream = hello.at("stream_id").get<std::string>();
        if (m_StreamID != stream)
        {
            m_StreamID = stream;
            m_CommittedStep = -1;
            SaveCheckpoint();
        }
        wanstreamer::SendJson(
            socket,
            {{"type", "resume"}, {"stream_id", m_StreamID}, {"committed_step", m_CommittedStep}});
    }
    catch (...)
    {
        wanstreamer::CloseSocket(socket);
        throw;
    }
    m_Socket = socket;
}

bool WANStreamerReader::ReceiveStep()
{
    while (true)
    {
        try
        {
            if (m_Socket < 0)
                AcceptConnection();
            const auto header = wanstreamer::ReceiveJson(m_Socket);
            const std::string type = header.value("type", "");
            if (type == "end")
            {
                m_EndOfStream = true;
                return false;
            }
            if (type != "data" || header.value("stream_id", "") != m_StreamID)
                throw std::runtime_error("WANStreamer received an invalid data header");
            auto step = std::make_shared<wanstreamer::StepBuffer>();
            step->StreamID = m_StreamID;
            step->Step = header.at("step").get<uint64_t>();
            for (const auto &description : header.at("variables"))
            {
                wanstreamer::VariableBuffer variable;
                variable.Name = description.at("name").get<std::string>();
                variable.Type =
                    description.contains("adios_type")
                        ? static_cast<DataType>(description.at("adios_type").get<int>())
                        : wanstreamer::DtypeToType(description.at("dtype").get<std::string>());
                variable.Shape = description.at("shape").get<Dims>();
                const size_t bytes = description.at("payload_nbytes").get<size_t>();
                const size_t expected =
                    GetTotalSize(variable.Shape, GetDataTypeSize(variable.Type));
                if (bytes != expected || description.at("nbytes").get<size_t>() != expected)
                    throw std::runtime_error("WANStreamer variable size is inconsistent");
                variable.Data.resize(bytes);
                wanstreamer::ReceiveAll(m_Socket, variable.Data.data(), bytes);
                step->Variables.emplace_back(std::move(variable));
            }
            if (static_cast<int64_t>(step->Step) <= m_CommittedStep)
            {
                wanstreamer::SendJson(
                    m_Socket, {{"type", "ack"}, {"stream_id", m_StreamID}, {"step", step->Step}});
                continue;
            }
            if (static_cast<int64_t>(step->Step) != m_CommittedStep + 1)
                throw std::runtime_error("WANStreamer detected a gap in the step sequence");
            m_Step = std::move(step);
            return true;
        }
        catch (const std::exception &error)
        {
            std::cerr << "WANStreamer reader: " << error.what()
                      << "; waiting for producer reconnect" << std::endl;
            Disconnect();
        }
    }
}

template <class T>
void WANStreamerReader::Define(const wanstreamer::VariableBuffer &source)
{
    Dims start(source.Shape.size(), 0);
    auto *variable = m_IO.InquireVariable<T>(source.Name);
    if (variable == nullptr)
        variable = &m_IO.DefineVariable<T>(source.Name, source.Shape, start, source.Shape, false);
    else
    {
        if (variable->m_Shape != source.Shape)
            variable->SetShape(source.Shape);
        variable->SetSelection({start, source.Shape});
    }
    variable->m_Engine = this;
    variable->m_FirstStreamingStep = false;
}

void WANStreamerReader::DefineVariables()
{
    m_IO.RemoveAllVariables();
    for (const auto &source : m_Step->Variables)
    {
        switch (source.Type)
        {
        case DataType::Int8:
            Define<int8_t>(source);
            break;
        case DataType::Int16:
            Define<int16_t>(source);
            break;
        case DataType::Int32:
            Define<int32_t>(source);
            break;
        case DataType::Int64:
            Define<int64_t>(source);
            break;
        case DataType::UInt8:
            Define<uint8_t>(source);
            break;
        case DataType::UInt16:
            Define<uint16_t>(source);
            break;
        case DataType::UInt32:
            Define<uint32_t>(source);
            break;
        case DataType::UInt64:
            Define<uint64_t>(source);
            break;
        case DataType::Char:
            Define<char>(source);
            break;
        case DataType::Float:
            Define<float>(source);
            break;
        case DataType::Double:
            Define<double>(source);
            break;
        case DataType::FloatComplex:
            Define<std::complex<float>>(source);
            break;
        case DataType::DoubleComplex:
            Define<std::complex<double>>(source);
            break;
        default:
            throw std::runtime_error("WANStreamer received an unsupported variable type");
        }
    }
}

StepStatus WANStreamerReader::BeginStep(StepMode, float)
{
    if (m_Step)
        throw std::logic_error("WANStreamer BeginStep called before EndStep");
    if (m_EndOfStream)
        return StepStatus::EndOfStream;
    if (!ReceiveStep())
        return StepStatus::EndOfStream;
    m_CurrentStep = static_cast<int64_t>(m_Step->Step);
    DefineVariables();
    return StepStatus::OK;
}

size_t WANStreamerReader::CurrentStep() const { return static_cast<size_t>(m_CurrentStep); }

const wanstreamer::VariableBuffer &WANStreamerReader::FindVariable(const std::string &name) const
{
    for (const auto &variable : m_Step->Variables)
        if (variable.Name == name)
            return variable;
    throw std::invalid_argument("WANStreamer step does not contain variable " + name);
}

template <class T>
void WANStreamerReader::Get(Variable<T> &variable, T *data)
{
    if constexpr (std::is_same_v<T, std::string>)
    {
        throw std::invalid_argument("WANStreamer does not support string variables");
    }
    else
    {
        if (!m_Step)
            throw std::logic_error("WANStreamer Get called outside a step");
        const auto &source = FindVariable(variable.m_Name);
        if (source.Type != variable.m_Type)
            throw std::invalid_argument("WANStreamer Get type does not match the transmitted type");
        const Dims zeros(source.Shape.size(), 0);
        if (variable.m_Start != zeros || variable.m_Count != source.Shape)
            throw std::invalid_argument("WANStreamer currently requires a full-variable selection");
        std::memcpy(data, source.Data.data(), source.Data.size());
    }
}

#define declare_type(T)                                                                            \
    void WANStreamerReader::DoGetSync(Variable<T> &variable, T *data) { Get(variable, data); }     \
    void WANStreamerReader::DoGetDeferred(Variable<T> &variable, T *data) { Get(variable, data); } \
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>>                                    \
    WANStreamerReader::DoAllStepsBlocksInfo(const Variable<T> &) const                             \
    {                                                                                              \
        return {};                                                                                 \
    }                                                                                              \
    std::vector<typename Variable<T>::BPInfo> WANStreamerReader::DoBlocksInfo(const Variable<T> &, \
                                                                              size_t) const        \
    {                                                                                              \
        return {};                                                                                 \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void WANStreamerReader::EndStep()
{
    if (!m_Step)
        throw std::logic_error("WANStreamer EndStep called without BeginStep");
    m_CommittedStep = static_cast<int64_t>(m_Step->Step);
    SaveCheckpoint();
    try
    {
        if (m_Socket >= 0)
            wanstreamer::SendJson(
                m_Socket, {{"type", "ack"}, {"stream_id", m_StreamID}, {"step", m_Step->Step}});
    }
    catch (...)
    {
        // The downstream write is already committed.  On reconnect the writer
        // learns the checkpoint and drops/replays the step without data loss.
        Disconnect();
    }
    m_Step.reset();
}

void WANStreamerReader::Disconnect() noexcept { wanstreamer::CloseSocket(m_Socket); }

void WANStreamerReader::DoClose(int)
{
    Disconnect();
    wanstreamer::CloseSocket(m_Listener);
    std::ifstream input(m_ConnectionFile);
    std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (contents == m_ConnectionContents)
        unlink(m_ConnectionFile.c_str());
    m_IsClosed = true;
}

} // namespace adios2::core::engine
