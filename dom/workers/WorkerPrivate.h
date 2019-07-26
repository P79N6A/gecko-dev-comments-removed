




#ifndef mozilla_dom_workers_workerprivate_h__
#define mozilla_dom_workers_workerprivate_h__

#include "Workers.h"

#include "nsIContentSecurityPolicy.h"
#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsIThreadInternal.h"
#include "nsPIDOMWindow.h"

#include "mozilla/Assertions.h"
#include "mozilla/CondVar.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDataHashtable.h"
#include "nsEventQueue.h"
#include "nsHashKeys.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "nsTPriorityQueue.h"
#include "StructuredCloneTags.h"

#include "EventTarget.h"
#include "Queue.h"
#include "WorkerFeature.h"

class JSAutoStructuredCloneBuffer;
class nsIChannel;
class nsIDocument;
class nsIPrincipal;
class nsIScriptContext;
class nsITimer;
class nsIURI;

namespace JS {
class RuntimeStats;
}

BEGIN_WORKERS_NAMESPACE

class MessagePort;
class SharedWorker;
class WorkerMessagePort;
class WorkerPrivate;

class WorkerRunnable : public nsIRunnable
{
public:
  enum Target { ParentThread, WorkerThread };
  enum BusyBehavior { ModifyBusyCount, UnchangedBusyCount };
  enum ClearingBehavior { SkipWhenClearing, RunWhenClearing };

protected:
  WorkerPrivate* mWorkerPrivate;
  Target mTarget;
  BusyBehavior mBusyBehavior;
  ClearingBehavior mClearingBehavior;

public:
  NS_DECL_THREADSAFE_ISUPPORTS

  bool
  Dispatch(JSContext* aCx);

  static bool
  DispatchToMainThread(nsIRunnable*);

  bool
  WantsToRunDuringClear()
  {
    return mClearingBehavior == RunWhenClearing;
  }

protected:
  WorkerRunnable(WorkerPrivate* aWorkerPrivate, Target aTarget,
                 BusyBehavior aBusyBehavior,
                 ClearingBehavior aClearingBehavior)
#ifdef DEBUG
  ;
#else
  : mWorkerPrivate(aWorkerPrivate), mTarget(aTarget),
    mBusyBehavior(aBusyBehavior), mClearingBehavior(aClearingBehavior)
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
  DispatchInternal();

  virtual bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) = 0;

  virtual void
  PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate, bool aRunResult);

public:
  NS_DECL_NSIRUNNABLE
};

class WorkerSyncRunnable : public WorkerRunnable
{
protected:
  uint32_t mSyncQueueKey;
  bool mBypassSyncQueue;

protected:
  friend class WorkerPrivate;

  WorkerSyncRunnable(WorkerPrivate* aWorkerPrivate, uint32_t aSyncQueueKey,
                     bool aBypassSyncQueue = false,
                     ClearingBehavior aClearingBehavior = SkipWhenClearing)
  : WorkerRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount,
                   aClearingBehavior),
    mSyncQueueKey(aSyncQueueKey), mBypassSyncQueue(aBypassSyncQueue)
  { }

  virtual ~WorkerSyncRunnable()
  { }

  virtual bool
  DispatchInternal() MOZ_OVERRIDE;
};

class MainThreadSyncRunnable : public WorkerSyncRunnable
{
public:
  MainThreadSyncRunnable(WorkerPrivate* aWorkerPrivate,
                         ClearingBehavior aClearingBehavior,
                         uint32_t aSyncQueueKey,
                         bool aBypassSyncEventQueue)
  : WorkerSyncRunnable(aWorkerPrivate, aSyncQueueKey, aBypassSyncEventQueue,
                       aClearingBehavior)
  {
    AssertIsOnMainThread();
  }

  bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) MOZ_OVERRIDE
  {
    AssertIsOnMainThread();
    return true;
  }

  void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult) MOZ_OVERRIDE
  {
    AssertIsOnMainThread();
  }
};

class WorkerControlRunnable : public WorkerRunnable
{
protected:
  WorkerControlRunnable(WorkerPrivate* aWorkerPrivate, Target aTarget,
                        BusyBehavior aBusyBehavior)
  : WorkerRunnable(aWorkerPrivate, aTarget, aBusyBehavior, SkipWhenClearing)
  { }

