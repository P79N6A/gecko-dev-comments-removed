





#include "vm/ThreadPool.h"

#include "mozilla/Atomics.h"

#include "jslock.h"

#include "vm/ForkJoin.h"
#include "vm/Monitor.h"
#include "vm/Runtime.h"

using namespace js;

const size_t WORKER_THREAD_STACK_SIZE = 1*1024*1024;






class js::ThreadPoolBaseWorker
{
  protected:
    const uint32_t workerId_;
    ThreadPool *pool_;

  private:
    
    
    
    
    
    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> sliceBounds_;

  protected:
    static uint32_t ComposeSliceBounds(uint16_t from, uint16_t to) {
        MOZ_ASSERT(from <= to);
        return (uint32_t(from) << 16) | to;
    }

    static void DecomposeSliceBounds(uint32_t bounds, uint16_t *from, uint16_t *to) {
        *from = bounds >> 16;
        *to = bounds & uint16_t(~0);
        MOZ_ASSERT(*from <= *to);
    }

    bool hasWork() const {
        uint16_t from, to;
        DecomposeSliceBounds(sliceBounds_, &from, &to);
        return from != to;
    }

    bool popSliceFront(uint16_t *sliceId);
    bool popSliceBack(uint16_t *sliceId);
    bool stealFrom(ThreadPoolBaseWorker *victim, uint16_t *sliceId);

  public:
    ThreadPoolBaseWorker(uint32_t workerId, ThreadPool *pool)
      : workerId_(workerId),
        pool_(pool),
        sliceBounds_(0)
    { }

    void submitSlices(uint16_t sliceFrom, uint16_t sliceTo) {
        MOZ_ASSERT(!hasWork());
        MOZ_ASSERT(sliceFrom < sliceTo);
        sliceBounds_ = ComposeSliceBounds(sliceFrom, sliceTo);
    }

    void abort();
};









class js::ThreadPoolWorker : public ThreadPoolBaseWorker
{
    friend class ThreadPoolMainWorker;

    
    
    
    volatile enum WorkerState {
        CREATED, ACTIVE, TERMINATED
    } state_;

    
    static void ThreadMain(void *arg);
    void run();

    
    
    bool getSlice(uint16_t *sliceId);

  public:
    ThreadPoolWorker(uint32_t workerId, ThreadPool *pool)
      : ThreadPoolBaseWorker(workerId, pool),
        state_(CREATED)
    { }

    
    bool start();

    
    void terminate();
};






class js::ThreadPoolMainWorker : public ThreadPoolBaseWorker
{
    friend class ThreadPoolWorker;

    
    bool getSlice(uint16_t *sliceId);

  public:
    bool isActive;

    ThreadPoolMainWorker(ThreadPool *pool)
      : ThreadPoolBaseWorker(0, pool),
        isActive(false)
    { }

    
    void executeJob();
};

bool
ThreadPoolBaseWorker::popSliceFront(uint16_t *sliceId)
{
    uint32_t bounds;
    uint16_t from, to;
    do {
        bounds = sliceBounds_;
        DecomposeSliceBounds(bounds, &from, &to);
        if (from == to)
            return false;
    } while (!sliceBounds_.compareExchange(bounds, ComposeSliceBounds(from + 1, to)));

    *sliceId = from;
    pool_->pendingSlices_--;
    return true;
}

bool
ThreadPoolBaseWorker::popSliceBack(uint16_t *sliceId)
{
    uint32_t bounds;
    uint16_t from, to;
    do {
        bounds = sliceBounds_;
        DecomposeSliceBounds(bounds, &from, &to);
        if (from == to)
            return false;
    } while (!sliceBounds_.compareExchange(bounds, ComposeSliceBounds(from, to - 1)));

    *sliceId = to - 1;
    pool_->pendingSlices_--;
    return true;
}

void
ThreadPoolBaseWorker::abort()
{
    uint32_t bounds;
    uint16_t from, to;
    do {
        bounds = sliceBounds_;
        DecomposeSliceBounds(bounds, &from, &to);
    } while (!sliceBounds_.compareExchange(bounds, 0));

    pool_->pendingSlices_ -= to - from;
}

