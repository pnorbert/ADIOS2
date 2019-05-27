/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMPIFunctions.tcc : specialization of template functions defined in
 * adiosMPIFunctions.h
 *
 *  Created on: Aug 30, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSMPIFUNCTIONS_TCC_
#define ADIOS2_HELPER_ADIOSMPIFUNCTIONS_TCC_

#include "adiosMPIFunctions.h"

#include <algorithm> //std::foreach
#include <numeric>   //std::accumulate

#include "adios2/ADIOSTypes.h"
#include "adios2/helper/adiosType.h"
#include "adios2/toolkit/comm/AMPIComm.h"

namespace adios2
{
namespace helper
{

// BroadcastValue specializations
template <>
size_t BroadcastValue(const size_t &input, const AMPI_Comm &acomm,
                      const int rankSource)
{
    int rank;
    acomm.Rank(&rank);
    size_t output = 0;

    if (rank == rankSource)
    {
        output = input;
    }

    acomm.MPI()->Bcast(&output, 1, AMPI_SIZE_T, rankSource, acomm);

    return output;
}

template <>
std::string BroadcastValue(const std::string &input, const AMPI_Comm &acomm,
                           const int rankSource)
{
    int rank;
    acomm.Rank(&rank);
    const size_t inputSize = input.size();
    const size_t length = BroadcastValue(inputSize, acomm, rankSource);
    std::string output;

    if (rank == rankSource)
    {
        output = input;
    }
    else
    {
        output.resize(length);
    }

    acomm.MPI()->Bcast(const_cast<char *>(output.data()),
                       static_cast<int>(length), AMPI_CHAR, rankSource, acomm);

    return output;
}

// ReduceValue specializations
template <>
unsigned int ReduceValues(const unsigned int source, const AMPI_Comm &acomm,
                          AMPI_Op operation, const int rankDestination)
{
    unsigned int sourceLocal = source;
    unsigned int reduceValue = 0;
    acomm.MPI()->Reduce(&sourceLocal, &reduceValue, 1, AMPI_UNSIGNED, operation,
                        rankDestination, acomm);
    return reduceValue;
}

template <>
unsigned long int ReduceValues(const unsigned long int source, const AMPI_Comm &acomm,
                               AMPI_Op operation, const int rankDestination)
{
    unsigned long int sourceLocal = source;
    unsigned long int reduceValue = 0;
    acomm.MPI()->Reduce(&sourceLocal, &reduceValue, 1, AMPI_UNSIGNED_LONG,
                        operation, rankDestination, acomm);
    return reduceValue;
}

template <>
unsigned long long int ReduceValues(const unsigned long long int source,
                                    const AMPI_Comm &acomm, AMPI_Op operation,
                                    const int rankDestination)
{
    unsigned long long int sourceLocal = source;
    unsigned long long int reduceValue = 0;
    acomm.MPI()->Reduce(&sourceLocal, &reduceValue, 1, AMPI_UNSIGNED_LONG_LONG,
                        operation, rankDestination, acomm);
    return reduceValue;
}

// BroadcastVector specializations
template <>
void BroadcastVector(std::vector<char> &vector, const AMPI_Comm &acomm,
                     const int rankSource)
{
    int size;
    acomm.Size(&size);

    if (size == 1)
    {
        return;
    }

    // First Broadcast the size, then the contents
    size_t inputSize = BroadcastValue(vector.size(), acomm, rankSource);
    int rank;
    acomm.Rank(&rank);

    if (rank != rankSource)
    {
        vector.resize(inputSize);
    }

    const int MAXBCASTSIZE = 1073741824;
    size_t blockSize = (inputSize > MAXBCASTSIZE ? MAXBCASTSIZE : inputSize);
    char *buffer = vector.data();
    while (inputSize > 0)
    {
        acomm.MPI()->Bcast(buffer, static_cast<int>(blockSize), AMPI_CHAR,
                           rankSource, acomm);
        buffer += blockSize;
        inputSize -= blockSize;
        blockSize = (inputSize > MAXBCASTSIZE ? MAXBCASTSIZE : inputSize);
    }
}

// GatherArrays specializations
template <>
void GatherArrays(const char *source, const size_t sourceCount,
                  char *destination, const AMPI_Comm &acomm,
                  const int rankDestination)
{
    int countsInt = static_cast<int>(sourceCount);
    int result = acomm.MPI()->Gather(const_cast<char *>(source), countsInt,
                                     AMPI_CHAR, destination, countsInt,
                                     AMPI_CHAR, rankDestination, acomm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Gather type MPI_CHAR function\n");
    }
}

template <>
void GatherArrays(const size_t *source, const size_t sourceCount,
                  size_t *destination, const AMPI_Comm &acomm,
                  const int rankDestination)
{
    int countsInt = static_cast<int>(sourceCount);
    int result = acomm.MPI()->Gather(const_cast<size_t *>(source), countsInt,
                                     AMPI_SIZE_T, destination, countsInt,
                                     AMPI_SIZE_T, rankDestination, acomm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Gather type size_t function\n");
    }
}

// AllGatherArray specializations
template <>
void AllGatherArrays(const size_t *source, const size_t sourceCount,
                     size_t *destination, const AMPI_Comm &acomm)
{
    int countsInt = static_cast<int>(sourceCount);
    int result = acomm.MPI()->Allgather(const_cast<size_t *>(source), countsInt,
                                        AMPI_SIZE_T, destination, countsInt,
                                        AMPI_SIZE_T, acomm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Allgather type size_t function\n");
    }
}

// GathervArrays specializations
template <>
void GathervArrays(const char *source, const size_t sourceCount,
                   const size_t *counts, const size_t countsSize,
                   char *destination, const AMPI_Comm &acomm,
                   const int rankDestination)
{
    int result = 0;
    int rank;
    acomm.Rank(&rank);

    std::vector<int> countsInt, displacementsInt;

    if (rank == rankDestination)
    {
        countsInt = NewVectorTypeFromArray<size_t, int>(counts, countsSize);
        displacementsInt = GetGathervDisplacements(counts, countsSize);
    }

    int sourceCountInt = static_cast<int>(sourceCount);
    result = acomm.MPI()->Gatherv(const_cast<char *>(source), sourceCountInt,
                                  AMPI_CHAR, destination, countsInt.data(),
                                  displacementsInt.data(), AMPI_CHAR,
                                  rankDestination, acomm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Gatherv type MPI_CHAR function\n");
    }
}

template <>
void GathervArrays(const size_t *source, const size_t sourceCount,
                   const size_t *counts, const size_t countsSize,
                   size_t *destination, const AMPI_Comm &acomm,
                   const int rankDestination)
{
    int result = 0;
    int rank;
    acomm.Rank(&rank);

    std::vector<int> countsInt =
        NewVectorTypeFromArray<size_t, int>(counts, countsSize);

    std::vector<int> displacementsInt =
        GetGathervDisplacements(counts, countsSize);

    int sourceCountInt = static_cast<int>(sourceCount);

    result = acomm.MPI()->Gatherv(const_cast<size_t *>(source), sourceCountInt,
                                  AMPI_SIZE_T, destination, countsInt.data(),
                                  displacementsInt.data(), AMPI_SIZE_T,
                                  rankDestination, acomm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Gather type size_t function\n");
    }
}

template <>
std::vector<AMPI_Request> Isend64<char>(const char *buffer, const size_t count,
                                        int dest, int tag, const AMPI_Comm &acomm,
                                        const std::string &hint)
{
    const size_t batches = count / DefaultMaxFileBatchSize;
    std::vector<AMPI_Request> requests(batches + 1);

    if (batches > 1)
    {
        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            int batchSize = static_cast<int>(DefaultMaxFileBatchSize);
            CheckMPIReturn(
                acomm.MPI()->Isend(const_cast<char *>(buffer + position),
                                   batchSize, AMPI_CHAR, dest, tag, acomm,
                                   &requests[b]),
                "in call to Isend64 batch " + std::to_string(b) + "/" +
                    std::to_string(batches) + " " + hint + "\n");

            position += DefaultMaxFileBatchSize;
        }
        const size_t remainder = count % DefaultMaxFileBatchSize;
        if (remainder > 0)
        {
            int batchSize = static_cast<int>(remainder);
            CheckMPIReturn(
                acomm.MPI()->Isend(const_cast<char *>(buffer + position),
                                   batchSize, AMPI_CHAR, dest, tag, acomm,
                                   &requests[batches - 1]),
                "in call to Isend64 last batch " + hint + "\n");
        }
    }
    else
    {
        int batchSize = static_cast<int>(count);
        CheckMPIReturn(acomm.MPI()->Isend(const_cast<char *>(buffer), batchSize,
                                          AMPI_CHAR, dest, tag, acomm,
                                          &requests[0]),
                       " in call to Isend64 with single batch " + hint + "\n");
    }

