/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <complex>
#include <cstdint>
#include <string>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "SmallTestData260.h"

std::string dataPath;   // comes from command line
std::string engineName; // comes from command line

class ReadAttributes : public ::testing::Test
{
public:
    ReadAttributes() = default;

    SmallTestData260 m_TestData;
};

// ADIOS2 write, read for single value attributes
TEST_F(ReadAttributes, SingleTypes)
{
    const std::string fName = dataPath +
                              std::string(&adios2::PathSeparator, 1) +
                              engineName + "AttributesSingleTypes.bp";

    const std::string zero = std::to_string(0);
    const std::string s1_Single = std::string("s1_Single_") + zero;
    const std::string s1_Array = std::string("s1_Array_") + zero;
    const std::string i8_Single = std::string("i8_Single_") + zero;
    const std::string i16_Single = std::string("i16_Single_") + zero;
    const std::string i32_Single = std::string("i32_Single_") + zero;
    const std::string i64_Single = std::string("i64_Single_") + zero;
    const std::string u8_Single = std::string("u8_Single_") + zero;
    const std::string u16_Single = std::string("u16_Single_") + zero;
    const std::string u32_Single = std::string("u32_Single_") + zero;
    const std::string u64_Single = std::string("u64_Single_") + zero;
    const std::string r32_Single = std::string("r32_Single_") + zero;
    const std::string r64_Single = std::string("r64_Single_") + zero;
    const std::string r128_Single = std::string("r128_Single_") + zero;
    const std::string cr32_Single = std::string("cr32_Single_") + zero;
    const std::string cr64_Single = std::string("cr64_Single_") + zero;

    // When collective meta generation has landed, use
    // generateNewSmallTestData(m_TestData, 0, mpiRank, mpiSize);
    // Generate current testing data
    SmallTestData260 currentTestData =
        generateNewSmallTestData(m_TestData, 0, 0, 0);

    adios2::ADIOS adios;

    {
        adios2::IO ioRead = adios.DeclareIO("ioRead");
        if (!engineName.empty())
        {
            ioRead.SetEngine(engineName);
        }

        adios2::Engine bpRead = ioRead.Open(fName, adios2::Mode::Read);

        auto attr_s1 = ioRead.InquireAttribute<std::string>(s1_Single);
        auto attr_s1a = ioRead.InquireAttribute<std::string>(s1_Array);
        auto attr_i8 = ioRead.InquireAttribute<int8_t>(i8_Single);
        auto attr_i16 = ioRead.InquireAttribute<int16_t>(i16_Single);
        auto attr_i32 = ioRead.InquireAttribute<int32_t>(i32_Single);
        auto attr_i64 = ioRead.InquireAttribute<int64_t>(i64_Single);

        auto attr_u8 = ioRead.InquireAttribute<uint8_t>(u8_Single);
        auto attr_u16 = ioRead.InquireAttribute<uint16_t>(u16_Single);
        auto attr_u32 = ioRead.InquireAttribute<uint32_t>(u32_Single);
        auto attr_u64 = ioRead.InquireAttribute<uint64_t>(u64_Single);

        auto attr_r32 = ioRead.InquireAttribute<float>(r32_Single);
        auto attr_r64 = ioRead.InquireAttribute<double>(r64_Single);
        auto attr_r128 = ioRead.InquireAttribute<long double>(r128_Single);

        auto attr_cr32 =
            ioRead.InquireAttribute<std::complex<float>>(cr32_Single);
        auto attr_cr64 =
            ioRead.InquireAttribute<std::complex<double>>(cr64_Single);

        EXPECT_TRUE(attr_s1);
        ASSERT_EQ(attr_s1.Name(), s1_Single);
        ASSERT_EQ(attr_s1.Data().size() == 1, true);
        ASSERT_EQ(attr_s1.Type(), adios2::GetType<std::string>());
        ASSERT_EQ(attr_s1.Data().front(), currentTestData.S1);

        EXPECT_TRUE(attr_s1a);
        ASSERT_EQ(attr_s1a.Name(), s1_Array);
        ASSERT_EQ(attr_s1a.Data().size() == 1, true);
        ASSERT_EQ(attr_s1a.Type(), adios2::GetType<std::string>());
        ASSERT_EQ(attr_s1a.Data()[0], currentTestData.S1array[0]);

        EXPECT_TRUE(attr_i8);
        ASSERT_EQ(attr_i8.Name(), i8_Single);
        ASSERT_EQ(attr_i8.Data().size() == 1, true);
        ASSERT_EQ(attr_i8.Type(), adios2::GetType<int8_t>());
        ASSERT_EQ(attr_i8.Data().front(), currentTestData.I8.front());

        EXPECT_TRUE(attr_i16);
        ASSERT_EQ(attr_i16.Name(), i16_Single);
        ASSERT_EQ(attr_i16.Data().size() == 1, true);
        ASSERT_EQ(attr_i16.Type(), adios2::GetType<int16_t>());
        ASSERT_EQ(attr_i16.Data().front(), currentTestData.I16.front());

        EXPECT_TRUE(attr_i32);
        ASSERT_EQ(attr_i32.Name(), i32_Single);
        ASSERT_EQ(attr_i32.Data().size() == 1, true);
        ASSERT_EQ(attr_i32.Type(), adios2::GetType<int32_t>());
        ASSERT_EQ(attr_i32.Data().front(), currentTestData.I32.front());

        EXPECT_TRUE(attr_i64);
        ASSERT_EQ(attr_i64.Name(), i64_Single);
        ASSERT_EQ(attr_i64.Data().size() == 1, true);
        ASSERT_EQ(attr_i64.Type(), adios2::GetType<int64_t>());
        ASSERT_EQ(attr_i64.Data().front(), currentTestData.I64.front());

        EXPECT_TRUE(attr_u8);
        ASSERT_EQ(attr_u8.Name(), u8_Single);
        ASSERT_EQ(attr_u8.Data().size() == 1, true);
        ASSERT_EQ(attr_u8.Type(), adios2::GetType<uint8_t>());
        ASSERT_EQ(attr_u8.Data().front(), currentTestData.U8.front());

        EXPECT_TRUE(attr_u16);
        ASSERT_EQ(attr_u16.Name(), u16_Single);
        ASSERT_EQ(attr_u16.Data().size() == 1, true);
        ASSERT_EQ(attr_u16.Type(), adios2::GetType<uint16_t>());
        ASSERT_EQ(attr_u16.Data().front(), currentTestData.U16.front());

        EXPECT_TRUE(attr_u32);
        ASSERT_EQ(attr_u32.Name(), u32_Single);
        ASSERT_EQ(attr_u32.Data().size() == 1, true);
        ASSERT_EQ(attr_u32.Type(), adios2::GetType<uint32_t>());
        ASSERT_EQ(attr_u32.Data().front(), currentTestData.U32.front());

        EXPECT_TRUE(attr_u64);
        ASSERT_EQ(attr_u64.Name(), u64_Single);
        ASSERT_EQ(attr_u64.Data().size() == 1, true);
        ASSERT_EQ(attr_u64.Type(), adios2::GetType<uint64_t>());
        ASSERT_EQ(attr_u64.Data().front(), currentTestData.U64.front());

        EXPECT_TRUE(attr_r32);
        ASSERT_EQ(attr_r32.Name(), r32_Single);
        ASSERT_EQ(attr_r32.Data().size() == 1, true);
        ASSERT_EQ(attr_r32.Type(), adios2::GetType<float>());
        ASSERT_EQ(attr_r32.Data().front(), currentTestData.R32.front());

        EXPECT_TRUE(attr_r64);
        ASSERT_EQ(attr_r64.Name(), r64_Single);
        ASSERT_EQ(attr_r64.Data().size() == 1, true);
        ASSERT_EQ(attr_r64.Type(), adios2::GetType<double>());
        ASSERT_EQ(attr_r64.Data().front(), currentTestData.R64.front());

        EXPECT_TRUE(attr_r128);
        ASSERT_EQ(attr_r128.Name(), r128_Single);
        ASSERT_EQ(attr_r128.Data().size() == 1, true);
        ASSERT_EQ(attr_r128.Type(), adios2::GetType<long double>());
        ASSERT_EQ(attr_r128.Data().front(), currentTestData.R128.front());

        EXPECT_TRUE(attr_cr32);
        ASSERT_EQ(attr_cr32.Name(), cr32_Single);
        ASSERT_EQ(attr_cr32.Data().size() == 1, true);
        ASSERT_EQ(attr_cr32.Type(), adios2::GetType<std::complex<float>>());
        ASSERT_EQ(attr_cr32.Data().front(), currentTestData.CR32.front());

        EXPECT_TRUE(attr_cr64);
        ASSERT_EQ(attr_cr64.Name(), cr64_Single);
        ASSERT_EQ(attr_cr64.Data().size() == 1, true);
        ASSERT_EQ(attr_cr64.Type(), adios2::GetType<std::complex<double>>());
        ASSERT_EQ(attr_cr64.Data().front(), currentTestData.CR64.front());

        bpRead.Close();
    }
}