  virtual ~WorkerControlRunnable()
  { }

  virtual bool
  DispatchInternal() MOZ_OVERRIDE;
};




class SharedMutex
{
  typedef mozilla::Mutex Mutex;

  class RefCountedMutex : public Mutex
  {
  public:
    RefCountedMutex(const char* aName)
    : Mutex(aName)
    { }

    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RefCountedMutex)

  private:
    ~RefCountedMutex()
    { }
  };

  nsRefPtr<RefCountedMutex> mMutex;

public:
  SharedMutex(const char* aName)
  : mMutex(new RefCountedMutex(aName))
  { }

  SharedMutex(SharedMutex& aOther)
  : mMutex(aOther.mMutex)
  { }

  operator Mutex&()
  {
    MOZ_ASSERT(mMutex);
    return *mMutex;
  }

  operator const Mutex&() const
  {
    MOZ_ASSERT(mMutex);
    return *mMutex;
  }

  void
  AssertCurrentThreadOwns() const
  {
    MOZ_ASSERT(mMutex);
    mMutex->AssertCurrentThreadOwns();
  }
};

template <class Derived>
class WorkerPrivateParent : public EventTarget
{
  class SynchronizeAndResumeRunnable;

public:
  struct LocationInfo
  {
    nsCString mHref;
    nsCString mProtocol;
    nsCString mHost;
    nsCString mHostname;
    nsCString mPort;
    nsCString mPathname;
    nsCString mSearch;
    nsCString mHash;
  };

  struct LoadInfo
  {
    
    nsCOMPtr<nsIURI> mBaseURI;
    nsCOMPtr<nsIURI> mResolvedScriptURI;
    nsCOMPtr<nsIPrincipal> mPrincipal;
    nsCOMPtr<nsIScriptContext> mScriptContext;
    nsCOMPtr<nsPIDOMWindow> mWindow;
    nsCOMPtr<nsIContentSecurityPolicy> mCSP;
    nsCOMPtr<nsIChannel> mChannel;

    nsCString mDomain;

    bool mEvalAllowed;
    bool mReportCSPViolations;
    bool mXHRParamsAllowed;
    bool mPrincipalIsSystem;

    LoadInfo()
    : mEvalAllowed(false), mReportCSPViolations(false),
      mXHRParamsAllowed(false), mPrincipalIsSystem(false)
    { }

    void
    StealFrom(LoadInfo& aOther)
    {
      mBaseURI = aOther.mBaseURI.forget();
      mResolvedScriptURI = aOther.mResolvedScriptURI.forget();
      mPrincipal = aOther.mPrincipal.forget();
      mScriptContext = aOther.mScriptContext.forget();
      mWindow = aOther.mWindow.forget();
      mCSP = aOther.mCSP.forget();
      mChannel = aOther.mChannel.forget();
      mDomain = aOther.mDomain;
      mEvalAllowed = aOther.mEvalAllowed;
      mReportCSPViolations = aOther.mReportCSPViolations;
      mXHRParamsAllowed = aOther.mXHRParamsAllowed;
      mPrincipalIsSystem = aOther.mPrincipalIsSystem;
    }
  };

protected:
  typedef mozilla::ErrorResult ErrorResult;

  SharedMutex mMutex;
  mozilla::CondVar mCondVar;
  mozilla::CondVar mMemoryReportCondVar;

private:
  JSObject* mJSObject;
  WorkerPrivate* mParent;
  nsString mScriptURL;
  nsString mSharedWorkerName;
  LocationInfo mLocationInfo;
  LoadInfo mLoadInfo;

  
  nsTArray<nsRefPtr<WorkerRunnable> > mQueuedRunnables;
  nsRevocableEventPtr<SynchronizeAndResumeRunnable> mSynchronizeRunnable;

  
  nsTArray<nsCString> mHostObjectURIs;

  
  JSSettings mJSSettings;

  
  
  nsDataHashtable<nsUint64HashKey, SharedWorker*> mSharedWorkers;