bool
ThreadPoolBaseWorker::stealFrom(ThreadPoolBaseWorker *victim, uint16_t *sliceId)
{
    
    
    if (!victim->popSliceBack(sliceId))
        return false;
#ifdef DEBUG
    pool_->stolenSlices_++;
#endif
    return true;
}

bool
ThreadPoolWorker::start()
{
#ifndef JS_THREADSAFE
    return false;
#else
    MOZ_ASSERT(state_ == CREATED);

    
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
    ThreadPoolWorker *worker = (ThreadPoolWorker*) arg;
    worker->run();
}

bool
ThreadPoolWorker::getSlice(uint16_t *sliceId)
{
    
    if (popSliceFront(sliceId))
        return true;

    
    if (!pool_->workStealing())
        return false;

    ThreadPoolBaseWorker *victim;
    do {
        if (!pool_->hasWork())
            return false;

        
        uint32_t victimId = rand() % (pool_->numWorkers() + 1);

        
        if (victimId == 0)
            victim = pool_->mainWorker_;
        else
            victim = pool_->workers_[victimId - 1];
    } while (!stealFrom(victim, sliceId));

    return true;
}

void
ThreadPoolWorker::run()
{
    
    
    
    
    uintptr_t stackLimitOffset = WORKER_THREAD_STACK_SIZE - 10*1024;
    uintptr_t stackLimit = (((uintptr_t)&stackLimitOffset) +
                             stackLimitOffset * JS_STACK_GROWTH_DIRECTION);

    for (;;) {
        
        {
            AutoLockMonitor lock(*pool_);
            while (state_ == ACTIVE && !pool_->hasWork())
                lock.wait();

            if (state_ == TERMINATED) {
                pool_->join();
                return;
            }

            pool_->activeWorkers_++;
        }

        ParallelJob *job = pool_->job();
        uint16_t sliceId;
        while (getSlice(&sliceId)) {
            if (!job->executeFromWorker(sliceId, workerId_, stackLimit)) {
                pool_->abortJob();
                break;
            }
        }

        
        {
            AutoLockMonitor lock(*pool_);
            pool_->join();
        }
    }
}

void
ThreadPoolWorker::terminate()
{
    MOZ_ASSERT(state_ != TERMINATED);
    pool_->assertIsHoldingLock();
    state_ = TERMINATED;
}

void
ThreadPoolMainWorker::executeJob()
{
    ParallelJob *job = pool_->job();
    uint16_t sliceId;
    while (getSlice(&sliceId)) {
        if (!job->executeFromMainThread(sliceId)) {
            pool_->abortJob();
            return;
        }
    }
}

bool
ThreadPoolMainWorker::getSlice(uint16_t *sliceId)
{
    
    if (popSliceFront(sliceId))
        return true;

    
    if (!pool_->workStealing())
        return false;

    
    ThreadPoolWorker *victim;
    do {
        if (!pool_->hasWork())
            return false;

        victim = pool_->workers_[rand() % pool_->numWorkers()];
    } while (!stealFrom(victim, sliceId));

    return true;
}







ThreadPool::ThreadPool(JSRuntime *rt)
  : runtime_(rt),
    mainWorker_(nullptr),
    activeWorkers_(0),
    joinBarrier_(nullptr),
    job_(nullptr),
#ifdef DEBUG
    stolenSlices_(0),
#endif
    pendingSlices_(0)
{ }

ThreadPool::~ThreadPool()
{
    terminateWorkers();
    if (joinBarrier_)
        PR_DestroyCondVar(joinBarrier_);
}

bool
ThreadPool::init()
{
#ifdef JS_THREADSAFE
    if (!Monitor::init())
        return false;
    joinBarrier_ = PR_NewCondVar(lock_);
    return !!joinBarrier_;
#else
    return true;
#endif
}

uint32_t
ThreadPool::numWorkers() const
{
    
    return runtime_->cpuCount() - 1;
}

bool
ThreadPool::workStealing() const
{
#ifdef DEBUG
    if (char *stealEnv = getenv("JS_THREADPOOL_STEAL"))
        return !!strtol(stealEnv, nullptr, 10);
#endif

    return true;
}

bool
ThreadPool::isMainThreadActive() const
{
    return mainWorker_ && mainWorker_->isActive;
}

