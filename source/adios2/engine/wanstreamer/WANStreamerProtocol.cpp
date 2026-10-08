/* SPDX-License-Identifier: Apache-2.0 */
#include "WANStreamerProtocol.h"

#include <arpa/inet.h>
#include <endian.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sodium.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace adios2::core::engine::wanstreamer
{
namespace
{
std::vector<unsigned char> ReadKey(const std::string &path, const size_t size)
{
    std::ifstream input(path, std::ios::binary);
    std::vector<unsigned char> key(size);
    input.read(reinterpret_cast<char *>(key.data()), static_cast<std::streamsize>(key.size()));
    if (!input || input.gcount() != static_cast<std::streamsize>(key.size()))
    {
        throw std::runtime_error("WANStreamer could not read key " + path);
    }
    return key;
}

std::vector<unsigned char> DecodeBase64(const std::string &encoded)
{
    std::vector<unsigned char> decoded(encoded.size());
    size_t size = 0;
    if (sodium_base642bin(decoded.data(), decoded.size(), encoded.data(), encoded.size(), nullptr,
                          &size, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0)
    {
        throw std::runtime_error("WANStreamer received invalid base64");
    }
    decoded.resize(size);
    return decoded;
}

std::string EncodeBase64(const unsigned char *data, const size_t size)
{
    std::string encoded(sodium_base64_ENCODED_LEN(size, sodium_base64_VARIANT_ORIGINAL), '\0');
    sodium_bin2base64(encoded.data(), encoded.size(), data, size, sodium_base64_VARIANT_ORIGINAL);
    encoded.resize(std::strlen(encoded.c_str()));
    return encoded;
}

void EnsureSodium()
{
    if (sodium_init() < 0)
    {
        throw std::runtime_error("WANStreamer could not initialize libsodium");
    }
}

std::pair<std::vector<unsigned char>, std::vector<unsigned char>>
PrivateKeyPair(const std::string &path)
{
    auto secret = ReadKey(path, crypto_box_SECRETKEYBYTES);
    std::vector<unsigned char> publicKey(crypto_box_PUBLICKEYBYTES);
    if (crypto_scalarmult_base(publicKey.data(), secret.data()) != 0)
    {
        throw std::runtime_error("WANStreamer could not derive the public key");
    }
    return {std::move(publicKey), std::move(secret)};
}
} // namespace

std::string TypeToDtype(const DataType type)
{
    switch (type)
    {
    case DataType::Int8:
        return "|i1";
    case DataType::UInt8:
        return "|u1";
    case DataType::Char:
        return "|i1";
    case DataType::Int16:
        return "<i2";
    case DataType::UInt16:
        return "<u2";
    case DataType::Int32:
        return "<i4";
    case DataType::UInt32:
        return "<u4";
    case DataType::Int64:
        return "<i8";
    case DataType::UInt64:
        return "<u8";
    case DataType::Float:
        return "<f4";
    case DataType::Double:
        return "<f8";
    case DataType::FloatComplex:
        return "<c8";
    case DataType::DoubleComplex:
        return "<c16";
    default:
        throw std::invalid_argument("WANStreamer does not support this variable type");
    }
}

DataType DtypeToType(const std::string &dtype)
{
    if (dtype == "|i1" || dtype == "<i1")
        return DataType::Int8;
    if (dtype == "|u1" || dtype == "<u1")
        return DataType::UInt8;
    if (dtype == "<i2")
        return DataType::Int16;
    if (dtype == "<u2")
        return DataType::UInt16;
    if (dtype == "<i4")
        return DataType::Int32;
    if (dtype == "<u4")
        return DataType::UInt32;
    if (dtype == "<i8")
        return DataType::Int64;
    if (dtype == "<u8")
        return DataType::UInt64;
    if (dtype == "<f4")
        return DataType::Float;
    if (dtype == "<f8")
        return DataType::Double;
    if (dtype == "<c8")
        return DataType::FloatComplex;
    if (dtype == "<c16")
        return DataType::DoubleComplex;
    throw std::invalid_argument("WANStreamer received unsupported dtype " + dtype);
}

std::string RandomID()
{
    EnsureSodium();
    unsigned char bytes[16];
    randombytes_buf(bytes, sizeof(bytes));
    std::string result(sizeof(bytes) * 2 + 1, '\0');
    sodium_bin2hex(result.data(), result.size(), bytes, sizeof(bytes));
    result.resize(sizeof(bytes) * 2);
    return result;
}

std::string RankPath(std::string value, const int rank, const int size)
{
    const std::string marker = "{rank}";
    size_t offset = 0;
    bool replaced = false;
    while ((offset = value.find(marker, offset)) != std::string::npos)
    {
        value.replace(offset, marker.size(), std::to_string(rank));
        offset += std::to_string(rank).size();
        replaced = true;
    }
    if (size > 1 && !replaced)
    {
        value += "." + std::to_string(rank);
    }
    return value;
}

void SendAll(const int socket, const void *data, size_t size)
{
    const char *position = static_cast<const char *>(data);
    while (size > 0)
    {
        const ssize_t count = send(socket, position, size, MSG_NOSIGNAL);
        if (count <= 0)
        {
            throw std::runtime_error("WANStreamer socket send failed: " +
                                     std::string(std::strerror(errno)));
        }
        position += count;
        size -= static_cast<size_t>(count);
    }
}

void ReceiveAll(const int socket, void *data, size_t size)
{
    char *position = static_cast<char *>(data);
    while (size > 0)
    {
        const ssize_t count = recv(socket, position, size, 0);
        if (count <= 0)
        {
            throw std::runtime_error("WANStreamer socket disconnected");
        }
        position += count;
        size -= static_cast<size_t>(count);
    }
}

void SendJson(const int socket, const nlohmann::json &message)
{
    const std::string encoded = message.dump();
    const uint64_t size = htobe64(encoded.size());
    SendAll(socket, &size, sizeof(size));
    SendAll(socket, encoded.data(), encoded.size());
}

nlohmann::json ReceiveJson(const int socket)
{
    uint64_t encodedSize = 0;
    ReceiveAll(socket, &encodedSize, sizeof(encodedSize));
    const uint64_t size = be64toh(encodedSize);
    if (size > MaxHeaderSize)
    {
        throw std::runtime_error("WANStreamer message header is too large");
    }
    std::string encoded(size, '\0');
    ReceiveAll(socket, encoded.data(), encoded.size());
    return nlohmann::json::parse(encoded);
}

void CloseSocket(int &socket) noexcept
{
    if (socket >= 0)
    {
        close(socket);
        socket = -1;
    }
}

int ConnectTcp(const std::string &host, const int port)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo *addresses = nullptr;
    const std::string service = std::to_string(port);
    const int lookup = getaddrinfo(host.c_str(), service.c_str(), &hints, &addresses);
    if (lookup != 0)
    {
        throw std::runtime_error("WANStreamer cannot resolve " + host + ": " +
                                 gai_strerror(lookup));
    }
    int connected = -1;
    for (addrinfo *address = addresses; address != nullptr; address = address->ai_next)
    {
        int candidate = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (candidate < 0)
            continue;
        if (connect(candidate, address->ai_addr, address->ai_addrlen) == 0)
        {
            connected = candidate;
            break;
        }
        close(candidate);
    }
    freeaddrinfo(addresses);
    if (connected < 0)
    {
        throw std::runtime_error("WANStreamer could not connect to " + host + ":" + service);
    }
    int enabled = 1;
    setsockopt(connected, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof(enabled));
    return connected;
}

int CreateListener(const std::string &host, const int requestedPort, int &actualPort)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    addrinfo *addresses = nullptr;
    const std::string service = std::to_string(requestedPort);
    const char *node = (host.empty() || host == "*") ? nullptr : host.c_str();
    const int lookup = getaddrinfo(node, service.c_str(), &hints, &addresses);
    if (lookup != 0)
    {
        throw std::runtime_error("WANStreamer cannot resolve bind address: " +
                                 std::string(gai_strerror(lookup)));
    }
    int listener = -1;
    for (addrinfo *address = addresses; address != nullptr; address = address->ai_next)
    {
        int candidate = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (candidate < 0)
            continue;
        int enabled = 1;
        setsockopt(candidate, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
        if (bind(candidate, address->ai_addr, address->ai_addrlen) == 0 &&
            listen(candidate, 1) == 0)
        {
            listener = candidate;
            sockaddr_storage bound{};
            socklen_t size = sizeof(bound);
            if (getsockname(listener, reinterpret_cast<sockaddr *>(&bound), &size) == 0)
            {
                actualPort = bound.ss_family == AF_INET
                                 ? ntohs(reinterpret_cast<sockaddr_in *>(&bound)->sin_port)
                                 : ntohs(reinterpret_cast<sockaddr_in6 *>(&bound)->sin6_port);
            }
            break;
        }
        close(candidate);
    }
    freeaddrinfo(addresses);
    if (listener < 0)
        throw std::runtime_error("WANStreamer could not create listening socket");
    return listener;
}

int Accept(const int listener)
{
    int socket = accept(listener, nullptr, nullptr);
    if (socket < 0)
        throw std::runtime_error("WANStreamer accept failed");
    int enabled = 1;
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof(enabled));
    return socket;
}

