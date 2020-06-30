/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * AsyncTaskManagerSingleThread.h
 *
 *  Created on: June 29, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_ASYNCTASKMANAGERSINGLETHREAD_H_
#define ADIOS2_TOOLKIT_TRANSPORT_ASYNCTASKMANAGERSINGLETHREAD_H_

#include "adios2/toolkit/transportman/AsyncTaskManager.h"
#include "adios2/toolkit/transportman/TransportMan.h"

#include <thread>

namespace adios2
{
namespace transportman
{

class AsyncTaskManagerSingleThread : public AsyncTaskManager
{

public:
    static const size_t defaultBufferSize = 4194304; // 4MB

    AsyncTaskManagerSingleThread();

    ~AsyncTaskManagerSingleThread();

    void SetBufferSize(size_t bufferSizeBytes);

    /** Create thread.
     * This will create a thread to continuously run and idle if there
     *  are no operations given.
     *  Finish() will complete all work then join the thread
     */
    void Start();

    /** Tell thread to terminate when all draining has finished. */
    void Finish();

    /** Join the thread. Main thread will block until thread terminates */
    void Join();

private:
    size_t bufferSize = defaultBufferSize;
    std::thread th; // created by constructor
    bool finish = false;
    std::mutex finishMutex;
    void DrainThread(); // the thread function
};

} // end namespace transportman
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORT_ASYNCTASKMANAGERSINGLETHREAD_H_ */