// ADIOS2 write read for array attributes
TEST_F(ReadAttributes, ArrayTypes)
{
    const std::string fName = dataPath +
                              std::string(&adios2::PathSeparator, 1) +
                              engineName + "AttributesArrayTypes.bp";

    const std::string zero = std::to_string(0);
    const std::string s1_Array = std::string("s1_Array_") + zero;
    const std::string i8_Array = std::string("i8_Array_") + zero;
    const std::string i16_Array = std::string("i16_Array_") + zero;
    const std::string i32_Array = std::string("i32_Array_") + zero;
    const std::string i64_Array = std::string("i64_Array_") + zero;
    const std::string u8_Array = std::string("u8_Array_") + zero;
    const std::string u16_Array = std::string("u16_Array_") + zero;
    const std::string u32_Array = std::string("u32_Array_") + zero;
    const std::string u64_Array = std::string("u64_Array_") + zero;
    const std::string r32_Array = std::string("r32_Array_") + zero;
    const std::string r64_Array = std::string("r64_Array_") + zero;
    const std::string r128_Array = std::string("r128_Array_") + zero;
    const std::string cr32_Array = std::string("cr32_Array_") + zero;
    const std::string cr64_Array = std::string("cr64_Array_") + zero;

    // When collective meta generation has landed, use
    // generateNewSmallTestData(m_TestData, 0, mpiRank, mpiSize);
    // Generate current testing data
    SmallTestData260 currentTestData =
        generateNewSmallTestData(m_TestData, 0, 0, 0);

    adios2::ADIOS adios;

    {
        adios2::IO ioRead = adios.DeclareIO("ioRead");

        if (!engineName.empty())
        {
            ioRead.SetEngine(engineName);
        }

        adios2::Engine bpRead = ioRead.Open(fName, adios2::Mode::Read);

        auto attr_s1 = ioRead.InquireAttribute<std::string>(s1_Array);

        auto attr_i8 = ioRead.InquireAttribute<int8_t>(i8_Array);
        auto attr_i16 = ioRead.InquireAttribute<int16_t>(i16_Array);
        auto attr_i32 = ioRead.InquireAttribute<int32_t>(i32_Array);
        auto attr_i64 = ioRead.InquireAttribute<int64_t>(i64_Array);

        auto attr_u8 = ioRead.InquireAttribute<uint8_t>(u8_Array);
        auto attr_u16 = ioRead.InquireAttribute<uint16_t>(u16_Array);
        auto attr_u32 = ioRead.InquireAttribute<uint32_t>(u32_Array);
        auto attr_u64 = ioRead.InquireAttribute<uint64_t>(u64_Array);

        auto attr_r32 = ioRead.InquireAttribute<float>(r32_Array);
        auto attr_r64 = ioRead.InquireAttribute<double>(r64_Array);
        auto attr_r128 = ioRead.InquireAttribute<long double>(r128_Array);

        auto attr_cr32 =
            ioRead.InquireAttribute<std::complex<float>>(cr32_Array);
        auto attr_cr64 =
            ioRead.InquireAttribute<std::complex<double>>(cr64_Array);

        EXPECT_TRUE(attr_s1);
        ASSERT_EQ(attr_s1.Name(), s1_Array);
        ASSERT_EQ(attr_s1.Data().size() == 1, false);
        ASSERT_EQ(attr_s1.Type(), adios2::GetType<std::string>());

        EXPECT_TRUE(attr_i8);
        ASSERT_EQ(attr_i8.Name(), i8_Array);
        ASSERT_EQ(attr_i8.Data().size() == 1, false);
        ASSERT_EQ(attr_i8.Type(), adios2::GetType<int8_t>());

        EXPECT_TRUE(attr_i16);
        ASSERT_EQ(attr_i16.Name(), i16_Array);
        ASSERT_EQ(attr_i16.Data().size() == 1, false);
        ASSERT_EQ(attr_i16.Type(), adios2::GetType<int16_t>());

        EXPECT_TRUE(attr_i32);
        ASSERT_EQ(attr_i32.Name(), i32_Array);
        ASSERT_EQ(attr_i32.Data().size() == 1, false);
        ASSERT_EQ(attr_i32.Type(), adios2::GetType<int32_t>());

        EXPECT_TRUE(attr_i64);
        ASSERT_EQ(attr_i64.Name(), i64_Array);
        ASSERT_EQ(attr_i64.Data().size() == 1, false);
        ASSERT_EQ(attr_i64.Type(), adios2::GetType<int64_t>());

        EXPECT_TRUE(attr_u8);
        ASSERT_EQ(attr_u8.Name(), u8_Array);
        ASSERT_EQ(attr_u8.Data().size() == 1, false);
        ASSERT_EQ(attr_u8.Type(), adios2::GetType<uint8_t>());

        EXPECT_TRUE(attr_u16);
        ASSERT_EQ(attr_u16.Name(), u16_Array);
        ASSERT_EQ(attr_u16.Data().size() == 1, false);
        ASSERT_EQ(attr_u16.Type(), adios2::GetType<uint16_t>());

        EXPECT_TRUE(attr_u32);
        ASSERT_EQ(attr_u32.Name(), u32_Array);
        ASSERT_EQ(attr_u32.Data().size() == 1, false);
        ASSERT_EQ(attr_u32.Type(), adios2::GetType<uint32_t>());

        EXPECT_TRUE(attr_u64);
        ASSERT_EQ(attr_u64.Name(), u64_Array);
        ASSERT_EQ(attr_u64.Data().size() == 1, false);
        ASSERT_EQ(attr_u64.Type(), adios2::GetType<uint64_t>());

        EXPECT_TRUE(attr_r32);
        ASSERT_EQ(attr_r32.Name(), r32_Array);
        ASSERT_EQ(attr_r32.Data().size() == 1, false);
        ASSERT_EQ(attr_r32.Type(), adios2::GetType<float>());

        EXPECT_TRUE(attr_r64);
        ASSERT_EQ(attr_r64.Name(), r64_Array);
        ASSERT_EQ(attr_r64.Data().size() == 1, false);
        ASSERT_EQ(attr_r64.Type(), adios2::GetType<double>());

        EXPECT_TRUE(attr_r128);
        ASSERT_EQ(attr_r128.Name(), r128_Array);
        ASSERT_EQ(attr_r128.Data().size() == 1, false);
        ASSERT_EQ(attr_r128.Type(), adios2::GetType<long double>());

        EXPECT_TRUE(attr_cr32);
        ASSERT_EQ(attr_cr32.Name(), cr32_Array);
        ASSERT_EQ(attr_cr32.Data().size() == 1, false);
        ASSERT_EQ(attr_cr32.Type(), adios2::GetType<std::complex<float>>());

        EXPECT_TRUE(attr_cr64);
        ASSERT_EQ(attr_cr64.Name(), cr64_Array);
        ASSERT_EQ(attr_cr64.Data().size() == 1, false);
        ASSERT_EQ(attr_cr64.Type(), adios2::GetType<std::complex<double>>());

        auto I8 = attr_i8.Data();
        auto I16 = attr_i16.Data();
        auto I32 = attr_i32.Data();
        auto I64 = attr_i64.Data();

        auto U8 = attr_u8.Data();
        auto U16 = attr_u16.Data();
        auto U32 = attr_u32.Data();
        auto U64 = attr_u64.Data();

        const size_t Nx = 10;
        for (size_t i = 0; i < Nx; ++i)
        {
            EXPECT_EQ(I8[i], currentTestData.I8[i]);
            EXPECT_EQ(I16[i], currentTestData.I16[i]);
            EXPECT_EQ(I32[i], currentTestData.I32[i]);
            EXPECT_EQ(I64[i], currentTestData.I64[i]);

            EXPECT_EQ(U8[i], currentTestData.U8[i]);
            EXPECT_EQ(U16[i], currentTestData.U16[i]);
            EXPECT_EQ(U32[i], currentTestData.U32[i]);
            EXPECT_EQ(U64[i], currentTestData.U64[i]);
        }

        bpRead.Close();
    }
}

