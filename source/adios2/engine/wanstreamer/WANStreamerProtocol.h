/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ADIOS2_ENGINE_WANSTREAMER_PROTOCOL_H_
#define ADIOS2_ENGINE_WANSTREAMER_PROTOCOL_H_

#include "adios2/common/ADIOSTypes.h"
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace adios2::core::engine::wanstreamer
{

constexpr int ProtocolVersion = 5;
constexpr size_t MaxHeaderSize = 1024 * 1024;

struct VariableBuffer
{
    std::string Name;
    DataType Type = DataType::None;
    Dims Shape;
    std::vector<char> Data;
};

struct StepBuffer
{
    std::string StreamID;
    uint64_t Step = 0;
    std::vector<VariableBuffer> Variables;
};

std::string TypeToDtype(DataType type);
DataType DtypeToType(const std::string &dtype);
std::string RandomID();
std::string RankPath(std::string value, int rank, int size);

void SendAll(int socket, const void *data, size_t size);
void ReceiveAll(int socket, void *data, size_t size);
void SendJson(int socket, const nlohmann::json &message);
nlohmann::json ReceiveJson(int socket);
void CloseSocket(int &socket) noexcept;

int ConnectTcp(const std::string &host, int port);
int CreateListener(const std::string &host, int requestedPort, int &actualPort);
int Accept(int listener);

nlohmann::json DecryptConnectionFile(const std::string &path, const std::string &privateKeyPath);
std::string EncryptConnectionInfo(const nlohmann::json &connectionInfo,
                                  const std::string &publicKeyPath);
void WriteFileAtomically(const std::string &path, const std::string &contents);

void AnswerChallenge(int socket, const std::string &privateKeyPath);
void VerifyPrivateKey(int socket, const std::string &publicKeyPath);

} // namespace adios2::core::engine::wanstreamer

#endif
