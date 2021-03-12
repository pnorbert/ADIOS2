/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosBlockMerge.h : Algorithm to find bigger container blocks so that
 * data blocks can be merged together.
 *
 * Original algorithm proposed by
 * M. Berger and I. Rigoutsos,
 * "An algorithm for point clustering and grid generation,"
 * in IEEE Transactions on Systems, Man, and Cybernetics, vol. 21, no. 5,
 * pp. 1278-1286, Sept.-Oct. 1991, doi: 10.1109/21.120081.
 *
 * Extended and Implemented for our purposes by Lipeng Wan lwan@ornl.gov
 *
 * Constraint for blocks: the blocks should fit a rectilinear grid in space
 * so that a straight cut in a dimension can be found so that blocks fall
 * completely to the left or the right side of that cut.
 *
 * This is an allowed layout of 2D blocks:
 * +---+----+--+--------+     +--+-----+---+
 * |###|    |  |########|     |  |#####|###|
 * |###|    |  |########|     |  |#####|###|
 * +---+----+--+--------+     +--+-----+---+
 * |###|####|  |########|     |##|     |###|
 * +---+----+--+--------+     +--+-----+---+
 * |   |####|##|        |     |  |     |###|
 * |   |####|##|        |     |  |     |###|
 * |   |####|##|        |     |  |     |###|
 * +---+----+--+--------+     +--+-----+---+
 * 
 * 
 * Created on: Mar 8, 2021
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_HELPER_BLOCKMERGE_H_
#define ADIOS2_HELPER_BLOCKMERGE_H_

#include "adios2/common/ADIOSTypes.h"

#include <list>
#include <unordered_map>
#include <vector>

namespace adios2
{
namespace helper
{

/* Variable block's ID (order in list of deferred blocks at output)
and its individual bounding box */
struct BlockRef
{
    const size_t ID;
    const Dims &start;
    const Dims &count;
    BlockRef(const size_t id, const Dims &start, const Dims &count)
    : ID(id), count(count), start(start){};
};

/* Enumeration of all Start (or Count) values of all blocks in each dimension
   It looks like a rectilinear mesh description */
using BoundaryList = std::vector<std::vector<size_t>>;

/** Struct to hold together all info about
 *  - the container bounding box, uncomputed yet
 *  - all the blocks within the container
 */
struct ContainerInfo
{
    BoundaryList boundaryList;
    std::vector<BlockRef> blocks;
};

using ContainerInfoList = std::list<ContainerInfo>;

/**
 * @brief Run the block merging algorithm.
 * Run the algorithm on a bunch of blocks and return a list of container
 * boxes and for each box the list of blocks that compose it.
 */
ContainerInfoList FindContainerBoxes(const size_t ndim,
                                     ContainerInfoList &boxList);

} // end namespace helper
} // end namespace adios2

#endif // ADIOS2_HELPER_BLOCKMERGE_H_