  uint64_t mBusyCount;
  uint64_t mMessagePortSerial;
  Status mParentStatus;
  bool mJSObjectRooted;
  bool mParentSuspended;
  bool mIsChromeWorker;
  bool mMainThreadObjectsForgotten;
  bool mIsSharedWorker;

protected:
  WorkerPrivateParent(JSContext* aCx, JS::HandleObject aObject,
                      WorkerPrivate* aParent, const nsAString& aScriptURL,
                      bool aIsChromeWorker, bool aIsSharedWorker,
                      const nsAString& aSharedWorkerName, LoadInfo& aLoadInfo);

  ~WorkerPrivateParent();

private:
  Derived*
  ParentAsWorkerPrivate() const
  {
    return static_cast<Derived*>(const_cast<WorkerPrivateParent*>(this));
  }

  
  bool
  NotifyPrivate(JSContext* aCx, Status aStatus);

  
  bool
  TerminatePrivate(JSContext* aCx)
  {
    return NotifyPrivate(aCx, Terminating);
  }

  bool
  PostMessageInternal(JSContext* aCx, JS::Handle<JS::Value> aMessage,
                      JS::Handle<JS::Value> aTransferable,
                      bool aToMessagePort, uint64_t aMessagePortSerial);

public:
  
  bool
  Start();

  
  bool
  Notify(JSContext* aCx, Status aStatus)
  {
    return NotifyPrivate(aCx, aStatus);
  }

  bool
  Cancel(JSContext* aCx)
  {
    return Notify(aCx, Canceling);
  }

  bool
  Kill(JSContext* aCx)
  {
    return Notify(aCx, Killing);
  }

  bool
  Suspend(JSContext* aCx, nsPIDOMWindow* aWindow);

  bool
  Resume(JSContext* aCx, nsPIDOMWindow* aWindow);

  bool
  SynchronizeAndResume(JSContext* aCx, nsPIDOMWindow* aWindow,
                       nsIScriptContext* aScriptContext);

  virtual void
  _trace(JSTracer* aTrc) MOZ_OVERRIDE;

  virtual void
  _finalize(JSFreeOp* aFop) MOZ_OVERRIDE;

  void
  Finish(JSContext* aCx)
  {
    RootJSObject(aCx, false);
  }

  bool
  Terminate(JSContext* aCx)
  {
    AssertIsOnParentThread();
    RootJSObject(aCx, false);
    return TerminatePrivate(aCx);
  }

  bool
  Close(JSContext* aCx);

  bool
  ModifyBusyCount(JSContext* aCx, bool aIncrease);

  bool
  RootJSObject(JSContext* aCx, bool aRoot);

  void
  ForgetMainThreadObjects(nsTArray<nsCOMPtr<nsISupports> >& aDoomed);

  bool
  PostMessage(JSContext* aCx, JS::Handle<JS::Value> aMessage,
              JS::Handle<JS::Value> aTransferable)
  {
    return PostMessageInternal(aCx, aMessage, aTransferable, false, 0);
  }

  void
  PostMessageToMessagePort(JSContext* aCx,
                           uint64_t aMessagePortSerial,
                           JS::Handle<JS::Value> aMessage,
                           const Optional<Sequence<JS::Value > >& aTransferable,
                           ErrorResult& aRv);

  bool
  DispatchMessageEventToMessagePort(
                               JSContext* aCx,
                               uint64_t aMessagePortSerial,
                               JSAutoStructuredCloneBuffer& aBuffer,
                               nsTArray<nsCOMPtr<nsISupports>>& aClonedObjects);

  uint64_t
  GetInnerWindowId();

  void
  UpdateJSContextOptions(JSContext* aCx, uint32_t aChromeOptions,
                         uint32_t aContentOptions);

  void
  UpdateJSWorkerMemoryParameter(JSContext* aCx, JSGCParamKey key,
                                uint32_t value);

#ifdef JS_GC_ZEAL
  void
  UpdateGCZeal(JSContext* aCx, uint8_t aGCZeal, uint32_t aFrequency);
#endif

  void
  UpdateJITHardening(JSContext* aCx, bool aJITHardening);

  void
  GarbageCollect(JSContext* aCx, bool aShrinking);

  bool
  RegisterSharedWorker(JSContext* aCx, SharedWorker* aSharedWorker);

  void
  UnregisterSharedWorker(JSContext* aCx, SharedWorker* aSharedWorker);

