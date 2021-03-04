/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferList.h
 *
 *  Created on: Mar 1, 2021
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFER_LIST_BUFFERLIST_H_
#define ADIOS2_TOOLKIT_FORMAT_BUFFER_LIST_BUFFERLIST_H_

#include "adios2/toolkit/buffering/BufferBlock.h"

#include "adios2/common/ADIOSMacros.h"

namespace adios2
{
namespace format
{

constexpr size_t c_1GB = 1073741824;
constexpr size_t c_4MB = 4194304;

class BufferList
{
public:
    const std::string m_ID; // just an ID for buffer
    std::vector<BufferBlock> m_BufferList;
    const size_t m_BlockSize;
    const size_t m_AlignmentSize;

    BufferList(const std::string ID, const size_t BlockSize = c_1GB,
               const size_t AlignmentSize = c_4MB);
    ~BufferList();

    /** Allocate enough blocks to cover the requested size */
    void Resize(const size_t size, const std::string hint);

    void Reset(const bool zeroInitialize = false);

    /** Total allocated size = Sum{blocks}(allocated size) */
    size_t GetAllocatedSize() const;
    /** Total consumed size = Sum{blocks}(positions) */
    size_t GetUsedSize() const;
    /** How much bytes do we have left of the allocated memory
     *  = in last block, the allocated size - current position
     * Not all the unused bytes in all blocks!
     */
    size_t GetAvailableSize() const;

    void Delete();

private:
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BUFFER_LIST_BUFFERLIST_H_ */