    return requests;
}

template <>
std::vector<AMPI_Request> Irecv64<char>(char *buffer, const size_t count,
                                        int source, int tag, const AMPI_Comm &acomm,
                                        const std::string &hint)
{
    const size_t batches = count / DefaultMaxFileBatchSize;
    std::vector<AMPI_Request> requests(batches + 1);

    if (requests.size() != batches + 1)
    {
        throw std::runtime_error(
            "ERROR: number of Irecv requests doesn't match number of batches = "
            "count/DefaultMaxFileBatchSize\n");
    }

    if (batches > 1)
    {
        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            int batchSize = static_cast<int>(DefaultMaxFileBatchSize);
            CheckMPIReturn(
                acomm.MPI()->Irecv(buffer + position, batchSize, AMPI_CHAR,
                                   source, tag, acomm, &requests[b]),
                "in call to Irecv64 batch " + std::to_string(b) + "/" +
                    std::to_string(batches) + " " + hint + "\n");

            position += DefaultMaxFileBatchSize;
        }
        const size_t remainder = count % DefaultMaxFileBatchSize;
        if (remainder > 0)
        {
            int batchSize = static_cast<int>(remainder);
            CheckMPIReturn(acomm.MPI()->Irecv(buffer + position, batchSize,
                                              AMPI_CHAR, source, tag, acomm,
                                              &requests[batches - 1]),
                           "in call to Irecv64 last batch " + hint + "\n");
        }
    }
    else
    {
        int batchSize = static_cast<int>(count);
        CheckMPIReturn(acomm.MPI()->Irecv(buffer, batchSize, AMPI_CHAR, source,
                                          tag, acomm, &requests[0]),
                       " in call to Isend64 with single batch " + hint + "\n");
    }
    return requests;
}

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSMPIFUNCTIONS_TCC_ */
