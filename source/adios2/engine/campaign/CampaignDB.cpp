/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignDB.cpp
 * Campaign database operation for recording write events
 *
 *  Created on: March 2, 2024
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "CampaignDB.h"

#include "adios2/helper/adiosFunctions.h"

#include <chrono>
#include <ctime>

namespace adios2
{
namespace core
{
namespace engine
{

/*
 * Recording events in some database
 */

CampaignDB::~CampaignDB()
{
    if (m_DB)
    {
        Close();
    }
};

CampaignDB::operator bool() const noexcept { return (m_DB == nullptr) ? false : true; }

static int sqlcb_info(void *p, int argc, char **argv, char **azColName)
{
    std::string *oldversion = (std::string *)p;
    *oldversion = std::string(argv[0]);
    return 0;
};

bool CampaignDB::Open(const std::string &name)
{
    m_Name = name;
    int rc;
    rc = sqlite3_open(m_Name.c_str(), &m_DB);

    if (rc != SQLITE_OK)
    {
        const std::string m(sqlite3_errstr(rc));
        m_DB = nullptr;
        helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Open",
                                             "SQL error on opening database in " + m_Name + ": " +
                                                 m);
    }

    char *zErrMsg = nullptr;
    std::string sqlcmd = "CREATE TABLE if not exists bpdataset (name PRIMARY KEY);";
    rc = sqlite3_exec(m_DB, sqlcmd.c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::string m(zErrMsg);
        sqlite3_free(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Open",
                                             "SQL error when creating table bpdataset in " +
                                                 m_Name + ": " + m);
    }

    sqlcmd = "CREATE TABLE if not exists info (id, name, version, ctime INT);";
    rc = sqlite3_exec(m_DB, sqlcmd.c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::string m(zErrMsg);
        sqlite3_free(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Open",
                                             "SQL error when creating table info in " + m_Name +
                                                 ": " + m);
    }

    sqlcmd = "CREATE TABLE if not exists step (bpdatasetid INT, enginestep INT, physstep INT, "
             "phystime INT, ctime INT, PRIMARY KEY (bpdatasetid, enginestep));";
    rc = sqlite3_exec(m_DB, sqlcmd.c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::string m(zErrMsg);
        sqlite3_free(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Open",
                                             "SQL error when creating table step in " + m_Name +
                                                 ": " + m);
    }

    std::string oldversion;
    sqlcmd = "SELECT version FROM info";
    rc = sqlite3_exec(m_DB, sqlcmd.c_str(), sqlcb_info, &oldversion, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        sqlite3_free(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Open",
                                             "SQL error when reading table info in " + m_Name +
                                                 ": " + m);
    }

    if (oldversion.empty())
    {
        int64_t wallclockTime_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count();
        sqlcmd = "INSERT INTO info (id, name, version, ctime) VALUES ('ACR', 'ADIOS "
                 "Campaign Recording', '" +
                 CampaignDBVersion + "', " + std::to_string(wallclockTime_us) + ")";
        rc = sqlite3_exec(m_DB, sqlcmd.c_str(), 0, 0, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            std::cout << "SQL error: " << zErrMsg << std::endl;
            std::string m(zErrMsg);
            sqlite3_free(zErrMsg);
            helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Open",
                                                 "SQL error when inserting into table info in " +
                                                     m_Name + ": " + m);
        }
    }
    else if (oldversion != CampaignDBVersion)
    {
        helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Open",
                                             "Old recording was found with version " + oldversion +
                                                 " but we are now on version " + CampaignDBVersion);
    }

    return true;
}

void CampaignDB::Close()
{
    if (m_DB)
    {
        int rc = sqlite3_close(m_DB);
        m_DB = nullptr;
        if (rc != SQLITE_OK)
        {
            const std::string m(sqlite3_errstr(rc));
            m_DB = nullptr;
            helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Close",
                                                 "SQL error when closing database " + m_Name +
                                                     ": " + m);
        }
    }
}

static int sqlcb_bpdataset(void *p, int argc, char **argv, char **azColName)
{
    int64_t *oldFileID = (int64_t *)p;
    *oldFileID = helper::StringTo<int64_t>(argv[0], "Converting SQL rowid from table bpdataset");
    return 0;
};

int64_t CampaignDB::AddFile(const std::string &name, const size_t startStep)
{
    int64_t fileID;
    char *zErrMsg = nullptr;
    int64_t oldFileID = -1;
    std::string sqlcmd = "SELECT rowid FROM bpdataset WHERE name = '" + name + "'";
    int rc = sqlite3_exec(m_DB, sqlcmd.c_str(), sqlcb_bpdataset, &oldFileID, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        sqlite3_free(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "RecordOutput",
                                             "SQL error when reading table bpdataset in " + m_Name +
                                                 ": " + m);
    }

    if (oldFileID >= 0)
    {
        // remove all existing steps from startStep for this file
        std::string sqlcmd = "DELETE FROM step WHERE bpdatasetid = " + std::to_string(oldFileID) +
                             " AND enginestep >= " + std::to_string(startStep);
        int rc = sqlite3_exec(m_DB, sqlcmd.c_str(), 0, 0, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            std::cout << "SQL error: " << zErrMsg << std::endl;
            std::string m(zErrMsg);
            sqlite3_free(zErrMsg);
            helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "RecordOutput",
                                                 "SQL error when reading table bpdataset in " +
                                                     m_Name + ": " + m);
        }
        fileID = oldFileID;
    }
    else
    {
        sqlcmd = "INSERT INTO bpdataset (name)\n";
        sqlcmd += "VALUES('" + name + "');";
        rc = sqlite3_exec(m_DB, sqlcmd.c_str(), 0, 0, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            std::string m(zErrMsg);
            sqlite3_free(zErrMsg);
            helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "RecordOutput",
                                                 "SQL error when recording a new filename in " +
                                                     m_Name + ": " + m);
        }
        fileID = (int64_t)sqlite3_last_insert_rowid(m_DB);
    }

    return fileID;
}

int64_t CampaignDB::AddFileStep(int64_t fileID, size_t engineStep, size_t physStep, double physTime,
                                int64_t ctime)
{
    std::string sqlcmd = "INSERT INTO step (bpdatasetid, enginestep, physstep, phystime, ctime)\n";
    char *zErrMsg = nullptr;
    sqlcmd += "VALUES(" + std::to_string(fileID) + ", " + std::to_string(engineStep) + ", " +
              std::to_string(physStep) + ", " + std::to_string(physTime) + ", " +
              std::to_string(ctime) + ");";
    int rc = sqlite3_exec(m_DB, sqlcmd.c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::string m(zErrMsg);
        sqlite3_free(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Record",
                                             "SQL error when recording a new step in " + m_Name +
                                                 ": " + m);
    }
    return (int64_t)sqlite3_last_insert_rowid(m_DB);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
