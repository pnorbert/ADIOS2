/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_ENGINE_REFACTORWRITER_H_
#define ADIOS2_ENGINE_REFACTORWRITER_H_

#include "adios2/core/Engine.h"

namespace adios2
{
namespace core
{
namespace engine
{

class RefactorWriter : public Engine
{

public:
    RefactorWriter(IO &adios, const std::string &name, const Mode mode, helper::Comm comm);
    virtual ~RefactorWriter();

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = -1.0) final;
    size_t CurrentStep() const final;
    void PerformPuts() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

private:
    IO *m_DataIO;
    IO *m_MDRIO;
    Engine *m_DataEngine;
    Engine *m_MDREngine;
    // std::unordered_map<std::string, std::shared_ptr<Operator>> m_TransportMap;

    void PutSubEngine(bool finalPut = false);

#define declare_type(T)                                                                            \
    void DoPutSync(Variable<T> &, const T *) final;                                                \
    void DoPutDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1) final;

    /**
     * Called if destructor is called on an open engine.  Should warn or take
     * any non-complex measure that might help recover.
     */
    void DestructorClose(bool Verbose) noexcept final{};

    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PutRefactored(Variable<T> &variable, const T *values);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_REFACTORWRITER_H_
