/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferBlock.h
 *
 *  Created on: Mar 1, 2021
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *
 * Block of contiguous memory. Constructor only creates the object. Allocate()
 * must be called separately. Destructor will free memory automatically.
 *
 * User block is a block created from an existing allocated pointer, which
 * cannot be allocated or freed. Only m_Position can change (Reset to zero)
 *
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFERBLOCK_H_
#define ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFERBLOCK_H_

#include <cstddef> // size_t
#include <string>

namespace adios2
{
namespace format
{

class BufferBlock
{
public:
    /** Create the block object but do not allocate memory yet */
    BufferBlock();
    /** Create a user block that cannot be freed or allocated */
    BufferBlock(char *ptr, size_t size);
    ~BufferBlock(); // Free() is called automatically

    void Reset(const bool zeroInitialize);
    void Allocate(const size_t size, const std::string hint);
    void Free();

    char *Data() const { return m_Block; };
    size_t Size() const { return m_BlockSize; };
    bool IsUserBlock() const { return m_IsUserBlock; };
    size_t GetPos() const { return m_Position; };

    /** Set position. Throws runtime_error if not 0<=pos<=m_BlockSize */
    void SetPos(size_t pos);

    size_t Align(const size_t alignment, const size_t size) const noexcept;

private:
    size_t m_BlockSize = 0;
    size_t m_Position = 0; // 0..m_BlockSize
    char *m_Block = nullptr;
    bool m_IsUserBlock = false;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFERBLOCK_H_ */
