






#ifndef ThreadPool_h__
#define ThreadPool_h__

#include <stddef.h>
#include "mozilla/StandardInteger.h"
#include "js/Vector.h"
#include "jsalloc.h"

#ifdef JS_THREADSAFE
#  include "prtypes.h"
#  include "prlock.h"
#  include "prcvar.h"
#endif

struct JSContext;
struct JSRuntime;
struct JSCompartment;
struct JSScript;

namespace js {

class ThreadPoolWorker;

typedef void (*TaskFun)(void *userdata, size_t workerId, uintptr_t stackLimit);

class TaskExecutor
{
  public:
    virtual void executeFromWorker(size_t workerId, uintptr_t stackLimit) = 0;
};





























class ThreadPool
{
  private:
    friend class ThreadPoolWorker;

    
    JSRuntime *const runtime_;
    js::Vector<ThreadPoolWorker*, 8, SystemAllocPolicy> workers_;

    
    size_t nextId_;

    void terminateWorkers();

  public:
    ThreadPool(JSRuntime *rt);
    ~ThreadPool();

    bool init();

    
    size_t numWorkers() { return workers_.length(); }

    
    bool submitOne(TaskExecutor *executor);
    bool submitAll(TaskExecutor *executor);

    
    
    
    bool terminate();
};

} 

#endif 
