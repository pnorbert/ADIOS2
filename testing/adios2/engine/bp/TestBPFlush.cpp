/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

/* Test to write 2 steps but each step is constructed in multiple iterations.
    We use BeginStep(Update) to indicate constructing the same step.
    BeginStep(Append) indicates the start of a new step.
*/

std::string engineName; // comes from command line

// Number of elements per process within one iteration
const std::size_t Nx = 10;
using DataArray = std::array<int32_t, Nx>;

// Number of iterations to create the step
const std::size_t nIterations = 3;

class BPFlush : public ::testing::Test
{
protected:
    BPFlush() = default;

    const DataArray I32 = {
        {512, 513, -510, 515, -508, 517, -506, 519, -504, 521}};

    DataArray GenerateData(int step, int block, int rank, int size)
    {
        DataArray d;
        int j = rank + 1 + ((step * nIterations) + block) * size;
        for (size_t i = 0; i < d.size(); ++i)
        {
            d[i] = I32[i] + j;
        }
        return d;
    }

    std::string ArrayToString(int32_t *data, size_t nelems)
    {
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < nelems; ++i)
        {
            ss << data[i];
            if (i < nelems - 1)
            {
                ss << " ";
            }
        }
        ss << "]";
        return ss.str();
    }
};

enum class ReadMode
{
    ReadFileAll,
    ReadFileStepByStep,
    ReadFileStepByStepBlocks,
    ReadStream,
    ReadStreamBlocks
};

std::string ReadModeToString(ReadMode r)
{
    switch (r)
    {
    case ReadMode::ReadFileAll:
        return "ReadFileAll";
    case ReadMode::ReadFileStepByStep:
        return "ReadFileStepByStep";
    case ReadMode::ReadFileStepByStepBlocks:
        return "ReadFileStepByStepBlocks";
    case ReadMode::ReadStream:
        return "ReadStream";
    case ReadMode::ReadStreamBlocks:
        return "ReadStreamBlocks";
    }
    return "unknown";
}

class BPFlushReaders : public BPFlush,
                       public ::testing::WithParamInterface<ReadMode>
{
protected:
    ReadMode GetReadMode() { return GetParam(); };
};

