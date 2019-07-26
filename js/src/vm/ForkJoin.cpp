






#include "jscntxt.h"
#include "jscompartment.h"

#include "vm/ForkJoin.h"
#include "vm/Monitor.h"
#include "gc/Marking.h"

#include "jsinferinlines.h"

#ifdef JS_THREADSAFE
#  include "prthread.h"
#endif

using namespace js;

#if defined(JS_THREADSAFE) && defined(JS_ION)

unsigned ForkJoinSlice::ThreadPrivateIndex;
bool ForkJoinSlice::TLSInitialized;

class js::ForkJoinShared : public TaskExecutor, public Monitor
{
    
    

    JSContext *const cx_;          
    ThreadPool *const threadPool_; 
    ForkJoinOp &op_;               
    const uint32_t numSlices_;     
    PRCondVar *rendezvousEnd_;     

    
    
    
    

    Vector<Allocator *, 16> allocators_;

    
    
    
    

    uint32_t uncompleted_;         
    uint32_t blocked_;             
    uint32_t rendezvousIndex_;     
    bool gcRequested_;             
    gcreason::Reason gcReason_;    
    Zone *gcZone_;                 

    
    
    
    
    

    
    volatile bool abort_;

    
    volatile bool fatal_;

    
    volatile bool rendezvous_;

    
    void executeFromMainThread();

    
    
    void executePortion(PerThreadData *perThread, uint32_t threadId);

    
    
    
    

    friend class AutoRendezvous;

    
    
    void initiateRendezvous(ForkJoinSlice &threadCx);

    
    
    void joinRendezvous(ForkJoinSlice &threadCx);

    
    
    void endRendezvous(ForkJoinSlice &threadCx);

  public:
    ForkJoinShared(JSContext *cx,
                   ThreadPool *threadPool,
                   ForkJoinOp &op,
                   uint32_t numSlices,
                   uint32_t uncompleted);
    ~ForkJoinShared();

    bool init();

    ParallelResult execute();

    
    virtual void executeFromWorker(uint32_t threadId, uintptr_t stackLimit);

    
    
    
    
    void transferArenasToCompartmentAndProcessGCRequests();

    
    bool check(ForkJoinSlice &threadCx);

    
    void requestGC(gcreason::Reason reason);
    void requestZoneGC(JS::Zone *zone, gcreason::Reason reason);

    
    void setAbortFlag(bool fatal);

    JSRuntime *runtime() { return cx_->runtime; }
};

class js::AutoRendezvous
{
  private:
    ForkJoinSlice &threadCx;

  public:
    AutoRendezvous(ForkJoinSlice &threadCx)
        : threadCx(threadCx)
    {
        threadCx.shared->initiateRendezvous(threadCx);
    }

    ~AutoRendezvous() {
        threadCx.shared->endRendezvous(threadCx);
    }
};

class js::AutoSetForkJoinSlice
{
  public:
    AutoSetForkJoinSlice(ForkJoinSlice *threadCx) {
        PR_SetThreadPrivate(ForkJoinSlice::ThreadPrivateIndex, threadCx);
    }

    ~AutoSetForkJoinSlice() {
        PR_SetThreadPrivate(ForkJoinSlice::ThreadPrivateIndex, NULL);
    }
};





ForkJoinShared::ForkJoinShared(JSContext *cx,
                               ThreadPool *threadPool,
                               ForkJoinOp &op,
                               uint32_t numSlices,
                               uint32_t uncompleted)
  : cx_(cx),
    threadPool_(threadPool),
    op_(op),
    numSlices_(numSlices),
    rendezvousEnd_(NULL),
    allocators_(cx),
    uncompleted_(uncompleted),
    blocked_(0),
    rendezvousIndex_(0),
    gcRequested_(false),
    gcReason_(gcreason::NUM_REASONS),
    gcZone_(NULL),
    abort_(false),
    fatal_(false),
    rendezvous_(false)
{ }

bool
ForkJoinShared::init()
{
    
    
    
    
    
    
    
    
    

    if (!Monitor::init())
        return false;

    rendezvousEnd_ = PR_NewCondVar(lock_);
    if (!rendezvousEnd_)
        return false;

    for (unsigned i = 0; i < numSlices_; i++) {
        Allocator *allocator = cx_->runtime->new_<Allocator>(cx_->zone());
        if (!allocator)
            return false;

        if (!allocators_.append(allocator)) {
            js_delete(allocator);
            return false;
        }
    }

    return true;
}

