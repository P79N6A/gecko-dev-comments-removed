






#include "jscntxt.h"
#include "jscompartment.h"

#include "vm/ForkJoin.h"
#include "vm/Monitor.h"

#include "vm/ForkJoin-inl.h"

#ifdef JS_THREADSAFE
#  include "prthread.h"
#endif

using namespace js;

#ifdef JS_THREADSAFE

class js::ForkJoinShared : public TaskExecutor, public Monitor
{
    
    

    JSContext *const cx_;          
    ThreadPool *const threadPool_; 
    ForkJoinOp &op_;               
    const size_t numThreads_;      
    PRCondVar *rendezvousEnd_;     

    
    
    
    

    Vector<gc::ArenaLists *, 16> arenaListss_;

    
    
    
    

    size_t uncompleted_;     
    size_t blocked_;         
    size_t rendezvousIndex_; 

    
    
    
    

    
    
    
    
    volatile bool abort_;

    
    volatile bool fatal_;

    
    
    
    volatile bool rendezvous_;

    
    void executeFromMainThread(uintptr_t stackLimit);

    
    
    void executePortion(PerThreadData *perThread, size_t threadId, uintptr_t stackLimit);

    
    
    
    

    friend class AutoRendezvous;

    
    
    void initiateRendezvous(ForkJoinSlice &threadCx);

    
    
    void joinRendezvous(ForkJoinSlice &threadCx);

    
    
    void endRendezvous(ForkJoinSlice &threadCx);

  public:
    ForkJoinShared(JSContext *cx,
                   ThreadPool *threadPool,
                   ForkJoinOp &op,
                   size_t numThreads,
                   size_t uncompleted);
    ~ForkJoinShared();

    bool init();

    ParallelResult execute();

    
    virtual void executeFromWorker(size_t threadId, uintptr_t stackLimit);

    
    
    
    void transferArenasToCompartment();

    
    bool check(ForkJoinSlice &threadCx);

    
    bool setFatal();

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

unsigned ForkJoinSlice::ThreadPrivateIndex;

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
                               size_t numThreads,
                               size_t uncompleted)
    : cx_(cx),
      threadPool_(threadPool),
      op_(op),
      numThreads_(numThreads),
      arenaListss_(cx),
      uncompleted_(uncompleted),
      blocked_(0),
      rendezvousIndex_(0),
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

    for (unsigned i = 0; i < numThreads_; i++) {
        gc::ArenaLists *arenaLists = cx_->new_<gc::ArenaLists>();
        if (!arenaLists)
            return false;

        if (!arenaListss_.append(arenaLists)) {
            delete arenaLists;
            return false;
        }
    }

    return true;
}

ForkJoinShared::~ForkJoinShared()
{
    PR_DestroyCondVar(rendezvousEnd_);

    while (arenaListss_.length() > 0)
        delete arenaListss_.popCopy();
}

ParallelResult
ForkJoinShared::execute()
{
    AutoLockMonitor lock(*this);

    
    if (!op_.pre(numThreads_))
        return TP_RETRY_SEQUENTIALLY;

    
    {
        AutoUnlockMonitor unlock(*this);
        if (!threadPool_->submitAll(cx_, this))
            return TP_FATAL;
        executeFromMainThread(cx_->runtime->ionStackLimit);
    }

    
    while (uncompleted_ > 0)
        lock.wait();

    
    if (abort_) {
        if (fatal_)
            return TP_FATAL;
        else
            return TP_RETRY_SEQUENTIALLY;
    }

    transferArenasToCompartment();

    
    if (!op_.post(numThreads_))
        return TP_RETRY_SEQUENTIALLY;

    
    return TP_SUCCESS;
}

void
ForkJoinShared::transferArenasToCompartment()
{
#if 0
    

    JSRuntime *rt = cx_->runtime;
    JSCompartment *comp = cx_->compartment;
    for (unsigned i = 0; i < numThreads_; i++)
        comp->arenas.adoptArenas(rt, arenaListss_[i]);
#endif
}

