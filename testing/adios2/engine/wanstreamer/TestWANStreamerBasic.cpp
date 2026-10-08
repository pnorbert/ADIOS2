/* SPDX-License-Identifier: Apache-2.0 */
#include <adios2.h>
#include <gtest/gtest.h>

#include <chrono>
#include <complex>
#include <condition_variable>
#include <cstdio>
#include <exception>
#include <fstream>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

namespace
{
const unsigned char PrivateKey[32] = {
    0x10, 0x52, 0x0b, 0x37, 0x0a, 0xb1, 0x5f, 0xb4, 0xcf, 0x69, 0x4b, 0x5d, 0xb7, 0x7b, 0x66, 0xf5,
    0x92, 0x8e, 0x55, 0x26, 0x87, 0x42, 0x15, 0x5f, 0x96, 0xd8, 0xf6, 0x10, 0xdf, 0x6d, 0xdc, 0xc4};
const unsigned char PublicKey[32] = {
    0x28, 0x0e, 0x9f, 0x04, 0x85, 0x22, 0x1f, 0xf4, 0x90, 0xc4, 0xa5, 0x1b, 0x93, 0x74, 0x12, 0x36,
    0xf4, 0xc6, 0x73, 0xab, 0xfe, 0x3c, 0xb4, 0xc8, 0x28, 0x1e, 0x4b, 0x05, 0xde, 0x37, 0x8f, 0x30};

void WriteBytes(const std::string &path, const unsigned char *bytes, const size_t size)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char *>(bytes), static_cast<std::streamsize>(size));
}
} // namespace

TEST(WANStreamerEngineTest, Basic)
{
    const std::string prefix = "/tmp/adios2-wanstreamer-" + std::to_string(getpid());
    const std::string connection = prefix + ".connection";
    const std::string publicKey = prefix + ".public";
    const std::string privateKey = prefix + ".private";
    WriteBytes(publicKey, PublicKey, sizeof(PublicKey));
    WriteBytes(privateKey, PrivateKey, sizeof(PrivateKey));

    std::vector<double> received;
    std::exception_ptr readerError;
    std::thread reader([&] {
        try
        {
            adios2::ADIOS adios;
            auto io = adios.DeclareIO("WANStreamerReader");
            io.SetEngine("WANStreamer");
            io.SetParameters({{"ConnectionFile", connection},
                              {"PublicKey", publicKey},
                              {"BindAddress", "127.0.0.1"},
                              {"AdvertiseAddress", "127.0.0.1"}});
            auto engine = io.Open("stream", adios2::Mode::Read);
            if (engine.BeginStep() != adios2::StepStatus::OK)
                throw std::runtime_error("reader did not receive a step");
            auto variable = io.InquireVariable<double>("samples");
            if (!variable)
                throw std::runtime_error("samples variable is unavailable");
            received.resize(4);
            engine.Get(variable, received.data(), adios2::Mode::Sync);
            engine.EndStep(); // WANStreamer commit and acknowledgement
            engine.Close();
        }
        catch (...)
        {
            readerError = std::current_exception();
        }
    });

    for (size_t attempt = 0; attempt < 500 && access(connection.c_str(), F_OK) != 0; ++attempt)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_EQ(access(connection.c_str(), F_OK), 0);

    adios2::ADIOS adios;
    auto io = adios.DeclareIO("WANStreamerWriter");
    io.SetEngine("WANStreamer");
    io.SetParameters({{"ConnectionFile", connection},
                      {"PrivateKey", privateKey},
                      {"RetryIntervalSeconds", "0.01"},
                      {"ReconnectTimeoutSeconds", "10"}});
    auto variable = io.DefineVariable<double>("samples", {4}, {0}, {4});
    auto engine = io.Open("stream", adios2::Mode::Write);
    const std::vector<double> expected{1.0, 2.0, 3.0, 4.0};
    engine.BeginStep();
    engine.Put(variable, expected.data(), adios2::Mode::Sync);
    engine.EndStep();
    engine.Close();
    reader.join();

    if (readerError)
        std::rethrow_exception(readerError);
    EXPECT_EQ(received, expected);

    std::remove(connection.c_str());
    std::remove((connection + ".checkpoint").c_str());
    std::remove(publicKey.c_str());
    std::remove(privateKey.c_str());
}

