/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignManager.cpp
 *
 * This is NOT a writer Engine but the CampaignReader is a reader Engine.
 *
 *  Created on: May 15, 2023
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "CampaignManager.h"

#include "adios2/helper/adiosFunctions.h"

#include <iostream>

#include <nlohmann_json.hpp>
#include <sqlite3.h>

namespace adios2
{
namespace core
{
namespace engine
{

CampaignManager::CampaignManager(adios2::helper::Comm &comm)

{
    // Note: ADIOS::adios_refcount() + comm.Rank() is still not unique
    // There could be a split communicator used for two ADIOS object
    // with same refcount and same (local) 0 rank
    m_WriterRank = comm.World().Rank();
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << " constructor called" << std::endl;
    }
}

CampaignManager::~CampaignManager()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << " desctructor called\n";
    }
    if (m_CampaignDB)
    {
        Close();
    }
}

void CampaignManager::Open(const std::string &name)
{
    m_Name = m_CampaignDir + "/" + name + "_" + std::to_string(m_WriterRank) + ".acr";
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << " Open(" << m_Name << ")\n";
    }
    // postpone creating directory and creating database until there is something to record
}

void CampaignManager::FirstEvent()
{
    if (m_FirstEvent)
    {
        helper::CreateDirectory(m_CampaignDir);
        m_CampaignDB.Open(m_Name);
        m_FirstEvent = false;
    }
}

int64_t CampaignManager::RecordOutput(const std::string &name, const size_t startStep)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << "   Record Output, name = " << name
                  << " start step = " << adios2::helper::ValueToString(startStep) << "\n";
    }
    FirstEvent();
    // new entry
    int64_t dbFileID = -1;
    if (m_CampaignDB)
    {
        dbFileID = m_CampaignDB.AddFile(name, startStep);
        cmap.emplace(name, dbFileID);
    }
    return dbFileID;
}

void CampaignManager::RecordOutputStep(const std::string &name, const size_t physStep,
                                       const double physTime, const size_t engineStep)
{
    int64_t wallclockTime_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();

    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << "   Record Step, name = " << name
                  << " physStep = " << physStep << " time = " << physTime
                  << " engineStep = " << engineStep << " clock = " << wallclockTime_us << " us\n";
    }

    FirstEvent();

    int dbFileID;
    auto r = cmap.find(name);
    if (r == cmap.end())
    {
        dbFileID = RecordOutput(name, true);
    }
    else
    {
        dbFileID = r->second;
    }
    m_CampaignDB.AddFileStep(dbFileID, engineStep, physStep, physTime, wallclockTime_us);
}

void CampaignManager::Close()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << " Close()\n";
    }
    if (m_CampaignDB)
    {
        m_CampaignDB.Close();
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