ForkJoinShared::~ForkJoinShared()
{
    if (rendezvousEnd_)
        PR_DestroyCondVar(rendezvousEnd_);

    while (allocators_.length() > 0)
        js_delete(allocators_.popCopy());
}

ParallelResult
ForkJoinShared::execute()
{
    
    
    
    if (cx_->runtime->interrupt)
        return TP_RETRY_SEQUENTIALLY;

    AutoLockMonitor lock(*this);

    
    {
        AutoUnlockMonitor unlock(*this);
        if (!threadPool_->submitAll(cx_, this))
            return TP_FATAL;
        executeFromMainThread();
    }

    
    while (uncompleted_ > 0)
        lock.wait();

    transferArenasToCompartmentAndProcessGCRequests();

    
    if (abort_) {
        if (fatal_)
            return TP_FATAL;
        else
            return TP_RETRY_SEQUENTIALLY;
    }

    
    return TP_SUCCESS;
}

void
ForkJoinShared::transferArenasToCompartmentAndProcessGCRequests()
{
    JSCompartment *comp = cx_->compartment;
    for (unsigned i = 0; i < numSlices_; i++)
        comp->adoptWorkerAllocator(allocators_[i]);

    if (gcRequested_) {
        if (!gcZone_)
            TriggerGC(cx_->runtime, gcReason_);
        else
            TriggerZoneGC(gcZone_, gcReason_);
        gcRequested_ = false;
        gcZone_ = NULL;
    }
}

void
ForkJoinShared::executeFromWorker(uint32_t workerId, uintptr_t stackLimit)
{
    JS_ASSERT(workerId < numSlices_ - 1);

    PerThreadData thisThread(cx_->runtime);
    TlsPerThreadData.set(&thisThread);
    thisThread.ionStackLimit = stackLimit;
    executePortion(&thisThread, workerId);
    TlsPerThreadData.set(NULL);

    AutoLockMonitor lock(*this);
    uncompleted_ -= 1;
    if (blocked_ == uncompleted_) {
        
        
        
        lock.notify();
    }
}

void
ForkJoinShared::executeFromMainThread()
{
    executePortion(&cx_->mainThread(), numSlices_ - 1);
}

void
ForkJoinShared::executePortion(PerThreadData *perThread,
                               uint32_t threadId)
{
    Allocator *allocator = allocators_[threadId];
    ForkJoinSlice slice(perThread, threadId, numSlices_, allocator, this);
    AutoSetForkJoinSlice autoContext(&slice);

    if (!op_.parallel(slice))
        setAbortFlag(false);
}

bool
ForkJoinShared::check(ForkJoinSlice &slice)
{
    JS_ASSERT(cx_->runtime->interrupt);

    if (abort_)
        return false;

    if (slice.isMainThread()) {
        JS_ASSERT(!cx_->runtime->gcIsNeeded);

        if (cx_->runtime->interrupt) {
            
            
            
            JS_ASSERT(!cx_->runtime->gcIsNeeded);

            
            
            
            
            
            setAbortFlag(false);
            return false;
        }
    } else if (rendezvous_) {
        joinRendezvous(slice);
    }

    return true;
}

void
ForkJoinShared::initiateRendezvous(ForkJoinSlice &slice)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JS_ASSERT(slice.isMainThread());
    JS_ASSERT(!rendezvous_ && blocked_ == 0);
    JS_ASSERT(cx_->runtime->interrupt);

    AutoLockMonitor lock(*this);

    
    rendezvous_ = true;

    
    while (blocked_ != uncompleted_)
        lock.wait();
}

void
ForkJoinShared::joinRendezvous(ForkJoinSlice &slice)
{
    JS_ASSERT(!slice.isMainThread());
    JS_ASSERT(rendezvous_);

    AutoLockMonitor lock(*this);
    const uint32_t index = rendezvousIndex_;
    blocked_ += 1;

    
    if (blocked_ == uncompleted_)
        lock.notify();

    
    
    
    
    while (rendezvousIndex_ == index)
        PR_WaitCondVar(rendezvousEnd_, PR_INTERVAL_NO_TIMEOUT);
}

void
ForkJoinShared::endRendezvous(ForkJoinSlice &slice)
{
    JS_ASSERT(slice.isMainThread());

    AutoLockMonitor lock(*this);
    rendezvous_ = false;
    blocked_ = 0;
    rendezvousIndex_++;

    
    PR_NotifyAllCondVar(rendezvousEnd_);
}

void
ForkJoinShared::setAbortFlag(bool fatal)
{
    AutoLockMonitor lock(*this);

    abort_ = true;
    fatal_ = fatal_ || fatal;

    cx_->runtime->triggerOperationCallback();
}

