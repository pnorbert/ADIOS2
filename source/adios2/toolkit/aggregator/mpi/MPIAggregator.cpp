/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MPIAggregator.cpp
 *
 *  Created on: Feb 20, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "MPIAggregator.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace aggregator
{

MPIAggregator::MPIAggregator() : m_Comm(AMPI_Comm()) {}

MPIAggregator::~MPIAggregator() {}

// void MPIAggregator::Init(const size_t subStreams, const AMPI_Comm
// &parentComm) {}

void MPIAggregator::SwapBuffers(const int step) noexcept {}

void MPIAggregator::ResetBuffers() noexcept {}

BufferSTL &MPIAggregator::GetConsumerBuffer(BufferSTL &bufferSTL)
{
    return bufferSTL;
}

void MPIAggregator::Close()
{
    if (m_IsActive)
    {
        helper::CheckMPIReturn(m_Comm.Free(),
                               "freeing aggregators comm at Close\n");
        m_IsActive = false;
    }
}

// PROTECTED
void MPIAggregator::InitComm(const size_t subStreams,
                             const AMPI_Comm &parentComm)
{
    int parentRank;
    int parentSize;
    parentComm.Rank(&parentRank);
    parentComm.Size(&parentSize);

    const size_t processes = static_cast<size_t>(parentSize);
    size_t stride = processes / subStreams + 1;
    const size_t remainder = processes % subStreams;

    size_t consumer = 0;

    for (auto s = 0; s < subStreams; ++s)
    {
        if (s >= remainder)
        {
            stride = processes / subStreams;
        }

        if (static_cast<size_t>(parentRank) >= consumer &&
            static_cast<size_t>(parentRank) < consumer + stride)
        {
            helper::CheckMPIReturn(
                parentComm.Split(static_cast<int>(consumer), parentRank,
                                 &m_Comm),
                "creating aggregators comm with split at Open");
            m_ConsumerRank = static_cast<int>(consumer);
            m_SubStreamIndex = static_cast<size_t>(s);
        }

        consumer += stride;
    }

    m_Comm.Rank(&m_Rank);
    m_Comm.Size(&m_Size);

    if (m_Rank != 0)
    {
        m_IsConsumer = false;
    }

    m_IsActive = true;
    m_SubStreams = subStreams;
}

void MPIAggregator::HandshakeRank(const int rank)
{
    int message = -1;
    if (m_Rank == rank)
    {
        message = m_Rank;
    }

    helper::CheckMPIReturn(
        m_Comm.MPI()->Bcast(&message, 1, AMPI_INT, rank, m_Comm),
        "handshake with aggregator rank 0 at Open");
}

} // end namespace aggregator
} // end namespace adios2
