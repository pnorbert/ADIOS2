/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.tcc
 *
 *  Created on: Oct 9, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ATTRIBUTE_TCC_
#define ADIOS2_CORE_ATTRIBUTE_TCC_

#include "Attribute.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{

template <class T>
Params Attribute<T>::DoGetInfo() const noexcept
{
    Params info;
    info["Type"] = m_Type;

    if (m_IsSingleValue)
    {
        info["Value"] = helper::ValueToString(SingleValue());
        info["Elements"] = std::to_string(1);
    }
    else
    {
        auto &v = DataArray();
        info["Value"] = "{ " + helper::VectorToCSV(v) + " }";
        info["Elements"] = std::to_string(v.size());
    }
    return info;
}

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_ATTRIBUTE_TCC_ */
