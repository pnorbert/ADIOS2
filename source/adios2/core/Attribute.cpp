/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.cpp : needed for template separation using Attribute.tcc
 *
 *  Created on: Aug 3, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Attribute.h"
#include "Attribute.tcc"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

#include <type_traits>

namespace adios2
{
namespace core
{

namespace // anonymous
{

template <class T>
struct RequiresZeroPadding : std::false_type
{
};

template <>
struct RequiresZeroPadding<long double> : std::true_type
{
};

}

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute(const Attribute<T> &other) : AttributeBase(other)  \
    {                                                                          \
        /* TODO: Padding */                                                    \
        m_DataSingleValueVector = other.m_DataSingleValueVector;               \
        m_DataArrayVector = other.m_DataArrayVector;                           \
        m_UpdateSteps = other.m_UpdateSteps;                                   \
        m_CurrentStep = other.m_CurrentStep;                                   \
        m_CurrentStepPos = other.m_CurrentStepPos;                             \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute(const std::string &name, const T *array,           \
                            const size_t elements, const size_t step)          \
    : AttributeBase(name, helper::GetType<T>(), elements)                      \
    {                                                                          \
        if (RequiresZeroPadding<T>::value)                                     \
        {                                                                      \
            m_DataArrayVector.resize(1);                                       \
            m_DataArrayVector[0].resize(elements);                             \
            for (int i = 0; i < elements; ++i)                                 \
            {                                                                  \
                std::memset(&m_DataArrayVector[0][i], 0, sizeof(T));           \
                m_DataArrayVector[0][i] = array[i];                            \
            }                                                                  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            m_DataArrayVector.push_back(                                       \
                std::vector<T>(array, array + elements));                      \
        }                                                                      \
        m_UpdateSteps.push_back(step);                                         \
        m_CurrentStep = step;                                                  \
        m_CurrentStepPos = m_UpdateSteps.size() - 1;                           \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute(const std::string &name, const T &value,           \
                            const size_t step)                                 \
    : AttributeBase(name, helper::GetType<T>())                                \
    {                                                                          \
        if (RequiresZeroPadding<T>::value)                                     \
        {                                                                      \
            m_DataSingleValueVector.resize(1);                                 \
            std::memset(&m_DataSingleValueVector[0], 0, sizeof(T));            \
            m_DataSingleValueVector[0] = value;                                \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            m_DataSingleValueVector.push_back(value);                          \
        }                                                                      \
        m_UpdateSteps.push_back(step);                                         \
        m_CurrentStep = step;                                                  \
        m_CurrentStepPos = m_UpdateSteps.size() - 1;                           \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Params Attribute<T>::GetInfo() const noexcept                              \
    {                                                                          \
        return DoGetInfo();                                                    \
    }                                                                          \
    template <>                                                                \
    void Attribute<T>::SetMutable()                                            \
    {                                                                          \
        m_Mutable = true;                                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    bool Attribute<T>::IsMutable() const noexcept                              \
    {                                                                          \
        return m_Mutable;                                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Attribute<T>::AddUpdate(const T &value, const size_t step)            \
    {                                                                          \
        if (RequiresZeroPadding<T>::value)                                     \
        {                                                                      \
            size_t n = m_DataSingleValueVector.size();                         \
            m_DataSingleValueVector.resize(n + 1);                             \
            std::memset(&m_DataSingleValueVector[n], 0, sizeof(T));            \
            m_DataSingleValueVector[n] = value;                                \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            m_DataSingleValueVector.push_back(value);                          \
        }                                                                      \
        m_UpdateSteps.push_back(step);                                         \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Attribute<T>::AddUpdate(const T *array, const size_t elements,        \
                                 const size_t step)                            \
    {                                                                          \
        m_DataArrayVector.push_back(std::vector<T>(array, array + elements));  \
        m_UpdateSteps.push_back(step);                                         \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Attribute<T>::SetStep(const size_t step) noexcept                     \
    {                                                                          \
        m_CurrentStep = step;                                                  \
        if (m_IsSingleValue)                                                   \
        {                                                                      \
            m_CurrentStepPos = m_DataSingleValueVector.size(); /*TODO*/        \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            m_CurrentStepPos = m_DataArrayVector.size(); /*TODO*/              \
        }                                                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    const T &Attribute<T>::SingleValue() const noexcept                        \
    {                                                                          \
        return m_DataSingleValueVector[m_CurrentStepPos];                      \
    }                                                                          \
    template <>                                                                \
    const std::vector<T> &Attribute<T>::DataArray() const noexcept             \
    {                                                                          \
        return m_DataArrayVector[m_CurrentStepPos];                            \
    }                                                                          \
    template <>                                                                \
    const size_t Attribute<T>::NElements() const noexcept                      \
    {                                                                          \
        if (m_IsSingleValue)                                                   \
        {                                                                      \
            return 1;                                                          \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            return DataArray().size();                                         \
        }                                                                      \
    }

ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type

} // end namespace core
} // end namespace adios2
