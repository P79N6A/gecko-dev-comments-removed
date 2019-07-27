





#include "mozilla/dom/cache/Context.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/dom/cache/Action.h"
#include "mozilla/dom/cache/Manager.h"
#include "mozilla/dom/cache/ManagerId.h"
#include "mozilla/dom/cache/OfflineStorage.h"
#include "mozilla/dom/quota/OriginOrPatternString.h"
#include "mozilla/dom/quota/QuotaManager.h"
#include "nsIFile.h"
#include "nsIPrincipal.h"
#include "nsIRunnable.h"
#include "nsThreadUtils.h"

namespace {

using mozilla::dom::Nullable;
using mozilla::dom::cache::QuotaInfo;
using mozilla::dom::quota::Client;
using mozilla::dom::quota::OriginOrPatternString;
using mozilla::dom::quota::QuotaManager;
using mozilla::dom::quota::PERSISTENCE_TYPE_DEFAULT;
using mozilla::dom::quota::PersistenceType;


class QuotaReleaseRunnable final : public nsRunnable
{
public:
  explicit QuotaReleaseRunnable(const QuotaInfo& aQuotaInfo)
    : mQuotaInfo(aQuotaInfo)
  { }

  NS_IMETHOD Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());
    QuotaManager* qm = QuotaManager::Get();
    MOZ_ASSERT(qm);
    qm->AllowNextSynchronizedOp(OriginOrPatternString::FromOrigin(mQuotaInfo.mOrigin),
                                Nullable<PersistenceType>(PERSISTENCE_TYPE_DEFAULT),
                                mQuotaInfo.mStorageId);
    return NS_OK;
  }

private:
  ~QuotaReleaseRunnable() { }

  const QuotaInfo mQuotaInfo;
};

} 

namespace mozilla {
namespace dom {
namespace cache {

using mozilla::DebugOnly;
using mozilla::dom::quota::OriginOrPatternString;
using mozilla::dom::quota::QuotaManager;
using mozilla::dom::quota::PERSISTENCE_TYPE_DEFAULT;
using mozilla::dom::quota::PersistenceType;




class Context::QuotaInitRunnable final : public nsIRunnable
                                       , public Action::Resolver
{
public:
  QuotaInitRunnable(Context* aContext,
                    Manager* aManager,
                    Action* aQuotaIOThreadAction)
    : mContext(aContext)
    , mThreadsafeHandle(aContext->CreateThreadsafeHandle())
    , mManager(aManager)
    , mQuotaIOThreadAction(aQuotaIOThreadAction)
    , mInitiatingThread(NS_GetCurrentThread())
    , mResult(NS_OK)
    , mState(STATE_INIT)
    , mNeedsQuotaRelease(false)
  {
    MOZ_ASSERT(mContext);
    MOZ_ASSERT(mManager);
    MOZ_ASSERT(mInitiatingThread);
  }

  nsresult Dispatch()
  {
    NS_ASSERT_OWNINGTHREAD(Action::Resolver);
    MOZ_ASSERT(mState == STATE_INIT);

    mState = STATE_CALL_WAIT_FOR_OPEN_ALLOWED;
    nsresult rv = NS_DispatchToMainThread(this, nsIThread::DISPATCH_NORMAL);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      mState = STATE_COMPLETE;
      Clear();
    }
    return rv;
  }

  virtual void Resolve(nsresult aRv) override
  {
    
    
    
    
    MOZ_ASSERT(mState == STATE_RUNNING || NS_FAILED(aRv));

    mResult = aRv;
    mState = STATE_COMPLETING;

    nsresult rv = mInitiatingThread->Dispatch(this, nsIThread::DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      
      
      MOZ_CRASH("Failed to dispatch QuotaInitRunnable to initiating thread.");
    }
  }

private:
  ~QuotaInitRunnable()
  {
    MOZ_ASSERT(mState == STATE_COMPLETE);
    MOZ_ASSERT(!mContext);
    MOZ_ASSERT(!mQuotaIOThreadAction);
  }

  enum State
  {
    STATE_INIT,
    STATE_CALL_WAIT_FOR_OPEN_ALLOWED,
    STATE_WAIT_FOR_OPEN_ALLOWED,
    STATE_ENSURE_ORIGIN_INITIALIZED,
    STATE_RUNNING,
    STATE_COMPLETING,
    STATE_COMPLETE
  };

  void Clear()
  {
    NS_ASSERT_OWNINGTHREAD(Action::Resolver);
    MOZ_ASSERT(mContext);
    mContext = nullptr;
    mManager = nullptr;
    mQuotaIOThreadAction = nullptr;
  }