bool
ThreadPool::lazyStartWorkers(JSContext *cx)
{
    
    
    
    
    

#ifdef JS_THREADSAFE
    if (!workers_.empty()) {
        MOZ_ASSERT(workers_.length() == numWorkers());
        return true;
    }

    
    
    
    
    for (uint32_t workerId = 0; workerId < numWorkers(); workerId++) {
        ThreadPoolWorker *worker = cx->new_<ThreadPoolWorker>(workerId, this);
        if (!worker || !workers_.append(worker)) {
            terminateWorkersAndReportOOM(cx);
            return false;
        }
    }

    for (uint32_t workerId = 0; workerId < numWorkers(); workerId++) {
        if (!workers_[workerId]->start()) {
            
            
            
            terminateWorkersAndReportOOM(cx);
            return false;
        }
    }
#endif

    return true;
}

void
ThreadPool::terminateWorkersAndReportOOM(JSContext *cx)
{
    terminateWorkers();
    MOZ_ASSERT(workers_.empty());
    js_ReportOutOfMemory(cx);
}

void
ThreadPool::terminateWorkers()
{
    if (workers_.length() > 0) {
        AutoLockMonitor lock(*this);

        
        for (uint32_t i = 0; i < workers_.length(); i++)
            workers_[i]->terminate();

        
        
        activeWorkers_ = workers_.length();
        lock.notifyAll();

        
        waitForWorkers();

        while (workers_.length() > 0)
            js_delete(workers_.popCopy());
    }

    js_delete(mainWorker_);
}

void
ThreadPool::terminate()
{
    terminateWorkers();
}

void
ThreadPool::join()
{
#ifdef JS_THREADSAFE
    assertIsHoldingLock();
    if (--activeWorkers_ == 0)
        PR_NotifyCondVar(joinBarrier_);
#endif
}

void
ThreadPool::waitForWorkers()
{
#ifdef JS_THREADSAFE
    assertIsHoldingLock();
    while (activeWorkers_ > 0) {
        mozilla::DebugOnly<PRStatus> status =
            PR_WaitCondVar(joinBarrier_, PR_INTERVAL_NO_TIMEOUT);
        MOZ_ASSERT(status == PR_SUCCESS);
    }
    job_ = nullptr;
#endif
}

ParallelResult
ThreadPool::executeJob(JSContext *cx, ParallelJob *job, uint16_t numSlices)
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(runtime_));
    MOZ_ASSERT(activeWorkers_ == 0);
    MOZ_ASSERT(!hasWork());

    
    if (!mainWorker_) {
        mainWorker_ = cx->new_<ThreadPoolMainWorker>(this);
        if (!mainWorker_) {
            terminateWorkersAndReportOOM(cx);
            return TP_FATAL;
        }
    }

    if (!lazyStartWorkers(cx))
        return TP_FATAL;

    
    uint16_t slicesPerWorker = numSlices / (numWorkers() + 1);
    uint16_t leftover = numSlices % slicesPerWorker;
    uint16_t sliceFrom = 0;
    uint16_t sliceTo = 0;
    for (uint32_t workerId = 0; workerId < numWorkers(); workerId++) {
        if (leftover > 0) {
            sliceTo += slicesPerWorker + 1;
            leftover--;
        } else {
            sliceTo += slicesPerWorker;
        }
        workers_[workerId]->submitSlices(sliceFrom, sliceTo);
        sliceFrom = sliceTo;
    }
    MOZ_ASSERT(leftover == 0);
    mainWorker_->submitSlices(sliceFrom, sliceFrom + slicesPerWorker);

    
    {
        job_ = job;
        pendingSlices_ = numSlices;
#ifdef DEBUG
        stolenSlices_ = 0;
#endif
        AutoLockMonitor lock(*this);
        lock.notifyAll();
    }

    
    mainWorker_->isActive = true;
    mainWorker_->executeJob();
    mainWorker_->isActive = false;

    
    
    {
        AutoLockMonitor lock(*this);
        waitForWorkers();
    }

    
    return TP_SUCCESS;
}

void
ThreadPool::abortJob()
{
    mainWorker_->abort();
    for (uint32_t workerId = 0; workerId < numWorkers(); workerId++)
        workers_[workerId]->abort();
}
