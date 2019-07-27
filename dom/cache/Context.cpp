





#include "mozilla/dom/cache/Context.h"

#include "mozilla/AutoRestore.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/dom/cache/Action.h"
#include "mozilla/dom/cache/FileUtils.h"
#include "mozilla/dom/cache/Manager.h"
#include "mozilla/dom/cache/ManagerId.h"
#include "mozIStorageConnection.h"
#include "nsIFile.h"
#include "nsIPrincipal.h"
#include "nsIRunnable.h"
#include "nsThreadUtils.h"

namespace {

using mozilla::dom::cache::Action;
using mozilla::dom::cache::QuotaInfo;

class NullAction final : public Action
{
public:
  NullAction()
  {
  }

  virtual void
  RunOnTarget(Resolver* aResolver, const QuotaInfo&, Data*) override
  {
    
    MOZ_ASSERT(aResolver);
    aResolver->Resolve(NS_OK);
  }
};

} 

namespace mozilla {
namespace dom {
namespace cache {

using mozilla::DebugOnly;
using mozilla::dom::quota::AssertIsOnIOThread;
using mozilla::dom::quota::OpenDirectoryListener;
using mozilla::dom::quota::QuotaManager;
using mozilla::dom::quota::PERSISTENCE_TYPE_DEFAULT;
using mozilla::dom::quota::PersistenceType;

class Context::Data final : public Action::Data
{
public:
  explicit Data(nsIThread* aTarget)
    : mTarget(aTarget)
  {
    MOZ_ASSERT(mTarget);
  }

  virtual mozIStorageConnection*
  GetConnection() const override
  {
    MOZ_ASSERT(mTarget == NS_GetCurrentThread());
    return mConnection;
  }

  virtual void
  SetConnection(mozIStorageConnection* aConn) override
  {
    MOZ_ASSERT(mTarget == NS_GetCurrentThread());
    MOZ_ASSERT(!mConnection);
    mConnection = aConn;
    MOZ_ASSERT(mConnection);
  }

private:
  ~Data()
  {
    
    
    
    
    MOZ_ASSERT(mTarget == NS_GetCurrentThread());
  }

  nsCOMPtr<nsIThread> mTarget;
  nsCOMPtr<mozIStorageConnection> mConnection;

  
  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Context::Data)
};




class Context::QuotaInitRunnable final : public nsIRunnable
                                       , public OpenDirectoryListener
{
public:
  QuotaInitRunnable(Context* aContext,
                    Manager* aManager,
                    Data* aData,
                    nsIThread* aTarget,
                    Action* aInitAction)
    : mContext(aContext)
    , mThreadsafeHandle(aContext->CreateThreadsafeHandle())
    , mManager(aManager)
    , mData(aData)
    , mTarget(aTarget)
    , mInitAction(aInitAction)
    , mInitiatingThread(NS_GetCurrentThread())
    , mResult(NS_OK)
    , mState(STATE_INIT)
    , mCanceled(false)
  {
    MOZ_ASSERT(mContext);
    MOZ_ASSERT(mManager);
    MOZ_ASSERT(mData);
    MOZ_ASSERT(mTarget);
    MOZ_ASSERT(mInitiatingThread);
    MOZ_ASSERT(mInitAction);
  }

  nsresult Dispatch()
  {
    NS_ASSERT_OWNINGTHREAD(QuotaInitRunnable);
    MOZ_ASSERT(mState == STATE_INIT);

    mState = STATE_OPEN_DIRECTORY;
    nsresult rv = NS_DispatchToMainThread(this, nsIThread::DISPATCH_NORMAL);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      mState = STATE_COMPLETE;
      Clear();
    }
    return rv;
  }

  void Cancel()
  {
    NS_ASSERT_OWNINGTHREAD(QuotaInitRunnable);
    MOZ_ASSERT(!mCanceled);
    mCanceled = true;
    mInitAction->CancelOnInitiatingThread();
  }

  
  virtual void
  DirectoryLockAcquired(DirectoryLock* aLock) override;

  virtual void
  DirectoryLockFailed() override;

private:
  class SyncResolver final : public Action::Resolver
  {
  public:
    SyncResolver()
      : mResolved(false)
      , mResult(NS_OK)
    { }

    virtual void
    Resolve(nsresult aRv) override
    {
      MOZ_ASSERT(!mResolved);
      mResolved = true;
      mResult = aRv;
    };

    bool Resolved() const { return mResolved; }
    nsresult Result() const { return mResult; }

