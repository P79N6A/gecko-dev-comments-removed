





#ifndef vm_ThreadPool_h
#define vm_ThreadPool_h

#include "mozilla/Atomics.h"

#include "jsalloc.h"
#include "jslock.h"
#include "jspubtd.h"

#include "js/Vector.h"
#include "vm/Monitor.h"

struct JSRuntime;
struct JSCompartment;

namespace js {

class ThreadPoolBaseWorker;
class ThreadPoolWorker;
class ThreadPoolMainWorker;






class ParallelJob
{
  public:
    virtual bool executeFromWorker(uint32_t workerId, uintptr_t stackLimit) = 0;
    virtual bool executeFromMainThread() = 0;
};








































class ThreadPool : public Monitor
{
  private:
    friend class ThreadPoolBaseWorker;
    friend class ThreadPoolWorker;
    friend class ThreadPoolMainWorker;

    
    JSRuntime *const runtime_;

    
    
    js::Vector<ThreadPoolWorker *, 8, SystemAllocPolicy> workers_;
    ThreadPoolMainWorker *mainWorker_;

    
    uint32_t activeWorkers_;
    PRCondVar *joinBarrier_;

    
    ParallelJob *job_;

#ifdef DEBUG
    
    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> stolenSlices_;
#endif

    
    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> pendingSlices_;

    bool lazyStartWorkers(JSContext *cx);
    void terminateWorkers();
    void terminateWorkersAndReportOOM(JSContext *cx);
    void join(AutoLockMonitor &lock);
    void waitForWorkers(AutoLockMonitor &lock);

  public:
    ThreadPool(JSRuntime *rt);
    ~ThreadPool();

    bool init();

    
    uint32_t numWorkers() const;

    
    bool hasWork() const { return pendingSlices_ != 0; }

    
    ParallelJob *job() const {
        MOZ_ASSERT(job_);
        return job_;
    }

    
    bool workStealing() const;

    
    bool isMainThreadActive() const;

#ifdef DEBUG
    
    uint16_t stolenSlices() { return stolenSlices_; }
#endif

    
    
    
    void terminate();

    
    
    ParallelResult executeJob(JSContext *cx, ParallelJob *job, uint16_t sliceStart,
                              uint16_t numSlices);

    
    
    bool getSliceForWorker(uint32_t workerId, uint16_t *sliceId);
    bool getSliceForMainThread(uint16_t *sliceId);

    
    void abortJob();
};

} 

#endif 
