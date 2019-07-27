





#ifndef mozilla_dom_cache_Context_h
#define mozilla_dom_cache_Context_h

#include "mozilla/dom/cache/Types.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsISupportsImpl.h"
#include "nsProxyRelease.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsTObserverArray.h"

class nsIEventTarget;
class nsIThread;

namespace mozilla {
namespace dom {
namespace cache {

class Action;
class Manager;
class OfflineStorage;































class Context final
{
public:
  
  
  class ThreadsafeHandle final
  {
    friend class Context;
  public:
    void AllowToClose();
    void InvalidateAndAllowToClose();
  private:
    explicit ThreadsafeHandle(Context* aContext);
    ~ThreadsafeHandle();

    
    ThreadsafeHandle(const ThreadsafeHandle&) = delete;
    ThreadsafeHandle& operator=(const ThreadsafeHandle&) = delete;

    void AllowToCloseOnOwningThread();
    void InvalidateAndAllowToCloseOnOwningThread();

    void ContextDestroyed(Context* aContext);

    
    
    nsRefPtr<Context> mStrongRef;

    
    
    
    Context* mWeakRef;

    nsCOMPtr<nsIThread> mOwningThread;

    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(cache::Context::ThreadsafeHandle)
  };

  
  
  
  
  
  class Activity
  {
  public:
    virtual void Cancel() = 0;
    virtual bool MatchesCacheId(CacheId aCacheId) const = 0;
  };

  
  
  
  static already_AddRefed<Context>
  Create(Manager* aManager, nsIThread* aTarget,
         Action* aInitAction, Context* aOldContext);

  
  
  
  
  void Dispatch(Action* aAction);

  
  
  
  
  
  void CancelAll();

  
  bool IsCanceled() const;

  
  void Invalidate();

  
  
  void AllowToClose();

  
  
  
  
  void CancelForCacheId(CacheId aCacheId);

  void AddActivity(Activity* aActivity);
  void RemoveActivity(Activity* aActivity);

  const QuotaInfo&
  GetQuotaInfo() const
  {
    return mQuotaInfo;
  }

private:
  class Data;
  class QuotaInitRunnable;
  class ActionRunnable;

  enum State
  {
    STATE_CONTEXT_PREINIT,
    STATE_CONTEXT_INIT,
    STATE_CONTEXT_READY,
    STATE_CONTEXT_CANCELED
  };

  struct PendingAction
  {
    nsCOMPtr<nsIEventTarget> mTarget;
    nsRefPtr<Action> mAction;
  };

  Context(Manager* aManager, nsIThread* aTarget);
  ~Context();
  void Init(Action* aInitAction, Context* aOldContext);
  void Start();
  void DispatchAction(Action* aAction, bool aDoomData = false);
  void OnQuotaInit(nsresult aRv, const QuotaInfo& aQuotaInfo,
                   nsMainThreadPtrHandle<OfflineStorage>& aOfflineStorage);

  already_AddRefed<ThreadsafeHandle>
  CreateThreadsafeHandle();

  void
  SetNextContext(Context* aNextContext);

  void
  DoomTargetData();

  nsRefPtr<Manager> mManager;
  nsCOMPtr<nsIThread> mTarget;
  nsRefPtr<Data> mData;
  State mState;
  QuotaInfo mQuotaInfo;
  nsRefPtr<QuotaInitRunnable> mInitRunnable;
  nsTArray<PendingAction> mPendingActions;

  
  
  typedef nsTObserverArray<Activity*> ActivityList;
  ActivityList mActivityList;

  
  
  
  nsRefPtr<ThreadsafeHandle> mThreadsafeHandle;

  nsMainThreadPtrHandle<OfflineStorage> mOfflineStorage;
  nsRefPtr<Context> mNextContext;

public:
  NS_INLINE_DECL_REFCOUNTING(cache::Context)
};

} 
} 
} 

#endif
