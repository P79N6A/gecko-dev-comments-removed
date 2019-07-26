






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
class JSScript;

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

    
    JSRuntime *const runtime_;
    js::Vector<ThreadPoolWorker*, 8, SystemAllocPolicy> workers_;

    
    size_t numWorkers_;

    
    uint32_t nextId_;

    bool lazyStartWorkers(JSContext *cx);
    void terminateWorkers();
    void terminateWorkersAndReportOOM(JSContext *cx);

  public:
    ThreadPool(JSRuntime *rt);
    ~ThreadPool();

    bool init();

    
    size_t numWorkers() { return numWorkers_; }

    
    bool submitOne(JSContext *cx, TaskExecutor *executor);
    bool submitAll(JSContext *cx, TaskExecutor *executor);

    
    
    
    bool terminate();
};

} 

#endif 
