// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "taskscheduler.h"
#include "taskscheduler_sys.h"
#if defined(__MIC__)
#include "taskscheduler_mic.h"
#endif
#include "sys/sysinfo.h"
#include "sys/sync/atomic.h"
#include "math/math.h"

namespace embree
{
  /* initialization structure for threads */
  struct Thread 
  {
    Thread (size_t threadIndex, size_t threadCount, TaskScheduler* scheduler) 
      : threadIndex(threadIndex), threadCount(threadCount), scheduler(scheduler) {}

  public:
    size_t threadIndex;
    size_t threadCount;
    TaskScheduler* scheduler;
  };
  
  TaskScheduler* TaskScheduler::instance = NULL;

  __dllexport void TaskScheduler::create(size_t numThreads)
  {
    if (instance)
      THROW_RUNTIME_ERROR("Embree threads already running.");

    /* enable fast pthreads tasking system */
#if defined(__MIC__)
    instance = new TaskSchedulerMIC; 
    //instance = new TaskSchedulerSys;
#else
    instance = new TaskSchedulerSys; 
#endif

    instance->createThreads(numThreads);
  }

  __dllexport size_t TaskScheduler::getNumThreads() 
  {
    if (!instance) THROW_RUNTIME_ERROR("Embree threads not running.");
    return instance->numEnabledThreads;
  }

  size_t TaskScheduler::enableThreads(size_t N)
  {
    if (!instance) THROW_RUNTIME_ERROR("Embree threads not running.");
    // if (!instance->defaultNumThreads) return; // FIXME: enable
    N = min(N,instance->numThreads);
    //TaskScheduler::init(N);
    return instance->numEnabledThreads = N;
  }

  __dllexport void TaskScheduler::addTask(ssize_t threadIndex, QUEUE queue, Task* task)
  {
    if (!instance) THROW_RUNTIME_ERROR("Embree threads not running.");
    instance->add(threadIndex,queue,task);
  }

  void TaskScheduler::executeTask(size_t threadIndex, size_t threadCount, 
                                  runFunction run, void* runData, size_t elts, completeFunction complete, void* completeData, const char* name)
  {
    TaskScheduler::Event event;
    TaskScheduler::Task task(&event,run,runData,elts,complete,completeData,name);
    instance->add(threadIndex,TaskScheduler::GLOBAL_FRONT,&task);
    instance->wait(threadIndex,threadCount,&event);
  }

  void TaskScheduler::executeTask(size_t threadIndex, size_t threadCount,  
                                  runFunction run, void* runData, size_t elts, const char* name)
  {
    TaskScheduler::Event event;
    TaskScheduler::Task task(&event,run,runData,elts,NULL,NULL,name);
    instance->add(threadIndex,TaskScheduler::GLOBAL_FRONT,&task);
    instance->wait(threadIndex,threadCount,&event);
  }

  void TaskScheduler::executeTask(size_t threadIndex, size_t threadCount,  
                                  completeFunction complete, void* completeData, const char* name)
  {
    TaskScheduler::Event event;
    TaskScheduler::Task task(&event,NULL,NULL,1,complete,completeData,name);
    instance->add(threadIndex,TaskScheduler::GLOBAL_FRONT,&task);
    instance->wait(threadIndex,threadCount,&event);
  }

  void TaskScheduler::waitForEvent(Event* event) {
    instance->wait(0,instance->getNumThreads(),event);
  }

  void TaskScheduler::destroy() 
  {
    enableThreads(-1);
    if (instance) {
      instance->destroyThreads();
      delete instance; 
      instance = NULL;
    }
  }
  
  TaskScheduler::TaskScheduler () 
    : terminateThreads(false), defaultNumThreads(true), numThreads(0), numEnabledThreads(0) {}

  void TaskScheduler::createThreads(size_t numThreads_in)
  {
    numThreads = numThreads_in;
    defaultNumThreads = false;
#if defined(__MIC__)
    if (numThreads == 0) {
      numThreads = getNumberOfLogicalThreads()-4;
      defaultNumThreads = true;
    }
#else
    if (numThreads == 0) {
      numThreads = getNumberOfLogicalThreads();
      defaultNumThreads = true;
    }
#endif
    numEnabledThreads = numThreads;

    /* generate all threads */
    for (size_t t=0; t<numThreads; t++) {
      threads.push_back(createThread((thread_func)threadFunction,new Thread(t,numThreads,this),4*1024*1024,t));
    }

    //TaskLogger::init(numThreads);
    //taskBarrier.init(numThreads);
  }

  void TaskScheduler::threadFunction(void* ptr) try 
  {
    Thread thread = *(Thread*) ptr;
    thread.scheduler->run(thread.threadIndex,thread.threadCount);
    delete (Thread*) ptr;
  }
  catch (const std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl;
    exit(1);
  }

  __thread LockStepTaskScheduler* LockStepTaskScheduler::t_scheduler = NULL;

  __dllexport LockStepTaskScheduler* LockStepTaskScheduler::instance() {
    return t_scheduler;
  }