  private:
    ~SyncResolver() { }

    bool mResolved;
    nsresult mResult;

    NS_INLINE_DECL_REFCOUNTING(Context::QuotaInitRunnable::SyncResolver, override)
  };

  ~QuotaInitRunnable()
  {
    MOZ_ASSERT(mState == STATE_COMPLETE);
    MOZ_ASSERT(!mContext);
    MOZ_ASSERT(!mInitAction);
  }

  enum State
  {
    STATE_INIT,
    STATE_OPEN_DIRECTORY,
    STATE_WAIT_FOR_DIRECTORY_LOCK,
    STATE_ENSURE_ORIGIN_INITIALIZED,
    STATE_RUN_ON_TARGET,
    STATE_RUNNING,
    STATE_COMPLETING,
    STATE_COMPLETE
  };

  void Complete(nsresult aResult)
  {
    MOZ_ASSERT(mState == STATE_RUNNING || NS_FAILED(aResult));

    MOZ_ASSERT(NS_SUCCEEDED(mResult));
    mResult = aResult;

    mState = STATE_COMPLETING;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      mInitiatingThread->Dispatch(this, nsIThread::DISPATCH_NORMAL)));
  }

  void Clear()
  {
    NS_ASSERT_OWNINGTHREAD(QuotaInitRunnable);
    MOZ_ASSERT(mContext);
    mContext = nullptr;
    mManager = nullptr;
    mInitAction = nullptr;
  }

  nsRefPtr<Context> mContext;
  nsRefPtr<ThreadsafeHandle> mThreadsafeHandle;
  nsRefPtr<Manager> mManager;
  nsRefPtr<Data> mData;
  nsCOMPtr<nsIThread> mTarget;
  nsRefPtr<Action> mInitAction;
  nsCOMPtr<nsIThread> mInitiatingThread;
  nsresult mResult;
  QuotaInfo mQuotaInfo;
  nsMainThreadPtrHandle<DirectoryLock> mDirectoryLock;
  State mState;
  Atomic<bool> mCanceled;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE
};

void
Context::QuotaInitRunnable::DirectoryLockAcquired(DirectoryLock* aLock)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == STATE_WAIT_FOR_DIRECTORY_LOCK);
  MOZ_ASSERT(!mDirectoryLock);

  mDirectoryLock = new nsMainThreadPtrHolder<DirectoryLock>(aLock);

  if (mCanceled) {
    Complete(NS_ERROR_ABORT);
    return;
  }

  QuotaManager* qm = QuotaManager::Get();
  MOZ_ASSERT(qm);

  mState = STATE_ENSURE_ORIGIN_INITIALIZED;
  nsresult rv = qm->IOThread()->Dispatch(this, nsIThread::DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    Complete(rv);
    return;
  }
}

void
Context::QuotaInitRunnable::DirectoryLockFailed()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == STATE_WAIT_FOR_DIRECTORY_LOCK);
  MOZ_ASSERT(!mDirectoryLock);

  NS_WARNING("Failed to acquire a directory lock!");

  Complete(NS_ERROR_FAILURE);
}

NS_IMPL_ISUPPORTS(mozilla::dom::cache::Context::QuotaInitRunnable, nsIRunnable);







































