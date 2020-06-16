/*
 * stat.h
 *
 *  Created on: June 2020
 *      Author: Norbert Podhorszki
 */

#ifndef STAT_H_
#define STAT_H_

#include <fstream>
#include <stddef.h> // size_t

#include "processConfig.h"
#include "settings.h"

class Stat
{

public:
    /**
     */
    Stat(const Config &cfg, const Settings &settings);
    ~Stat();

    void Save(size_t step);

private:
    const Config &cfg;
    const Settings &settings;
    const std::string statDir;

    bool statDirCreated; // True if stat dir exists and can proceed with writing

    std::ofstream of_statm;
    void SaveStatm(size_t step);

    std::ofstream of_io;
    void SaveIO(size_t step);

    void SaveMeminfo(size_t step);
};

#endif /* STAT_H */