  void
  BroadcastErrorToSharedWorkers(JSContext* aCx,
                                const nsAString& aMessage,
                                const nsAString& aFilename,
                                const nsAString& aLine,
                                uint32_t aLineNumber,
                                uint32_t aColumnNumber,
                                uint32_t aFlags);

  void
  WorkerScriptLoaded();

  void
  QueueRunnable(WorkerRunnable* aRunnable)
  {
    AssertIsOnMainThread();
    mQueuedRunnables.AppendElement(aRunnable);
  }

  WorkerPrivate*
  GetParent() const
  {
    return mParent;
  }

  bool
  IsSuspended() const
  {
    AssertIsOnParentThread();
    return mParentSuspended;
  }

  bool
  IsAcceptingEvents()
  {
    AssertIsOnParentThread();
    bool acceptingEvents;
    {
      mozilla::MutexAutoLock lock(mMutex);
      acceptingEvents = mParentStatus < Terminating;
    }
    return acceptingEvents;
  }

  Status
  ParentStatus() const
  {
    mMutex.AssertCurrentThreadOwns();
    return mParentStatus;
  }

  JSContext*
  ParentJSContext() const;

  nsIScriptContext*
  GetScriptContext() const
  {
    AssertIsOnMainThread();
    return mLoadInfo.mScriptContext;
  }

  JSObject*
  GetJSObject() const
  {
    return mJSObject;
  }

  const nsString&
  ScriptURL() const
  {
    return mScriptURL;
  }

  const nsCString&
  Domain() const
  {
    return mLoadInfo.mDomain;
  }

  nsIURI*
  GetBaseURI() const
  {
    AssertIsOnMainThread();
    return mLoadInfo.mBaseURI;
  }

  void
  SetBaseURI(nsIURI* aBaseURI);

  nsIURI*
  GetResolvedScriptURI() const
  {
    AssertIsOnMainThread();
    return mLoadInfo.mResolvedScriptURI;
  }

  nsIPrincipal*
  GetPrincipal() const
  {
    AssertIsOnMainThread();
    return mLoadInfo.mPrincipal;
  }

  void
  SetPrincipal(nsIPrincipal* aPrincipal);

  bool
  UsesSystemPrincipal() const
  {
    return mLoadInfo.mPrincipalIsSystem;
  }

  already_AddRefed<nsIChannel>
  ForgetWorkerChannel()
  {
    AssertIsOnMainThread();
    return mLoadInfo.mChannel.forget();
  }

  nsIDocument*
  GetDocument() const
  {
    AssertIsOnMainThread();
    return mLoadInfo.mWindow ? mLoadInfo.mWindow->GetExtantDoc() : nullptr;
  }

  nsPIDOMWindow*
  GetWindow()
  {
    AssertIsOnMainThread();
    return mLoadInfo.mWindow;
  }

  nsIContentSecurityPolicy*
  GetCSP() const
  {
    AssertIsOnMainThread();
    return mLoadInfo.mCSP;
  }

  void
  SetCSP(nsIContentSecurityPolicy* aCSP)
  {
    AssertIsOnMainThread();
    mLoadInfo.mCSP = aCSP;
  }

  bool
  IsEvalAllowed() const
  {
    return mLoadInfo.mEvalAllowed;
  }

  void
  SetEvalAllowed(bool aEvalAllowed)
  {
    mLoadInfo.mEvalAllowed = aEvalAllowed;
  }

  bool
  GetReportCSPViolations() const
  {
    return mLoadInfo.mReportCSPViolations;
  }

  bool
  XHRParamsAllowed() const
  {
    return mLoadInfo.mXHRParamsAllowed;
  }

  void
  SetXHRParamsAllowed(bool aAllowed)
  {
    mLoadInfo.mXHRParamsAllowed = aAllowed;
  }

  LocationInfo&
  GetLocationInfo()
  {
    return mLocationInfo;
  }

  void
  CopyJSSettings(JSSettings& aSettings)
  {
    mozilla::MutexAutoLock lock(mMutex);
    aSettings = mJSSettings;
  }

  bool
  IsChromeWorker() const
  {
    return mIsChromeWorker;
  }

