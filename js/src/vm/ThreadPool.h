





#ifndef vm_ThreadPool_h
#define vm_ThreadPool_h

#include "mozilla/Atomics.h"

#include "jsalloc.h"
#include "jslock.h"
#include "jsmath.h"
#include "jspubtd.h"

#include "js/Vector.h"
#include "vm/Monitor.h"

struct JSRuntime;
struct JSCompartment;

namespace js {

class ThreadPool;








class ThreadPoolWorker
{
    const uint32_t workerId_;
    ThreadPool *pool_;

    
    
    
    
    
    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> sliceBounds_;

    
    volatile enum WorkerState {
        CREATED, ACTIVE, TERMINATED
    } state_;

    
    
    uint32_t schedulerRNGState_;

    
    static void HelperThreadMain(void *arg);
    void helperLoop();

    bool hasWork() const;
    bool popSliceFront(uint16_t *sliceId);
    bool popSliceBack(uint16_t *sliceId);
    bool stealFrom(ThreadPoolWorker *victim, uint16_t *sliceId);

    
    
    
    
    
  public:
    static const uint32_t XORSHIFT_A = 11;
    static const uint32_t XORSHIFT_B = 21;
    static const uint32_t XORSHIFT_C = 13;

  private:
    ThreadPoolWorker *randomWorker();

  public:
    ThreadPoolWorker(uint32_t workerId, uint32_t rngSeed, ThreadPool *pool);

    uint32_t id() const { return workerId_; }
    bool isMainThread() const { return id() == 0; }

    
    void submitSlices(uint16_t sliceStart, uint16_t sliceEnd);

    
    
    bool getSlice(ForkJoinContext *cx, uint16_t *sliceId);

    
    void discardSlices();

    
    bool start();

    
    void terminate(AutoLockMonitor &lock);

    static size_t offsetOfSliceBounds() {
        return offsetof(ThreadPoolWorker, sliceBounds_);
    }

    static size_t offsetOfSchedulerRNGState() {
        return offsetof(ThreadPoolWorker, schedulerRNGState_);
    }
};








class ParallelJob
{
  public:
    virtual bool executeFromWorker(ThreadPoolWorker *worker, uintptr_t stackLimit) = 0;
    virtual bool executeFromMainThread(ThreadPoolWorker *mainWorker) = 0;
};









































class ThreadPool : public Monitor
{
  private:
    friend class ThreadPoolWorker;

    
    js::Vector<ThreadPoolWorker *, 8, SystemAllocPolicy> workers_;

    
    uint32_t activeWorkers_;
    PRCondVar *joinBarrier_;

    
    ParallelJob *job_;

#ifdef DEBUG
    
    JSRuntime *const runtime_;

    
    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> stolenSlices_;
#endif

    
    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> pendingSlices_;

    
    bool isMainThreadActive_;

    bool lazyStartWorkers(JSContext *cx);
    void terminateWorkers();
    void terminateWorkersAndReportOOM(JSContext *cx);
    void join(AutoLockMonitor &lock);
    void waitForWorkers(AutoLockMonitor &lock);
    ThreadPoolWorker *mainThreadWorker() { return workers_[0]; }

  public:
#ifdef DEBUG
    static size_t offsetOfStolenSlices() {
        return offsetof(ThreadPool, stolenSlices_);
    }
#endif
    static size_t offsetOfPendingSlices() {
        return offsetof(ThreadPool, pendingSlices_);
    }
    static size_t offsetOfWorkers() {
        return offsetof(ThreadPool, workers_);
    }

    static const uint16_t MAX_SLICE_ID = UINT16_MAX;

    explicit ThreadPool(JSRuntime *rt);
    ~ThreadPool();

    bool init();

    
    uint32_t numWorkers() const;

    
    bool hasWork() const { return pendingSlices_ != 0; }

    
    ParallelJob *job() const {
        MOZ_ASSERT(job_);
        return job_;
    }

    
    bool workStealing() const;

    
    bool isMainThreadActive() const { return isMainThreadActive_; }

#ifdef DEBUG
    
    uint16_t stolenSlices() { return stolenSlices_; }
#endif

    
    
    
    void terminate();

    
    
    ParallelResult executeJob(JSContext *cx, ParallelJob *job, uint16_t sliceStart,
                              uint16_t numSlices);

    
    void abortJob();
};

} 

#endif 
