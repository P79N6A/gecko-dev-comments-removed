




#ifndef mozilla_dom_workers_workerrunnable_h__
#define mozilla_dom_workers_workerrunnable_h__

#include "Workers.h"

#include "nsICancelableRunnable.h"

#include "mozilla/Atomics.h"
#include "nsISupportsImpl.h"
#include "nsThreadUtils.h" 

struct JSContext;
class nsIEventTarget;

BEGIN_WORKERS_NAMESPACE

class WorkerPrivate;




class WorkerRunnable : public nsICancelableRunnable
{
public:
  enum TargetAndBusyBehavior {
    
    
    ParentThreadUnchangedBusyCount,

    
    
    
    WorkerThreadModifyBusyCount,

    
    
    
    WorkerThreadUnchangedBusyCount
  };

protected:
  
  WorkerPrivate* mWorkerPrivate;

  
  TargetAndBusyBehavior mBehavior;

  
  
  Atomic<uint32_t> mCanceled;

private:
  
  
  
  bool mCallingCancelWithinRun;

public:
  NS_DECL_THREADSAFE_ISUPPORTS

  
  
  
  NS_DECL_NSICANCELABLERUNNABLE

  
  
  
  
  bool
  Dispatch(JSContext* aCx);

  
  virtual bool
  IsCanceled() const
  {
    return mCanceled != 0;
  }

  static WorkerRunnable*
  FromRunnable(nsIRunnable* aRunnable);

protected:
  WorkerRunnable(WorkerPrivate* aWorkerPrivate, TargetAndBusyBehavior aBehavior)
#ifdef DEBUG
  ;
#else
  : mWorkerPrivate(aWorkerPrivate), mBehavior(aBehavior), mCanceled(0),
    mCallingCancelWithinRun(false)
  { }
#endif

  
  virtual ~WorkerRunnable()
  { }

  
  
  
  
  virtual bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate);

  
  
  
  
  virtual void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult);

  
  virtual bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) = 0;

  
  
  
  
  virtual void
  PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate, bool aRunResult);

  virtual bool
  DispatchInternal();

  
  
  NS_DECL_NSIRUNNABLE
};


class WorkerSyncRunnable : public WorkerRunnable
{
protected:
  nsCOMPtr<nsIEventTarget> mSyncLoopTarget;

  
  
  WorkerSyncRunnable(WorkerPrivate* aWorkerPrivate,
                     nsIEventTarget* aSyncLoopTarget);

  WorkerSyncRunnable(WorkerPrivate* aWorkerPrivate,
                     already_AddRefed<nsIEventTarget>&& aSyncLoopTarget);

  virtual ~WorkerSyncRunnable();

private:
  virtual bool
  DispatchInternal() MOZ_OVERRIDE;
};



class MainThreadWorkerSyncRunnable : public WorkerSyncRunnable
{
protected:
  
  
  MainThreadWorkerSyncRunnable(WorkerPrivate* aWorkerPrivate,
                               nsIEventTarget* aSyncLoopTarget)
  : WorkerSyncRunnable(aWorkerPrivate, aSyncLoopTarget)
  {
    AssertIsOnMainThread();
  }

  MainThreadWorkerSyncRunnable(WorkerPrivate* aWorkerPrivate,
                               already_AddRefed<nsIEventTarget>&& aSyncLoopTarget)
  : WorkerSyncRunnable(aWorkerPrivate, Move(aSyncLoopTarget))
  {
    AssertIsOnMainThread();
  }

  virtual ~MainThreadWorkerSyncRunnable()
  { }

private:
  virtual bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) MOZ_OVERRIDE
  {
    AssertIsOnMainThread();
    return true;
  }

  virtual void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult) MOZ_OVERRIDE;
};




class StopSyncLoopRunnable : public WorkerSyncRunnable
{
  bool mResult;

public:
  
  StopSyncLoopRunnable(WorkerPrivate* aWorkerPrivate,
                       already_AddRefed<nsIEventTarget>&& aSyncLoopTarget,
                       bool aResult);

  
  
  NS_DECL_NSICANCELABLERUNNABLE

protected:
  virtual ~StopSyncLoopRunnable()
  { }

  
  
  virtual void
  MaybeSetException(JSContext* aCx)
  { }

private:
  virtual bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) MOZ_OVERRIDE;

  virtual bool
  DispatchInternal() MOZ_OVERRIDE;
};



class MainThreadStopSyncLoopRunnable : public StopSyncLoopRunnable
{
public:
  
  MainThreadStopSyncLoopRunnable(
                               WorkerPrivate* aWorkerPrivate,
                               already_AddRefed<nsIEventTarget>&& aSyncLoopTarget,
                               bool aResult)
  : StopSyncLoopRunnable(aWorkerPrivate, Move(aSyncLoopTarget), aResult)
  {
    AssertIsOnMainThread();
  }

protected:
  virtual ~MainThreadStopSyncLoopRunnable()
  { }

private:
  virtual bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) MOZ_OVERRIDE
  {
    AssertIsOnMainThread();
    return true;
  }

  virtual void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult) MOZ_OVERRIDE;
};






class WorkerControlRunnable : public WorkerRunnable
{
  friend class WorkerPrivate;

protected:
  WorkerControlRunnable(WorkerPrivate* aWorkerPrivate,
                        TargetAndBusyBehavior aBehavior)
#ifdef DEBUG
  ;
#else
  : WorkerRunnable(aWorkerPrivate, aBehavior)
  { }
#endif

  virtual ~WorkerControlRunnable()
  { }

public:
  NS_DECL_ISUPPORTS_INHERITED

private:
  virtual bool
  DispatchInternal() MOZ_OVERRIDE;

  
  using WorkerRunnable::Cancel;
};



class MainThreadWorkerControlRunnable : public WorkerControlRunnable
{
protected:
  MainThreadWorkerControlRunnable(WorkerPrivate* aWorkerPrivate)
  : WorkerControlRunnable(aWorkerPrivate, WorkerThreadUnchangedBusyCount)
  { }

  virtual ~MainThreadWorkerControlRunnable()
  { }

  virtual bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) MOZ_OVERRIDE
  {
    AssertIsOnMainThread();
    return true;
  }

  virtual void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult) MOZ_OVERRIDE;
};







class WorkerSameThreadRunnable : public WorkerRunnable
{
protected:
  WorkerSameThreadRunnable(WorkerPrivate* aWorkerPrivate)
  : WorkerRunnable(aWorkerPrivate, WorkerThreadModifyBusyCount)
  { }

  virtual ~WorkerSameThreadRunnable()
  { }

  virtual bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) MOZ_OVERRIDE;

  virtual void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult) MOZ_OVERRIDE;

  virtual void
  PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
          bool aRunResult) MOZ_OVERRIDE;
};





class WorkerMainThreadRunnable : public nsRunnable
{
protected:
  WorkerPrivate* mWorkerPrivate;
  nsCOMPtr<nsIEventTarget> mSyncLoopTarget;

  WorkerMainThreadRunnable(WorkerPrivate* aWorkerPrivate);
  ~WorkerMainThreadRunnable() {}

  virtual bool MainThreadRun() = 0;

public:
  bool Dispatch(JSContext* aCx);

private:
  NS_IMETHOD Run() MOZ_OVERRIDE;  
};

END_WORKERS_NAMESPACE

#endif
