/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Created on: Mar 8, 2021
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "adiosBlockMerge.h"

#include <cstring>
#include <iostream>
#include <math.h>

namespace adios2
{
namespace helper
{

double *laplacian_1d(int n, double h, double u[])
{
    int i;
    double *l;

    if (n < 4)
    {
        std::cerr << "laplacian_1d: the size of input 1d array needs to be "
                     "greater than 4\n";
        exit(1);
    }

    l = new double[n];
    for (i = 1; i < n - 1; i++)
    {
        l[i] = (u[i - 1] + u[i + 1] - 2.0 * u[i]) / h / h;
    }
    l[0] = 2 * l[1] - l[2];
    l[n - 1] = 2 * l[n - 2] - l[n - 3];
    return l;
}

std::pair<int, double> find_zero_crossing(int n, double l[])
{
    if (n < 4)
    {
        std::cerr << "find_zero_crossing: the size of input 1d array needs to "
                     "be greater than 4\n";
        exit(1);
    }
    double maxStdDeviation = 0.0;
    int bestZeroCrossingPoint = -1;
    for (size_t i = 1; i < n - 1; i++)
    {
        if ((l[i - 1] > 0 && l[i] <= 0) || (l[i - 1] < 0 && l[i] >= 0))
        {
            // std::cout << l[i-1] << ", " << l[i] << std::endl;
            double sum = 0.0, variance = 0.0;
            ;
            double mean, stdDeviation;

            sum = l[i - 1] + l[i] + l[i + 1];
            mean = sum / 3;
            variance = pow(l[i - 1] - mean, 2) + pow(l[i] - mean, 2) +
                       pow(l[i + 1] - mean, 2);
            variance = variance / 3;

            stdDeviation = sqrt(variance);
            // std::cout << l[i-1] << ", " << l[i] << ", " << l[i+1] << ": " <<
            // stdDeviation << std::endl;
            if (stdDeviation > maxStdDeviation)
            {
                maxStdDeviation = stdDeviation;
                bestZeroCrossingPoint = i;
            }
        }
    }
    // std::cout << bestZeroCrossingPoint << ", " << maxStdDeviation <<
    // std::endl;
    return std::make_pair(bestZeroCrossingPoint, maxStdDeviation);
}

void test()
{
    std::list<
        std::tuple<std::vector<std::vector<size_t>>,
                   std::unordered_map<size_t, std::tuple<std::vector<size_t>,
                                                         std::vector<size_t>>>>>
        boundingBoxList;
    std::list<
        std::tuple<std::vector<std::vector<size_t>>,
                   std::unordered_map<size_t, std::tuple<std::vector<size_t>,
                                                         std::vector<size_t>>>>>
        finalBlocksList;
    size_t spaceDimensions = 3;
    while (!boundingBoxList.empty())
    {
        std::tuple<std::vector<std::vector<size_t>>,
                   std::unordered_map<size_t, std::tuple<std::vector<size_t>,
                                                         std::vector<size_t>>>>
            bBox = boundingBoxList.front();
        boundingBoxList.pop_front();
        std::vector<std::vector<size_t>> bound = std::get<0>(bBox);
        std::unordered_map<size_t,
                           std::tuple<std::vector<size_t>, std::vector<size_t>>>
            blocksInBoundingBox = std::get<1>(bBox);
        size_t boundingBoxSize = 1;
        for (size_t i = 0; i < bound.size(); i++)
        {
            boundingBoxSize *= bound[i].size();
        }

        if (boundingBoxSize == blocksInBoundingBox.size())
        {
            finalBlocksList.push_back(bBox);
            continue;
        }

        if (blocksInBoundingBox.size() == 1)
        {
            std::vector<std::vector<size_t>> oneBlockBound(spaceDimensions);
            for (size_t i = 0; i < oneBlockBound.size(); i++)
            {
                auto x = blocksInBoundingBox.begin()->second;
                oneBlockBound[i].push_back(
                    std::get<0>(blocksInBoundingBox.begin()->second)[i]);
            }
            std::unordered_map<
                size_t, std::tuple<std::vector<size_t>, std::vector<size_t>>>
                oneBlock;
            oneBlock[blocksInBoundingBox.begin()->first] =
                blocksInBoundingBox.begin()->second;
            finalBlocksList.push_back(std::make_tuple(oneBlockBound, oneBlock));
            continue;
        }

        std::vector<std::unordered_map<size_t, double>> planeHist(
            spaceDimensions);
        for (size_t i = 0; i < spaceDimensions; i++)
        {
            double planeSize = boundingBoxSize / bound[i].size();
            for (auto j : bound[i])
            {
                if (planeHist[i].find(j) == planeHist[i].end())
                {
                    planeHist[i].emplace(j, 0);
                }
                for (auto blk : blocksInBoundingBox)
                {
                    if (std::get<0>(blk.second)[i] == j)
                    {
                        planeHist[i][j] += 1;
                    }
                }
                planeHist[i][j] = planeHist[i][j] / planeSize;
            }
        }

        int bestDimToCut = -1, bestPlaneToCut = -1;
        double maxStdDev = 0.0;
        for (size_t i = 0; i < spaceDimensions; i++)
        {
            double *u = new double[bound[i].size()];
            double *l;
            size_t idx = 0;
            for (auto j : bound[i])
            {
                u[idx] = planeHist[i][j];
                idx++;
            }
            if (bound[i].size() > 3)
            {
                l = laplacian_1d(bound[i].size(), 1, u);
                std::pair<int, double> bestZeroCrossingPoint =
                    find_zero_crossing(bound[i].size(), l);
                if (bestZeroCrossingPoint.first >= 0 &&
                    bestZeroCrossingPoint.second > maxStdDev)
                {
                    maxStdDev = bestZeroCrossingPoint.second;
                    bestDimToCut = i;
                    bestPlaneToCut = bestZeroCrossingPoint.first;
                }
            }
            else
            {
                for (size_t j = 0; j < bound[i].size(); j++)
                {
                    if (planeHist[i][bound[i][j]] == 0)
                    {
                        bestDimToCut = i;
                        bestPlaneToCut = j;
                        break;
                    }
                }
                if (bestDimToCut >= 0 && bestPlaneToCut >= 0)
                {
                    break;
                }
            }
        }

        if (bestDimToCut < 0 || bestPlaneToCut < 0)
        {
            double minPlaneHist = 1.0;
            for (size_t i = 0; i < bound.size(); i++)
            {
                if (bound[i].size() == 1)
                {
                    continue;
                }

                for (size_t j = 0; j < bound[i].size(); j++)
                {
                    if (planeHist[i][bound[i][j]] < minPlaneHist)
                    {
                        bestDimToCut = i;
                        bestPlaneToCut = j;
                        minPlaneHist = planeHist[i][bound[i][j]];
                    }
                }
            }
        }

        std::vector<std::vector<size_t>> boundLeft(spaceDimensions),
            boundRight(spaceDimensions);
        std::unordered_map<size_t,
                           std::tuple<std::vector<size_t>, std::vector<size_t>>>
            blocksInBoundLeft, blocksInBoundRight;
        if (bestDimToCut >= 0 && bestPlaneToCut >= 0)
        {
            for (size_t i = 0; i < spaceDimensions; i++)
            {
                if (i == bestDimToCut)
                {
                    if (bestPlaneToCut > 0 &&
                        bestPlaneToCut < bound[i].size() - 1)
                    {
                        if (std::abs(
                                planeHist[bestDimToCut]
                                         [bound[bestDimToCut][bestPlaneToCut]] -
                                planeHist[bestDimToCut]
                                         [bound[bestDimToCut]
                                               [bestPlaneToCut - 1]]) <
                            std::abs(
                                planeHist[bestDimToCut]
                                         [bound[bestDimToCut]
                                               [bestPlaneToCut + 1]] -
                                planeHist[bestDimToCut]
                                         [bound[bestDimToCut][bestPlaneToCut]]))
                        {
                            std::vector<size_t> left(bound[i].begin(),
                                                     bound[i].begin() +
                                                         bestPlaneToCut + 1);
                            std::vector<size_t> right(bound[i].begin() +
                                                          bestPlaneToCut + 1,
                                                      bound[i].end());
                            boundLeft[i] = left;
                            boundRight[i] = right;
                        }
                        else
                        {
                            std::vector<size_t> left(bound[i].begin(),
                                                     bound[i].begin() +
                                                         bestPlaneToCut);
                            std::vector<size_t> right(bound[i].begin() +
                                                          bestPlaneToCut,
                                                      bound[i].end());
                            boundLeft[i] = left;
                            boundRight[i] = right;
                        }
                    }
                    else if (bestPlaneToCut == 0)
                    {
                        std::vector<size_t> left(bound[i].begin(),
                                                 bound[i].begin() +
                                                     bestPlaneToCut + 1);
                        std::vector<size_t> right(bound[i].begin() +
                                                      bestPlaneToCut + 1,
                                                  bound[i].end());
                        boundLeft[i] = left;
                        boundRight[i] = right;
                    }
                    else if (bestPlaneToCut == bound[i].size() - 1)
                    {
                        std::vector<size_t> left(bound[i].begin(),
                                                 bound[i].begin() +
                                                     bestPlaneToCut);
                        std::vector<size_t> right(
                            bound[i].begin() + bestPlaneToCut, bound[i].end());
                        boundLeft[i] = left;
                        boundRight[i] = right;
                    }

                    for (auto blk : blocksInBoundingBox)
                    {
                        if (std::get<0>(blk.second)[i] <= boundLeft[i].back())
                        {
                            blocksInBoundLeft.insert(blk);
                        }
                        else
                        {
                            blocksInBoundRight.insert(blk);
                        }
                    }
                }
                else
                {
                    boundLeft[i] = bound[i];
                    boundRight[i] = bound[i];
                }
            }
        }
        else
        {
            std::cerr << "it shouldn't come here!\n";
            exit(1);
        }
        if (blocksInBoundLeft.size() > 0)
        {
            boundingBoxList.push_back(
                std::make_tuple(boundLeft, blocksInBoundLeft));
        }

        if (blocksInBoundRight.size() > 0)
        {
            boundingBoxList.push_back(
                std::make_tuple(boundRight, blocksInBoundRight));
        }
    }
}

ContainerInfoList FindContainerBoxes(const size_t ndim,
                                     ContainerInfoList &containerInfoList)
{
    ContainerInfoList finalBlocksList;
    while (!containerInfoList.empty())
    {
        ContainerInfo bBox = containerInfoList.front();
        containerInfoList.pop_front();
        BoundaryList &bound = bBox.boundaryList;
        std::vector<BlockRef> &blocksInBoundingBox = bBox.blocks;
        size_t boundingBoxSize = 1;
        if (bound.size() != ndim)
        {
            throw std::runtime_error(
                "ERROR: adiosBlockMerge:FindContainerBoxes() was called with "
                "ndim = " +
                std::to_string(ndim) + " but the boundary has " +
                std::to_string(bound.size()) + " dimensions");
        }

        /* case 1: If current box is completely filled with blocks, add it to
         * results */
        for (size_t i = 0; i < ndim; i++)
        {
            boundingBoxSize *= bound[i].size();
        }

        if (boundingBoxSize == blocksInBoundingBox.size())
        {
            finalBlocksList.push_back(bBox);
            continue;
        }

        /* case 2: If there is only one block left in box, add box to results */
        if (blocksInBoundingBox.size() == 1)
        {
            std::vector<std::vector<size_t>> oneBlockBound(ndim);
            for (size_t i = 0; i < oneBlockBound.size(); i++)
            {

                oneBlockBound[i].push_back(
                    std::get<0>(blocksInBoundingBox.begin()->second)[i]);
            }
            std::unordered_map<
                size_t, std::tuple<std::vector<size_t>, std::vector<size_t>>>
                oneBlock;
            oneBlock[blocksInBoundingBox.begin()->first] =
                blocksInBoundingBox.begin()->second;
            finalBlocksList.push_back(std::make_tuple(oneBlockBound, oneBlock));
            continue;
        }

        /* case 3: we need to cut the box into two */
        std::vector<std::unordered_map<size_t, double>> planeHist(ndim);
        for (size_t i = 0; i < ndim; i++)
        {
            double planeSize = boundingBoxSize / bound[i].size();
            for (auto j : bound[i])
            {
                if (planeHist[i].find(j) == planeHist[i].end())
                {
                    planeHist[i].emplace(j, 0);
                }
                for (auto blk : blocksInBoundingBox)
                {
                    if (std::get<0>(blk.second)[i] == j)
                    {
                        planeHist[i][j] += 1;
                    }
                }
                planeHist[i][j] = planeHist[i][j] / planeSize;
            }
        }

        int bestDimToCut = -1, bestPlaneToCut = -1;
        double maxStdDev = 0.0;
        for (size_t i = 0; i < ndim; i++)
        {
            double *u = new double[bound[i].size()];
            double *l;
            size_t idx = 0;
            for (auto j : bound[i])
            {
                u[idx] = planeHist[i][j];
                idx++;
            }
            if (bound[i].size() > 3)
            {
                l = laplacian_1d(bound[i].size(), 1, u);
                std::pair<int, double> bestZeroCrossingPoint =
                    find_zero_crossing(bound[i].size(), l);
                if (bestZeroCrossingPoint.first >= 0 &&
                    bestZeroCrossingPoint.second > maxStdDev)
                {
                    maxStdDev = bestZeroCrossingPoint.second;
                    bestDimToCut = i;
                    bestPlaneToCut = bestZeroCrossingPoint.first;
                }
            }
            else
            {
                for (size_t j = 0; j < bound[i].size(); j++)
                {
                    if (planeHist[i][bound[i][j]] == 0)
                    {
                        bestDimToCut = i;
                        bestPlaneToCut = j;
                        break;
                    }
                }
                if (bestDimToCut >= 0 && bestPlaneToCut >= 0)
                {
                    break;
                }
            }
        }

        if (bestDimToCut < 0 || bestPlaneToCut < 0)
        {
            double minPlaneHist = 1.0;
            for (size_t i = 0; i < bound.size(); i++)
            {
                if (bound[i].size() == 1)
                {
                    continue;
                }

                for (size_t j = 0; j < bound[i].size(); j++)
                {
                    if (planeHist[i][bound[i][j]] < minPlaneHist)
                    {
                        bestDimToCut = i;
                        bestPlaneToCut = j;
                        minPlaneHist = planeHist[i][bound[i][j]];
                    }
                }
            }
        }

        std::vector<std::vector<size_t>> boundLeft(ndim), boundRight(ndim);
        std::unordered_map<size_t,
                           std::tuple<std::vector<size_t>, std::vector<size_t>>>
            blocksInBoundLeft, blocksInBoundRight;
        if (bestDimToCut >= 0 && bestPlaneToCut >= 0)
        {
            for (size_t i = 0; i < ndim; i++)
            {
                if (i == bestDimToCut)
                {
                    if (bestPlaneToCut > 0 &&
                        bestPlaneToCut < bound[i].size() - 1)
                    {
                        if (std::abs(
                                planeHist[bestDimToCut]
                                         [bound[bestDimToCut][bestPlaneToCut]] -
                                planeHist[bestDimToCut]
                                         [bound[bestDimToCut]
                                               [bestPlaneToCut - 1]]) <
                            std::abs(
                                planeHist[bestDimToCut]
                                         [bound[bestDimToCut]
                                               [bestPlaneToCut + 1]] -
                                planeHist[bestDimToCut]
                                         [bound[bestDimToCut][bestPlaneToCut]]))
                        {
                            std::vector<size_t> left(bound[i].begin(),
                                                     bound[i].begin() +
                                                         bestPlaneToCut + 1);
                            std::vector<size_t> right(bound[i].begin() +
                                                          bestPlaneToCut + 1,
                                                      bound[i].end());
                            boundLeft[i] = left;
                            boundRight[i] = right;
                        }
                        else
                        {
                            std::vector<size_t> left(bound[i].begin(),
                                                     bound[i].begin() +
                                                         bestPlaneToCut);
                            std::vector<size_t> right(bound[i].begin() +
                                                          bestPlaneToCut,
                                                      bound[i].end());
                            boundLeft[i] = left;
                            boundRight[i] = right;
                        }
                    }
                    else if (bestPlaneToCut == 0)
                    {
                        std::vector<size_t> left(bound[i].begin(),
                                                 bound[i].begin() +
                                                     bestPlaneToCut + 1);
                        std::vector<size_t> right(bound[i].begin() +
                                                      bestPlaneToCut + 1,
                                                  bound[i].end());
                        boundLeft[i] = left;
                        boundRight[i] = right;
                    }
                    else if (bestPlaneToCut == bound[i].size() - 1)
                    {
                        std::vector<size_t> left(bound[i].begin(),
                                                 bound[i].begin() +
                                                     bestPlaneToCut);
                        std::vector<size_t> right(
                            bound[i].begin() + bestPlaneToCut, bound[i].end());
                        boundLeft[i] = left;
                        boundRight[i] = right;
                    }

                    for (auto blk : blocksInBoundingBox)
                    {
                        if (std::get<0>(blk.second)[i] <= boundLeft[i].back())
                        {
                            blocksInBoundLeft.insert(blk);
                        }
                        else
                        {
                            blocksInBoundRight.insert(blk);
                        }
                    }
                }
                else
                {
                    boundLeft[i] = bound[i];
                    boundRight[i] = bound[i];
                }
            }
        }
        else
        {
            throw std::runtime_error(
                "ERROR: adiosBlockMerge:FindContainerBoxes() reached invalid "
                "branch. This is a programming error. ");
        }
        if (blocksInBoundLeft.size() > 0)
        {
            containerInfoList.push_back(
                std::make_tuple(boundLeft, blocksInBoundLeft));
        }

        if (blocksInBoundRight.size() > 0)
        {
            containerInfoList.push_back(
                std::make_tuple(boundRight, blocksInBoundRight));
        }
    }
}

} // end namespace helper
} // end namespace adios2