  bool
  IsSharedWorker() const
  {
    return mIsSharedWorker;
  }

  const nsString&
  SharedWorkerName() const
  {
    return mSharedWorkerName;
  }

  uint64_t
  NextMessagePortSerial()
  {
    AssertIsOnMainThread();
    return mMessagePortSerial++;
  }

  void
  GetAllSharedWorkers(nsTArray<nsRefPtr<SharedWorker>>& aSharedWorkers);

  void
  CloseSharedWorkersForWindow(nsPIDOMWindow* aWindow);

  void
  RegisterHostObjectURI(const nsACString& aURI);

  void
  UnregisterHostObjectURI(const nsACString& aURI);

  void
  StealHostObjectURIs(nsTArray<nsCString>& aArray);

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::HandleObject aScope) MOZ_OVERRIDE;

#ifdef DEBUG
  void
  AssertIsOnParentThread() const;

  void
  AssertInnerWindowIsCorrect() const;
#else
  void
  AssertIsOnParentThread() const
  { }

  void
  AssertInnerWindowIsCorrect() const
  { }
#endif
};

class WorkerPrivate : public WorkerPrivateParent<WorkerPrivate>
{
  friend class WorkerPrivateParent<WorkerPrivate>;
  typedef WorkerPrivateParent<WorkerPrivate> ParentType;

  struct TimeoutInfo;

  typedef Queue<WorkerRunnable*, 50> EventQueue;
  EventQueue mQueue;
  EventQueue mControlQueue;

  struct SyncQueue
  {
    Queue<WorkerRunnable*, 10> mQueue;
    bool mComplete;
    bool mResult;

    SyncQueue()
    : mComplete(false), mResult(false)
    { }

    ~SyncQueue()
    {
      WorkerRunnable* event;
      while (mQueue.Pop(event)) {
        event->Release();
      }
    }
  };

  class MemoryReporter;
  friend class MemoryReporter;

  nsTArray<nsAutoPtr<SyncQueue> > mSyncQueues;

  
  JSContext* mJSContext;
  nsRefPtr<WorkerCrossThreadDispatcher> mCrossThreadDispatcher;

  
  nsTArray<ParentType*> mChildWorkers;
  nsTArray<WorkerFeature*> mFeatures;
  nsTArray<nsAutoPtr<TimeoutInfo> > mTimeouts;

  nsCOMPtr<nsITimer> mTimer;
  nsRefPtr<MemoryReporter> mMemoryReporter;

  nsDataHashtable<nsUint64HashKey, WorkerMessagePort*> mWorkerPorts;

  mozilla::TimeStamp mKillTime;
  uint32_t mErrorHandlerRecursionCount;
  uint32_t mNextTimeoutId;
  Status mStatus;
  bool mSuspended;
  bool mTimerRunning;
  bool mRunningExpiredTimeouts;
  bool mCloseHandlerStarted;
  bool mCloseHandlerFinished;
  bool mMemoryReporterRunning;
  bool mBlockedForMemoryReporter;

#ifdef DEBUG
  nsCOMPtr<nsIThread> mThread;
#endif

public:
  ~WorkerPrivate();

  static already_AddRefed<WorkerPrivate>
  Create(JSContext* aCx, JS::HandleObject aObject, WorkerPrivate* aParent,
         const nsAString& aScriptURL, bool aIsChromeWorker,
         bool aIsSharedWorker, const nsAString& aSharedWorkerName,
         LoadInfo* aLoadInfo = nullptr);

  static nsresult
  GetLoadInfo(JSContext* aCx, nsPIDOMWindow* aWindow, WorkerPrivate* aParent,
              const nsAString& aScriptURL, bool aIsChromeWorker,
              LoadInfo* aLoadInfo);

  void
  DoRunLoop(JSContext* aCx);

  bool
  OperationCallback(JSContext* aCx);

  bool
  Dispatch(WorkerRunnable* aEvent)
  {
    return Dispatch(aEvent, &mQueue);
  }

  bool
  Dispatch(WorkerSyncRunnable* aEvent)
  {
    if (aEvent->mBypassSyncQueue) {
      return Dispatch(aEvent, &mQueue);
    }

    return DispatchToSyncQueue(aEvent);
  }

