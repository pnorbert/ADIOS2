/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignDB.h
 * Campaign database operation for recording write events
 *
 *  Created on: March 2, 2024
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_CAMPAIGNDB_H_
#define ADIOS2_ENGINE_CAMPAIGNDB_H_

#include <sqlite3.h>

#include <string>

namespace adios2
{
namespace core
{
namespace engine
{

/*
 * Recording events in some database
 */

const std::string CampaignDBVersion = "1.1";

class CampaignDB
{
public:
    CampaignDB(){};
    ~CampaignDB();

    bool Open(const std::string &name);
    void Close();

    /** Add new entry to list of files
     *  return: an id in the database table (rowidx)
     */
    int64_t AddFile(const std::string &name);

    // check if open was successful
    explicit operator bool() const noexcept;

private:
    std::string m_Name;
    sqlite3 *m_DB = nullptr;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_CAMPAIGNDB_H_ */