void
ForkJoinShared::executeFromWorker(size_t workerId, uintptr_t stackLimit)
{
    JS_ASSERT(workerId < numThreads_ - 1);

    PerThreadData thisThread(cx_->runtime);
    TlsPerThreadData.set(&thisThread);
    executePortion(&thisThread, workerId, stackLimit);
    TlsPerThreadData.set(NULL);

    AutoLockMonitor lock(*this);
    uncompleted_ -= 1;
    if (blocked_ == uncompleted_) {
        
        
        
        lock.notify();
    }
}

void
ForkJoinShared::executeFromMainThread(uintptr_t stackLimit)
{
    executePortion(&cx_->runtime->mainThread, numThreads_ - 1, stackLimit);
}

void
ForkJoinShared::executePortion(PerThreadData *perThread,
                               size_t threadId,
                               uintptr_t stackLimit)
{
    gc::ArenaLists *arenaLists = arenaListss_[threadId];
    ForkJoinSlice slice(perThread, threadId, numThreads_,
                        stackLimit, arenaLists, this);
    AutoSetForkJoinSlice autoContext(&slice);

    if (!op_.parallel(slice))
        abort_ = true;
}

bool
ForkJoinShared::setFatal()
{
    
    
    abort_ = true;
    fatal_ = true;
    return false;
}

bool
ForkJoinShared::check(ForkJoinSlice &slice)
{
    if (abort_)
        return false;

    if (slice.isMainThread()) {
        if (cx_->runtime->interrupt) {
            
            
            AutoRendezvous autoRendezvous(slice);
            if (!js_HandleExecutionInterrupt(cx_))
                return setFatal();
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
    const size_t index = rendezvousIndex_;
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
    rendezvousIndex_ += 1;

    
    PR_NotifyAllCondVar(rendezvousEnd_);
}

#endif 





ForkJoinSlice::ForkJoinSlice(PerThreadData *perThreadData,
                             size_t sliceId, size_t numSlices,
                             uintptr_t stackLimit, gc::ArenaLists *arenaLists,
                             ForkJoinShared *shared)
    : perThreadData(perThreadData),
      sliceId(sliceId),
      numSlices(numSlices),
      ionStackLimit(stackLimit),
      arenaLists(arenaLists),
      shared(shared)
{ }

bool
ForkJoinSlice::isMainThread()
{
#ifdef JS_THREADSAFE
    return perThreadData == &shared->runtime()->mainThread;
#else
    return true;
#endif
}

JSRuntime *
ForkJoinSlice::runtime()
{
#ifdef JS_THREADSAFE
    return shared->runtime();
#else
    return NULL;
#endif
}

bool
ForkJoinSlice::check()
{
#ifdef JS_THREADSAFE
    return shared->check(*this);
#else
    return false;
#endif
}

bool
ForkJoinSlice::setFatal()
{
#ifdef JS_THREADSAFE
    return shared->setFatal();
#else
    return false;
#endif
}

bool
ForkJoinSlice::Initialize()
{
#ifdef JS_THREADSAFE
    PRStatus status = PR_NewThreadPrivateIndex(&ThreadPrivateIndex, NULL);
    return status == PR_SUCCESS;
#else
    return true;
#endif
}



ParallelResult
js::ExecuteForkJoinOp(JSContext *cx, ForkJoinOp &op)
{
#ifdef JS_THREADSAFE
    
    JS_ASSERT(!InParallelSection());

    ThreadPool *threadPool = &cx->runtime->threadPool;
    
    size_t numThreads = threadPool->numWorkers() + 1;

    ForkJoinShared shared(cx, threadPool, op, numThreads, numThreads - 1);
    if (!shared.init())
        return TP_RETRY_SEQUENTIALLY;

    return shared.execute();
#else
    return TP_RETRY_SEQUENTIALLY;
#endif
}