NS_IMETHODIMP
Context::QuotaInitRunnable::Run()
{
  
  

  nsRefPtr<SyncResolver> resolver = new SyncResolver();

  switch(mState) {
    
    case STATE_OPEN_DIRECTORY:
    {
      MOZ_ASSERT(NS_IsMainThread());

      if (mCanceled || QuotaManager::IsShuttingDown()) {
        resolver->Resolve(NS_ERROR_ABORT);
        break;
      }

      QuotaManager* qm = QuotaManager::GetOrCreate();
      if (!qm) {
        resolver->Resolve(NS_ERROR_FAILURE);
        break;
      }

      nsRefPtr<ManagerId> managerId = mManager->GetManagerId();
      nsCOMPtr<nsIPrincipal> principal = managerId->Principal();
      nsresult rv = qm->GetInfoFromPrincipal(principal,
                                             &mQuotaInfo.mGroup,
                                             &mQuotaInfo.mOrigin,
                                             &mQuotaInfo.mIsApp);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        resolver->Resolve(rv);
        break;
      }

      
      
      
      mState = STATE_WAIT_FOR_DIRECTORY_LOCK;
      qm->OpenDirectory(PERSISTENCE_TYPE_DEFAULT,
                        mQuotaInfo.mGroup,
                        mQuotaInfo.mOrigin,
                        mQuotaInfo.mIsApp,
                        quota::Client::DOMCACHE,
                         false,
                        this);
      break;
    }
    
    case STATE_ENSURE_ORIGIN_INITIALIZED:
    {
      AssertIsOnIOThread();

      if (mCanceled) {
        resolver->Resolve(NS_ERROR_ABORT);
        break;
      }

      QuotaManager* qm = QuotaManager::Get();
      MOZ_ASSERT(qm);
      nsresult rv = qm->EnsureOriginIsInitialized(PERSISTENCE_TYPE_DEFAULT,
                                                  mQuotaInfo.mGroup,
                                                  mQuotaInfo.mOrigin,
                                                  mQuotaInfo.mIsApp,
                                                  getter_AddRefs(mQuotaInfo.mDir));
      if (NS_FAILED(rv)) {
        resolver->Resolve(rv);
        break;
      }

      mState = STATE_RUN_ON_TARGET;

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        mTarget->Dispatch(this, nsIThread::DISPATCH_NORMAL)));
      break;
    }
    
    case STATE_RUN_ON_TARGET:
    {
      MOZ_ASSERT(NS_GetCurrentThread() == mTarget);

      mState = STATE_RUNNING;

      
      
      mInitAction->RunOnTarget(resolver, mQuotaInfo, mData);
      MOZ_ASSERT(resolver->Resolved());

      mData = nullptr;

      
      
      
      if (NS_SUCCEEDED(resolver->Result())) {
        MOZ_ALWAYS_TRUE(NS_SUCCEEDED(CreateMarkerFile(mQuotaInfo)));
      }

      break;
    }
    
    case STATE_COMPLETING:
    {
      NS_ASSERT_OWNINGTHREAD(QuotaInitRunnable);
      mInitAction->CompleteOnInitiatingThread(mResult);
      mContext->OnQuotaInit(mResult, mQuotaInfo, mDirectoryLock);
      mState = STATE_COMPLETE;

      
      
      Clear();
      break;
    }
    
    case STATE_WAIT_FOR_DIRECTORY_LOCK:
    default:
    {
      MOZ_CRASH("unexpected state in QuotaInitRunnable");
    }
  }

  if (resolver->Resolved()) {
    Complete(resolver->Result());
  }

  return NS_OK;
}




class Context::ActionRunnable final : public nsIRunnable
                                    , public Action::Resolver
                                    , public Context::Activity
{
public:
  ActionRunnable(Context* aContext, Data* aData, nsIEventTarget* aTarget,
                 Action* aAction, const QuotaInfo& aQuotaInfo)
    : mContext(aContext)
    , mData(aData)
    , mTarget(aTarget)
    , mAction(aAction)
    , mQuotaInfo(aQuotaInfo)
    , mInitiatingThread(NS_GetCurrentThread())
    , mState(STATE_INIT)
    , mResult(NS_OK)
    , mExecutingRunOnTarget(false)
  {
    MOZ_ASSERT(mContext);
    
    MOZ_ASSERT(mTarget);
    MOZ_ASSERT(mAction);
    
    MOZ_ASSERT(mInitiatingThread);
  }

  nsresult Dispatch()
  {
    NS_ASSERT_OWNINGTHREAD(ActionRunnable);
    MOZ_ASSERT(mState == STATE_INIT);

    mState = STATE_RUN_ON_TARGET;
    nsresult rv = mTarget->Dispatch(this, nsIEventTarget::DISPATCH_NORMAL);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      mState = STATE_COMPLETE;
      Clear();
    }
    return rv;
  }

  virtual bool
  MatchesCacheId(CacheId aCacheId) const override
  {
    NS_ASSERT_OWNINGTHREAD(ActionRunnable);
    return mAction->MatchesCacheId(aCacheId);
  }

  virtual void
  Cancel() override
  {
    NS_ASSERT_OWNINGTHREAD(ActionRunnable);
    mAction->CancelOnInitiatingThread();
  }

  virtual void Resolve(nsresult aRv) override
  {
    MOZ_ASSERT(mTarget == NS_GetCurrentThread());
    MOZ_ASSERT(mState == STATE_RUNNING);

    mResult = aRv;

    
    
    
    mState = STATE_RESOLVING;

    
    
    
    
    if (mExecutingRunOnTarget) {
      return;
    }

    
    
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      mTarget->Dispatch(this, nsIThread::DISPATCH_NORMAL)));
  }

