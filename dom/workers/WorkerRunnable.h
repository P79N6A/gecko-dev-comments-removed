




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
  IsDebuggerRunnable() const;

  nsIGlobalObject*
  DefaultGlobalObject() const;

  
  
  
  
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


class WorkerDebuggerRunnable : public WorkerRunnable
{
protected:
  explicit WorkerDebuggerRunnable(WorkerPrivate* aWorkerPrivate)
  : WorkerRunnable(aWorkerPrivate, WorkerThreadUnchangedBusyCount)
  {
  }

  virtual ~WorkerDebuggerRunnable()
  { }

private:
  virtual bool
  IsDebuggerRunnable() const override
  {
    return true;
  }

  virtual bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    AssertIsOnMainThread();

    return true;
  }

  virtual void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult) override;
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
  DispatchInternal() override;
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
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    AssertIsOnMainThread();
    return true;
  }

  virtual void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult) override;
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
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override;

  virtual bool
  DispatchInternal() override;
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
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    AssertIsOnMainThread();
    return true;
  }

  virtual void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult) override;
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

  NS_IMETHOD
  Cancel() override;

public:
  NS_DECL_ISUPPORTS_INHERITED

private:
  virtual bool
  DispatchInternal() override;

  
  using WorkerRunnable::Cancel;
};



class MainThreadWorkerControlRunnable : public WorkerControlRunnable
{
protected:
  explicit MainThreadWorkerControlRunnable(WorkerPrivate* aWorkerPrivate)
  : WorkerControlRunnable(aWorkerPrivate, WorkerThreadUnchangedBusyCount)
  { }

  virtual ~MainThreadWorkerControlRunnable()
  { }

  virtual bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    AssertIsOnMainThread();
    return true;
  }

  virtual void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult) override;
};







class WorkerSameThreadRunnable : public WorkerRunnable
{
protected:
  explicit WorkerSameThreadRunnable(WorkerPrivate* aWorkerPrivate)
  : WorkerRunnable(aWorkerPrivate, WorkerThreadModifyBusyCount)
  { }

  virtual ~WorkerSameThreadRunnable()
  { }

  virtual bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override;

  virtual void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult) override;

  virtual void
  PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
          bool aRunResult) override;
};





class WorkerMainThreadRunnable : public nsRunnable
{
protected:
  WorkerPrivate* mWorkerPrivate;
  nsCOMPtr<nsIEventTarget> mSyncLoopTarget;

  explicit WorkerMainThreadRunnable(WorkerPrivate* aWorkerPrivate);
  ~WorkerMainThreadRunnable() {}

  virtual bool MainThreadRun() = 0;

public:
  bool Dispatch(JSContext* aCx);

private:
  NS_IMETHOD Run() override;
};

END_WORKERS_NAMESPACE

#endif
