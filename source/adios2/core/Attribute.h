/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.h : template class that defines typed attributes
 *
 *  Created on: Aug 1, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ATTRIBUTE_H_
#define ADIOS2_CORE_ATTRIBUTE_H_

#include "adios2/core/AttributeBase.h"

namespace adios2
{
namespace core
{

/** @brief Attributes provide complementary information to IO Variables*/
template <class T>
class Attribute : public AttributeBase
{

public:
    std::vector<T> m_DataArray; ///< holds data for array attributes
    T m_DataSingleValue;        ///< holds data for single value attributes

    /**
     * Copy constructor (enforces zero-padding)
     * @param other
     */
    Attribute<T>(const Attribute<T> &other);

    /**
     * Data array constructor
     * @param name
     * @param data
     * @param elements
     */
    Attribute<T>(const std::string &name, const T *data, const size_t elements);

    /**
     * Single value constructor
     * @param name
     * @param data
     * @param elements
     */
    Attribute<T>(const std::string &name, const T &data);

    ~Attribute<T>() = default;

    Params GetInfo() const noexcept;

    void SetMutable();
    bool IsMutable() const noexcept;

    void AddUpdate(const T &data, const size_t step);
    void AddUpdate(const T *data, const size_t elements, const size_t step);

private:
    bool m_Mutable = false;
    Params DoGetInfo() const noexcept;

    /* Update management for mutable attributes */
    // holds data for array attributes
    std::vector<std::vector<T>> m_DataArrayVector;
    // holds data for single value attributes
    std::vector<T> m_DataSingleValueVector;
    // steps in which attribute value is changing
    std::vector<size_t> m_UpdateSteps;
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_ATTRIBUTE_H_ */
