/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * AttributeBase.h : base class for Attribute<T> class, allows RTTI at read time
 *
 *  Created on: Aug 1, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ATTRIBUTEBASE_H_
#define ADIOS2_CORE_ATTRIBUTEBASE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"

namespace adios2
{
namespace core
{

class AttributeBase
{

public:
    const std::string m_Name;
    const std::string m_Type;
    const bool m_IsSingleValue;

    /**
     * Single value constructor used by Attribute<T> derived class
     * @param name
     * @param type
     */
    AttributeBase(const std::string &name, const std::string type);

    /**
     * Array constructor used by Attribute<T> derived class
     * @param name
     * @param type
     * @param elements
     */
    AttributeBase(const std::string &name, const std::string type,
                  const size_t elements);

    virtual ~AttributeBase() = default;
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_ATTRIBUTEBASE_H_ */