TEST_F(ReadAttributes, OfVariableSingleTypes)
{
    const std::string fName = dataPath +
                              std::string(&adios2::PathSeparator, 1) +
                              engineName + "AttributesOfVariableSingleTypes.bp";

    const std::string zero = std::to_string(0);
    const std::string s1_Single = std::string("s1_Single_") + zero;
    const std::string i8_Single = std::string("i8_Single_") + zero;
    const std::string i16_Single = std::string("i16_Single_") + zero;
    const std::string i32_Single = std::string("i32_Single_") + zero;
    const std::string i64_Single = std::string("i64_Single_") + zero;
    const std::string u8_Single = std::string("u8_Single_") + zero;
    const std::string u16_Single = std::string("u16_Single_") + zero;
    const std::string u32_Single = std::string("u32_Single_") + zero;
    const std::string u64_Single = std::string("u64_Single_") + zero;
    const std::string r32_Single = std::string("r32_Single_") + zero;
    const std::string r64_Single = std::string("r64_Single_") + zero;
    const std::string r128_Single = std::string("r128_Single_") + zero;
    const std::string cr32_Single = std::string("cr32_Single_") + zero;
    const std::string cr64_Single = std::string("cr64_Single_") + zero;

    // When collective meta generation has landed, use
    // generateNewSmallTestData(m_TestData, 0, mpiRank, mpiSize);
    // Generate current testing data
    SmallTestData260 currentTestData =
        generateNewSmallTestData(m_TestData, 0, 0, 0);

    const std::string separator = "/";

    adios2::ADIOS adios;

    {
        adios2::IO ioRead = adios.DeclareIO("ioRead");
        if (!engineName.empty())
        {
            ioRead.SetEngine(engineName);
        }

        adios2::Engine bpRead = ioRead.Open(fName, adios2::Mode::Read);

        auto var = ioRead.InquireVariable<int>("myVar");

        auto attr_s1 =
            ioRead.InquireAttribute<std::string>(s1_Single, var.Name());
        auto attr_i8 = ioRead.InquireAttribute<int8_t>(i8_Single, var.Name());
        auto attr_i16 =
            ioRead.InquireAttribute<int16_t>(i16_Single, var.Name());
        auto attr_i32 =
            ioRead.InquireAttribute<int32_t>(i32_Single, var.Name());
        auto attr_i64 =
            ioRead.InquireAttribute<int64_t>(i64_Single, var.Name());

        auto attr_u8 = ioRead.InquireAttribute<uint8_t>(u8_Single, var.Name());
        auto attr_u16 =
            ioRead.InquireAttribute<uint16_t>(u16_Single, var.Name());
        auto attr_u32 =
            ioRead.InquireAttribute<uint32_t>(u32_Single, var.Name());
        auto attr_u64 =
            ioRead.InquireAttribute<uint64_t>(u64_Single, var.Name());

        auto attr_r32 = ioRead.InquireAttribute<float>(r32_Single, var.Name());
        auto attr_r64 = ioRead.InquireAttribute<double>(r64_Single, var.Name());
        auto attr_r128 =
            ioRead.InquireAttribute<long double>(r128_Single, var.Name());

        auto attr_cr32 = ioRead.InquireAttribute<std::complex<float>>(
            cr32_Single, var.Name());
        auto attr_cr64 = ioRead.InquireAttribute<std::complex<double>>(
            cr64_Single, var.Name());

        EXPECT_TRUE(attr_s1);
        ASSERT_EQ(attr_s1.Name(), var.Name() + separator + s1_Single);
        ASSERT_EQ(attr_s1.Data().size() == 1, true);
        ASSERT_EQ(attr_s1.Type(), adios2::GetType<std::string>());
        ASSERT_EQ(attr_s1.Data().front(), currentTestData.S1);

        EXPECT_TRUE(attr_i8);
        ASSERT_EQ(attr_i8.Name(), var.Name() + separator + i8_Single);
        ASSERT_EQ(attr_i8.Data().size() == 1, true);
        ASSERT_EQ(attr_i8.Type(), adios2::GetType<int8_t>());
        ASSERT_EQ(attr_i8.Data().front(), currentTestData.I8.front());

        EXPECT_TRUE(attr_i16);
        ASSERT_EQ(attr_i16.Name(), var.Name() + separator + i16_Single);
        ASSERT_EQ(attr_i16.Data().size() == 1, true);
        ASSERT_EQ(attr_i16.Type(), adios2::GetType<int16_t>());
        ASSERT_EQ(attr_i16.Data().front(), currentTestData.I16.front());

        EXPECT_TRUE(attr_i32);
        ASSERT_EQ(attr_i32.Name(), var.Name() + separator + i32_Single);
        ASSERT_EQ(attr_i32.Data().size() == 1, true);
        ASSERT_EQ(attr_i32.Type(), adios2::GetType<int32_t>());
        ASSERT_EQ(attr_i32.Data().front(), currentTestData.I32.front());

        EXPECT_TRUE(attr_i64);
        ASSERT_EQ(attr_i64.Name(), var.Name() + separator + i64_Single);
        ASSERT_EQ(attr_i64.Data().size() == 1, true);
        ASSERT_EQ(attr_i64.Type(), adios2::GetType<int64_t>());
        ASSERT_EQ(attr_i64.Data().front(), currentTestData.I64.front());

        EXPECT_TRUE(attr_u8);
        ASSERT_EQ(attr_u8.Name(), var.Name() + separator + u8_Single);
        ASSERT_EQ(attr_u8.Data().size() == 1, true);
        ASSERT_EQ(attr_u8.Type(), adios2::GetType<uint8_t>());
        ASSERT_EQ(attr_u8.Data().front(), currentTestData.U8.front());

        EXPECT_TRUE(attr_u16);
        ASSERT_EQ(attr_u16.Name(), var.Name() + separator + u16_Single);
        ASSERT_EQ(attr_u16.Data().size() == 1, true);
        ASSERT_EQ(attr_u16.Type(), adios2::GetType<uint16_t>());
        ASSERT_EQ(attr_u16.Data().front(), currentTestData.U16.front());

        EXPECT_TRUE(attr_u32);
        ASSERT_EQ(attr_u32.Name(), var.Name() + separator + u32_Single);
        ASSERT_EQ(attr_u32.Data().size() == 1, true);
        ASSERT_EQ(attr_u32.Type(), adios2::GetType<uint32_t>());
        ASSERT_EQ(attr_u32.Data().front(), currentTestData.U32.front());

        EXPECT_TRUE(attr_u64);
        ASSERT_EQ(attr_u64.Name(), var.Name() + separator + u64_Single);
        ASSERT_EQ(attr_u64.Data().size() == 1, true);
        ASSERT_EQ(attr_u64.Type(), adios2::GetType<uint64_t>());
        ASSERT_EQ(attr_u64.Data().front(), currentTestData.U64.front());

        EXPECT_TRUE(attr_r32);
        ASSERT_EQ(attr_r32.Name(), var.Name() + separator + r32_Single);
        ASSERT_EQ(attr_r32.Data().size() == 1, true);
        ASSERT_EQ(attr_r32.Type(), adios2::GetType<float>());
        ASSERT_EQ(attr_r32.Data().front(), currentTestData.R32.front());

        EXPECT_TRUE(attr_r64);
        ASSERT_EQ(attr_r64.Name(), var.Name() + separator + r64_Single);
        ASSERT_EQ(attr_r64.Data().size() == 1, true);
        ASSERT_EQ(attr_r64.Type(), adios2::GetType<double>());
        ASSERT_EQ(attr_r64.Data().front(), currentTestData.R64.front());

        EXPECT_TRUE(attr_r128);
        ASSERT_EQ(attr_r128.Name(), var.Name() + separator + r128_Single);
        ASSERT_EQ(attr_r128.Data().size() == 1, true);
        ASSERT_EQ(attr_r128.Type(), adios2::GetType<long double>());
        ASSERT_EQ(attr_r128.Data().front(), currentTestData.R128.front());

        EXPECT_TRUE(attr_cr32);
        ASSERT_EQ(attr_cr32.Name(), var.Name() + separator + cr32_Single);
        ASSERT_EQ(attr_cr32.Data().size() == 1, true);
        ASSERT_EQ(attr_cr32.Type(), adios2::GetType<std::complex<float>>());
        ASSERT_EQ(attr_cr32.Data().front(), currentTestData.CR32.front());

        EXPECT_TRUE(attr_cr64);
        ASSERT_EQ(attr_cr64.Name(), var.Name() + separator + cr64_Single);
        ASSERT_EQ(attr_cr64.Data().size() == 1, true);
        ASSERT_EQ(attr_cr64.Type(), adios2::GetType<std::complex<double>>());
        ASSERT_EQ(attr_cr64.Data().front(), currentTestData.CR64.front());

        bpRead.Close();
    }
}