nlohmann::json DecryptConnectionFile(const std::string &path, const std::string &privateKeyPath)
{
    EnsureSodium();
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("WANStreamer connection file is not available: " + path);
    nlohmann::json envelope;
    input >> envelope;
    if (envelope.value("format", "") != "lapd-curve25519-sealed-box-v1")
    {
        throw std::runtime_error("WANStreamer connection file has an unsupported format");
    }
    auto cipher = DecodeBase64(envelope.at("ciphertext").get<std::string>());
    if (cipher.size() < crypto_box_SEALBYTES)
    {
        throw std::runtime_error("WANStreamer connection ciphertext is too short");
    }
    auto keys = PrivateKeyPair(privateKeyPath);
    std::vector<unsigned char> plain(cipher.size() - crypto_box_SEALBYTES);
    if (crypto_box_seal_open(plain.data(), cipher.data(), cipher.size(), keys.first.data(),
                             keys.second.data()) != 0)
    {
        throw std::runtime_error("WANStreamer could not decrypt connection information");
    }
    return nlohmann::json::parse(plain.begin(), plain.end());
}

std::string EncryptConnectionInfo(const nlohmann::json &connectionInfo,
                                  const std::string &publicKeyPath)
{
    EnsureSodium();
    auto publicKey = ReadKey(publicKeyPath, crypto_box_PUBLICKEYBYTES);
    const std::string plain = connectionInfo.dump();
    std::vector<unsigned char> cipher(plain.size() + crypto_box_SEALBYTES);
    if (crypto_box_seal(cipher.data(), reinterpret_cast<const unsigned char *>(plain.data()),
                        plain.size(), publicKey.data()) != 0)
    {
        throw std::runtime_error("WANStreamer could not encrypt connection information");
    }
    nlohmann::json envelope = {{"format", "lapd-curve25519-sealed-box-v1"},
                               {"ciphertext", EncodeBase64(cipher.data(), cipher.size())}};
    return envelope.dump(2) + "\n";
}