  bool
  Dispatch(WorkerControlRunnable* aEvent)
  {
    return Dispatch(aEvent, &mControlQueue);
  }

  bool
  CloseInternal(JSContext* aCx)
  {
    AssertIsOnWorkerThread();
    return NotifyInternal(aCx, Closing);
  }

  bool
  SuspendInternal(JSContext* aCx);

  bool
  ResumeInternal(JSContext* aCx);

  void
  TraceInternal(JSTracer* aTrc);

  bool
  ModifyBusyCountFromWorker(JSContext* aCx, bool aIncrease);

  bool
  AddChildWorker(JSContext* aCx, ParentType* aChildWorker);

  void
  RemoveChildWorker(JSContext* aCx, ParentType* aChildWorker);

  bool
  AddFeature(JSContext* aCx, WorkerFeature* aFeature);

  void
  RemoveFeature(JSContext* aCx, WorkerFeature* aFeature);

  void
  NotifyFeatures(JSContext* aCx, Status aStatus);

  bool
  HasActiveFeatures()
  {
    return !(mChildWorkers.IsEmpty() && mTimeouts.IsEmpty() &&
             mFeatures.IsEmpty());
  }

  uint32_t
  CreateNewSyncLoop();

  bool
  RunSyncLoop(JSContext* aCx, uint32_t aSyncLoopKey);

  void
  StopSyncLoop(uint32_t aSyncLoopKey, bool aSyncResult);

  void
  DestroySyncLoop(uint32_t aSyncLoopKey);

  bool
  PostMessageToParent(JSContext* aCx,
                      JS::Handle<JS::Value> aMessage,
                      JS::Handle<JS::Value> aTransferable)
  {
    return PostMessageToParentInternal(aCx, aMessage, aTransferable, false, 0);
  }

  void
  PostMessageToParentMessagePort(
                             JSContext* aCx,
                             uint64_t aMessagePortSerial,
                             JS::HandleValue aMessage,
                             const Optional<Sequence<JS::Value>>& aTransferable,
                             ErrorResult& aRv);

  bool
  NotifyInternal(JSContext* aCx, Status aStatus);

  void
  ReportError(JSContext* aCx, const char* aMessage, JSErrorReport* aReport);

  bool
  SetTimeout(JSContext* aCx, unsigned aArgc, jsval* aVp, bool aIsInterval);

  bool
  ClearTimeout(JSContext* aCx, uint32_t aId);

  bool
  RunExpiredTimeouts(JSContext* aCx);

  bool
  RescheduleTimeoutTimer(JSContext* aCx);

  void
  CloseHandlerStarted()
  {
    AssertIsOnWorkerThread();
    mCloseHandlerStarted = true;
  }

  void
  CloseHandlerFinished()
  {
    AssertIsOnWorkerThread();
    mCloseHandlerFinished = true;
  }

  void
  UpdateJSContextOptionsInternal(JSContext* aCx, uint32_t aContentOptions,
                                 uint32_t aChromeOptions);

  void
  UpdateJSWorkerMemoryParameterInternal(JSContext* aCx, JSGCParamKey key, uint32_t aValue);

  void
  ScheduleDeletion(bool aWasPending);

  bool
  BlockAndCollectRuntimeStats(JS::RuntimeStats* aRtStats);

#ifdef JS_GC_ZEAL
  void
  UpdateGCZealInternal(JSContext* aCx, uint8_t aGCZeal, uint32_t aFrequency);
#endif

  void
  UpdateJITHardeningInternal(JSContext* aCx, bool aJITHardening);

  void
  GarbageCollectInternal(JSContext* aCx, bool aShrinking,
                         bool aCollectChildren);

  JSContext*
  GetJSContext() const
  {
    AssertIsOnWorkerThread();
    return mJSContext;
  }

#ifdef DEBUG
  void
  AssertIsOnWorkerThread() const;

  void
  SetThread(nsIThread* aThread)
  {
    mThread = aThread;
  }
#else
  void
  AssertIsOnWorkerThread() const
  { }
#endif

  WorkerCrossThreadDispatcher*
  GetCrossThreadDispatcher();

  
  void
  BeginCTypesCall();

  
  void
  EndCTypesCall();

  void
  BeginCTypesCallback()
  {
    
    
    EndCTypesCall();
  }