private:
  ~ActionRunnable()
  {
    MOZ_ASSERT(mState == STATE_COMPLETE);
    MOZ_ASSERT(!mContext);
    MOZ_ASSERT(!mAction);
  }

  void Clear()
  {
    NS_ASSERT_OWNINGTHREAD(ActionRunnable);
    MOZ_ASSERT(mContext);
    MOZ_ASSERT(mAction);
    mContext->RemoveActivity(this);
    mContext = nullptr;
    mAction = nullptr;
  }

  enum State
  {
    STATE_INIT,
    STATE_RUN_ON_TARGET,
    STATE_RUNNING,
    STATE_RESOLVING,
    STATE_COMPLETING,
    STATE_COMPLETE
  };

  nsRefPtr<Context> mContext;
  nsRefPtr<Data> mData;
  nsCOMPtr<nsIEventTarget> mTarget;
  nsRefPtr<Action> mAction;
  const QuotaInfo mQuotaInfo;
  nsCOMPtr<nsIThread> mInitiatingThread;
  State mState;
  nsresult mResult;

  
  bool mExecutingRunOnTarget;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE
};

NS_IMPL_ISUPPORTS(mozilla::dom::cache::Context::ActionRunnable, nsIRunnable);


































NS_IMETHODIMP
Context::ActionRunnable::Run()
{
  switch(mState) {
    
    case STATE_RUN_ON_TARGET:
    {
      MOZ_ASSERT(NS_GetCurrentThread() == mTarget);
      MOZ_ASSERT(!mExecutingRunOnTarget);

      
      
      AutoRestore<bool> executingRunOnTarget(mExecutingRunOnTarget);
      mExecutingRunOnTarget = true;

      mState = STATE_RUNNING;
      mAction->RunOnTarget(this, mQuotaInfo, mData);

      mData = nullptr;

      
      
      
      if (mState == STATE_RESOLVING) {
        
        
        Run();
      }

      break;
    }
    
    case STATE_RESOLVING:
    {
      MOZ_ASSERT(NS_GetCurrentThread() == mTarget);
      
      
      
      mState = STATE_COMPLETING;
      
      
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        mInitiatingThread->Dispatch(this, nsIThread::DISPATCH_NORMAL)));
      break;
    }
    
    case STATE_COMPLETING:
    {
      NS_ASSERT_OWNINGTHREAD(ActionRunnable);
      mAction->CompleteOnInitiatingThread(mResult);
      mState = STATE_COMPLETE;
      
      
      Clear();
      break;
    }
    
    default:
    {
      MOZ_CRASH("unexpected state in ActionRunnable");
      break;
    }
  }
  return NS_OK;
}

void
Context::ThreadsafeHandle::AllowToClose()
{
  if (mOwningThread == NS_GetCurrentThread()) {
    AllowToCloseOnOwningThread();
    return;
  }

  
  
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &ThreadsafeHandle::AllowToCloseOnOwningThread);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    mOwningThread->Dispatch(runnable, nsIThread::DISPATCH_NORMAL)));
}

void
Context::ThreadsafeHandle::InvalidateAndAllowToClose()
{
  if (mOwningThread == NS_GetCurrentThread()) {
    InvalidateAndAllowToCloseOnOwningThread();
    return;
  }

  
  
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &ThreadsafeHandle::InvalidateAndAllowToCloseOnOwningThread);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    mOwningThread->Dispatch(runnable, nsIThread::DISPATCH_NORMAL)));
}

Context::ThreadsafeHandle::ThreadsafeHandle(Context* aContext)
  : mStrongRef(aContext)
  , mWeakRef(aContext)
  , mOwningThread(NS_GetCurrentThread())
{
}

Context::ThreadsafeHandle::~ThreadsafeHandle()
{
  
  
  
  
  if (!mStrongRef || mOwningThread == NS_GetCurrentThread()) {
    return;
  }

  
  
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewNonOwningRunnableMethod(mStrongRef.forget().take(), &Context::Release);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    mOwningThread->Dispatch(runnable, nsIThread::DISPATCH_NORMAL)));
}

void
Context::ThreadsafeHandle::AllowToCloseOnOwningThread()
{
  MOZ_ASSERT(mOwningThread == NS_GetCurrentThread());

  
  
  
  

  
  
  
  
  if (mStrongRef) {
    mStrongRef->DoomTargetData();
  }

  
  
  mStrongRef = nullptr;
}

