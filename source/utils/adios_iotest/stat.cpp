/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 * stat.cpp
 *
 *  Created on: June 2020
 *      Author: Norbert Podhorszki
 */

#include "stat.h"
#include <adios2sys/SystemTools.hxx>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

Stat::Stat(const Config &cfg, const Settings &settings)
: cfg(cfg), settings(settings),
  statDir("stat_" + std::to_string(settings.appId))
{
#ifndef _WIN32
    if (!settings.myRank && settings.saveStats)
    {
        statDirCreated = adios2sys::SystemTools::MakeDirectory(statDir);
        if (statDirCreated)
        {
            of_statm.open(statDir + "/statm.txt",
                          std::ios_base::out | std::ios_base::trunc);
            of_io.open(statDir + "/io.txt",
                       std::ios_base::out | std::ios_base::trunc);
        }
        if (of_statm.is_open())
        {
            of_statm
                << "/proc/self/statm"
                   "  Provides information about memory usage, measured in "
                   "pages. The columns are:\n"
                   "    size(1)       total program size(same as VmSize in "
                   "/proc/self/status) \n"
                   "    resident(2)   resident set size(same as VmRSS in "
                   "/proc/self/status)\n"
                   "    shared(3)     number of resident shared pages(i.e., "
                   "backed by a file)\n"
                   "                  same as RssFile + RssShmem in "
                   "/proc/self/status)\n"
                   "    text(4)       text(code) \n"
                   "    lib(5)        library (always 0) \n"
                   "    data(6)       data + stack \n"
                   "    dt(7)         dirty pages(always 0)\n";
        }
        if (of_io.is_open())
        {
            of_io << "/proc/self/io\n"
                     "Step    rchar           wchar           syscr          "
                     " syscw           read_bytes      write_bytes     "
                     "cancelled_write_bytes\n";
        }
    }
    else
    {
        statDirCreated = false;
    }
#else
    statDirCreated = false;
#endif
}

Stat::~Stat()
{
    if (of_statm.is_open())
    {
        of_statm.close();
    }
    if (of_io.is_open())
    {
        of_io.close();
    }
}

void Stat::Save(size_t step)
{
    if (!settings.myRank && statDirCreated)
    {
        SaveStatm(step);
        SaveIO(step);
        SaveMeminfo(step);
    }
}

void Stat::SaveStatm(size_t step)
{
    if (!of_statm.is_open() || !of_statm.good())
    {
        return;
    }
    std::ifstream ifs("/proc/self/statm", std::ios_base::in);
    if (ifs.good())
    {
        for (std::string line; getline(ifs, line);)
        {
            of_statm << "Step " << std::to_string(step) << ": " << line
                     << std::endl;
        }
        ifs.close();
    }
}

void Stat::SaveIO(size_t step)
{
    if (!of_statm.is_open() || !of_statm.good())
    {
        return;
    }
    std::ifstream ifs("/proc/self/io", std::ios_base::in);
    if (ifs.good())
    {
        of_io << std::left << std::setw(8) << step;
        for (std::string line; getline(ifs, line);)
        {
            const auto s = line.substr(line.find(" ") + 1);
            of_io << std::left << std::setw(16) << s;
        }
        of_io << std::endl;
        ifs.close();
    }
    else
    {
        std::cout << "ERROR: /proc/self/io cannot be opened" << std::endl;
    }
}

void Stat::SaveMeminfo(size_t step)
{
    std::ifstream ifs("/proc/meminfo",
                      std::ios_base::in | std::ios_base::binary);
    if (ifs.good())
    {
        std::stringstream ss;
        ss << statDir << "/meminfo_" << std::setfill('0') << std::setw(6)
           << step << ".txt";
        std::ofstream ofs(ss.str(), std::ios_base::out | std::ios_base::trunc |
                                        std::ios_base::binary);
        if (ofs.good())
        {
            ofs << "Step: " << step << "\n------------\n";
            char buf[2048];
            do
            {
                ifs.read(&buf[0], sizeof(buf));
                ofs.write(&buf[0], ifs.gcount());
            } while (ifs.gcount() > 0);
            ifs.close();
            ofs.close();
        }
    }
    else
    {
        std::cout << "ERROR: /proc/meminfo cannot be opened" << std::endl;
    }
}
