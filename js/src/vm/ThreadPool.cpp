





#include "vm/ThreadPool.h"

#include "mozilla/Atomics.h"

#include "jslock.h"

#include "vm/ForkJoin.h"
#include "vm/Monitor.h"
#include "vm/Runtime.h"

using namespace js;

const size_t WORKER_THREAD_STACK_SIZE = 1*1024*1024;

static inline uint32_t
ComposeSliceBounds(uint16_t from, uint16_t to)
{
    MOZ_ASSERT(from <= to);
    return (uint32_t(from) << 16) | to;
}

static inline void
DecomposeSliceBounds(uint32_t bounds, uint16_t *from, uint16_t *to)
{
    *from = bounds >> 16;
    *to = bounds & uint16_t(~0);
    MOZ_ASSERT(*from <= *to);
}

ThreadPoolWorker::ThreadPoolWorker(uint32_t workerId, uint32_t rngSeed, ThreadPool *pool)
  : workerId_(workerId),
    pool_(pool),
    sliceBounds_(0),
    state_(CREATED),
    schedulerRNGState_(rngSeed)
{ }

bool
ThreadPoolWorker::hasWork() const
{
    uint16_t from, to;
    DecomposeSliceBounds(sliceBounds_, &from, &to);
    return from != to;
}

bool
ThreadPoolWorker::popSliceFront(uint16_t *sliceId)
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
ThreadPoolWorker::popSliceBack(uint16_t *sliceId)
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
ThreadPoolWorker::discardSlices()
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
ThreadPoolWorker::stealFrom(ThreadPoolWorker *victim, uint16_t *sliceId)
{
    
    
    if (!victim->popSliceBack(sliceId))
        return false;
#ifdef DEBUG
    pool_->stolenSlices_++;
#endif
    return true;
}

ThreadPoolWorker *
ThreadPoolWorker::randomWorker()
{
    
    uint32_t x = schedulerRNGState_;
    x ^= x << XORSHIFT_A;
    x ^= x >> XORSHIFT_B;
    x ^= x << XORSHIFT_C;
    schedulerRNGState_ = x;
    return pool_->workers_[x % pool_->numWorkers()];
}

bool
ThreadPoolWorker::start()
{
#ifndef JS_THREADSAFE
    return false;
#else
    if (isMainThread())
        return true;

    MOZ_ASSERT(state_ == CREATED);

    
    state_ = ACTIVE;

    if (!PR_CreateThread(PR_USER_THREAD,
                         HelperThreadMain, this,
                         PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                         PR_UNJOINABLE_THREAD,
                         WORKER_THREAD_STACK_SIZE))
    {
        
        state_ = TERMINATED;
        return false;
    }

    return true;
#endif
}

#ifdef MOZ_NUWA_PROCESS
extern "C" {
MFBT_API bool IsNuwaProcess();
MFBT_API void NuwaMarkCurrentThread(void (*recreate)(void *), void *arg);
}
#endif

void
ThreadPoolWorker::HelperThreadMain(void *arg)
{
    ThreadPoolWorker *worker = (ThreadPoolWorker*) arg;

#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
        JS_ASSERT(NuwaMarkCurrentThread != nullptr);
        NuwaMarkCurrentThread(nullptr, nullptr);
    }
#endif

    worker->helperLoop();
}

void
ThreadPoolWorker::helperLoop()
{
    MOZ_ASSERT(!isMainThread());

    
    
    
    
    uintptr_t stackLimitOffset = WORKER_THREAD_STACK_SIZE - 100*1024;
    uintptr_t stackLimit = (((uintptr_t)&stackLimitOffset) +
                             stackLimitOffset * JS_STACK_GROWTH_DIRECTION);


    for (;;) {
        
        {
            AutoLockMonitor lock(*pool_);
            while (state_ == ACTIVE && !pool_->hasWork())
                lock.wait();

            if (state_ == TERMINATED) {
                pool_->join(lock);
                return;
            }

            pool_->activeWorkers_++;
        }

        if (!pool_->job()->executeFromWorker(this, stackLimit))
            pool_->abortJob();

        
        {
            AutoLockMonitor lock(*pool_);
            pool_->join(lock);
        }
    }
}

void
ThreadPoolWorker::submitSlices(uint16_t sliceStart, uint16_t sliceEnd)
{
    MOZ_ASSERT(!hasWork());
    sliceBounds_ = ComposeSliceBounds(sliceStart, sliceEnd);
}