void
Context::ThreadsafeHandle::InvalidateAndAllowToCloseOnOwningThread()
{
  MOZ_ASSERT(mOwningThread == NS_GetCurrentThread());
  
  
  
  if (mWeakRef) {
    mWeakRef->Invalidate();
  }
  
  
  MOZ_ASSERT(!mStrongRef);
}

void
Context::ThreadsafeHandle::ContextDestroyed(Context* aContext)
{
  MOZ_ASSERT(mOwningThread == NS_GetCurrentThread());
  MOZ_ASSERT(!mStrongRef);
  MOZ_ASSERT(mWeakRef);
  MOZ_ASSERT(mWeakRef == aContext);
  mWeakRef = nullptr;
}


already_AddRefed<Context>
Context::Create(Manager* aManager, nsIThread* aTarget,
                Action* aInitAction, Context* aOldContext)
{
  nsRefPtr<Context> context = new Context(aManager, aTarget);
  context->Init(aInitAction, aOldContext);
  return context.forget();
}

Context::Context(Manager* aManager, nsIThread* aTarget)
  : mManager(aManager)
  , mTarget(aTarget)
  , mData(new Data(aTarget))
  , mState(STATE_CONTEXT_PREINIT)
  , mOrphanedData(false)
{
  MOZ_ASSERT(mManager);
}

void
Context::Dispatch(Action* aAction)
{
  NS_ASSERT_OWNINGTHREAD(Context);
  MOZ_ASSERT(aAction);

  MOZ_ASSERT(mState != STATE_CONTEXT_CANCELED);
  if (mState == STATE_CONTEXT_CANCELED) {
    return;
  } else if (mState == STATE_CONTEXT_INIT ||
             mState == STATE_CONTEXT_PREINIT) {
    PendingAction* pending = mPendingActions.AppendElement();
    pending->mAction = aAction;
    return;
  }

  MOZ_ASSERT(STATE_CONTEXT_READY);
  DispatchAction(aAction);
}

void
Context::CancelAll()
{
  NS_ASSERT_OWNINGTHREAD(Context);

  
  
  if (mState == STATE_CONTEXT_PREINIT) {
    mInitRunnable = nullptr;

  
  
  
  } else if (mState == STATE_CONTEXT_INIT) {
    mInitRunnable->Cancel();
  }

  mState = STATE_CONTEXT_CANCELED;
  mPendingActions.Clear();
  {
    ActivityList::ForwardIterator iter(mActivityList);
    while (iter.HasMore()) {
      iter.GetNext()->Cancel();
    }
  }
  AllowToClose();
}

bool
Context::IsCanceled() const
{
  NS_ASSERT_OWNINGTHREAD(Context);
  return mState == STATE_CONTEXT_CANCELED;
}

void
Context::Invalidate()
{
  NS_ASSERT_OWNINGTHREAD(Context);
  mManager->NoteClosing();
  CancelAll();
}

void
Context::AllowToClose()
{
  NS_ASSERT_OWNINGTHREAD(Context);
  if (mThreadsafeHandle) {
    mThreadsafeHandle->AllowToClose();
  }
}

void
Context::CancelForCacheId(CacheId aCacheId)
{
  NS_ASSERT_OWNINGTHREAD(Context);

  
  for (int32_t i = mPendingActions.Length() - 1; i >= 0; --i) {
    if (mPendingActions[i].mAction->MatchesCacheId(aCacheId)) {
      mPendingActions.RemoveElementAt(i);
    }
  }

  
  ActivityList::ForwardIterator iter(mActivityList);
  while (iter.HasMore()) {
    Activity* activity = iter.GetNext();
    if (activity->MatchesCacheId(aCacheId)) {
      activity->Cancel();
    }
  }
}

Context::~Context()
{
  NS_ASSERT_OWNINGTHREAD(Context);
  MOZ_ASSERT(mManager);
  MOZ_ASSERT(!mData);

  if (mThreadsafeHandle) {
    mThreadsafeHandle->ContextDestroyed(this);
  }

  
  mManager->RemoveContext(this);

  if (mQuotaInfo.mDir && !mOrphanedData) {
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(DeleteMarkerFile(mQuotaInfo)));
  }

  if (mNextContext) {
    mNextContext->Start();
  }
}

