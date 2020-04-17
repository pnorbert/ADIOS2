/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 *
 * Created on: Apr 17, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#include <algorithm> //std::for_each
#include <array>
#include <chrono>
#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <sstream>
#include <stdexcept> //std::invalid_argument std::exception
#include <string>
#include <thread>
#include <vector>

#include <adios2.h>

std::string DimsToString(const adios2::Dims &dims)
{
    std::string s = "{";
    for (int i = 0; i < dims.size(); i++)
    {
        if (i > 0)
        {
            s += ", ";
        }
        s += std::to_string(dims[i]);
    }
    s += "}";
    return s;
}

void ReadVariable(const std::string &name, adios2::IO &io,
                  adios2::Engine &reader, size_t step)
{
    adios2::Variable<double> variable = io.InquireVariable<double>(name);

    if (variable)
    {
        adios2::Dims shape = variable.Shape();

        std::vector<double> data;
        reader.Get<double>(variable, data, adios2::Mode::Deferred);
        reader.PerformGets();

        // data vector is now filled with data

        std::cout << "    " << name << " type = " << variable.Type()
                  << " shape = " << DimsToString(shape) << " content:\n";

        if (shape.size() == 2)
        {
            int i = 0;
            for (int x = 0; x < shape[0]; ++x)
            {
                std::cout << "        ";
                for (int y = 0; y < shape[1]; ++y)
                {
                    std::cout << data[i++] << " ";
                }
                std::cout << "\n";
            }
            std::cout << std::endl;
        }
        else
        {
            std::cout << "        ";
            for (const auto datum : data)
            {
                std::cout << datum << " ";
            }
            std::cout << std::endl;
        }
    }
    else
    {
        std::cout << "    Variable " << name << " not found in step " << step
                  << std::endl;
    }
}

template <class T>
std::string vectorToString(std::vector<T> v)
{
    std::stringstream ss;
    ss << "{";
    for (int i = 0; i < v.size(); ++i)
    {
        ss << v[i];
        if (i < v.size() - 1)
        {
            ss << ", ";
        }
    }
    ss << "}";
    return ss.str();
}

template <class T>
void ReadAttribute(const std::string &name, const std::string &varname,
                   adios2::IO &io, adios2::Engine &reader, size_t step)
{
    std::string fullName;
    if (varname.empty())
    {
        fullName = name;
    }
    else
    {
        fullName = varname + adios2::PathSeparator + name;
    }

    adios2::Attribute<T> a = io.InquireAttribute<T>(name, varname);
    if (a)
    {
        std::cout << "    Attribute " << fullName << " type " << a.Type()
                  << " value = " << vectorToString(a.Data()) << std::endl;
    }
    else
    {
        std::cout << "    ERROR in ReadAttribute: " << fullName
                  << " was not found" << std::endl;
    }
}

int main(int argc, char *argv[])
{
    try
    {
        adios2::ADIOS adios;

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines
         * Inline uses single IO for write/read */
        adios2::IO io = adios.DeclareIO("Input");
        io.SetEngine("FileStream");

        adios2::Engine reader = io.Open("attributes.bp", adios2::Mode::Read);

        while (true)
        {

            // Begin step
            adios2::StepStatus read_status =
                reader.BeginStep(adios2::StepMode::Read, 10.0f);
            if (read_status == adios2::StepStatus::NotReady)
            {
                // std::cout << "Stream not ready yet. Waiting...\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }
            else if (read_status != adios2::StepStatus::OK)
            {
                break;
            }

            size_t step = reader.CurrentStep();
            std::cout << "Process step " << step << ": " << std::endl;

            ReadAttribute<int>("Nproc", "", io, reader, step);
            ReadAttribute<int>("Step", "", io, reader, step);
            ReadVariable("data/GlobalArray", io, reader, step);
            ReadAttribute<std::string>("description", "data/GlobalArray", io,
                                       reader, step);
            ReadAttribute<double>("Average", "data/GlobalArray", io, reader,
                                  step);

            reader.EndStep();
        }

        reader.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank\n";
        std::cout << e.what() << "\n";
    }
}
