/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Transform.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: wfg
 */
#include <utility>

#include "core/Transform.h"

namespace adios
{

Transform::Transform(std::string method) : m_Method(std::move(method)) {}

void Transform::Compress(const std::vector<char> & /*bufferIn*/,
                         std::vector<char> & /*bufferOut*/)
{
}

void Transform::Decompress(const std::vector<char> & /*bufferIn*/,
                           std::vector<char> & /*bufferOut*/)
{
}

} // end namespace adios
