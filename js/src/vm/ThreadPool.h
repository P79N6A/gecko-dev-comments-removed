





#ifndef vm_ThreadPool_h
#define vm_ThreadPool_h

#include <stddef.h>
#include <stdint.h>

#include "jsalloc.h"
#include "jspubtd.h"
#ifdef JS_THREADSAFE
# include "prcvar.h"
# include "prlock.h"
# include "prtypes.h"
#endif

#include "js/Vector.h"

struct JSRuntime;
struct JSCompartment;

namespace js {

class ThreadPoolWorker;

typedef void (*TaskFun)(void *userdata, uint32_t workerId, uintptr_t stackLimit);

class TaskExecutor
{
  public:
    virtual void executeFromWorker(uint32_t workerId, uintptr_t stackLimit) = 0;
};






















class ThreadPool
{
  private:
    friend class ThreadPoolWorker;

    
#if defined(JS_THREADSAFE) || defined(DEBUG)
    JSRuntime *const runtime_;
#endif
    js::Vector<ThreadPoolWorker*, 8, SystemAllocPolicy> workers_;

    
    size_t numWorkers_;

    bool lazyStartWorkers(JSContext *cx);
    void terminateWorkers();
    void terminateWorkersAndReportOOM(JSContext *cx);

  public:
    ThreadPool(JSRuntime *rt);
    ~ThreadPool();

    bool init();

    
    size_t numWorkers() { return numWorkers_; }

    
    bool submitAll(JSContext *cx, TaskExecutor *executor);

    
    
    
    bool terminate();
};

} 

#endif 
