





#if !defined(MediaPromise_h_)
#define MediaPromise_h_

#include "prlog.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/Maybe.h"
#include "mozilla/Mutex.h"
#include "mozilla/Monitor.h"

#include "MediaTaskQueue.h"
#include "nsIEventTarget.h"


#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

namespace mozilla {

extern PRLogModuleInfo* gMediaPromiseLog;
void EnsureMediaPromiseLog();

#define PROMISE_LOG(x, ...) \
  MOZ_ASSERT(gMediaPromiseLog); \
  PR_LOG(gMediaPromiseLog, PR_LOG_DEBUG, (x, ##__VA_ARGS__))










template<typename T> class MediaPromiseHolder;
template<typename ResolveValueT, typename RejectValueT>
class MediaPromise
{
public:
  typedef ResolveValueT ResolveValueType;
  typedef RejectValueT RejectValueType;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaPromise)
  MediaPromise(const char* aCreationSite)
    : mCreationSite(aCreationSite)
    , mMutex("MediaPromise Mutex")
  {
    MOZ_COUNT_CTOR(MediaPromise);
    PROMISE_LOG("%s creating MediaPromise (%p)", mCreationSite, this);
  }

protected:

  





  class ThenValueBase
  {
  public:
    class ThenValueRunnable : public nsRunnable
    {
    public:
      ThenValueRunnable(ThenValueBase* aThenValue, ResolveValueType aResolveValue)
        : mThenValue(aThenValue)
      {
        MOZ_COUNT_CTOR(ThenValueRunnable);
        mResolveValue.emplace(aResolveValue);
      }

      ThenValueRunnable(ThenValueBase* aThenValue, RejectValueType aRejectValue)
        : mThenValue(aThenValue)
      {
        MOZ_COUNT_CTOR(ThenValueRunnable);
        mRejectValue.emplace(aRejectValue);
      }

      ~ThenValueRunnable()
      {
        MOZ_COUNT_DTOR(ThenValueRunnable);
        MOZ_ASSERT(!mThenValue);
      }

      NS_IMETHODIMP Run()
      {
        PROMISE_LOG("ThenValueRunnable::Run() [this=%p]", this);
        if (mResolveValue.isSome()) {
          mThenValue->DoResolve(mResolveValue.ref());
        } else {
          mThenValue->DoReject(mRejectValue.ref());
        }

        delete mThenValue;
        mThenValue = nullptr;
        return NS_OK;
      }

    private:
      ThenValueBase* mThenValue;
      Maybe<ResolveValueType> mResolveValue;
      Maybe<RejectValueType> mRejectValue;
    };

    ThenValueBase(const char* aCallSite) : mCallSite(aCallSite)
    {
      MOZ_COUNT_CTOR(ThenValueBase);
    }

    virtual void Dispatch(MediaPromise *aPromise) = 0;

  protected:
    
    virtual ~ThenValueBase() { MOZ_COUNT_DTOR(ThenValueBase); }

    virtual void DoResolve(ResolveValueType aResolveValue) = 0;
    virtual void DoReject(RejectValueType aRejectValue) = 0;

    const char* mCallSite;
  };

  template<typename TargetType, typename ThisType,
           typename ResolveMethodType, typename RejectMethodType>
  class ThenValue : public ThenValueBase
  {
  public:
    ThenValue(TargetType* aResponseTarget, ThisType* aThisVal,
              ResolveMethodType aResolveMethod, RejectMethodType aRejectMethod,
              const char* aCallSite)
      : ThenValueBase(aCallSite)
      , mResponseTarget(aResponseTarget)
      , mThisVal(aThisVal)
      , mResolveMethod(aResolveMethod)
      , mRejectMethod(aRejectMethod) {}

    void Dispatch(MediaPromise *aPromise) MOZ_OVERRIDE
    {
      aPromise->mMutex.AssertCurrentThreadOwns();
      MOZ_ASSERT(!aPromise->IsPending());
      bool resolved = aPromise->mResolveValue.isSome();
      nsRefPtr<nsRunnable> runnable =
        resolved ? new (typename ThenValueBase::ThenValueRunnable)(this, aPromise->mResolveValue.ref())
                 : new (typename ThenValueBase::ThenValueRunnable)(this, aPromise->mRejectValue.ref());
      PROMISE_LOG("%s Then() call made from %s [Runnable=%p, Promise=%p, ThenValue=%p]",
                  resolved ? "Resolving" : "Rejecting", ThenValueBase::mCallSite,
                  runnable.get(), aPromise, this);
      DebugOnly<nsresult> rv = DoDispatch(mResponseTarget, runnable);
      MOZ_ASSERT(NS_SUCCEEDED(rv));
    }

  protected:
    virtual void DoResolve(ResolveValueType aResolveValue)
    {
      ((*mThisVal).*mResolveMethod)(aResolveValue);
    }

    virtual void DoReject(RejectValueType aRejectValue)
    {
      ((*mThisVal).*mRejectMethod)(aRejectValue);
    }

    static nsresult DoDispatch(MediaTaskQueue* aTaskQueue, nsIRunnable* aRunnable)
    {
      return aTaskQueue->ForceDispatch(aRunnable);
    }

    static nsresult DoDispatch(nsIEventTarget* aEventTarget, nsIRunnable* aRunnable)
    {
      return aEventTarget->Dispatch(aRunnable, NS_DISPATCH_NORMAL);
    }

    virtual ~ThenValue() {}

  private:
    nsRefPtr<TargetType> mResponseTarget;
    nsRefPtr<ThisType> mThisVal;
    ResolveMethodType mResolveMethod;
    RejectMethodType mRejectMethod;
  };
public:

  template<typename TargetType, typename ThisType,
           typename ResolveMethodType, typename RejectMethodType>
  void Then(TargetType* aResponseTarget, ThisType* aThisVal,
            ResolveMethodType aResolveMethod, RejectMethodType aRejectMethod,
            const char* aCallSite)
  {
    MutexAutoLock lock(mMutex);
    ThenValueBase* thenValue = new ThenValue<TargetType, ThisType, ResolveMethodType,
                                             RejectMethodType>(aResponseTarget, aThisVal,
                                                               aResolveMethod, aRejectMethod,
                                                               aCallSite);
    PROMISE_LOG("%s invoking Then() [this=%p, thenValue=%p, aThisVal=%p, isPending=%d]",
                aCallSite, this, thenValue, aThisVal, (int) IsPending());
    if (!IsPending()) {
      thenValue->Dispatch(this);
    } else {
      mThenValues.AppendElement(thenValue);
    }
  }

private:
  
  friend class MediaPromiseHolder<MediaPromise<ResolveValueType, RejectValueType>>;
  void Resolve(ResolveValueType aResolveValue, const char* aResolveSite)
  {
    MutexAutoLock lock(mMutex);
    MOZ_ASSERT(IsPending());
    PROMISE_LOG("%s resolving MediaPromise (%p created at %s)", aResolveSite, this, mCreationSite);
    mResolveValue.emplace(aResolveValue);
    DispatchAll();
  }

  void Reject(RejectValueType aRejectValue, const char* aRejectSite)
  {
    MutexAutoLock lock(mMutex);
    MOZ_ASSERT(IsPending());
    PROMISE_LOG("%s rejecting MediaPromise (%p created at %s)", aRejectSite, this, mCreationSite);
    mRejectValue.emplace(aRejectValue);
    DispatchAll();
  }

protected:
  bool IsPending() { return mResolveValue.isNothing() && mRejectValue.isNothing(); }
  void DispatchAll()
  {
    mMutex.AssertCurrentThreadOwns();
    for (size_t i = 0; i < mThenValues.Length(); ++i)
      mThenValues[i]->Dispatch(this);
    mThenValues.Clear();
  }

  ~MediaPromise()
  {
    MOZ_COUNT_DTOR(MediaPromise);
    PROMISE_LOG("MediaPromise::~MediaPromise [this=%p]", this);
    MOZ_ASSERT(!IsPending());
  };

  const char* mCreationSite; 
  Mutex mMutex;
  Maybe<ResolveValueType> mResolveValue;
  Maybe<RejectValueType> mRejectValue;
  nsTArray<ThenValueBase*> mThenValues;
};





template<typename PromiseType>
class MediaPromiseHolder
{
public:
  MediaPromiseHolder()
    : mMonitor(nullptr) {}

  ~MediaPromiseHolder() { MOZ_ASSERT(!mPromise); }

  already_AddRefed<PromiseType> Ensure(const char* aMethodName) {
    if (mMonitor) {
      mMonitor->AssertCurrentThreadOwns();
    }
    if (!mPromise) {
      mPromise = new PromiseType(aMethodName);
    }
    nsRefPtr<PromiseType> p = mPromise;
    return p.forget();
  }

  
  void SetMonitor(Monitor* aMonitor) { mMonitor = aMonitor; }

  bool IsEmpty()
  {
    if (mMonitor) {
      mMonitor->AssertCurrentThreadOwns();
    }
    return !mPromise;
  }

  void Resolve(typename PromiseType::ResolveValueType aResolveValue,
               const char* aMethodName)
  {
    if (mMonitor) {
      mMonitor->AssertCurrentThreadOwns();
    }
    MOZ_ASSERT(mPromise);
    mPromise->Resolve(aResolveValue, aMethodName);
    mPromise = nullptr;
  }

  void Reject(typename PromiseType::RejectValueType aRejectValue,
              const char* aMethodName)
  {
    if (mMonitor) {
      mMonitor->AssertCurrentThreadOwns();
    }
    MOZ_ASSERT(mPromise);
    mPromise->Reject(aRejectValue, aMethodName);
    mPromise = nullptr;
  }

private:
  Monitor* mMonitor;
  nsRefPtr<PromiseType> mPromise;
};

#undef PROMISE_LOG

} 

#endif
