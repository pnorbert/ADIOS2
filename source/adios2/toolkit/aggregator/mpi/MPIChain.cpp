/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MPIChain.cpp
 *
 *  Created on: Feb 21, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "MPIChain.h"

#include "adios2/helper/adiosFunctions.h" //helper::CheckMPIReturn
#include "adios2/toolkit/comm/AMPI.h"

namespace adios2
{
namespace aggregator
{

MPIChain::MPIChain() : MPIAggregator() {}

void MPIChain::Init(const size_t subStreams, AMPI_Comm parentComm)
{
    InitComm(subStreams, parentComm);
    HandshakeRank(0);
    HandshakeLinks();

    // add a receiving buffer except for the last rank (only sends)
    if (m_Rank < m_Size)
    {
        m_Buffers.emplace_back(); // just one for now
    }
}

std::vector<std::vector<AMPI_Request>> MPIChain::IExchange(BufferSTL &bufferSTL,
                                                           const int step)
{
    if (m_Size == 1)
    {
        return std::vector<std::vector<AMPI_Request>>();
    }

    BufferSTL &sendBuffer = GetSender(bufferSTL);
    const int endRank = m_Size - 1 - step;
    const bool sender = (m_Rank >= 1 && m_Rank <= endRank) ? true : false;
    const bool receiver = (m_Rank < endRank) ? true : false;

    std::vector<std::vector<AMPI_Request>> requests(2);

    if (sender) // sender
    {
        requests[0].resize(1);

        helper::CheckMPIReturn(m_Comm.MPI()->Isend(&sendBuffer.m_Position, 1,
                                                   AMPI_SIZE_T, m_Rank - 1, 0,
                                                   m_Comm, &requests[0][0]),
                               ", aggregation Isend size at iteration " +
                                   std::to_string(step) + "\n");

        const std::vector<AMPI_Request> requestsISend64 = helper::Isend64(
            sendBuffer.m_Buffer.data(), sendBuffer.m_Position, m_Rank - 1, 1,
            m_Comm,
            ", aggregation Isend64 data at iteration " + std::to_string(step));

        requests[0].insert(requests[0].end(), requestsISend64.begin(),
                           requestsISend64.end());
    }
    // receive size, resize receiving buffer and receive data
    if (receiver)
    {
        size_t bufferSize = 0;
        AMPI_Request receiveSizeRequest;
        helper::CheckMPIReturn(m_Comm.MPI()->Irecv(&bufferSize, 1, AMPI_SIZE_T,
                                                   m_Rank + 1, 0, m_Comm,
                                                   &receiveSizeRequest),
                               ", aggregation Irecv size at iteration " +
                                   std::to_string(step) + "\n");

        AMPI_Status receiveStatus;
        helper::CheckMPIReturn(
            m_Comm.MPI()->Wait(&receiveSizeRequest, &receiveStatus),
            ", aggregation waiting for receiver size at iteration " +
                std::to_string(step) + "\n");

        BufferSTL &receiveBuffer = GetReceiver(bufferSTL);
        ResizeUpdateBufferSTL(
            bufferSize, receiveBuffer,
            "in aggregation, when resizing receiving buffer to size " +
                std::to_string(bufferSize));

        requests[1] = helper::Irecv64(
            receiveBuffer.m_Buffer.data(), receiveBuffer.m_Position, m_Rank + 1,
            1, m_Comm,
            ", aggregation Irecv64 data at iteration " + std::to_string(step));
    }

    return requests;
}

std::vector<std::vector<AMPI_Request>>
MPIChain::IExchangeAbsolutePosition(BufferSTL &bufferSTL, const int step)
{
    if (m_Size == 1)
    {
        return std::vector<std::vector<AMPI_Request>>();
    }

    if (m_IsInExchangeAbsolutePosition)
    {
        throw std::runtime_error("ERROR: MPIChain::IExchangeAbsolutePosition: "
                                 "An existing exchange is still active.");
    }

    const int destination = (step != m_Size - 1) ? step + 1 : 0;
    std::vector<std::vector<AMPI_Request>> requests(
        2, std::vector<AMPI_Request>(1));

    if (step == 0)
    {
        m_SizeSend =
            (m_Rank == 0) ? bufferSTL.m_AbsolutePosition : bufferSTL.m_Position;
    }

    if (m_Rank == step)
    {
        m_ExchangeAbsolutePosition =
            (m_Rank == 0) ? m_SizeSend
                          : m_SizeSend + bufferSTL.m_AbsolutePosition;

        helper::CheckMPIReturn(
            m_Comm.MPI()->Isend(&m_ExchangeAbsolutePosition, 1, AMPI_SIZE_T,
                                destination, 0, m_Comm, &requests[0][0]),
            ", aggregation Isend absolute position at iteration " +
                std::to_string(step) + "\n");
    }
    else if (m_Rank == destination)
    {
        helper::CheckMPIReturn(
            m_Comm.MPI()->Irecv(&bufferSTL.m_AbsolutePosition, 1, AMPI_SIZE_T,
                                step, 0, m_Comm, &requests[1][0]),
            ", aggregation Irecv absolute position at iteration " +
                std::to_string(step) + "\n");
    }

    m_IsInExchangeAbsolutePosition = true;
    return requests;
}

void MPIChain::Wait(std::vector<std::vector<AMPI_Request>> &requests,
                    const int step)
{
    if (m_Size == 1)
    {
        return;
    }

    const int endRank = m_Size - 1 - step;
    const bool sender = (m_Rank >= 1 && m_Rank <= endRank) ? true : false;
    const bool receiver = (m_Rank < endRank) ? true : false;

    AMPI_Status status;
    if (receiver)
    {
        for (auto &req : requests[1])
        {
            helper::CheckMPIReturn(
                m_Comm.MPI()->Wait(&req, &status),
                ", aggregation waiting for receiver request at iteration " +
                    std::to_string(step) + "\n");
        }
    }

    if (sender)
    {
        for (auto &req : requests[0])
        {
            helper::CheckMPIReturn(
                m_Comm.MPI()->Wait(&req, &status),
                ", aggregation waiting for sender request at iteration " +
                    std::to_string(step) + "\n");
        }
    }
}

void MPIChain::WaitAbsolutePosition(
    std::vector<std::vector<AMPI_Request>> &requests, const int step)
{
    if (m_Size == 1)
    {
        return;
    }

    if (!m_IsInExchangeAbsolutePosition)
    {
        throw std::runtime_error("ERROR: MPIChain::WaitAbsolutePosition: An "
                                 "existing exchange is not active.");
    }

    AMPI_Status status;
    const int destination = (step != m_Size - 1) ? step + 1 : 0;

    if (m_Rank == destination)
    {
        helper::CheckMPIReturn(
            m_Comm.MPI()->Wait(&requests[1][0], &status),
            ", aggregation Irecv Wait absolute position at iteration " +
                std::to_string(step) + "\n");
    }

    if (m_Rank == step)
    {
        helper::CheckMPIReturn(
            m_Comm.MPI()->Wait(&requests[0][0], &status),
            ", aggregation Isend Wait absolute position at iteration " +
                std::to_string(step) + "\n");
    }
    m_IsInExchangeAbsolutePosition = false;
}

void MPIChain::SwapBuffers(const int /*step*/) noexcept
{
    m_CurrentBufferOrder = (m_CurrentBufferOrder == 0) ? 1 : 0;
}

void MPIChain::ResetBuffers() noexcept { m_CurrentBufferOrder = 0; }

BufferSTL &MPIChain::GetConsumerBuffer(BufferSTL &bufferSTL)
{
    return GetSender(bufferSTL);
}

// PRIVATE
void MPIChain::HandshakeLinks()
{
    int link = -1;

    AMPI_Request sendRequest;
    if (m_Rank > 0) // send
    {
        helper::CheckMPIReturn(
            m_Comm.MPI()->Isend(&m_Rank, 1, AMPI_INT, m_Rank - 1, 0, m_Comm,
                                &sendRequest),
            "Isend handshake with neighbor, MPIChain aggregator, at Open");
    }

    if (m_Rank < m_Size - 1) // receive
    {
        AMPI_Request receiveRequest;
        helper::CheckMPIReturn(
            m_Comm.MPI()->Irecv(&link, 1, AMPI_INT, m_Rank + 1, 0, m_Comm,
                                &receiveRequest),
            "Irecv handshake with neighbor, MPIChain aggregator, at Open");

        AMPI_Status receiveStatus;
        helper::CheckMPIReturn(
            m_Comm.MPI()->Wait(&receiveRequest, &receiveStatus),
            "Irecv Wait handshake with neighbor, MPIChain aggregator, at Open");
    }

    if (m_Rank > 0)
    {
        AMPI_Status sendStatus;
        helper::CheckMPIReturn(
            m_Comm.MPI()->Wait(&sendRequest, &sendStatus),
            "Isend wait handshake with neighbor, MPIChain aggregator, at Open");
    }
}

BufferSTL &MPIChain::GetSender(BufferSTL &bufferSTL)
{
    if (m_CurrentBufferOrder == 0)
    {
        return bufferSTL;
    }
    else
    {
        return m_Buffers.front();
    }
}

BufferSTL &MPIChain::GetReceiver(BufferSTL &bufferSTL)
{
    if (m_CurrentBufferOrder == 0)
    {
        return m_Buffers.front();
    }
    else
    {
        return bufferSTL;
    }
}

void MPIChain::ResizeUpdateBufferSTL(const size_t newSize, BufferSTL &bufferSTL,
                                     const std::string hint)
{
    bufferSTL.Resize(newSize, hint);
    bufferSTL.m_Position = bufferSTL.m_Buffer.size();
}

} // end namespace aggregator
} // end namespace adios2
