





#include "jscntxt.h"
#include "jslock.h"
#include "vm/threadpool.h"
#include "monitor.h"

#ifdef JS_THREADSAFE
#  include "prthread.h"
#endif

namespace js {










#define WORKER_THREAD_STACK_SIZE (1*1024*1024)

enum WorkerState {
    CREATED, ACTIVE, TERMINATING, TERMINATED
};

class ThreadPoolWorker : public Monitor
{
    const size_t workerId_;
    ThreadPool *const threadPool_;

    


    WorkerState state_;

    


    js::Vector<TaskExecutor*, 4, SystemAllocPolicy> worklist_;

    
    static void ThreadMain(void *arg);
    void run();

public:
    ThreadPoolWorker(size_t workerId, ThreadPool *tp);
    ~ThreadPoolWorker();

    bool init();

    
    bool start();

    


    bool submit(TaskExecutor *task);

    

    void terminate();
};

ThreadPoolWorker::ThreadPoolWorker(size_t workerId, ThreadPool *tp)
    : workerId_(workerId), threadPool_(tp), state_(CREATED), worklist_()
{}

ThreadPoolWorker::~ThreadPoolWorker()
{}

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
    
    
    
    uintptr_t stackLimitOffset = WORKER_THREAD_STACK_SIZE - 2*1024;
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
        while (state_ != TERMINATED) {
            lock.wait();
        }
    } else {
        JS_ASSERT(state_ == TERMINATED);
    }
}








ThreadPool::ThreadPool(JSRuntime *rt)
    : runtime_(rt),
      nextId_(0)
{
}

ThreadPool::~ThreadPool() {
    terminateWorkers();
    while (workers_.length() > 0) {
        ThreadPoolWorker *worker = workers_.popCopy();
        js_delete(worker);
    }
}

bool
ThreadPool::init()
{
#ifdef JS_THREADSAFE
    
    size_t numWorkers = 0;
    char *pathreads = getenv("PATHREADS");
    if (pathreads != NULL) {
        numWorkers = strtol(pathreads, NULL, 10);
    } else {
        numWorkers = GetCPUCount() - 1;
    }

    
    
    
    for (size_t workerId = 0; workerId < numWorkers; workerId++) {
        ThreadPoolWorker *worker = js_new<ThreadPoolWorker>(workerId, this);
        if (!worker->init()) {
            js_delete(worker);
            return false;
        }
        if (!workers_.append(worker)) {
            js_delete(worker);
            return false;
        }
        if (!worker->start()) {
            return false;
        }
    }
#endif

    return true;
}

void
ThreadPool::terminateWorkers()
{
    for (size_t i = 0; i < workers_.length(); i++) {
        workers_[i]->terminate();
    }
}

bool
ThreadPool::submitOne(TaskExecutor *executor) {
    runtime_->assertValidThread();

    if (numWorkers() == 0)
        return false;

    
    size_t id = JS_ATOMIC_INCREMENT(&nextId_) % workers_.length();
    return workers_[id]->submit(executor);
}

bool
ThreadPool::submitAll(TaskExecutor *executor) {
    for (size_t id = 0; id < workers_.length(); id++) {
        if (!workers_[id]->submit(executor))
            return false;
    }
    return true;
}

bool
ThreadPool::terminate() {
    terminateWorkers();
    return true;
}

}
