/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Writer.cpp
 *
 */

#include "BP5Writer.h"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange, PaddingToAlignOffset
#include "adios2/toolkit/format/buffer/chunk/ChunkV.h"
#include "adios2/toolkit/format/buffer/malloc/MallocV.h"
#include "adios2/toolkit/transport/file/FileFStream.h"
#include <adios2-perfstubs-interface.h>

#include <ctime>
#include <iomanip>
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

using namespace adios2::format;

void BP5Writer::WriteData_TwoLevelShm(format::BufferV *Data,
                                      bool CollectMetadata,
                                      std::vector<char> &LocalMetaData,
                                      std::vector<char> &AggregatedMetadata,
                                      std::vector<size_t> &MetadataLocalSizes)
{
    m_Profiler.Start("AWD");
    aggregator::MPIShmChain *a =
        dynamic_cast<aggregator::MPIShmChain *>(m_Aggregator);

    /* Two level aggregation of metadata
     * 1. Gather metadata inside the chain interleaved with data writing */
    std::vector<size_t> MetadataSizesOnAggregators(a->m_Comm.Size());
    std::vector<char> MetaDataOnAggregators;

    // new step writing starts at offset m_DataPos on master aggregator
    // other aggregators to the same file will need to wait for the position
    // to arrive from the rank below

    // align to PAGE_SIZE (only valid on master aggregator at this point)
    m_DataPos += helper::PaddingToAlignOffset(m_DataPos,
                                              m_Parameters.FileSystemPageSize);

    /*
    // Each aggregator needs to know the total size they write
    // including alignment to page size
    // This calculation is valid on aggregators only
    std::vector<uint64_t> mySizes = a->m_Comm.GatherValues(Data->Size());
    uint64_t myTotalSize = 0;
    uint64_t pos = m_DataPos;
    for (auto s : mySizes)
    {
        uint64_t alignment =
            helper::PaddingToAlignOffset(pos, m_Parameters.FileSystemPageSize);
        myTotalSize += alignment + s;
        pos += alignment + s;
    }
    */

    // Each aggregator needs to know the total size they write
    // This calculation is valid on aggregators only
    std::vector<uint64_t> mySizes = a->m_Comm.GatherValues(Data->Size());
    uint64_t myTotalSize = 0;
    uint64_t maxSize = 0;
    for (auto s : mySizes)
    {
        myTotalSize += s;
        if (s > maxSize)
        {
            maxSize = s;
        }
    }

    if (a->m_Comm.Size() > 1)
    {
        a->CreateShm(static_cast<size_t>(maxSize), m_Parameters.MaxShmSize);
    }

    if (a->m_IsAggregator)
    {
        // In each aggregator chain, send from master down the line
        // these total sizes, so every aggregator knows where to start
        if (a->m_AggregatorChainComm.Rank() > 0)
        {
            a->m_AggregatorChainComm.Recv(
                &m_DataPos, 1, a->m_AggregatorChainComm.Rank() - 1, 0,
                "AggregatorChain token in BP5Writer::WriteData_TwoLevelShm");
            // align to PAGE_SIZE
            m_DataPos += helper::PaddingToAlignOffset(
                m_DataPos, m_Parameters.FileSystemPageSize);
        }
        m_StartDataPos = m_DataPos; // metadata needs this info
        if (a->m_AggregatorChainComm.Rank() <
            a->m_AggregatorChainComm.Size() - 1)
        {
            uint64_t nextWriterPos = m_DataPos + myTotalSize;
            a->m_AggregatorChainComm.Isend(
                &nextWriterPos, 1, a->m_AggregatorChainComm.Rank() + 1, 0,
                "Chain token in BP5Writer::WriteData");
        }
        else if (a->m_AggregatorChainComm.Size() > 1)
        {
            // send back final position from last aggregator in file to master
            // aggregator
            uint64_t nextWriterPos = m_DataPos + myTotalSize;
            a->m_AggregatorChainComm.Isend(
                &nextWriterPos, 1, 0, 0, "Chain token in BP5Writer::WriteData");
        }

        /*std::cout << "Rank " << m_Comm.Rank()
                  << " aggregator start writing step " << m_WriterStep
                  << " to subfile " << a->m_SubStreamIndex << " at pos "
                  << m_DataPos << " totalsize " << myTotalSize << std::endl;*/

        // Send token to first non-aggregator to start filling shm
        // Also informs next process its starting offset (for correct metadata)
        if (a->m_Comm.Size() > 1)
        {
            uint64_t nextWriterPos = m_DataPos + Data->Size();
            a->m_Comm.Isend(&nextWriterPos, 1, a->m_Comm.Rank() + 1, 0,
                            "Shm token in BP5Writer::WriteData_TwoLevelShm");
        }

        WriteMyOwnData(Data);

        /* Write from shm until every non-aggr sent all data */
        if (a->m_Comm.Size() > 1)
        {
            WriteOthersData(myTotalSize - Data->Size());
        }

        /* Aggregator collects individual metadata blocks */
        // guess at metadata size to resize as few times as possible
        size_t mdoffset = LocalMetaData.size();
        MetaDataOnAggregators.resize(mdoffset * m_Comm.Size());
        // aggregators own metadata
        std::memcpy(MetaDataOnAggregators.data(), LocalMetaData.data(),
                    mdoffset);
        for (int r = 1; r < m_Comm.Size(); ++r)
        {
            a->m_Comm.Recv(&MetadataSizesOnAggregators[r], 1, r, 0,
                           "local MD size in BP5Writer::WriteData_TwoLevelShm");
            if (mdoffset + MetadataSizesOnAggregators[r] >
                MetaDataOnAggregators.size())
            {
                MetaDataOnAggregators.resize(mdoffset +
                                             MetadataSizesOnAggregators[r]);
            }
            a->m_Comm.Recv(MetaDataOnAggregators.data() + mdoffset,
                           MetadataSizesOnAggregators[r], r, 0,
                           "local MD size in BP5Writer::WriteData_TwoLevelShm");
            mdoffset += MetadataSizesOnAggregators[r];
        }

        // Master aggregator needs to know where the last writing ended by the
        // last aggregator in the chain, so that it can start from the correct
        // position at the next output step
        if (a->m_AggregatorChainComm.Size() > 1 &&
            !a->m_AggregatorChainComm.Rank())
        {
            a->m_AggregatorChainComm.Recv(
                &m_DataPos, 1, a->m_AggregatorChainComm.Size() - 1, 0,
                "Chain token in BP5Writer::WriteData");
        }
    }
    else
    {
        // first send my metadata towards the aggregator
        size_t LocalMetaDataSize = LocalMetaData.size();
        a->m_Comm.Isend(&LocalMetaDataSize, 1, 0, 0,
                        "local MD size in BP5Writer::WriteData_TwoLevelShm");
        a->m_Comm.Isend(LocalMetaData.data(), LocalMetaDataSize, 0, 0,
                        "local MD in BP5Writer::WriteData_TwoLevelShm");

        // non-aggregators fill shared buffer in marching order
        // they also receive their starting offset this way
        a->m_Comm.Recv(&m_StartDataPos, 1, a->m_Comm.Rank() - 1, 0,
                       "Shm token in BP5Writer::WriteData_TwoLevelShm");

        /*std::cout << "Rank " << m_Comm.Rank()
                  << " non-aggregator recv token to fill shm = "
                  << m_StartDataPos << std::endl;*/

        SendDataToAggregator(Data);

        if (a->m_Comm.Rank() < a->m_Comm.Size() - 1)
        {
            uint64_t nextWriterPos = m_StartDataPos + Data->Size();
            a->m_Comm.Isend(&nextWriterPos, 1, a->m_Comm.Rank() + 1, 0,
                            "Shm token in BP5Writer::WriteData_TwoLevelShm");
        }
    }
    m_Profiler.Stop("AWD");

    m_Profiler.Start("meta_gather");
    if (CollectMetadata)
    {
        /* Two level aggregation of metadata, so far not interleaved with data
         * writing */
        /* 1. Gather metadata inside the chain */
        size_t LocalSize = LocalMetaData.size();
        std::vector<size_t> MetadataSizesOnAggregators =
            a->m_Comm.GatherValues(LocalSize, 0);

        std::vector<char> MetaDataOnAggregators;
        if (a->m_IsAggregator)
        {
            uint64_t TotalSize = 0;
            for (auto &n : MetadataSizesOnAggregators)
                TotalSize += n;
            MetaDataOnAggregators.resize(TotalSize);
        }
        m_Comm.GathervArrays(
            LocalMetaData.data(), LocalSize, MetadataSizesOnAggregators.data(),
            MetadataSizesOnAggregators.size(), MetaDataOnAggregators.data(), 0);

        /* 2. Gather metadata from the Aggregators */
        if (a->m_IsAggregator)
        {
            GatherMetadataFromAggregators(
                a->m_AllAggregatorsComm, MetaDataOnAggregators,
                MetadataSizesOnAggregators, AggregatedMetadata,
                MetadataLocalSizes);
        }
    }
    m_Profiler.Stop("meta_gather");

    m_Profiler.Start("AWD");
    if (a->m_Comm.Size() > 1)
    {
        a->DestroyShm();
    }
    m_Profiler.Stop("AWD");
}