  void
  EndCTypesCallback()
  {
    
    
    BeginCTypesCall();
  }

  bool
  ConnectMessagePort(JSContext* aCx, uint64_t aMessagePortSerial);

  void
  DisconnectMessagePort(uint64_t aMessagePortSerial);

  WorkerMessagePort*
  GetMessagePort(uint64_t aMessagePortSerial);

private:
  WorkerPrivate(JSContext* aCx, JS::HandleObject aObject,
                WorkerPrivate* aParent, const nsAString& aScriptURL,
                bool aIsChromeWorker, bool aIsSharedWorker,
                const nsAString& aSharedWorkerName, LoadInfo& aLoadInfo);

  bool
  Dispatch(WorkerRunnable* aEvent, EventQueue* aQueue);

  bool
  DispatchToSyncQueue(WorkerSyncRunnable* aEvent);

  void
  ClearQueue(EventQueue* aQueue);

  bool
  MayContinueRunning()
  {
    AssertIsOnWorkerThread();

    Status status;
    {
      mozilla::MutexAutoLock lock(mMutex);
      status = mStatus;
    }

    if (status >= Killing) {
      return false;
    }
    if (status >= Running) {
      return mKillTime.IsNull() || RemainingRunTimeMS() > 0;
    }
    return true;
  }

  uint32_t
  RemainingRunTimeMS() const;

  void
  CancelAllTimeouts(JSContext* aCx);

  bool
  ScheduleKillCloseEventRunnable(JSContext* aCx);

  void
  StopAcceptingEvents()
  {
    AssertIsOnWorkerThread();

    mozilla::MutexAutoLock lock(mMutex);

    mStatus = Dead;
    mJSContext = nullptr;

    ClearQueue(&mControlQueue);
    ClearQueue(&mQueue);
  }

  bool
  ProcessAllControlRunnables();

  void
  EnableMemoryReporter();

  void
  DisableMemoryReporter();

  void
  WaitForWorkerEvents(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT);

  static PLDHashOperator
  TraceMessagePorts(const uint64_t& aKey,
                    WorkerMessagePort* aData,
                    void* aUserArg);

  bool
  PostMessageToParentInternal(JSContext* aCx,
                              JS::Handle<JS::Value> aMessage,
                              JS::Handle<JS::Value> aTransferable,
                              bool aToMessagePort,
                              uint64_t aMessagePortSerial);
};

WorkerPrivate*
GetWorkerPrivateFromContext(JSContext* aCx);

bool
IsCurrentThreadRunningChromeWorker();

JSContext*
GetCurrentThreadJSContext();

enum WorkerStructuredDataType
{
  DOMWORKER_SCTAG_FILE = SCTAG_DOM_MAX,
  DOMWORKER_SCTAG_BLOB,

  DOMWORKER_SCTAG_END
};

JSStructuredCloneCallbacks*
WorkerStructuredCloneCallbacks(bool aMainRuntime);

JSStructuredCloneCallbacks*
ChromeWorkerStructuredCloneCallbacks(bool aMainRuntime);

class AutoSyncLoopHolder
{
public:
  AutoSyncLoopHolder(WorkerPrivate* aWorkerPrivate)
  : mWorkerPrivate(aWorkerPrivate), mSyncLoopKey(UINT32_MAX)
  {
    mSyncLoopKey = mWorkerPrivate->CreateNewSyncLoop();
  }

  ~AutoSyncLoopHolder()
  {
    if (mWorkerPrivate) {
      mWorkerPrivate->StopSyncLoop(mSyncLoopKey, false);
      mWorkerPrivate->DestroySyncLoop(mSyncLoopKey);
    }
  }

  bool
  RunAndForget(JSContext* aCx)
  {
    WorkerPrivate* workerPrivate = mWorkerPrivate;
    mWorkerPrivate = nullptr;
    return workerPrivate->RunSyncLoop(aCx, mSyncLoopKey);
  }

  uint32_t
  SyncQueueKey() const
  {
    return mSyncLoopKey;
  }

private:
  WorkerPrivate* mWorkerPrivate;
  uint32_t mSyncLoopKey;
};

END_WORKERS_NAMESPACE

#endif 
