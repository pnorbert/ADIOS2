/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Write attributes to a file. There are four different use cases:
 * 1. Global attribute - a label that applies to the entire output
 * 2. Assigned to a variable - a label that applies to one variable
 * 3. Temporal attribute - an attribute that can change over time (output steps)
 *    - can be both global attribute or assigned to a variable
 * 4. An attribute that shows up later in the output (not at the first step)
 *    - any type of the above
 *
 * The purpose of temporal attributes is to allow for assigning labels to
 * variables in applications where the label should be able to change over time.
 *
 * Attributes are handled separately from time-varying temporal attributes in
 * ADIOS. Attributes cannot be redefined unless they are temporal. They are only
 * stored once in the output's metadata for efficiency. Temporal attributes are,
 * in contrast, written at every output step.
 *
 * Note: Instead of using a global temporal attribute, it is better to use a
 * Global Value variable that is by default can change over time (hence
 * called'variable'). At read time, if reading as a file, all the global values
 * can be seen and read. For an attribute, only the latest value can be seen.
 * With streaming read mode, there should be no difference.
 *
 * Created on: Apr 17, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <adios2.h>
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

int main(int argc, char *argv[])
{
    int rank = 0, nproc = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
#endif
    const int NSTEPS = 5;

    // generate different random numbers on each process,
    // but always the same sequence at each run
    srand(rank * 32767);

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif

    // Application variables/attributes for output
    // 1. Global attribute, constant over time
    // This is 'nproc'

    // 2. Temporal Global attribute, varying value over time
    // This is 'step'
    // Note: this would be better defined as a Global Value variable, not as
    // attribute

    // data for variables in the output
    const unsigned int Nx = 6;
    std::vector<double> row(Nx);
    std::string VariableName = "data/GlobalArray";

    try
    {
        // Get io settings from the config file or
        // create one with default settings here
        adios2::IO io = adios.DeclareIO("Output");

        /*
         * Define attributes (and variables)
         */
        // 1. Global constant, constant over time
        adios2::Attribute<int> attrNproc =
            io.DefineAttribute<int>("Nproc", nproc);

        // An array variable to which we will assign attributes
        adios2::Variable<double> varGA =
            io.DefineVariable<double>(VariableName, {(unsigned int)nproc, 6},
                                      {(unsigned int)rank, 0}, {1, 6});

        // 2. an attribute 'description' assigned to it
        adios2::Attribute<std::string> attrProcessIDDesc =
            io.DefineAttribute<std::string>(
                "description", "A string attribute assigned to a variable",
                VariableName);

        // Open file. "w" means we overwrite any existing file on disk,
        // but Advance() will append steps to the same file.
        adios2::Engine writer = io.Open("attributes.bp", adios2::Mode::Write);

        for (int step = 0; step < NSTEPS; step++)
        {
            if (!rank)
            {
                std::cout << "Output step " << step << std::endl;
            }
            writer.BeginStep();

            /* Calculation phase for variable */
            double avg = 0.0;
            for (size_t i = 0; i < Nx; i++)
            {
                row[i] = step * Nx * nproc * 1.0 + rank * Nx * 1.0 + (double)i;
                avg += row[i];
            }
#ifdef ADIOS2_HAVE_MPI
            double d;
            MPI_Reduce(&avg, &d, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
            avg = d / (nproc * Nx);
#else
            avg = avg / Nx;
#endif

            // 3.a. Global value, varying value over time
            // Note: Global Value Variable would serve this purpose better
            adios2::Attribute<int> attrStep =
                io.DefineAttribute<int>("Step", step);
            attrStep.SetTemporal();

            // 3.b. Attached to a variable, varying value over time
            adios2::Attribute<double> attrGAAverage =
                io.DefineAttribute<double>("Average", avg, VariableName);
            attrGAAverage.SetTemporal();

            // Write the variables. Attributes are automatically added in
            // EndStep
            writer.Put<double>(varGA, row.data());

            // Indicate we are done for this step.
            writer.EndStep();

            if (!rank)
            {
                std::cout << "Sleep for a few seconds... " << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }

        // Called once: indicate that we are done with this output for the run
        writer.Close();
    }
    catch (std::invalid_argument &e)
    {
        if (rank == 0)
        {
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch (std::ios_base::failure &e)
    {
        if (rank == 0)
        {
            std::cout << "System exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch (std::exception &e)
    {
        if (rank == 0)
        {
            std::cout << "Exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
}