// ADIOS2 write read for array attributes
TEST_F(ReadAttributes, OfVariableArrayTypes)
{
    const std::string fName = dataPath +
                              std::string(&adios2::PathSeparator, 1) +
                              engineName + "AttributesOfVariableArrayTypes.bp";

    const std::string zero = std::to_string(0);
    const std::string s1_Array = std::string("s1_Array_") + zero;
    const std::string i8_Array = std::string("i8_Array_") + zero;
    const std::string i16_Array = std::string("i16_Array_") + zero;
    const std::string i32_Array = std::string("i32_Array_") + zero;
    const std::string i64_Array = std::string("i64_Array_") + zero;
    const std::string u8_Array = std::string("u8_Array_") + zero;
    const std::string u16_Array = std::string("u16_Array_") + zero;
    const std::string u32_Array = std::string("u32_Array_") + zero;
    const std::string u64_Array = std::string("u64_Array_") + zero;
    const std::string r32_Array = std::string("r32_Array_") + zero;
    const std::string r64_Array = std::string("r64_Array_") + zero;
    const std::string r128_Array = std::string("r128_Array_") + zero;
    const std::string cr32_Array = std::string("cr32_Array_") + zero;
    const std::string cr64_Array = std::string("cr64_Array_") + zero;

    const std::string separator = "/";

    SmallTestData260 currentTestData =
        generateNewSmallTestData(m_TestData, 0, 0, 0);

    adios2::ADIOS adios;

    {
        adios2::IO ioRead = adios.DeclareIO("ioRead");
        if (!engineName.empty())
        {
            ioRead.SetEngine(engineName);
        }

        adios2::Engine bpRead = ioRead.Open(fName, adios2::Mode::Read);

        auto var = ioRead.InquireVariable<int>("myVar");

        auto attr_s1 =
            ioRead.InquireAttribute<std::string>(s1_Array, var.Name());

        auto attr_i8 = ioRead.InquireAttribute<int8_t>(i8_Array, var.Name());
        auto attr_i16 = ioRead.InquireAttribute<int16_t>(i16_Array, var.Name());
        auto attr_i32 = ioRead.InquireAttribute<int32_t>(i32_Array, var.Name());
        auto attr_i64 = ioRead.InquireAttribute<int64_t>(i64_Array, var.Name());

        auto attr_u8 = ioRead.InquireAttribute<uint8_t>(u8_Array, var.Name());
        auto attr_u16 =
            ioRead.InquireAttribute<uint16_t>(u16_Array, var.Name());
        auto attr_u32 =
            ioRead.InquireAttribute<uint32_t>(u32_Array, var.Name());
        auto attr_u64 =
            ioRead.InquireAttribute<uint64_t>(u64_Array, var.Name());

        auto attr_r32 = ioRead.InquireAttribute<float>(r32_Array, var.Name());
        auto attr_r64 = ioRead.InquireAttribute<double>(r64_Array, var.Name());
        auto attr_r128 =
            ioRead.InquireAttribute<long double>(r128_Array, var.Name());

        auto attr_cr32 = ioRead.InquireAttribute<std::complex<float>>(
            cr32_Array, var.Name());
        auto attr_cr64 = ioRead.InquireAttribute<std::complex<double>>(
            cr64_Array, var.Name());

        EXPECT_TRUE(attr_s1);
        ASSERT_EQ(attr_s1.Name(), var.Name() + separator + s1_Array);
        ASSERT_EQ(attr_s1.Data().size() == 1, false);
        ASSERT_EQ(attr_s1.Type(), adios2::GetType<std::string>());

        EXPECT_TRUE(attr_i8);
        ASSERT_EQ(attr_i8.Name(), var.Name() + separator + i8_Array);
        ASSERT_EQ(attr_i8.Data().size() == 1, false);
        ASSERT_EQ(attr_i8.Type(), adios2::GetType<int8_t>());

        EXPECT_TRUE(attr_i16);
        ASSERT_EQ(attr_i16.Name(), var.Name() + separator + i16_Array);
        ASSERT_EQ(attr_i16.Data().size() == 1, false);
        ASSERT_EQ(attr_i16.Type(), adios2::GetType<int16_t>());

        EXPECT_TRUE(attr_i32);
        ASSERT_EQ(attr_i32.Name(), var.Name() + separator + i32_Array);
        ASSERT_EQ(attr_i32.Data().size() == 1, false);
        ASSERT_EQ(attr_i32.Type(), adios2::GetType<int32_t>());

        EXPECT_TRUE(attr_i64);
        ASSERT_EQ(attr_i64.Name(), var.Name() + separator + i64_Array);
        ASSERT_EQ(attr_i64.Data().size() == 1, false);
        ASSERT_EQ(attr_i64.Type(), adios2::GetType<int64_t>());

        EXPECT_TRUE(attr_u8);
        ASSERT_EQ(attr_u8.Name(), var.Name() + separator + u8_Array);
        ASSERT_EQ(attr_u8.Data().size() == 1, false);
        ASSERT_EQ(attr_u8.Type(), adios2::GetType<uint8_t>());

        EXPECT_TRUE(attr_u16);
        ASSERT_EQ(attr_u16.Name(), var.Name() + separator + u16_Array);
        ASSERT_EQ(attr_u16.Data().size() == 1, false);
        ASSERT_EQ(attr_u16.Type(), adios2::GetType<uint16_t>());

        EXPECT_TRUE(attr_u32);
        ASSERT_EQ(attr_u32.Name(), var.Name() + separator + u32_Array);
        ASSERT_EQ(attr_u32.Data().size() == 1, false);
        ASSERT_EQ(attr_u32.Type(), adios2::GetType<uint32_t>());

        EXPECT_TRUE(attr_u64);
        ASSERT_EQ(attr_u64.Name(), var.Name() + separator + u64_Array);
        ASSERT_EQ(attr_u64.Data().size() == 1, false);
        ASSERT_EQ(attr_u64.Type(), adios2::GetType<uint64_t>());

        EXPECT_TRUE(attr_r32);
        ASSERT_EQ(attr_r32.Name(), var.Name() + separator + r32_Array);
        ASSERT_EQ(attr_r32.Data().size() == 1, false);
        ASSERT_EQ(attr_r32.Type(), adios2::GetType<float>());

        EXPECT_TRUE(attr_r64);
        ASSERT_EQ(attr_r64.Name(), var.Name() + separator + r64_Array);
        ASSERT_EQ(attr_r64.Data().size() == 1, false);
        ASSERT_EQ(attr_r64.Type(), adios2::GetType<double>());

        EXPECT_TRUE(attr_r128);
        ASSERT_EQ(attr_r128.Name(), var.Name() + separator + r128_Array);
        ASSERT_EQ(attr_r128.Data().size() == 1, false);
        ASSERT_EQ(attr_r128.Type(), adios2::GetType<long double>());

        EXPECT_TRUE(attr_cr32);
        ASSERT_EQ(attr_cr32.Name(), var.Name() + separator + cr32_Array);
        ASSERT_EQ(attr_cr32.Data().size() == 1, false);
        ASSERT_EQ(attr_cr32.Type(), adios2::GetType<std::complex<float>>());

        EXPECT_TRUE(attr_cr64);
        ASSERT_EQ(attr_cr64.Name(), var.Name() + separator + cr64_Array);
        ASSERT_EQ(attr_cr64.Data().size() == 1, false);
        ASSERT_EQ(attr_cr64.Type(), adios2::GetType<std::complex<double>>());

        auto I8 = attr_i8.Data();
        auto I16 = attr_i16.Data();
        auto I32 = attr_i32.Data();
        auto I64 = attr_i64.Data();

        auto U8 = attr_u8.Data();
        auto U16 = attr_u16.Data();
        auto U32 = attr_u32.Data();
        auto U64 = attr_u64.Data();

        const size_t Nx = 10;
        for (size_t i = 0; i < Nx; ++i)
        {
            EXPECT_EQ(I8[i], currentTestData.I8[i]);
            EXPECT_EQ(I16[i], currentTestData.I16[i]);
            EXPECT_EQ(I32[i], currentTestData.I32[i]);
            EXPECT_EQ(I64[i], currentTestData.I64[i]);

            EXPECT_EQ(U8[i], currentTestData.U8[i]);
            EXPECT_EQ(U16[i], currentTestData.U16[i]);
            EXPECT_EQ(U32[i], currentTestData.U32[i]);
            EXPECT_EQ(U64[i], currentTestData.U64[i]);
        }

        bpRead.Close();
    }
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 3)
    {
        throw std::runtime_error(
            "Test needs 2 arguments: path-to-data  engineName");
    }

    dataPath = std::string(argv[1]);
    engineName = std::string(argv[2]);

    result = RUN_ALL_TESTS();

    return result;
}