  nsRefPtr<Context> mContext;
  nsRefPtr<ThreadsafeHandle> mThreadsafeHandle;
  nsRefPtr<Manager> mManager;
  nsRefPtr<Action> mQuotaIOThreadAction;
  nsCOMPtr<nsIThread> mInitiatingThread;
  nsresult mResult;
  QuotaInfo mQuotaInfo;
  nsMainThreadPtrHandle<OfflineStorage> mOfflineStorage;
  State mState;
  bool mNeedsQuotaRelease;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIRUNNABLE
};

NS_IMPL_ISUPPORTS_INHERITED(mozilla::dom::cache::Context::QuotaInitRunnable,
                            Action::Resolver, nsIRunnable);



































NS_IMETHODIMP
Context::QuotaInitRunnable::Run()
{
  
  

  switch(mState) {
    
    case STATE_CALL_WAIT_FOR_OPEN_ALLOWED:
    {
      MOZ_ASSERT(NS_IsMainThread());
      QuotaManager* qm = QuotaManager::GetOrCreate();
      if (!qm) {
        Resolve(NS_ERROR_FAILURE);
        return NS_OK;
      }

      nsRefPtr<ManagerId> managerId = mManager->GetManagerId();
      nsCOMPtr<nsIPrincipal> principal = managerId->Principal();
      nsresult rv = qm->GetInfoFromPrincipal(principal,
                                             &mQuotaInfo.mGroup,
                                             &mQuotaInfo.mOrigin,
                                             &mQuotaInfo.mIsApp);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        Resolve(rv);
        return NS_OK;
      }

      QuotaManager::GetStorageId(PERSISTENCE_TYPE_DEFAULT,
                                 mQuotaInfo.mOrigin,
                                 Client::DOMCACHE,
                                 NS_LITERAL_STRING("cache"),
                                 mQuotaInfo.mStorageId);

      
      
      
      mState = STATE_WAIT_FOR_OPEN_ALLOWED;
      rv = qm->WaitForOpenAllowed(OriginOrPatternString::FromOrigin(mQuotaInfo.mOrigin),
                                  Nullable<PersistenceType>(PERSISTENCE_TYPE_DEFAULT),
                                  mQuotaInfo.mStorageId, this);
      if (NS_FAILED(rv)) {
        Resolve(rv);
        return NS_OK;
      }
      break;
    }
    
    case STATE_WAIT_FOR_OPEN_ALLOWED:
    {
      MOZ_ASSERT(NS_IsMainThread());

      mNeedsQuotaRelease = true;

      QuotaManager* qm = QuotaManager::Get();
      MOZ_ASSERT(qm);

      nsRefPtr<OfflineStorage> offlineStorage =
        OfflineStorage::Register(mThreadsafeHandle, mQuotaInfo);
      mOfflineStorage = new nsMainThreadPtrHolder<OfflineStorage>(offlineStorage);

      mState = STATE_ENSURE_ORIGIN_INITIALIZED;
      nsresult rv = qm->IOThread()->Dispatch(this, nsIThread::DISPATCH_NORMAL);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        Resolve(rv);
        return NS_OK;
      }
      break;
    }
    
    case STATE_ENSURE_ORIGIN_INITIALIZED:
    {
      
      
      MOZ_ASSERT(!NS_IsMainThread());
      MOZ_ASSERT(_mOwningThread.GetThread() != PR_GetCurrentThread());

      QuotaManager* qm = QuotaManager::Get();
      MOZ_ASSERT(qm);
      nsresult rv = qm->EnsureOriginIsInitialized(PERSISTENCE_TYPE_DEFAULT,
                                                  mQuotaInfo.mGroup,
                                                  mQuotaInfo.mOrigin,
                                                  mQuotaInfo.mIsApp,
                                                  getter_AddRefs(mQuotaInfo.mDir));
      if (NS_FAILED(rv)) {
        Resolve(rv);
        return NS_OK;
      }

      mState = STATE_RUNNING;

      if (!mQuotaIOThreadAction) {
        Resolve(NS_OK);
        return NS_OK;
      }

      
      
      
      mQuotaIOThreadAction->RunOnTarget(this, mQuotaInfo);

      break;
    }
    
    case STATE_COMPLETING:
    {
      NS_ASSERT_OWNINGTHREAD(Action::Resolver);
      if (mQuotaIOThreadAction) {
        mQuotaIOThreadAction->CompleteOnInitiatingThread(mResult);
      }
      mContext->OnQuotaInit(mResult, mQuotaInfo, mOfflineStorage);
      mState = STATE_COMPLETE;

      if (mNeedsQuotaRelease) {
        
        nsCOMPtr<nsIRunnable> runnable = new QuotaReleaseRunnable(mQuotaInfo);
        MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(runnable)));
      }

      
      
      Clear();
      break;
    }
    
    default:
    {
      MOZ_CRASH("unexpected state in QuotaInitRunnable");
      break;
    }
  }

  return NS_OK;
}




