/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Test if the ADIOS library using SST Engine is compatible with MPI
 *
 *  Created on: Sept 14, 2021
 *      Author: Norbert Podhorszki, pnorbert@ornl.gov
 */

#include <chrono>
#include <iostream>
#include <mpi.h>
#include <stdexcept>
#include <string>
#include <thread> // sleep_for
#include <vector>

#include <adios2.h>

MPI_Comm comm;    // Communicator of producers OR consumers
int mpi_rank;     // rank of process among producers OR consumers
int mpi_size;     // number of processes of producers OR consumers
int wrank, wsize; // rank and size in world comm
int nProducers;
int nConsumers;
bool amProducer;

std::vector<int> allranks; // array for MPI_Gather()

void producer();
void consumer();

void PrintUsage() noexcept
{
    std::cout << "Usage: adios_sst_mpi_compatibility [producerRanks] "
              << std::endl;
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

    if (amProducer)
    {
        producer();
    }
    else
    {
        consumer();
    }

    MPI_Finalize();
    return 0;
}

const std::vector<int> baseData = {1, 2,  3,  4,  5,  6,  7,  8,
                                   9, -1, -2, -3, -4, -5, -6, -7};
constexpr size_t NSTEPS = 2;

void producer()
{
    const std::size_t Nx = baseData.size();
    std::vector<int> data(Nx);
    try
    {
        adios2::ADIOS adios(comm);
        adios2::IO io = adios.DeclareIO("myIO");
        io.SetEngine("Sst");
        io.SetParameter("DataTransport", "rdma");

        auto var = io.DefineVariable<int>("data", {nProducers * Nx},
                                          {mpi_rank * Nx}, {Nx});

        adios2::Engine engine = io.Open("helloADIOSSst", adios2::Mode::Write);

        for (size_t step = 0; step < NSTEPS; ++step)
        {
            std::cout << "Producer " << mpi_rank << ": produce step " << step
                      << "\n";
            engine.BeginStep();
            for (size_t i = 0; i < Nx; ++i)
            {
                data[i] = mpi_rank + step + baseData[i];
            }
            engine.Put<int>(var, data.data());
            engine.EndStep();
        }

        engine.Close();

        // MPI communication test (which also makes producers disconnect first,
        // consumer last)
        // std::cout << "Producer " << mpi_rank << ": enter global MPI
        // Gather\n";
        MPI_Gather(&wrank, 1, MPI_INT, allranks.data(), 1, MPI_INT, 0,
                   MPI_COMM_WORLD);
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Producer " << mpi_rank
                  << ": Invalid argument exception, STOP from rank " << wrank
                  << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "Producer " << mpi_rank
                  << ": IO System base failure exception, STOP from rank "
                  << wrank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Producer " << mpi_rank << ": Exception, STOP from rank "
                  << wrank << "\n";
        std::cout << e.what() << "\n";
    }
    std::cout << "Rank " << wrank << ": Producer completed!" << std::endl;
}

void consumer()
{
    const std::size_t Nx = baseData.size();
    std::vector<int> data(nProducers * Nx);
    try
    {
        adios2::ADIOS adios(comm);
        adios2::IO io = adios.DeclareIO("myIO");
        io.SetEngine("Sst");
        io.SetParameter("DataTransport", "rdma");

        adios2::Engine engine = io.Open("helloADIOSSst", adios2::Mode::Read);

        size_t step = 0;
        while (true)
        {
            adios2::StepStatus status =
                engine.BeginStep(adios2::StepMode::Read, 60.0f);
            if (status == adios2::StepStatus::NotReady)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            else if (status != adios2::StepStatus::OK)
            {
                break;
            }

            std::cout << "Consumer " << mpi_rank << ": received step " << step
                      << "\n";

            // Every consumer gets all producer data in this test
            auto var = io.InquireVariable<int>("data");
            adios2::Dims shape = var.Shape();
            if (shape[0] != static_cast<size_t>(nProducers * Nx))
            {
                std::cout << "Consumer " << mpi_rank
                          << ": data size = " << shape[0] << " while expected "
                          << nProducers * Nx << ". STOP from rank " << wrank
                          << "\n";
                break;
            }
            engine.Get<int>(var, data.data());
            engine.EndStep();

            int idx = 0;
            for (int p = 0; p < nProducers; ++p)
            {
                for (size_t i = 0; i < Nx; ++i)
                {
                    int expected = p + step + baseData[i];
                    if (data[idx] != expected)
                    {
                        std::cout << "Consumer " << mpi_rank << ": data[" << idx
                                  << "] = " << data[idx]
                                  << " is wrong, expected " << expected
                                  << ". STOP from rank " << wrank << "\n";
                        // break;
                    }
                    ++idx;
                }
            }
            ++step;
        }

        // MPI communication test (which also makes producers disconnect first,
        // consumer last)
        // std::cout << "Consumer " << mpi_rank << ": enter global MPI
        // Gather\n";
        MPI_Gather(&wrank, 1, MPI_INT, allranks.data(), 1, MPI_INT, 0,
                   MPI_COMM_WORLD);

        engine.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Consumer " << mpi_rank
                  << ": Invalid argument exception, STOP from rank " << wrank
                  << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "Consumer " << mpi_rank
                  << ": IO System base failure exception, STOP from rank "
                  << wrank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Consumer " << mpi_rank << ": Exception, STOP from rank "
                  << wrank << "\n";
        std::cout << e.what() << "\n";
    }
    std::cout << "Rank " << wrank << ": Consumer completed!" << std::endl;
}
