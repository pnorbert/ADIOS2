/*
 * stat.h
 *
 *  Created on: June 2020
 *      Author: Norbert Podhorszki
 */

#ifndef STAT_H_
#define STAT_H_

#include <fstream>

namespace adios2
{
namespace helper
{

class Stat
{

public:
    Stat(const int rank, const std::string &name);
    ~Stat();

    void Save(const std::string &info);

private:
    const int rank;
    const std::string statDir;

    bool statDirCreated; // True if stat dir exists and can proceed with writing

    std::ofstream of_statm;
    void SaveStatm(const std::string &info);

    std::ofstream of_io;
    void SaveIO(const std::string &info);

    void SaveMeminfo(const std::string &info);
    int meminfoStep = 0; // counter for meminfo file names
};

} // end namespace helper
} // end namespace adios2

#endif /* STAT_H */
