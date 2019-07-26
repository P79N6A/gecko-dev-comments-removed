





#include "jscntxt.h"
#include "jslock.h"

#include "vm/Monitor.h"
#include "vm/ThreadPool.h"

#ifdef JS_THREADSAFE
#  include "prthread.h"
#endif

using namespace js;









const size_t WORKER_THREAD_STACK_SIZE = 1*1024*1024;

class js::ThreadPoolWorker : public Monitor
{
    const size_t workerId_;

    
    
    
    enum WorkerState {
        CREATED, ACTIVE, TERMINATING, TERMINATED
    } state_;

    
    
    
    js::Vector<TaskExecutor*, 4, SystemAllocPolicy> worklist_;

    
    static void ThreadMain(void *arg);
    void run();

  public:
    ThreadPoolWorker(size_t workerId);
    ~ThreadPoolWorker();

    bool init();

    
    bool start();

    
    
    
    bool submit(TaskExecutor *task);

    
    
    void terminate();
};

ThreadPoolWorker::ThreadPoolWorker(size_t workerId)
  : workerId_(workerId),
    state_(CREATED),
    worklist_()
{ }

ThreadPoolWorker::~ThreadPoolWorker()
{ }

bool
ThreadPoolWorker::init()
{
    return Monitor::init();
}

bool
ThreadPoolWorker::start()
{
#ifndef JS_THREADSAFE
    return false;
#else
    JS_ASSERT(state_ == CREATED);

    
    state_ = ACTIVE;

    if (!PR_CreateThread(PR_USER_THREAD,
                         ThreadMain, this,
                         PR_PRIORITY_NORMAL, PR_LOCAL_THREAD,
                         PR_UNJOINABLE_THREAD,
                         WORKER_THREAD_STACK_SIZE))
    {
        
        state_ = TERMINATED;
        return false;
    }

    return true;
#endif
}

void
ThreadPoolWorker::ThreadMain(void *arg)
{
    ThreadPoolWorker *thread = (ThreadPoolWorker*) arg;
    thread->run();
}

void
ThreadPoolWorker::run()
{
    
    
    
    
    uintptr_t stackLimitOffset = WORKER_THREAD_STACK_SIZE - 10*1024;
    uintptr_t stackLimit = (((uintptr_t)&stackLimitOffset) +
                             stackLimitOffset * JS_STACK_GROWTH_DIRECTION);

    AutoLockMonitor lock(*this);

    for (;;) {
        while (!worklist_.empty()) {
            TaskExecutor *task = worklist_.popCopy();
            {
                
                
                AutoUnlockMonitor unlock(*this);
                task->executeFromWorker(workerId_, stackLimit);
            }
        }

        if (state_ == TERMINATING)
            break;

        JS_ASSERT(state_ == ACTIVE);

        lock.wait();
    }

    JS_ASSERT(worklist_.empty() && state_ == TERMINATING);
    state_ = TERMINATED;
    lock.notify();
}

bool
ThreadPoolWorker::submit(TaskExecutor *task)
{
    AutoLockMonitor lock(*this);
    JS_ASSERT(state_ == ACTIVE);
    if (!worklist_.append(task))
        return false;
    lock.notify();
    return true;
}

void
ThreadPoolWorker::terminate()
{
    AutoLockMonitor lock(*this);

    if (state_ == CREATED) {
        state_ = TERMINATED;
        return;
    } else if (state_ == ACTIVE) {
        state_ = TERMINATING;
        lock.notify();
        while (state_ != TERMINATED)
            lock.wait();
    } else {
        JS_ASSERT(state_ == TERMINATED);
    }
}







ThreadPool::ThreadPool(JSRuntime *rt)
  : runtime_(rt),
    numWorkers_(0), 
    nextId_(0)
{
}

bool
ThreadPool::init()
{
    
    
    
    

#ifdef JS_THREADSAFE
    if (runtime_->useHelperThreads())
        numWorkers_ = GetCPUCount() - 1;
    else
        numWorkers_ = 0;

# ifdef DEBUG
    if (char *jsthreads = getenv("JS_THREADPOOL_SIZE"))
        numWorkers_ = strtol(jsthreads, NULL, 10);
# endif
#endif

    return true;
}

ThreadPool::~ThreadPool()
{
    terminateWorkers();
}

bool
ThreadPool::lazyStartWorkers(JSContext *cx)
{
    
    
    
    
    

#ifndef JS_THREADSAFE
    return true;
#else
    if (!workers_.empty()) {
        JS_ASSERT(workers_.length() == numWorkers());
        return true;
    }

    
    
    
    
    for (size_t workerId = 0; workerId < numWorkers(); workerId++) {
        ThreadPoolWorker *worker = js_new<ThreadPoolWorker>(workerId);
        if (!worker) {
            terminateWorkersAndReportOOM(cx);
            return false;
        }
        if (!worker->init() || !workers_.append(worker)) {
            js_delete(worker);
            terminateWorkersAndReportOOM(cx);
            return false;
        }
        if (!worker->start()) {
            
            
            
            terminateWorkersAndReportOOM(cx);
            return false;
        }
    }

    return true;
#endif
}

void
ThreadPool::terminateWorkersAndReportOOM(JSContext *cx)
{
    terminateWorkers();
    JS_ASSERT(workers_.empty());
    JS_ReportOutOfMemory(cx);
}

void
ThreadPool::terminateWorkers()
{
    while (workers_.length() > 0) {
        ThreadPoolWorker *worker = workers_.popCopy();
        worker->terminate();
        js_delete(worker);
    }
}

bool
ThreadPool::submitOne(JSContext *cx, TaskExecutor *executor)
{
    JS_ASSERT(numWorkers() > 0);

    runtime_->assertValidThread();

    if (!lazyStartWorkers(cx))
        return false;

    
    size_t id = JS_ATOMIC_INCREMENT(&nextId_) % numWorkers();
    return workers_[id]->submit(executor);
}

bool
ThreadPool::submitAll(JSContext *cx, TaskExecutor *executor)
{
    runtime_->assertValidThread();

    if (!lazyStartWorkers(cx))
        return false;

    for (size_t id = 0; id < numWorkers(); id++) {
        if (!workers_[id]->submit(executor))
            return false;
    }
    return true;
}

bool
ThreadPool::terminate()
{
    terminateWorkers();
    return true;
}
