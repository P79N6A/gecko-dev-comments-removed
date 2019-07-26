





#ifndef vm_ThreadPool_h
#define vm_ThreadPool_h

#include <stddef.h>
#include <stdint.h>

#include "jsalloc.h"
#include "jslock.h"
#include "jspubtd.h"

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

    
    JSRuntime *const runtime_;
    js::Vector<ThreadPoolWorker*, 8, SystemAllocPolicy> workers_;

    bool lazyStartWorkers(JSContext *cx);
    void terminateWorkers();
    void terminateWorkersAndReportOOM(JSContext *cx);

  public:
    ThreadPool(JSRuntime *rt);
    ~ThreadPool();

    
    size_t numWorkers() const;

    
    bool submitAll(JSContext *cx, TaskExecutor *executor);

    
    
    
    bool terminate();
};

} 

#endif 