bool
ThreadPoolWorker::getSlice(ForkJoinContext *cx, uint16_t *sliceId)
{
    
    if (popSliceFront(sliceId))
        return true;

    
    if (!pool_->workStealing())
        return false;

    do {
        if (!pool_->hasWork())
            return false;
    } while (!stealFrom(randomWorker(), sliceId));

    return true;
}

void
ThreadPoolWorker::terminate(AutoLockMonitor &lock)
{
    MOZ_ASSERT(lock.isFor(*pool_));
    MOZ_ASSERT(state_ != TERMINATED);
    state_ = TERMINATED;
}







ThreadPool::ThreadPool(JSRuntime *rt)
  : activeWorkers_(0),
    joinBarrier_(nullptr),
    job_(nullptr),
#ifdef DEBUG
    runtime_(rt),
    stolenSlices_(0),
#endif
    pendingSlices_(0),
    isMainThreadActive_(false)
{ }

ThreadPool::~ThreadPool()
{
    terminateWorkers();
#ifdef JS_THREADSAFE
    if (joinBarrier_)
        PR_DestroyCondVar(joinBarrier_);
#endif
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
#ifdef JS_THREADSAFE
    return WorkerThreadState().cpuCount;
#else
    return 1;
#endif
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

extern uint64_t random_next(uint64_t *, int);

bool
ThreadPool::lazyStartWorkers(JSContext *cx)
{
    
    
    
    
    

#ifdef JS_THREADSAFE
    if (!workers_.empty()) {
        MOZ_ASSERT(workers_.length() == numWorkers());
        return true;
    }

    
    
    
    
    uint64_t rngState = 0;
    for (uint32_t workerId = 0; workerId < numWorkers(); workerId++) {
        uint32_t rngSeed = uint32_t(random_next(&rngState, 32));
        ThreadPoolWorker *worker = cx->new_<ThreadPoolWorker>(workerId, rngSeed, this);
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
            workers_[i]->terminate(lock);

        
        
        activeWorkers_ = workers_.length() - 1;
        lock.notifyAll();

        
        waitForWorkers(lock);

        while (workers_.length() > 0)
            js_delete(workers_.popCopy());
    }
}

void
ThreadPool::terminate()
{
    terminateWorkers();
}

void
ThreadPool::join(AutoLockMonitor &lock)
{
    MOZ_ASSERT(lock.isFor(*this));
    if (--activeWorkers_ == 0)
        lock.notify(joinBarrier_);
}

void
ThreadPool::waitForWorkers(AutoLockMonitor &lock)
{
    MOZ_ASSERT(lock.isFor(*this));
    while (activeWorkers_ > 0)
        lock.wait(joinBarrier_);
    job_ = nullptr;
}

ParallelResult
ThreadPool::executeJob(JSContext *cx, ParallelJob *job, uint16_t sliceStart, uint16_t sliceMax)
{
    MOZ_ASSERT(sliceStart < sliceMax);
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(runtime_));
    MOZ_ASSERT(activeWorkers_ == 0);
    MOZ_ASSERT(!hasWork());

    if (!lazyStartWorkers(cx))
        return TP_FATAL;

    
    uint16_t numSlices = sliceMax - sliceStart;
    uint16_t slicesPerWorker = numSlices / numWorkers();
    uint16_t leftover = numSlices % numWorkers();
    uint16_t sliceEnd = sliceStart;
    for (uint32_t workerId = 0; workerId < numWorkers(); workerId++) {
        if (leftover > 0) {
            sliceEnd += slicesPerWorker + 1;
            leftover--;
        } else {
            sliceEnd += slicesPerWorker;
        }
        workers_[workerId]->submitSlices(sliceStart, sliceEnd);
        sliceStart = sliceEnd;
    }
    MOZ_ASSERT(leftover == 0);

    
    {
        job_ = job;
        pendingSlices_ = numSlices;
#ifdef DEBUG
        stolenSlices_ = 0;
#endif
        AutoLockMonitor lock(*this);
        lock.notifyAll();
    }

    
    isMainThreadActive_ = true;
    if (!job->executeFromMainThread(mainThreadWorker()))
        abortJob();
    isMainThreadActive_ = false;

    
    
    {
        AutoLockMonitor lock(*this);
        waitForWorkers(lock);
    }

    
    
    MOZ_ASSERT(!hasWork(), "User function did not process all the slices!");

    
    return TP_SUCCESS;
}

void
ThreadPool::abortJob()
{
    for (uint32_t workerId = 0; workerId < numWorkers(); workerId++)
        workers_[workerId]->discardSlices();

    
    
    
    
    
    
    
    
    while (hasWork());
}
