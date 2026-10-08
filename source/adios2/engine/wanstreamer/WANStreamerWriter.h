/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ADIOS2_ENGINE_WANSTREAMER_WRITER_H_
#define ADIOS2_ENGINE_WANSTREAMER_WRITER_H_

#include "WANStreamerProtocol.h"
#include "adios2/core/Engine.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace adios2::core::engine
{

class WANStreamerWriter final : public Engine
{
public:
    WANStreamerWriter(IO &, const std::string &, Mode, helper::Comm);
    ~WANStreamerWriter() override;

    StepStatus BeginStep(StepMode, float = -1.0) final;
    size_t CurrentStep() const final;
    void PerformPuts() final {}
    void EndStep() final;
    void Flush(int = -1) final {}

private:
    int m_Socket = -1;
    int64_t m_CurrentStep = -1;
    std::string m_StreamID;
    std::string m_ConnectionFile;
    std::string m_PrivateKey;
    std::string m_LaunchMode = "none";
    std::string m_LaunchCommand;
    float m_RetryIntervalSeconds = 1.0f;
    float m_ReconnectTimeoutSeconds = 0.0f;
    uint64_t m_ReplayBufferSteps = 64;
    uint64_t m_ReplayBufferBytes = 0;
    uint64_t m_ReplayBytes = 0;
    bool m_Launched = false;
    std::atomic<bool> m_Closing{false};
    bool m_CloseRequested = false;
    std::shared_ptr<wanstreamer::StepBuffer> m_BuildingStep;
    std::deque<std::shared_ptr<wanstreamer::StepBuffer>> m_Replay;
    std::mutex m_ReplayMutex;
    std::condition_variable m_ReplayChanged;
    std::thread m_SenderThread;
    std::exception_ptr m_SenderError;

    void ReadParameters();
    void LaunchConsumer(bool relaunch);
    void Connect();
    void Disconnect() noexcept;
    void SendStep(const wanstreamer::StepBuffer &step);
    void SendUntilAcknowledged();
    void SenderLoop() noexcept;
    void RaiseSenderError() const;
    static uint64_t StepBytes(const wanstreamer::StepBuffer &step);
    bool RetryExpired(const std::chrono::steady_clock::time_point &started) const;
    void DoClose(int = -1) final;

#define declare_type(T)                                                                            \
    void DoPutSync(Variable<T> &, const T *) final;                                                \
    void DoPutDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void Put(Variable<T> &, const T *);
};

} // namespace adios2::core::engine
#endif