void
Context::Init(Action* aInitAction, Context* aOldContext)
{
  NS_ASSERT_OWNINGTHREAD(Context);
  MOZ_ASSERT(!mInitRunnable);

  
  mInitRunnable = new QuotaInitRunnable(this, mManager, mData, mTarget,
                                        aInitAction);

  if (aOldContext) {
    aOldContext->SetNextContext(this);
    return;
  }

  Start();
}

void
Context::Start()
{
  NS_ASSERT_OWNINGTHREAD(Context);

  
  
  if (mState == STATE_CONTEXT_CANCELED) {
    MOZ_ASSERT(!mInitRunnable);
    return;
  }

  MOZ_ASSERT(mState == STATE_CONTEXT_PREINIT);
  mState = STATE_CONTEXT_INIT;

  nsresult rv = mInitRunnable->Dispatch();
  if (NS_FAILED(rv)) {
    
    
    
    MOZ_CRASH("Failed to dispatch QuotaInitRunnable.");
  }
}

void
Context::DispatchAction(Action* aAction, bool aDoomData)
{
  NS_ASSERT_OWNINGTHREAD(Context);

  nsRefPtr<ActionRunnable> runnable =
    new ActionRunnable(this, mData, mTarget, aAction, mQuotaInfo);

  if (aDoomData) {
    mData = nullptr;
  }

  nsresult rv = runnable->Dispatch();
  if (NS_FAILED(rv)) {
    
    
    MOZ_CRASH("Failed to dispatch ActionRunnable to target thread.");
  }
  AddActivity(runnable);
}

void
Context::OnQuotaInit(nsresult aRv, const QuotaInfo& aQuotaInfo,
                     nsMainThreadPtrHandle<DirectoryLock>& aDirectoryLock)
{
  NS_ASSERT_OWNINGTHREAD(Context);

  MOZ_ASSERT(mInitRunnable);
  mInitRunnable = nullptr;

  mQuotaInfo = aQuotaInfo;

  
  
  MOZ_ASSERT(!mDirectoryLock);
  mDirectoryLock = aDirectoryLock;

  if (mState == STATE_CONTEXT_CANCELED || NS_FAILED(aRv)) {
    for (uint32_t i = 0; i < mPendingActions.Length(); ++i) {
      mPendingActions[i].mAction->CompleteOnInitiatingThread(aRv);
    }
    mPendingActions.Clear();
    mThreadsafeHandle->AllowToClose();
    
    return;
  }

  MOZ_ASSERT(mState == STATE_CONTEXT_INIT);
  mState = STATE_CONTEXT_READY;

  for (uint32_t i = 0; i < mPendingActions.Length(); ++i) {
    DispatchAction(mPendingActions[i].mAction);
  }
  mPendingActions.Clear();
}

void
Context::AddActivity(Activity* aActivity)
{
  NS_ASSERT_OWNINGTHREAD(Context);
  MOZ_ASSERT(aActivity);
  MOZ_ASSERT(!mActivityList.Contains(aActivity));
  mActivityList.AppendElement(aActivity);
}

void
Context::RemoveActivity(Activity* aActivity)
{
  NS_ASSERT_OWNINGTHREAD(Context);
  MOZ_ASSERT(aActivity);
  MOZ_ALWAYS_TRUE(mActivityList.RemoveElement(aActivity));
  MOZ_ASSERT(!mActivityList.Contains(aActivity));
}

void
Context::NoteOrphanedData()
{
  NS_ASSERT_OWNINGTHREAD(Context);
  
  mOrphanedData = true;
}

already_AddRefed<Context::ThreadsafeHandle>
Context::CreateThreadsafeHandle()
{
  NS_ASSERT_OWNINGTHREAD(Context);
  if (!mThreadsafeHandle) {
    mThreadsafeHandle = new ThreadsafeHandle(this);
  }
  nsRefPtr<ThreadsafeHandle> ref = mThreadsafeHandle;
  return ref.forget();
}

void
Context::SetNextContext(Context* aNextContext)
{
  NS_ASSERT_OWNINGTHREAD(Context);
  MOZ_ASSERT(aNextContext);
  MOZ_ASSERT(!mNextContext);
  mNextContext = aNextContext;
}

void
Context::DoomTargetData()
{
  NS_ASSERT_OWNINGTHREAD(Context);
  MOZ_ASSERT(mData);

  
  
  

  
  
  
  
  nsRefPtr<Action> action = new NullAction();
  DispatchAction(action, true );

  MOZ_ASSERT(!mData);
}

} 
} 
} 
