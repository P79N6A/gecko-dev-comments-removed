






#ifndef jsthreadpool_h___
#define jsthreadpool_h___

#if defined(JS_THREADSAFE) && defined(JS_ION)
# define JS_THREADSAFE_ION
#endif

#include <stddef.h>
#include "mozilla/StandardInteger.h"
#include "prtypes.h"
#include "js/Vector.h"
#include "jsalloc.h"
#include "prlock.h"
#include "prcvar.h"

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