TEST(WANStreamerEngineTest, WriterEndStepQueuesBeforeReaderAcknowledgement)
{
    const std::string prefix = "/tmp/adios2-wanstreamer-async-" + std::to_string(getpid());
    const std::string connection = prefix + ".connection";
    const std::string publicKey = prefix + ".public";
    const std::string privateKey = prefix + ".private";
    WriteBytes(publicKey, PublicKey, sizeof(PublicKey));
    WriteBytes(privateKey, PrivateKey, sizeof(PrivateKey));

    std::mutex mutex;
    std::condition_variable changed;
    bool allowAcknowledgement = false;
    std::exception_ptr readerError;
    std::thread reader([&] {
        try
        {
            adios2::ADIOS adios;
            auto io = adios.DeclareIO("WANStreamerAsyncReader");
            io.SetEngine("WANStreamer");
            io.SetParameters({{"ConnectionFile", connection},
                              {"PublicKey", publicKey},
                              {"BindAddress", "127.0.0.1"},
                              {"AdvertiseAddress", "127.0.0.1"}});
            auto engine = io.Open("stream", adios2::Mode::Read);
            if (engine.BeginStep() != adios2::StepStatus::OK)
                throw std::runtime_error("reader did not receive a step");
            auto variable = io.InquireVariable<double>("value");
            double value = 0.0;
            engine.Get(variable, &value, adios2::Mode::Sync);
            std::unique_lock<std::mutex> lock(mutex);
            changed.wait(lock, [&] { return allowAcknowledgement; });
            lock.unlock();
            engine.EndStep();
            engine.Close();
        }
        catch (...)
        {
            readerError = std::current_exception();
        }
    });

    for (size_t attempt = 0; attempt < 500 && access(connection.c_str(), F_OK) != 0; ++attempt)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_EQ(access(connection.c_str(), F_OK), 0);

    adios2::ADIOS adios;
    auto io = adios.DeclareIO("WANStreamerAsyncWriter");
    io.SetEngine("WANStreamer");
    io.SetParameters({{"ConnectionFile", connection},
                      {"PrivateKey", privateKey},
                      {"RetryIntervalSeconds", "0.01"},
                      {"ReconnectTimeoutSeconds", "10"}});
    auto variable = io.DefineVariable<double>("value");
    auto engine = io.Open("stream", adios2::Mode::Write);
    const double value = 42.0;
    engine.BeginStep();
    engine.Put(variable, value, adios2::Mode::Sync);
    auto endStep = std::async(std::launch::async, [&] { engine.EndStep(); });
    const bool queued = endStep.wait_for(std::chrono::seconds(1)) == std::future_status::ready;
    {
        std::lock_guard<std::mutex> lock(mutex);
        allowAcknowledgement = true;
    }
    changed.notify_all();
    EXPECT_TRUE(queued);
    endStep.get();
    engine.Close();
    reader.join();

    if (readerError)
        std::rethrow_exception(readerError);
    std::remove(connection.c_str());
    std::remove((connection + ".checkpoint").c_str());
    std::remove(publicKey.c_str());
    std::remove(privateKey.c_str());
}

TEST(WANStreamerEngineTest, ReplaysAfterReaderRestartBeforeAcknowledgement)
{
    const std::string prefix = "/tmp/adios2-wanstreamer-replay-" + std::to_string(getpid());
    const std::string connection = prefix + ".connection";
    const std::string publicKey = prefix + ".public";
    const std::string privateKey = prefix + ".private";
    WriteBytes(publicKey, PublicKey, sizeof(PublicKey));
    WriteBytes(privateKey, PrivateKey, sizeof(PrivateKey));

    std::vector<double> replayed;
    std::exception_ptr readerError;
    std::thread reader([&] {
        try
        {
            {
                adios2::ADIOS adios;
                auto io = adios.DeclareIO("WANStreamerInterruptedReader");
                io.SetEngine("WANStreamer");
                io.SetParameters({{"ConnectionFile", connection},
                                  {"PublicKey", publicKey},
                                  {"BindAddress", "127.0.0.1"},
                                  {"AdvertiseAddress", "127.0.0.1"}});
                auto engine = io.Open("stream", adios2::Mode::Read);
                if (engine.BeginStep() != adios2::StepStatus::OK)
                    throw std::runtime_error("first reader did not receive a step");
                auto variable = io.InquireVariable<double>("samples");
                std::vector<double> discarded(4);
                engine.Get(variable, discarded.data(), adios2::Mode::Sync);
                engine.Close(); // disconnect without EndStep: no acknowledgement
            }
            {
                adios2::ADIOS adios;
                auto io = adios.DeclareIO("WANStreamerRestartedReader");
                io.SetEngine("WANStreamer");
                io.SetParameters({{"ConnectionFile", connection},
                                  {"PublicKey", publicKey},
                                  {"BindAddress", "127.0.0.1"},
                                  {"AdvertiseAddress", "127.0.0.1"}});
                auto engine = io.Open("stream", adios2::Mode::Read);
                if (engine.BeginStep() != adios2::StepStatus::OK)
                    throw std::runtime_error("restarted reader did not receive replay");
                auto variable = io.InquireVariable<double>("samples");
                replayed.resize(4);
                engine.Get(variable, replayed.data(), adios2::Mode::Sync);
                engine.EndStep();
                engine.Close();
            }
        }
        catch (...)
        {
            readerError = std::current_exception();
        }
    });

    for (size_t attempt = 0; attempt < 500 && access(connection.c_str(), F_OK) != 0; ++attempt)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_EQ(access(connection.c_str(), F_OK), 0);

    adios2::ADIOS adios;
    auto io = adios.DeclareIO("WANStreamerReplayWriter");
    io.SetEngine("WANStreamer");
    io.SetParameters({{"ConnectionFile", connection},
                      {"PrivateKey", privateKey},
                      {"RetryIntervalSeconds", "0.01"},
                      {"ReconnectTimeoutSeconds", "10"}});
    auto variable = io.DefineVariable<double>("samples", {4}, {0}, {4});
    auto engine = io.Open("stream", adios2::Mode::Write);
    const std::vector<double> expected{5.0, 6.0, 7.0, 8.0};
    engine.BeginStep();
    engine.Put(variable, expected.data(), adios2::Mode::Sync);
    engine.EndStep();
    engine.Close();
    reader.join();

    if (readerError)
        std::rethrow_exception(readerError);
    EXPECT_EQ(replayed, expected);
    std::remove(connection.c_str());
    std::remove((connection + ".checkpoint").c_str());
    std::remove(publicKey.c_str());
    std::remove(privateKey.c_str());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
