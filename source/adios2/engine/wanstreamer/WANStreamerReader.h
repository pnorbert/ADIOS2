/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ADIOS2_ENGINE_WANSTREAMER_READER_H_
#define ADIOS2_ENGINE_WANSTREAMER_READER_H_

#include "WANStreamerProtocol.h"
#include "adios2/core/Engine.h"

#include <limits>
#include <map>
#include <memory>
#include <string>

namespace adios2::core::engine
{

class WANStreamerReader final : public Engine
{
public:
    WANStreamerReader(IO &, const std::string &, Mode, helper::Comm);
    ~WANStreamerReader() override;

    StepStatus BeginStep(StepMode, float = -1.0) final;
    size_t CurrentStep() const final;
    void PerformGets() final {}
    void EndStep() final;
    void Flush(int = -1) final {}

private:
    int m_Listener = -1;
    int m_Socket = -1;
    int m_Port = 0;
    int64_t m_CommittedStep = -1;
    int64_t m_CurrentStep = -1;
    std::string m_StreamID;
    std::string m_ConsumerID;
    std::string m_ConnectionFile;
    std::string m_PublicKey;
    std::string m_BindAddress = "0.0.0.0";
    std::string m_AdvertiseAddress = "127.0.0.1";
    std::string m_CheckpointFile;
    std::string m_ConnectionContents;
    std::shared_ptr<wanstreamer::StepBuffer> m_Step;
    bool m_EndOfStream = false;

    void ReadParameters();
    void PublishConnectionInfo();
    void AcceptConnection();
    bool ReceiveStep();
    void DefineVariables();
    void LoadCheckpoint();
    void SaveCheckpoint();
    void Disconnect() noexcept;
    const wanstreamer::VariableBuffer &FindVariable(const std::string &) const;
    void DoClose(int = -1) final;

#define declare_type(T)                                                                            \
    void DoGetSync(Variable<T> &, T *) final;                                                      \
    void DoGetDeferred(Variable<T> &, T *) final;                                                  \
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>> DoAllStepsBlocksInfo(              \
        const Variable<T> &) const final;                                                          \
    std::vector<typename Variable<T>::BPInfo> DoBlocksInfo(const Variable<T> &, size_t) const final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void Get(Variable<T> &, T *);
    template <class T>
    void Define(const wanstreamer::VariableBuffer &);
};

} // namespace adios2::core::engine
#endif