void
ForkJoinShared::requestGC(gcreason::Reason reason)
{
    AutoLockMonitor lock(*this);

    gcZone_ = NULL;
    gcReason_ = reason;
    gcRequested_ = true;
}

void
ForkJoinShared::requestZoneGC(JS::Zone *zone, gcreason::Reason reason)
{
    AutoLockMonitor lock(*this);

    if (gcRequested_ && gcZone_ != zone) {
        
        
        gcZone_ = NULL;
        gcReason_ = reason;
        gcRequested_ = true;
    } else {
        
        gcZone_ = zone;
        gcReason_ = reason;
        gcRequested_ = true;
    }
}





ForkJoinSlice::ForkJoinSlice(PerThreadData *perThreadData,
                             uint32_t sliceId, uint32_t numSlices,
                             Allocator *allocator, ForkJoinShared *shared)
    : perThreadData(perThreadData),
      sliceId(sliceId),
      numSlices(numSlices),
      allocator(allocator),
      abortedScript(NULL),
      shared(shared)
{ }

bool
ForkJoinSlice::isMainThread()
{
    return perThreadData == &shared->runtime()->mainThread;
}

JSRuntime *
ForkJoinSlice::runtime()
{
    return shared->runtime();
}

bool
ForkJoinSlice::check()
{
    if (runtime()->interrupt)
        return shared->check(*this);
    else
        return true;
}

bool
ForkJoinSlice::InitializeTLS()
{
    if (!TLSInitialized) {
        TLSInitialized = true;
        PRStatus status = PR_NewThreadPrivateIndex(&ThreadPrivateIndex, NULL);
        return status == PR_SUCCESS;
    }
    return true;
}

void
ForkJoinSlice::requestGC(gcreason::Reason reason)
{
    shared->requestGC(reason);
    triggerAbort();
}

void
ForkJoinSlice::requestZoneGC(JS::Zone *zone, gcreason::Reason reason)
{
    shared->requestZoneGC(zone, reason);
    triggerAbort();
}

void
ForkJoinSlice::triggerAbort()
{
    shared->setAbortFlag(false);

    
    
    
    
    
    
    
    
    perThreadData->ionStackLimit = -1;
}



namespace js {
class AutoEnterParallelSection
{
  private:
    JSContext *cx_;
    uint8_t *prevIonTop_;

  public:
    AutoEnterParallelSection(JSContext *cx)
      : cx_(cx),
        prevIonTop_(cx->mainThread().ionTop)
    {
        
        
        
        

        if (IsIncrementalGCInProgress(cx->runtime)) {
            PrepareForIncrementalGC(cx->runtime);
            FinishIncrementalGC(cx->runtime, gcreason::API);
        }

        cx->runtime->gcHelperThread.waitBackgroundSweepEnd();
    }

    ~AutoEnterParallelSection() {
        cx_->runtime->mainThread.ionTop = prevIonTop_;
    }
};
} 

uint32_t
js::ForkJoinSlices(JSContext *cx)
{
    
    return cx->runtime->threadPool.numWorkers() + 1;
}

ParallelResult
js::ExecuteForkJoinOp(JSContext *cx, ForkJoinOp &op)
{
    
    JS_ASSERT(!InParallelSection());

    AutoEnterParallelSection enter(cx);

    ThreadPool *threadPool = &cx->runtime->threadPool;
    uint32_t numSlices = ForkJoinSlices(cx);

    ForkJoinShared shared(cx, threadPool, op, numSlices, numSlices - 1);
    if (!shared.init())
        return TP_RETRY_SEQUENTIALLY;

    return shared.execute();
}

#else

bool
ForkJoinSlice::isMainThread()
{
    return true;
}

JSRuntime *
ForkJoinSlice::runtime()
{
    return NULL;
}

bool
ForkJoinSlice::check()
{
    return false;
}

bool
ForkJoinSlice::InitializeTLS()
{
    return true;
}

void
ForkJoinSlice::requestGC(gcreason::Reason reason)
{
    JS_NOT_REACHED("No threadsafe, no ion");
}

void
ForkJoinSlice::requestZoneGC(JS::Zone *zone, gcreason::Reason reason)
{
    JS_NOT_REACHED("No threadsafe, no ion");
}

uint32_t
js::ForkJoinSlices(JSContext *cx)
{
    return 1;
}

ParallelResult
js::ExecuteForkJoinOp(JSContext *cx, ForkJoinOp &op)
{
    return TP_RETRY_SEQUENTIALLY;
}

#endif 

