/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_ENGINE_REFACTORREADER_H_
#define ADIOS2_ENGINE_REFACTORREADER_H_

#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/buffer/malloc/MallocV.h"

namespace adios2
{
namespace core
{
namespace engine
{

class RefactorReader : public Engine
{
public:
    RefactorReader(IO &adios, const std::string &name, const Mode mode, helper::Comm comm);
    virtual ~RefactorReader();

    StepStatus BeginStep(StepMode mode = StepMode::Read, const float timeoutSeconds = -1.0) final;
    size_t CurrentStep() const final;
    void PerformGets() final;
    void EndStep() final;

    MinVarInfo *MinBlocksInfo(const VariableBase &, const size_t Step) const;
    MinVarInfo *MinBlocksInfo(const VariableBase &, const size_t Step, const size_t WriterID,
                              const size_t BlockID) const;
    bool VarShape(const VariableBase &Var, const size_t Step, Dims &Shape) const;
    bool VariableMinMax(const VariableBase &, const size_t Step, MinMaxStruct &MinMax);
    std::string VariableExprStr(const VariableBase &Var);

private:
    IO *m_MDRIO;
    Engine *m_DataEngine;
    Engine *m_MDREngine;
    std::unique_ptr<adios2::core::Operator> m_RefactorOperator = nullptr;
    adios2::format::MallocV m_RefData = adios2::format::MallocV("RefactorWriter");

    double m_Accuracy = 0.01;

#define declare_type(T)                                                                            \
    void DoGetSync(Variable<T> &, T *) final;                                                      \
    void DoGetDeferred(Variable<T> &, T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1);

    /**
     * Called if destructor is called on an open engine.  Should warn or take
     * any non-complex measure that might help recover.
     */
    void DestructorClose(bool Verbose) noexcept final{};

    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetRefactored(Variable<T> &variable, T *values);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_REFACTORREADER_H_ */
