/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferList.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "BufferBlock.h"
#include <cstdlib>
#include <stdexcept>

namespace adios2
{
namespace format
{

BufferBlock::BufferBlock() {}

BufferBlock::~BufferBlock() { Free(); }

void BufferBlock::Reset(const bool zeroInitialize)
{
    if (!m_IsUserBlock)
    {
        if (zeroInitialize)
        {
            std::fill_n(m_Block, m_BlockSize, 0);
        }
        else
        {
            // just zero out the first and last 1kb
            const size_t bufsize = m_BlockSize;
            size_t s = (bufsize < 1024 ? bufsize : 1024);
            std::fill_n(m_Block, s, 0);
            if (bufsize > 1024)
            {
                size_t pos = bufsize - 1024;
                if (pos < 1024)
                {
                    pos = 1024;
                }
                s = bufsize - pos;
                std::fill_n(m_Block + pos, s, 0);
            }
        }
    }
    m_Position = 0;
}

void BufferBlock::Allocate(const size_t size, const std::string hint)
{
    m_Block = static_cast<char *>(std::malloc(size));
    if (m_Block == nullptr)
    {
        std::throw_with_nested(std::runtime_error(
            "ERROR: no memory for allocating " + std::to_string(size) +
            " bytes, " + hint + "\n"));
    }
    m_BlockSize = size;
    m_Position = 0;
    m_IsUserBlock = false;
}

void BufferBlock::Free()
{
    if (!m_IsUserBlock)
    {
        if (m_Block != nullptr)
        {
            std::free(m_Block);
        }
    }
    m_BlockSize = 0;
    m_Position = 0;
    m_Block = nullptr;
}

void BufferBlock::SetPos(size_t pos)
{
    if (pos > m_BlockSize)
    {
        throw(std::runtime_error("ERROR: Attempted to set position " +
                                 std::to_string(pos) + " in a block of size" +
                                 std::to_string(m_BlockSize) + "\n"));
    }
    m_Position = pos;
}

size_t BufferBlock::Align(const size_t alignment, const size_t elemsize) const
    noexcept
{
    // std::align implementation from llvm libc++
    // needed due to bug in gcc 4.8
    auto lf_align = [](const size_t alignment, const size_t size, void *&ptr,
                       size_t &space) {
        if (size <= space)
        {
            const char *p1 = static_cast<char *>(ptr);
            const char *p2 = reinterpret_cast<char *>(
                reinterpret_cast<size_t>(p1 + (alignment - 1)) & -alignment);
            const size_t d = static_cast<size_t>(p2 - p1);
            if (d <= space - size)
            {
                space -= d;
            }
        }
    };

    void *currentAddress = reinterpret_cast<void *>(m_Block + m_Position);
    size_t size = m_BlockSize;
    lf_align(alignment, elemsize, currentAddress, size);
    return m_BlockSize - size;
}

} // end namespace format
} // end namespace adios2