  __dllexport void LockStepTaskScheduler::setInstance(LockStepTaskScheduler* inst) {
    t_scheduler = inst;
  }

  __thread size_t LockStepTaskScheduler::t_threadIndex = -1;

  __dllexport size_t LockStepTaskScheduler::threadIndex() {
    return t_threadIndex;
  }

  __dllexport void LockStepTaskScheduler::setThreadIndex(size_t threadIndex) {
    t_threadIndex = threadIndex;
  }

  void TaskScheduler::destroyThreads ()
  {
    terminate();
    for (size_t i=0; i<threads.size(); i++) join(threads[i]);
    threads.clear();
    terminateThreads = false;
  }
  
  void LockStepTaskScheduler::syncThreads(const size_t threadID, const size_t numThreads) {
    taskBarrier.wait(threadID,numThreads);
  }

  void LockStepTaskScheduler::syncThreadsWithReduction(const size_t threadID, 
						       const size_t numThreads,
						       void (* reductionFct)(const size_t currentThreadID,
									     const size_t childThreadID,
									     void *ptr),
						       void *ptr)
  {
    taskBarrier.syncWithReduction(threadID,numThreads,reductionFct,ptr);
  }


  __dllexport bool LockStepTaskScheduler::enter(size_t threadIndex, size_t threadCount)
  {
    if (threadIndex == 0) return false;
    dispatchTaskMainLoop(threadIndex,threadCount);
    return true;
  }

  void LockStepTaskScheduler::dispatchTaskMainLoop(const size_t threadID, const size_t numThreads)
  {
    while (true) {
      bool dispatch = dispatchTask(threadID,numThreads);
      if (dispatch == true) break;
    }  
  }

  __dllexport bool LockStepTaskScheduler::dispatchTask(const size_t threadID, size_t numThreads)
  {
    if (threadID == 0) {
      taskCounter.reset(0);
    }
    
    syncThreads(threadID, numThreads);

    if (taskPtr) {
      (*taskPtr)((void*)data,threadID,numThreads);
      syncThreads(threadID, numThreads);
      return false;
    }

    if (taskPtr2) {
      while (true) {
        size_t taskID = taskCounter.inc();
        if (taskID >= numTasks) break;
        (*taskPtr2)((void*)data,threadID,numThreads,taskID,numTasks);
      }
      syncThreads(threadID, numThreads);
      return false;
    }
    return true;
  }

  __dllexport void LockStepTaskScheduler::leave(const size_t threadID, const size_t numThreads)
  {
    assert(threadID == 0);
    releaseThreads(numThreads);
  }

  void LockStepTaskScheduler::releaseThreads(const size_t numThreads)
  {
    taskPtr = NULL;
    taskPtr2 = NULL;
    data = NULL;
    numTasks = 0;
    dispatchTask(0,numThreads);  
  }

  // ================================================================================
  // ================================================================================
  // ================================================================================

  LockStepTaskScheduler4ThreadsLocalCore::LockStepTaskScheduler4ThreadsLocalCore()
  {
    taskPtr = NULL;
    data = NULL;
    for (size_t j=0;j<2;j++)
      for (size_t i=0;i<4;i++)
	threadState[j][i] = 0;
    mode = 0;
    coreID = 0;
  }

  void LockStepTaskScheduler4ThreadsLocalCore::syncThreads(const size_t localThreadID) {
    const unsigned int m = mode;

    if (localThreadID == 0)
      {		
	__memory_barrier();
	threadState[m][localThreadID] = 1;
	__memory_barrier();

	while( (*(volatile unsigned int*)&threadState[m][0]) !=  0x01010101 )
	  __pause_cpu(WAIT_CYCLES);

	mode = 1 - mode;

	__memory_barrier();
	*(volatile unsigned int*)&threadState[m][0] = 0; 
      }
    else
      {
	__memory_barrier();
	threadState[m][localThreadID] = 1;
	__memory_barrier();
	
	while (threadState[m][localThreadID] == 1)
	  __pause_cpu(WAIT_CYCLES);
      }
 
  }


  void LockStepTaskScheduler4ThreadsLocalCore::dispatchTaskMainLoop(const size_t localThreadID, const size_t globalThreadID)
  {
    while (true) {
      bool dispatch = dispatchTask(localThreadID,globalThreadID);
      if (dispatch == true) break;
    }  
  }

  bool LockStepTaskScheduler4ThreadsLocalCore::dispatchTask(const size_t localThreadID, const size_t globalThreadID)
  {
    syncThreads(localThreadID);

    if (taskPtr == NULL) 
      return true;

    (*taskPtr)((void*)data,localThreadID,globalThreadID);
    syncThreads(localThreadID);
    
    return false;
  }

  void LockStepTaskScheduler4ThreadsLocalCore::releaseThreads(const size_t localThreadID, const size_t globalThreadID)
  {
    taskPtr = NULL;
    data = NULL;
    dispatchTask(localThreadID,globalThreadID);  
  }
}

