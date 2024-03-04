/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignManager.h
 * Campaign manager object created by the ADIOS object as GlobalService.
 * Record all output under the ADIOS object so that CampaignReader can
 * see all data for as a single reader.
 * This is NOT a writer Engine but the CampaignReader is a reader Engine.
 *
 *  Created on: May 15, 2023
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_CAMPAIGNMANAGER_H_
#define ADIOS2_ENGINE_CAMPAIGNMANAGER_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/helper/adiosComm.h"

#ifdef ADIOS2_HAVE_CAMPAIGN
#include "CampaignDB.h"
#include "CampaignRecord.h"

#include <fstream>
#endif /* ADIOS2_HAVE_CAMPAIGN */

namespace adios2
{
namespace core
{

namespace engine
{
class CampaignManager
{
#ifdef ADIOS2_HAVE_CAMPAIGN
public:
    CampaignManager(helper::Comm &comm);
    ~CampaignManager();

    void Open(const std::string &name);
    /** Add a new output to the record, startStep = 0 if this output is created anew
     *  @return an integer ID of the inserted record to the database (for internal use only)
     */
    int64_t RecordOutput(const std::string &name, const size_t startStep);
    /** Add a new step to an output */
    void RecordOutputStep(const std::string &name, const size_t physStep, const double physTime,
                          const size_t engineStep);
    void Close();

private:
    bool m_FirstEvent = true;
    void FirstEvent();
    std::string m_Name;
    int m_WriterRank;
    int m_Verbosity = 0;
    CampaignDB m_CampaignDB;
    CampaignRecordMap cmap;
    std::ofstream m_Output;
    const std::string m_CampaignDir = "adios-campaign";

#else
public:
    CampaignManager(helper::Comm &comm){};
    ~CampaignManager() = default;
    void Open(const std::string &name){};
    void Record(const std::string &name, const size_t step, const double time){};
    void Close(){};

#endif /* ADIOS2_HAVE_CAMPAIGN */
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_CAMPAIGNMANAGER_H_ */
