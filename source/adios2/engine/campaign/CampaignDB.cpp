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

static std::string oldversion;
static int sqlcb_info(void *p, int argc, char **argv, char **azColName)
{
    oldversion = std::string(argv[0]);
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

    sqlcmd = "CREATE TABLE if not exists info (id, name, version, ctime REAL);";
    rc = sqlite3_exec(m_DB, sqlcmd.c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::string m(zErrMsg);
        sqlite3_free(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Open",
                                             "SQL error when creating table info in " + m_Name +
                                                 ": " + m);
    }

    sqlcmd = "SELECT version FROM info";
    rc = sqlite3_exec(m_DB, sqlcmd.c_str(), sqlcb_info, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Open",
                                             "SQL error when reading table info in " + m_Name +
                                                 ": " + m);
        sqlite3_free(zErrMsg);
    }

    if (oldversion.empty())
    {
        const auto p1 = std::chrono::system_clock::now();
        auto itime =
            std::chrono::duration_cast<std::chrono::microseconds>(p1.time_since_epoch()).count();
        double ctime = itime / 1000000.0;
        sqlcmd = "INSERT INTO info (id, name, version, ctime) VALUES ('ACR', 'ADIOS "
                 "Campaign Recording', '" +
                 CampaignDBVersion + "', " + std::to_string(ctime) + ")";
        std::cout << "SQL command: " << sqlcmd << std::endl;
        rc = sqlite3_exec(m_DB, sqlcmd.c_str(), 0, 0, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            std::cout << "SQL error: " << zErrMsg << std::endl;
            std::string m(zErrMsg);
            helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Open",
                                                 "SQL error when inserting into table info in " +
                                                     m_Name + ": " + m);
            sqlite3_free(zErrMsg);
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

int64_t CampaignDB::AddFile(const std::string &name)
{
    std::string sqlcmd = "INSERT OR IGNORE INTO bpdataset (name)\n";
    char *zErrMsg = nullptr;
    sqlcmd += "VALUES('" + name + "');";
    int rc = sqlite3_exec(m_DB, sqlcmd.c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::string m(zErrMsg);
        sqlite3_free(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignManager", "Record",
                                             "SQL error when recording a new filename in " +
                                                 m_Name + ": " + m);
    }
    return (int64_t)sqlite3_last_insert_rowid(m_DB);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
