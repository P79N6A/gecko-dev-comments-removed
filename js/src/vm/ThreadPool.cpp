





#include "vm/ThreadPool.h"

#include "mozilla/Atomics.h"

#include "jslock.h"
#include "jsmath.h"
#include "jsnum.h" 

#include "js/Utility.h"
#include "vm/ForkJoin.h"
#include "vm/Monitor.h"
#include "vm/Runtime.h"

#ifdef JSGC_FJGENERATIONAL
#include "prmjtime.h"
#endif

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
    if (isMainThread())
        return true;

    MOZ_ASSERT(state_ == CREATED);

    
    state_ = ACTIVE;

    MOZ_ASSERT(CanUseExtraThreads());

    return PR_CreateThread(PR_USER_THREAD,
                           HelperThreadMain, this,
                           PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                           PR_UNJOINABLE_THREAD,
                           WORKER_THREAD_STACK_SIZE);
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

    
    
    FIX_FPU();

    worker->helperLoop();
}

void
ThreadPoolWorker::helperLoop()
{
    MOZ_ASSERT(!isMainThread());
    MOZ_ASSERT(CanUseExtraThreads());

    
    
    
    
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
    runtime_(rt),
#ifdef DEBUG
    stolenSlices_(0),
#endif
    pendingSlices_(0),
    isMainThreadActive_(false),
    chunkLock_(nullptr),
    timeOfLastAllocation_(0),
    freeChunks_(nullptr)
{ }

ThreadPool::~ThreadPool()
{
    terminateWorkers();
    if (chunkLock_)
        clearChunkCache();
    if (chunkLock_)
        PR_DestroyLock(chunkLock_);
    if (joinBarrier_)
        PR_DestroyCondVar(joinBarrier_);
}

bool
ThreadPool::init()
{
    if (!Monitor::init())
        return false;
    joinBarrier_ = PR_NewCondVar(lock_);
    if (!joinBarrier_)
        return false;
    chunkLock_ = PR_NewLock();
    if (!chunkLock_)
        return false;
    return true;
}

uint32_t
ThreadPool::numWorkers() const
{
    return HelperThreadState().cpuCount;
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
ThreadPool::lazyStartWorkers(JSContext *cx)
{
    
    
    
    
    

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












gc::ForkJoinNurseryChunk *
ThreadPool::getChunk()
{
#ifdef JSGC_FJGENERATIONAL
    PR_Lock(chunkLock_);
    timeOfLastAllocation_ = PRMJ_Now()/1000000;
    ChunkFreeList *p = freeChunks_;
    if (p)
        freeChunks_ = p->next;
    PR_Unlock(chunkLock_);

    if (p) {
        
        return reinterpret_cast<gc::ForkJoinNurseryChunk *>(p);
    }
    gc::ForkJoinNurseryChunk *c =
        reinterpret_cast<gc::ForkJoinNurseryChunk *>(
            gc::MapAlignedPages(gc::ChunkSize, gc::ChunkSize));
    if (!c)
        return c;
    poisonChunk(c);
    return c;
#else
    return nullptr;
#endif
}

void
ThreadPool::putFreeChunk(gc::ForkJoinNurseryChunk *c)
{
#ifdef JSGC_FJGENERATIONAL
    poisonChunk(c);

    PR_Lock(chunkLock_);
    ChunkFreeList *p = reinterpret_cast<ChunkFreeList *>(c);
    p->next = freeChunks_;
    freeChunks_ = p;
    PR_Unlock(chunkLock_);
#endif
}

void
ThreadPool::poisonChunk(gc::ForkJoinNurseryChunk *c)
{
#ifdef JSGC_FJGENERATIONAL
#ifdef DEBUG
    memset(c, JS_POISONED_FORKJOIN_CHUNK, gc::ChunkSize);
#endif
    c->trailer.runtime = nullptr;
#endif
}

void
ThreadPool::pruneChunkCache()
{
#ifdef JSGC_FJGENERATIONAL
    if (PRMJ_Now()/1000000 - timeOfLastAllocation_ >= secondsBeforePrune)
        clearChunkCache();
#endif
}

void
ThreadPool::clearChunkCache()
{
#ifdef JSGC_FJGENERATIONAL
    PR_Lock(chunkLock_);
    ChunkFreeList *p = freeChunks_;
    freeChunks_ = nullptr;
    PR_Unlock(chunkLock_);

    while (p) {
        ChunkFreeList *victim = p;
        p = p->next;
        gc::UnmapPages(victim, gc::ChunkSize);
    }
#endif
}
