/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include "RefactorReader.h"
#include "RefactorReader.tcc"

#include "adios2/helper/adiosString.h"
#include "adios2/operator/refactor/RefactorMDR.h"

namespace adios2
{
namespace core
{
namespace engine
{

RefactorReader::RefactorReader(IO &io, const std::string &name, const Mode mode, helper::Comm comm)
: Engine("RefactorReader", io, name, mode, std::move(comm))
{
    // helper::GetParameter(m_IO.m_Parameters, "accuracy", m_Accuracy);
    // Params params = {{"accuracy", std::to_string(m_Accuracy)}};
    m_RefactorOperator = std::make_unique<refactor::RefactorMDR>(Params());

    io.SetEngine("BP5");
    m_DataEngine = &io.Open(m_Name, mode);

    /* Need to rename the BP5 engine's name in the IO's map because there cannot be
    two engines with the same name, and the RefactorReader has the same name, which
    IO is going to insert into its map after this call */
    io.RenameEngineInIO(m_Name, m_Name + "#data");

    m_MDRIO = &m_IO.m_ADIOS.DeclareIO(m_IO.m_Name + "#refactor#mdr");
    m_MDRIO->SetEngine("BP5");
    m_MDREngine = &m_MDRIO->Open(m_Name + "/md.r", mode);

    m_IsOpen = true;
}

RefactorReader::~RefactorReader()
{
    m_IO.m_ADIOS.RemoveIO(m_MDRIO->m_Name);
    if (m_IsOpen)
    {
        DestructorClose(m_FailVerbose);
    }
    m_IsOpen = false;
}

StepStatus RefactorReader::BeginStep(const StepMode mode, const float timeoutSeconds)
{
    StepStatus status = m_DataEngine->BeginStep(mode, timeoutSeconds);
    m_MDREngine->BeginStep(mode, timeoutSeconds);
    return status;
}

size_t RefactorReader::CurrentStep() const { return m_DataEngine->CurrentStep(); }

void RefactorReader::PerformGets()
{
    /* This should be called for non-refactored integer data */
    m_DataEngine->PerformGets();
}

void RefactorReader::EndStep()
{
    m_DataEngine->EndStep();
    m_MDREngine->EndStep();
}

MinVarInfo *RefactorReader::MinBlocksInfo(const VariableBase &Var, const size_t Step) const
{
    return m_DataEngine->MinBlocksInfo(Var, Step);
}

MinVarInfo *RefactorReader::MinBlocksInfo(const VariableBase &Var, const size_t Step,
                                          const size_t WriterID, const size_t BlockID) const
{
    return m_DataEngine->MinBlocksInfo(Var, Step, WriterID, BlockID);
}

bool RefactorReader::VarShape(const VariableBase &Var, const size_t Step, Dims &Shape) const
{
    return m_DataEngine->VarShape(Var, Step, Shape);
}

bool RefactorReader::VariableMinMax(const VariableBase &Var, const size_t Step,
                                    MinMaxStruct &MinMax)
{
    return m_DataEngine->VariableMinMax(Var, Step, MinMax);
}

std::string RefactorReader::VariableExprStr(const VariableBase &Var)
{
    return m_DataEngine->VariableExprStr(Var);
}

// PRIVATE

#define declare_type(T)                                                                            \
    void RefactorReader::DoGetSync(Variable<T> &variable, T *data)                                 \
    {                                                                                              \
        GetSyncCommon(variable, data);                                                             \
    }                                                                                              \
    void RefactorReader::DoGetDeferred(Variable<T> &variable, T *data)                             \
    {                                                                                              \
        GetDeferredCommon(variable, data);                                                         \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void RefactorReader::DoClose(const int transportIndex)
{
    m_DataEngine->Close();
    m_MDREngine->Close();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
