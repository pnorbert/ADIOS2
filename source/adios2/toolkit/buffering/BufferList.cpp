/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferList.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "BufferList.h"
#include "BufferList.tcc"
#include <cstdlib>
#include <cstring>

namespace adios2
{
namespace format
{

BufferList::BufferList(const std::string ID, const size_t BlockSize,
                       const size_t AlignmentSize)
: m_ID(ID), m_BlockSize(BlockSize), m_AlignmentSize(AlignmentSize)
{
}

BufferList::~BufferList() { m_BufferList.resize(0); }

void BufferList::Resize(const size_t size, const std::string hint)
{
    auto lf_nblocks = [](const size_t size, const size_t blocksize) -> size_t {
        size_t n = size / blocksize;
        if (size % blocksize)
        {
            ++n;
        }
        return n;
    };

    const size_t nBlocks = lf_nblocks(size, m_BlockSize);
    const size_t currentNBlocks = m_BufferList.size();
    if (currentNBlocks < nBlocks)
    {
        try
        {
            m_BufferList.resize(nBlocks);
            for (size_t i = currentNBlocks; i < nBlocks; ++i)
            {
                m_BufferList[i].Allocate(m_BlockSize, hint);
            }
        }
        catch (...)
        {
            // catch a bad_alloc
            std::throw_with_nested(std::runtime_error(
                "ERROR: buffer overflow when resizing to " +
                std::to_string(size) + " bytes, " + hint + "\n"));
        }
    }
    else if (currentNBlocks > nBlocks)
    {
        m_BufferList.resize(nBlocks);
    }
}

void BufferList::Reset(const bool zeroInitialize)
{
    for (auto b : m_BufferList)
    {
        b.Reset(zeroInitialize);
    }
}

size_t BufferList::GetAllocatedSize() const
{
    size_t size = 0;
    for (auto &b : m_BufferList)
    {
        size += b.Size();
    }
    return size;
}

size_t BufferList::GetUsedSize() const
{
    size_t size = 0;
    for (auto b : m_BufferList)
    {
        size += b.GetPos();
    }
    return size;
}

size_t BufferList::GetAvailableSize() const
{
    const auto b = m_BufferList.rbegin();
    return b->Size() - b->GetPos();
}

void BufferList::Delete() { std::vector<BufferBlock>().swap(m_BufferList); }

} // end namespace format
} // end namespace adios2