// Basic case: Variable written every step
TEST_P(BPFlushReaders, EveryStep)
{
    const ReadMode readMode = GetReadMode();
    std::string fname_prefix =
        "BPFlush.EveryStep." + ReadModeToString(readMode);
    int mpiRank = 0, mpiSize = 1;
    const std::size_t NSteps = 4;

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    DataArray m_TestData[NSteps];
    adios2::Dims shape{static_cast<unsigned int>(mpiSize * Nx * nIterations)};
    adios2::Dims start{static_cast<unsigned int>(mpiRank * Nx * nIterations)};
    adios2::Dims count{
        static_cast<unsigned int>(Nx)}; // one block in one iteration

    std::string fname;
#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
    fname = fname_prefix + ".MPI.bp";
#else
    adios2::ADIOS adios;
    fname = fname_prefix + ".Serial.bp";
#endif

    // Write test data using ADIOS2
    {
        if (!mpiRank)
        {
            std::cout << "Write one variable in every step" << std::endl;
        }
        adios2::IO io = adios.DeclareIO("Write");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        adios2::Engine engine = io.Open(fname, adios2::Mode::Write);

        auto var_i32 = io.DefineVariable<int32_t>("i32", shape, start, count);

        for (int step = 0; step < static_cast<int>(NSteps); ++step)
        {
            engine.BeginStep();
            for (int it = 0; it < static_cast<int>(nIterations); ++it)
            {
                // Generate test data for each process uniquely
                m_TestData[step] = GenerateData(step, it, mpiRank, mpiSize);
                std::cout << "Rank " << mpiRank << " write step " << step
                          << " iteration " << it << ": "
                          << ArrayToString(m_TestData[step].data(), Nx)
                          << std::endl;
                engine.Put(var_i32, m_TestData[step].data());
                engine.Flush();
            }
            engine.EndStep();
        }
        engine.Close();
    }
#if ADIOS2_USE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    adios2::IO io = adios.DeclareIO("Read");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    adios2::Engine engine = io.Open(fname, adios2::Mode::Read);
    EXPECT_TRUE(engine);

    if (readMode == ReadMode::ReadFileAll)
    {
        /// Read back data with File reading mode
        /// Read back the whole thing and check data
        if (!mpiRank)
        {
            std::cout << "Read with File reading mode, read all steps at once"
                      << std::endl;
        }
        auto var_i32 = io.InquireVariable<int32_t>("i32");
        EXPECT_TRUE(var_i32);
        EXPECT_EQ(var_i32.Steps(), NSteps);
        EXPECT_EQ(var_i32.StepsStart(), 0);

        auto absSteps = engine.GetAbsoluteSteps(var_i32);
        EXPECT_EQ(absSteps.size(), NSteps);
        std::cout << "Absolute steps of i32 = { ";
        for (const auto s : absSteps)
        {
            std::cout << s << " ";
        }
        std::cout << "}" << std::endl;
        for (std::size_t i = 0; i < NSteps; ++i)
        {
            EXPECT_EQ(absSteps[i], i);
        }

        var_i32.SetStepSelection({0, NSteps});
        size_t start = static_cast<size_t>(mpiRank) * Nx;
        var_i32.SetSelection({{start}, {Nx}});
        std::array<int32_t, NSteps * Nx> d;
        engine.Get(var_i32, d.data(), adios2::Mode::Sync);
        std::cout << "Rank " << mpiRank
                  << " read all steps: " << ArrayToString(d.data(), NSteps * Nx)
                  << std::endl;
        for (size_t step = 0; step < NSteps; ++step)
        {
            for (size_t i = 0; i < Nx; ++i)
            {
                EXPECT_EQ(d[step * Nx + i], m_TestData[step][i]);
            }
        }
        engine.Close();
    }
    else if (readMode == ReadMode::ReadFileStepByStep)
    {
        /// Read back data with File reading mode
        /// Read back step by step and check data
        if (!mpiRank)
        {
            std::cout << "Read with File reading mode, read step by step"
                      << std::endl;
        }
        auto var_i32 = io.InquireVariable<int32_t>("i32");
        EXPECT_TRUE(var_i32);
        EXPECT_EQ(var_i32.Steps(), NSteps);
        EXPECT_EQ(var_i32.StepsStart(), 0);
        for (size_t step = 0; step < NSteps; ++step)
        {
            var_i32.SetStepSelection({step, 1});
            size_t start = static_cast<size_t>(mpiRank) * Nx;
            var_i32.SetSelection({{start}, {Nx}});
            DataArray d;
            engine.Get(var_i32, d.data(), adios2::Mode::Sync);
            std::cout << "Rank " << mpiRank << " read step " << step << ": "
                      << ArrayToString(d.data(), Nx) << std::endl;
            for (size_t i = 0; i < Nx; ++i)
            {
                EXPECT_EQ(d[i], m_TestData[step][i]);
            }
        }
        engine.Close();
    }
    else if (readMode == ReadMode::ReadFileStepByStepBlocks)
    {
        /// Read back data with File reading mode
        /// Read back step by step and block by block and check data
        if (!mpiRank)
        {
            std::cout << "Read with File reading mode, read step by step, "
                         "block by block"
                      << std::endl;
        }
        auto var_i32 = io.InquireVariable<int32_t>("i32");
        EXPECT_TRUE(var_i32);
        EXPECT_EQ(var_i32.Steps(), NSteps);
        EXPECT_EQ(var_i32.StepsStart(), 0);
        for (size_t step = 0; step < NSteps; ++step)
        {
            var_i32.SetStepSelection({step, 1});
            size_t blockID = static_cast<size_t>(mpiRank);
            var_i32.SetBlockSelection(blockID);
            DataArray d;
            engine.Get(var_i32, d.data(), adios2::Mode::Sync);
            std::cout << "Rank " << mpiRank << " read step " << step
                      << " block " << blockID << ": "
                      << ArrayToString(d.data(), Nx) << std::endl;
            auto start = var_i32.Start();
            auto count = var_i32.Count();
            EXPECT_EQ(start[0], mpiRank * Nx);
            EXPECT_EQ(count[0], 1 * Nx);
            for (size_t i = 0; i < Nx; ++i)
            {
                EXPECT_EQ(d[i], m_TestData[step][i]);
            }
        }
        engine.Close();
    }
    else if (readMode == ReadMode::ReadStream)
    {
        /// Read back data with Stream reading mode
        /// Read back step by step and check data
        if (!mpiRank)
        {
            std::cout << "Read with Stream reading mode, read step by step"
                      << std::endl;
        }
        for (size_t step = 0; step < NSteps; ++step)
        {
            engine.BeginStep();
            auto var_i32 = io.InquireVariable<int32_t>("i32");
            EXPECT_TRUE(var_i32);
            // EXPECT_EQ(var_i32.Steps(), 1);
            EXPECT_EQ(var_i32.StepsStart(), 0);
            size_t start = static_cast<size_t>(mpiRank) * Nx;
            var_i32.SetSelection({{start}, {Nx}});
            DataArray d;
            engine.Get(var_i32, d.data(), adios2::Mode::Sync);
            std::cout << "Rank " << mpiRank << " read step " << step << ": "
                      << ArrayToString(d.data(), Nx) << std::endl;
            for (size_t i = 0; i < Nx; ++i)
            {
                EXPECT_EQ(d[i], m_TestData[step][i]);
            }
            engine.EndStep();
        }
        engine.Close();
    }
    else if (readMode == ReadMode::ReadStreamBlocks)
    {
        /// Read back data with Stream reading mode
        /// Read back step by step and check data
        if (!mpiRank)
        {
            std::cout << "Read with Stream reading mode, read step by step, "
                         "block by block"
                      << std::endl;
        }
        for (size_t step = 0; step < NSteps; ++step)
        {
            engine.BeginStep();
            auto var_i32 = io.InquireVariable<int32_t>("i32");
            EXPECT_TRUE(var_i32);
            // EXPECT_EQ(var_i32.Steps(), 1);
            EXPECT_EQ(var_i32.StepsStart(), 0);
            size_t blockID = static_cast<size_t>(mpiRank);
            var_i32.SetBlockSelection(blockID);
            DataArray d;
            engine.Get(var_i32, d.data(), adios2::Mode::Sync);
            std::cout << "Rank " << mpiRank << " read step " << step
                      << " block " << blockID << ": "
                      << ArrayToString(d.data(), Nx) << std::endl;
            auto start = var_i32.Start();
            auto count = var_i32.Count();
            EXPECT_EQ(start[0], mpiRank * Nx);
            EXPECT_EQ(count[0], 1 * Nx);
            for (size_t i = 0; i < Nx; ++i)
            {
                EXPECT_EQ(d[i], m_TestData[step][i]);
            }
            engine.EndStep();
        }
        engine.Close();
    }
#if ADIOS2_USE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
}

INSTANTIATE_TEST_SUITE_P(BPFlush, BPFlushReaders,
                         ::testing::Values(ReadMode::ReadFileAll,
                                           ReadMode::ReadFileStepByStep,
                                           ReadMode::ReadFileStepByStepBlocks,
                                           ReadMode::ReadStream,
                                           ReadMode::ReadStreamBlocks));

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