void WriteFileAtomically(const std::string &path, const std::string &contents)
{
    const std::string temporary = path + ".tmp." + std::to_string(getpid());
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output)
            throw std::runtime_error("WANStreamer cannot create " + temporary);
        output << contents;
        if (!output)
            throw std::runtime_error("WANStreamer cannot write " + temporary);
    }
    if (rename(temporary.c_str(), path.c_str()) != 0)
    {
        unlink(temporary.c_str());
        throw std::runtime_error("WANStreamer cannot publish " + path);
    }
}

void AnswerChallenge(const int socket, const std::string &privateKeyPath)
{
    EnsureSodium();
    const auto request = ReceiveJson(socket);
    if (request.value("type", "") != "auth_challenge")
        throw std::runtime_error("WANStreamer expected an authentication challenge");
    auto cipher = DecodeBase64(request.at("challenge").get<std::string>());
    auto keys = PrivateKeyPair(privateKeyPath);
    if (cipher.size() < crypto_box_SEALBYTES)
        throw std::runtime_error("WANStreamer authentication challenge is too short");
    std::vector<unsigned char> plain(cipher.size() - crypto_box_SEALBYTES);
    if (crypto_box_seal_open(plain.data(), cipher.data(), cipher.size(), keys.first.data(),
                             keys.second.data()) != 0)
        throw std::runtime_error("WANStreamer authentication failed");
    SendJson(socket,
             {{"type", "auth_response"}, {"response", EncodeBase64(plain.data(), plain.size())}});
}

void VerifyPrivateKey(const int socket, const std::string &publicKeyPath)
{
    EnsureSodium();
    auto publicKey = ReadKey(publicKeyPath, crypto_box_PUBLICKEYBYTES);
    unsigned char challenge[32];
    randombytes_buf(challenge, sizeof(challenge));
    std::vector<unsigned char> cipher(sizeof(challenge) + crypto_box_SEALBYTES);
    crypto_box_seal(cipher.data(), challenge, sizeof(challenge), publicKey.data());
    SendJson(socket, {{"type", "auth_challenge"},
                      {"challenge", EncodeBase64(cipher.data(), cipher.size())}});
    const auto response = ReceiveJson(socket);
    if (response.value("type", "") != "auth_response")
        throw std::runtime_error("WANStreamer peer did not answer authentication challenge");
    const auto answer = DecodeBase64(response.at("response").get<std::string>());
    if (answer.size() != sizeof(challenge) ||
        sodium_memcmp(answer.data(), challenge, sizeof(challenge)) != 0)
        throw std::runtime_error("WANStreamer peer failed authentication");
}

} // namespace adios2::core::engine::wanstreamer