void BP5Writer::WriteMyOwnData(format::BufferV *Data)
{
    std::vector<core::iovec> DataVec = Data->DataVec();
    m_StartDataPos = m_DataPos;
    m_FileDataManager.WriteFileAt(DataVec.data(), DataVec.size(),
                                  m_StartDataPos);
    m_DataPos += Data->Size();
}

/*std::string DoubleBufferToString(const double *b, int n)
{
    std::ostringstream out;
    out.precision(1);
    out << std::fixed << "[";
    char s[32];

    for (int i = 0; i < n; ++i)
    {
        snprintf(s, sizeof(s), "%g", b[i]);
        out << s;
        if (i < n - 1)
        {
            out << ", ";
        }
    }
    out << "]";
    return out.str();
}*/

void BP5Writer::SendDataToAggregator(format::BufferV *Data)
{
    /* Only one process is running this function at once
       See shmFillerToken in the caller function

       In a loop, copy the local data into the shared memory, alternating
       between the two segments.
    */

    aggregator::MPIShmChain *a =
        dynamic_cast<aggregator::MPIShmChain *>(m_Aggregator);

    std::vector<core::iovec> DataVec = Data->DataVec();
    size_t nBlocks = DataVec.size();

    size_t sent = 0;
    size_t block = 0;
    size_t temp_offset = 0;
    while (block < nBlocks)
    {
        // potentially blocking call waiting on Aggregator
        aggregator::MPIShmChain::ShmDataBuffer *b = a->LockProducerBuffer();
        // b->max_size: how much we can copy
        // b->actual_size: how much we actually copy
        b->actual_size = 0;
        while (true)
        {
            /* Copy n bytes from the current block, current offset to shm
                making sure to use up to shm_size bytes
            */
            size_t n = DataVec[block].iov_len - temp_offset;
            if (n > (b->max_size - b->actual_size))
            {
                n = b->max_size - b->actual_size;
            }
            std::memcpy(&b->buf[b->actual_size],
                        (const char *)DataVec[block].iov_base + temp_offset, n);
            b->actual_size += n;

            /* Have we processed the entire block or staying with it? */
            if (n + temp_offset < DataVec[block].iov_len)
            {
                temp_offset += n;
            }
            else
            {
                temp_offset = 0;
                ++block;
            }

            /* Have we reached the max allowed shm size ?*/
            if (b->actual_size >= b->max_size)
            {
                break;
            }
            if (block >= nBlocks)
            {
                break;
            }
        }
        sent += b->actual_size;

        /*if (m_RankMPI >= 42)
        {
            std::cout << "Rank " << m_Comm.Rank()
                      << " filled shm, data_size = " << b->actual_size
                      << " block = " << block
                      << " temp offset = " << temp_offset << " sent = " << sent
                      << " buf = " << static_cast<void *>(b->buf) << " = "
                      << DoubleBufferToString((double *)b->buf,
                                              b->actual_size / sizeof(double))
                      << std::endl;
        }*/

        a->UnlockProducerBuffer();
    }
}
void BP5Writer::WriteOthersData(size_t TotalSize)
{
    /* Only an Aggregator calls this function */
    aggregator::MPIShmChain *a =
        dynamic_cast<aggregator::MPIShmChain *>(m_Aggregator);

    size_t wrote = 0;
    while (wrote < TotalSize)
    {
        // potentially blocking call waiting on some non-aggr process
        aggregator::MPIShmChain::ShmDataBuffer *b = a->LockConsumerBuffer();

        /*std::cout << "Rank " << m_Comm.Rank()
                  << " write from shm, data_size = " << b->actual_size
                  << " total so far = " << wrote
                  << " buf = " << static_cast<void *>(b->buf) << " = "
                  << DoubleBufferToString((double *)b->buf,
                                          b->actual_size / sizeof(double))
                  << std::endl;*/
        /*<< " buf = " << static_cast<void *>(b->buf) << " = ["
        << (int)b->buf[0] << (int)b->buf[1] << "..."
        << (int)b->buf[b->actual_size - 2]
        << (int)b->buf[b->actual_size - 1] << "]" << std::endl;*/

        // b->actual_size: how much we need to write
        m_FileDataManager.WriteFiles(b->buf, b->actual_size);

        wrote += b->actual_size;

        a->UnlockConsumerBuffer();
    }
    m_DataPos += TotalSize;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