class Context::ActionRunnable final : public nsIRunnable
                                    , public Action::Resolver
                                    , public Context::Activity
{
public:
  ActionRunnable(Context* aContext, nsIEventTarget* aTarget, Action* aAction,
                 const QuotaInfo& aQuotaInfo)
    : mContext(aContext)
    , mTarget(aTarget)
    , mAction(aAction)
    , mQuotaInfo(aQuotaInfo)
    , mInitiatingThread(NS_GetCurrentThread())
    , mState(STATE_INIT)
    , mResult(NS_OK)
  {
    MOZ_ASSERT(mContext);
    MOZ_ASSERT(mTarget);
    MOZ_ASSERT(mAction);
    MOZ_ASSERT(mQuotaInfo.mDir);
    MOZ_ASSERT(mInitiatingThread);
  }

  nsresult Dispatch()
  {
    NS_ASSERT_OWNINGTHREAD(Action::Resolver);
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
    NS_ASSERT_OWNINGTHREAD(Action::Resolver);
    return mAction->MatchesCacheId(aCacheId);
  }

  virtual void
  Cancel() override
  {
    NS_ASSERT_OWNINGTHREAD(Action::Resolver);
    mAction->CancelOnInitiatingThread();
  }

  virtual void Resolve(nsresult aRv) override
  {
    MOZ_ASSERT(mTarget == NS_GetCurrentThread());
    MOZ_ASSERT(mState == STATE_RUNNING);
    mResult = aRv;
    mState = STATE_COMPLETING;
    nsresult rv = mInitiatingThread->Dispatch(this, nsIThread::DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      
      
      MOZ_CRASH("Failed to dispatch ActionRunnable to initiating thread.");
    }
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
    NS_ASSERT_OWNINGTHREAD(Action::Resolver);
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
    STATE_COMPLETING,
    STATE_COMPLETE
  };

  nsRefPtr<Context> mContext;
  nsCOMPtr<nsIEventTarget> mTarget;
  nsRefPtr<Action> mAction;
  const QuotaInfo mQuotaInfo;
  nsCOMPtr<nsIThread> mInitiatingThread;
  State mState;
  nsresult mResult;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIRUNNABLE
};

NS_IMPL_ISUPPORTS_INHERITED(mozilla::dom::cache::Context::ActionRunnable,
                            Action::Resolver, nsIRunnable);




























NS_IMETHODIMP
Context::ActionRunnable::Run()
{
  switch(mState) {
    
    case STATE_RUN_ON_TARGET:
    {
      MOZ_ASSERT(NS_GetCurrentThread() == mTarget);
      mState = STATE_RUNNING;
      mAction->RunOnTarget(this, mQuotaInfo);
      break;
    }
    
    case STATE_COMPLETING:
    {
      NS_ASSERT_OWNINGTHREAD(Action::Resolver);
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
Context::Create(Manager* aManager, Action* aQuotaIOThreadAction)
{
  nsRefPtr<Context> context = new Context(aManager);

  nsRefPtr<QuotaInitRunnable> runnable =
    new QuotaInitRunnable(context, aManager, aQuotaIOThreadAction);
  nsresult rv = runnable->Dispatch();
  if (NS_FAILED(rv)) {
    
    
    
    MOZ_CRASH("Failed to dispatch QuotaInitRunnable.");
  }

  return context.forget();
}

Context::Context(Manager* aManager)
  : mManager(aManager)
  , mState(STATE_CONTEXT_INIT)
{
  MOZ_ASSERT(mManager);
}

void
Context::Dispatch(nsIEventTarget* aTarget, Action* aAction)
{
  NS_ASSERT_OWNINGTHREAD(Context);
  MOZ_ASSERT(aTarget);
  MOZ_ASSERT(aAction);

  if (mState == STATE_CONTEXT_CANCELED) {
    return;
  } else if (mState == STATE_CONTEXT_INIT) {
    PendingAction* pending = mPendingActions.AppendElement();
    pending->mTarget = aTarget;
    pending->mAction = aAction;
    return;
  }

  MOZ_ASSERT(STATE_CONTEXT_READY);
  DispatchAction(aTarget, aAction);
}

void
Context::CancelAll()
{
  NS_ASSERT_OWNINGTHREAD(Context);
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

void
Context::Invalidate()
{
  NS_ASSERT_OWNINGTHREAD(Context);
  mManager->Invalidate();
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

  if (mThreadsafeHandle) {
    mThreadsafeHandle->ContextDestroyed(this);
  }

  mManager->RemoveContext(this);
}

void
Context::DispatchAction(nsIEventTarget* aTarget, Action* aAction)
{
  NS_ASSERT_OWNINGTHREAD(Context);

  nsRefPtr<ActionRunnable> runnable =
    new ActionRunnable(this, aTarget, aAction, mQuotaInfo);
  nsresult rv = runnable->Dispatch();
  if (NS_FAILED(rv)) {
    
    
    MOZ_CRASH("Failed to dispatch ActionRunnable to target thread.");
  }
  AddActivity(runnable);
}

void
Context::OnQuotaInit(nsresult aRv, const QuotaInfo& aQuotaInfo,
                     nsMainThreadPtrHandle<OfflineStorage>& aOfflineStorage)
{
  NS_ASSERT_OWNINGTHREAD(Context);

  mQuotaInfo = aQuotaInfo;

  
  
  MOZ_ASSERT(!mOfflineStorage);
  mOfflineStorage = aOfflineStorage;

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
    DispatchAction(mPendingActions[i].mTarget, mPendingActions[i].mAction);
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

} 
} 
} 
