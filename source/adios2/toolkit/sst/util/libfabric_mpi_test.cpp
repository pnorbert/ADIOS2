/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Test if the libfabric library is compatible with MPI
 *
 *  Created on: Sept 7, 2021
 *      Author: Norbert Podhorszki, pnorbert@ornl.gov
 */

#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>

#include <iostream>
#include <mpi.h>
#include <stdexcept>
#include <string>
#include <vector>

MPI_Comm comm;    // Communicator of producers OR consumers
int mpi_rank;     // rank of process among producers OR consumers
int mpi_size;     // number of processes of producers OR consumers
int wrank, wsize; // rank and size in world comm
int nProducers;
int nConsumers;
bool amProducer;

std::vector<int> allranks; // array for MPI_Gather()

void init_atoms();
void do_listen();
void do_connect();

void PrintUsage() noexcept
{
    std::cout << "Usage: libfabric_mpi_test[producerRanks] " << std::endl;
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    if (argc > 1)
    {
        amProducer = false;
        nProducers = 0;
        int j = 1;
        char *end;
        while (argc > j)
        {
            errno = 0;
            unsigned long v = std::strtoul(argv[j], &end, 10);
            if ((errno || (end != 0 && *end != '\0')) && !wrank)
            {
                std::string errmsg(
                    "ERROR: Invalid integer number in argument " +
                    std::to_string(j) + ": '" + std::string(argv[j]) + "'\n");
                PrintUsage();
                throw std::invalid_argument(errmsg);
            }
            if (v >= (unsigned long)wsize && !wrank)
            {
                std::string errmsg(
                    "ERROR: Argument " + std::to_string(j) + ": '" +
                    std::string(argv[j]) +
                    "' is larger than the total number of processes\n");
                PrintUsage();
                throw std::invalid_argument(errmsg);
            }
            if (v == (unsigned long)wrank)
            {
                amProducer = true;
                ++nProducers;
            }
            ++j;
        }
    }
    else
    {
        amProducer = (wrank < wsize / 2);
        nProducers = wsize / 2;
    }
    nConsumers = wsize - nProducers;
    std::cout << "Rank " << wrank << " is a "
              << (amProducer ? "Producer" : "Consumer") << std::endl;

    MPI_Comm_split(MPI_COMM_WORLD, (int)amProducer, 0, &comm);
    MPI_Comm_rank(comm, &mpi_rank);
    MPI_Comm_size(comm, &mpi_size);
    MPI_Barrier(comm);

    if (!wrank)
    {
        allranks.resize(wsize);
    }

    // init_atoms();
    MPI_Barrier(MPI_COMM_WORLD);

    if (amProducer)
    {
        do_listen();
    }
    else
    {
        do_connect();
    }

    MPI_Finalize();
    return 0;
}

void do_connect()
{

    // MPI communication test (which also makes producers disconnect first,
    // consumer last)
    MPI_Gather(&wrank, 1, MPI_INT, allranks.data(), 1, MPI_INT, 0,
               MPI_COMM_WORLD);

    std::cout << "Rank " << wrank << " Connection success, all is well!"
              << std::endl;
}

void do_listen()
{
    std::cout << "Rank " << wrank << " Connection success, all is well!"
              << std::endl;

    // MPI communication test
    MPI_Gather(&wrank, 1, MPI_INT, allranks.data(), 1, MPI_INT, 0,
               MPI_COMM_WORLD);
}
