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
    m_Name = m_CampaignDir + "/" + name + "_" + std::to_string(m_WriterRank) + ".db";
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << " Open(" << m_Name << ")\n";
    }
    // postpone creating directory and creating database until there is something to record
}

void CampaignManager::Record(const std::string &name, const size_t step, const double time)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << "   Record name = " << name
                  << " step = " << step << " time = " << time << "\n";
    }

    if (m_FirstEvent)
    {
        helper::CreateDirectory(m_CampaignDir);
        m_CampaignDB.Open(m_Name);
        m_FirstEvent = false;
    }

    auto r = cmap.find(name);
    if (r != cmap.end())
    {
        // update record
        size_t last_step = r->second.steps.back();
        size_t delta_step = step - last_step;
        double last_time = r->second.times.back();
        double delta_time = time - last_time;
        auto nsteps = r->second.steps.size();
        if (nsteps == 1)
        {
            r->second.delta_step = delta_step;
            r->second.delta_time = delta_time;
        }
        else
        {
            size_t old_delta_step = r->second.steps.back() - r->second.steps.rbegin()[1];
            if (old_delta_step != delta_step)
            {
                r->second.delta_step = 0;
                r->second.delta_time = 0.0;
                r->second.varying_deltas = true;
            }
        }
        r->second.steps.push_back(step);
        r->second.times.push_back(time);
    }
    else
    {
        // new entry

        if (m_CampaignDB)
        {
            int64_t dbFileID = m_CampaignDB.AddFile(name);
            CampaignRecord r(dbFileID, step, time);
            cmap.emplace(name, r);
        }
    }
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
