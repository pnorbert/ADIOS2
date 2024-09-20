/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include "RefactorWriter.tcc"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/operator/refactor/RefactorMDR.h"

namespace adios2
{
namespace core
{
namespace engine
{

RefactorWriter::RefactorStaticMap RefactorWriter::m_DataPtrToMDRVariable = {};

RefactorWriter::RefactorWriter(IO &io, const std::string &name, const Mode mode, helper::Comm comm)
: Engine("RefactorWriter", io, name, mode, std::move(comm))
{
    // helper::GetParameter(io.m_Parameters, "Tiers", m_Tiers);
    //  std::make_shared<refactor::RefactorMDR>(io.m_Parameters));
    // m_DataIO = &m_IO.m_ADIOS.DeclareIO(m_IO.m_Name + "#refactor#data");
    // m_DataIO->SetEngine("BP5");
    // m_DataEngine = &m_DataIO->Open(m_Name, adios2::Mode::Write);
    m_IO.SetEngine("BP5");
    m_DataEngine = &m_IO.Open(m_Name, adios2::Mode::Write);

    /* Need to rename the BP5 engine's name in the IO's map because there cannot be
    two engines with the same name, and the RefactorReader has the same name, which
    IO is going to insert into its map after this call */
    io.RenameEngineInIO(m_Name, m_Name + "#data");

    m_MDRIO = &m_IO.m_ADIOS.DeclareIO(m_IO.m_Name + "#refactor#mdr");
    m_MDRIO->SetEngine("BP5");
    m_MDREngine = &m_MDRIO->Open(m_Name + "/md.r", adios2::Mode::Write);

    m_RefactorOperator = std::make_shared<refactor::RefactorMDR>(io.m_Parameters);
    auto *mdr = reinterpret_cast<refactor::RefactorMDR *>(m_RefactorOperator.get());
    mdr->SetCallbackAfterOperate(CallbackFromRefactorOperator);

    m_IsOpen = true;
}

RefactorWriter::~RefactorWriter()
{
    m_IO.m_ADIOS.RemoveIO(m_MDRIO->m_Name);
    // m_IO.m_ADIOS.RemoveIO(m_DataIO->m_Name);
    if (m_IsOpen)
    {
        DestructorClose(m_FailVerbose);
    }
    m_IsOpen = false;
}

StepStatus RefactorWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    m_DataEngine->BeginStep(mode, timeoutSeconds);
    m_MDREngine->BeginStep(mode, timeoutSeconds);
    return StepStatus::OK;
}

size_t RefactorWriter::CurrentStep() const { return m_DataEngine->CurrentStep(); }

void RefactorWriter::PerformPuts()
{
    m_DataEngine->PerformPuts();
    m_MDREngine->PerformPuts();
}

void RefactorWriter::EndStep()
{
    m_DataEngine->EndStep();
    m_MDREngine->EndStep();
}

void RefactorWriter::Flush(const int transportIndex)
{
    m_DataEngine->Flush(transportIndex);
    m_MDREngine->Flush(transportIndex);
}

// PRIVATE

#define declare_type(T)                                                                            \
    void RefactorWriter::DoPutSync(Variable<T> &variable, const T *data)                           \
    {                                                                                              \
        PutSyncCommon(variable, data);                                                             \
    }                                                                                              \
    void RefactorWriter::DoPutDeferred(Variable<T> &variable, const T *data)                       \
    {                                                                                              \
        PutDeferredCommon(variable, data);                                                         \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void RefactorWriter::DoClose(const int transportIndex)
{
    m_DataEngine->Close();
    m_MDREngine->Close();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
