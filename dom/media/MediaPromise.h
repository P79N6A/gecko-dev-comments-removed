





#if !defined(MediaPromise_h_)
#define MediaPromise_h_

#include "prlog.h"

#include "nsTArray.h"
#include "nsThreadUtils.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/Maybe.h"
#include "mozilla/Mutex.h"
#include "mozilla/Monitor.h"


#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

class nsIEventTarget;
namespace mozilla {

extern PRLogModuleInfo* gMediaPromiseLog;

#define PROMISE_LOG(x, ...) \
  MOZ_ASSERT(gMediaPromiseLog); \
  PR_LOG(gMediaPromiseLog, PR_LOG_DEBUG, (x, ##__VA_ARGS__))

class MediaTaskQueue;
namespace detail {

nsresult DispatchMediaPromiseRunnable(MediaTaskQueue* aQueue, nsIRunnable* aRunnable);
nsresult DispatchMediaPromiseRunnable(nsIEventTarget* aTarget, nsIRunnable* aRunnable);

#ifdef DEBUG
void AssertOnThread(MediaTaskQueue* aQueue);
void AssertOnThread(nsIEventTarget* aTarget);
#endif

} 










template<typename T> class MediaPromiseHolder;
template<typename ResolveValueT, typename RejectValueT, bool IsExclusive>
class MediaPromise
{
public:
  typedef ResolveValueT ResolveValueType;
  typedef RejectValueT RejectValueType;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaPromise)
  explicit MediaPromise(const char* aCreationSite)
    : mCreationSite(aCreationSite)
    , mMutex("MediaPromise Mutex")
    , mHaveConsumer(false)
  {
    PROMISE_LOG("%s creating MediaPromise (%p)", mCreationSite, this);
  }

  static nsRefPtr<MediaPromise>
  CreateAndResolve(ResolveValueType aResolveValue, const char* aResolveSite)
  {
    nsRefPtr<MediaPromise> p = new MediaPromise(aResolveSite);
    p->Resolve(aResolveValue, aResolveSite);
    return p;
  }

  static nsRefPtr<MediaPromise>
  CreateAndReject(RejectValueType aRejectValue, const char* aRejectSite)
  {
    nsRefPtr<MediaPromise> p = new MediaPromise(aRejectSite);
    p->Reject(aRejectValue, aRejectSite);
    return p;
  }

  class Consumer
  {
  public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Consumer)

    void Disconnect()
    {
      AssertOnDispatchThread();
      MOZ_DIAGNOSTIC_ASSERT(!mComplete);
      mDisconnected = true;
    }

#ifdef DEBUG
    virtual void AssertOnDispatchThread() = 0;
#else
    void AssertOnDispatchThread() {}
#endif

  protected:
    Consumer() : mComplete(false), mDisconnected(false) {}
    virtual ~Consumer() {}

    bool mComplete;
    bool mDisconnected;
  };

protected:

  





  class ThenValueBase : public Consumer
  {
  public:
    class ResolveRunnable : public nsRunnable
    {
    public:
      ResolveRunnable(ThenValueBase* aThenValue, ResolveValueType aResolveValue)
        : mThenValue(aThenValue)
        , mResolveValue(aResolveValue) {}

      ~ResolveRunnable()
      {
        MOZ_ASSERT(!mThenValue);
      }

      NS_IMETHODIMP Run()
      {
        PROMISE_LOG("ResolveRunnable::Run() [this=%p]", this);
        mThenValue->DoResolve(mResolveValue);
        mThenValue = nullptr;
        return NS_OK;
      }

    private:
      nsRefPtr<ThenValueBase> mThenValue;
      ResolveValueType mResolveValue;
    };

    class RejectRunnable : public nsRunnable
    {
    public:
      RejectRunnable(ThenValueBase* aThenValue, RejectValueType aRejectValue)
        : mThenValue(aThenValue)
        , mRejectValue(aRejectValue) {}

      ~RejectRunnable()
      {
        MOZ_ASSERT(!mThenValue);
      }

      NS_IMETHODIMP Run()
      {
        PROMISE_LOG("RejectRunnable::Run() [this=%p]", this);
        mThenValue->DoReject(mRejectValue);
        mThenValue = nullptr;
        return NS_OK;
      }

    private:
      nsRefPtr<ThenValueBase> mThenValue;
      RejectValueType mRejectValue;
    };

    explicit ThenValueBase(const char* aCallSite) : mCallSite(aCallSite) {}

    virtual void Dispatch(MediaPromise *aPromise) = 0;

  protected:
    virtual void DoResolve(ResolveValueType aResolveValue) = 0;
    virtual void DoReject(RejectValueType aRejectValue) = 0;

    const char* mCallSite;
  };

  




  
  
  template <typename T>
  struct NonDeduced
  {
    typedef T type;
  };

  template<typename ThisType, typename ValueType>
  static void InvokeCallbackMethod(ThisType* aThisVal, void(ThisType::*aMethod)(ValueType),
                                   typename NonDeduced<ValueType>::type aValue)
  {
      ((*aThisVal).*aMethod)(aValue);
  }

  template<typename ThisType, typename ValueType>
  static void InvokeCallbackMethod(ThisType* aThisVal, void(ThisType::*aMethod)(), ValueType aValue)
  {
      ((*aThisVal).*aMethod)();
  }

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
        resolved ? static_cast<nsRunnable*>(new (typename ThenValueBase::ResolveRunnable)(this, aPromise->mResolveValue.ref()))
                 : static_cast<nsRunnable*>(new (typename ThenValueBase::RejectRunnable)(this, aPromise->mRejectValue.ref()));
      PROMISE_LOG("%s Then() call made from %s [Runnable=%p, Promise=%p, ThenValue=%p]",
                  resolved ? "Resolving" : "Rejecting", ThenValueBase::mCallSite,
                  runnable.get(), aPromise, this);
      DebugOnly<nsresult> rv = detail::DispatchMediaPromiseRunnable(mResponseTarget, runnable);
      MOZ_ASSERT(NS_SUCCEEDED(rv));
    }

#ifdef DEBUG
  
  virtual void AssertOnDispatchThread()
  {
    detail::AssertOnThread(mResponseTarget);
  }
#endif

  protected:
    virtual void DoResolve(ResolveValueType aResolveValue) MOZ_OVERRIDE
    {
      Consumer::mComplete = true;
      if (Consumer::mDisconnected) {
        PROMISE_LOG("ThenValue::DoResolve disconnected - bailing out [this=%p]", this);
        
        mResponseTarget = nullptr;
        mThisVal = nullptr;
        return;
      }
      InvokeCallbackMethod(mThisVal.get(), mResolveMethod, aResolveValue);

      
      
      
      
      mResponseTarget = nullptr;
      mThisVal = nullptr;
    }

    virtual void DoReject(RejectValueType aRejectValue) MOZ_OVERRIDE
    {
      Consumer::mComplete = true;
      if (Consumer::mDisconnected) {
        PROMISE_LOG("ThenValue::DoReject disconnected - bailing out [this=%p]", this);
        
        mResponseTarget = nullptr;
        mThisVal = nullptr;
        return;
      }
      InvokeCallbackMethod(mThisVal.get(), mRejectMethod, aRejectValue);

      
      
      
      
      mResponseTarget = nullptr;
      mThisVal = nullptr;
    }

  private:
    nsRefPtr<TargetType> mResponseTarget;
    nsRefPtr<ThisType> mThisVal;
    ResolveMethodType mResolveMethod;
    RejectMethodType mRejectMethod;
  };
public:

  template<typename TargetType, typename ThisType,
           typename ResolveMethodType, typename RejectMethodType>
  already_AddRefed<Consumer> RefableThen(TargetType* aResponseTarget, const char* aCallSite, ThisType* aThisVal,
                                         ResolveMethodType aResolveMethod, RejectMethodType aRejectMethod)
  {
    MutexAutoLock lock(mMutex);
    MOZ_DIAGNOSTIC_ASSERT(!IsExclusive || !mHaveConsumer);
    mHaveConsumer = true;
    nsRefPtr<ThenValueBase> thenValue = new ThenValue<TargetType, ThisType, ResolveMethodType,
                                                      RejectMethodType>(aResponseTarget, aThisVal,
                                                                        aResolveMethod, aRejectMethod,
                                                                        aCallSite);
    PROMISE_LOG("%s invoking Then() [this=%p, thenValue=%p, aThisVal=%p, isPending=%d]",
                aCallSite, this, thenValue.get(), aThisVal, (int) IsPending());
    if (!IsPending()) {
      thenValue->Dispatch(this);
    } else {
      mThenValues.AppendElement(thenValue);
    }

    return thenValue.forget();
  }

  template<typename TargetType, typename ThisType,
           typename ResolveMethodType, typename RejectMethodType>
  void Then(TargetType* aResponseTarget, const char* aCallSite, ThisType* aThisVal,
            ResolveMethodType aResolveMethod, RejectMethodType aRejectMethod)
  {
    nsRefPtr<Consumer> c =
      RefableThen(aResponseTarget, aCallSite, aThisVal, aResolveMethod, aRejectMethod);
    return;
  }

  void ChainTo(already_AddRefed<MediaPromise> aChainedPromise, const char* aCallSite)
  {
    MutexAutoLock lock(mMutex);
    MOZ_DIAGNOSTIC_ASSERT(!IsExclusive || !mHaveConsumer);
    mHaveConsumer = true;
    nsRefPtr<MediaPromise> chainedPromise = aChainedPromise;
    PROMISE_LOG("%s invoking Chain() [this=%p, chainedPromise=%p, isPending=%d]",
                aCallSite, this, chainedPromise.get(), (int) IsPending());
    if (!IsPending()) {
      ForwardTo(chainedPromise);
    } else {
      mChainedPromises.AppendElement(chainedPromise);
    }
  }

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
    for (size_t i = 0; i < mThenValues.Length(); ++i) {
      mThenValues[i]->Dispatch(this);
    }
    mThenValues.Clear();

    for (size_t i = 0; i < mChainedPromises.Length(); ++i) {
      ForwardTo(mChainedPromises[i]);
    }
    mChainedPromises.Clear();
  }

  void ForwardTo(MediaPromise* aOther)
  {
    MOZ_ASSERT(!IsPending());
    if (mResolveValue.isSome()) {
      aOther->Resolve(mResolveValue.ref(), "<chained promise>");
    } else {
      aOther->Reject(mRejectValue.ref(), "<chained promise>");
    }
  }

  ~MediaPromise()
  {
    PROMISE_LOG("MediaPromise::~MediaPromise [this=%p]", this);
    MOZ_ASSERT(!IsPending());
    MOZ_ASSERT(mThenValues.IsEmpty());
    MOZ_ASSERT(mChainedPromises.IsEmpty());
  };

  const char* mCreationSite; 
  Mutex mMutex;
  Maybe<ResolveValueType> mResolveValue;
  Maybe<RejectValueType> mRejectValue;
  nsTArray<nsRefPtr<ThenValueBase>> mThenValues;
  nsTArray<nsRefPtr<MediaPromise>> mChainedPromises;
  bool mHaveConsumer;
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

  already_AddRefed<PromiseType> Steal()
  {
    if (mMonitor) {
      mMonitor->AssertCurrentThreadOwns();
    }

    nsRefPtr<PromiseType> p = mPromise;
    mPromise = nullptr;
    return p.forget();
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

  void ResolveIfExists(typename PromiseType::ResolveValueType aResolveValue,
                       const char* aMethodName)
  {
    if (!IsEmpty()) {
      Resolve(aResolveValue, aMethodName);
    }
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

  void RejectIfExists(typename PromiseType::RejectValueType aRejectValue,
                      const char* aMethodName)
  {
    if (!IsEmpty()) {
      Reject(aRejectValue, aMethodName);
    }
  }

private:
  Monitor* mMonitor;
  nsRefPtr<PromiseType> mPromise;
};





template<typename PromiseType>
class MediaPromiseConsumerHolder
{
public:
  MediaPromiseConsumerHolder() {}
  ~MediaPromiseConsumerHolder() { MOZ_ASSERT(!mConsumer); }

  void Begin(already_AddRefed<typename PromiseType::Consumer> aConsumer)
  {
    MOZ_DIAGNOSTIC_ASSERT(!Exists());
    mConsumer = aConsumer;
  }

  void Complete()
  {
    MOZ_DIAGNOSTIC_ASSERT(Exists());
    mConsumer = nullptr;
  }

  
  
  void Disconnect() {
    MOZ_ASSERT(Exists());
    mConsumer->Disconnect();
    mConsumer = nullptr;
  }

  void DisconnectIfExists() {
    if (Exists()) {
      Disconnect();
    }
  }

  bool Exists() { return !!mConsumer; }

private:
  nsRefPtr<typename PromiseType::Consumer> mConsumer;
};










namespace detail {

template<typename PromiseType>
class MethodCallBase
{
public:
  MethodCallBase() { MOZ_COUNT_CTOR(MethodCallBase); }
  virtual nsRefPtr<PromiseType> Invoke() = 0;
  virtual ~MethodCallBase() { MOZ_COUNT_DTOR(MethodCallBase); };
};

template<typename PromiseType, typename ThisType>
class MethodCallWithNoArgs : public MethodCallBase<PromiseType>
{
public:
  typedef nsRefPtr<PromiseType>(ThisType::*Type)();
  MethodCallWithNoArgs(ThisType* aThisVal, Type aMethod)
    : mThisVal(aThisVal), mMethod(aMethod) {}
  nsRefPtr<PromiseType> Invoke() MOZ_OVERRIDE { return ((*mThisVal).*mMethod)(); }
protected:
  nsRefPtr<ThisType> mThisVal;
  Type mMethod;
};



template<typename PromiseType, typename ThisType, typename Arg1Type, typename Arg2Type>
class MethodCallWithTwoArgs : public MethodCallBase<PromiseType>
{
public:
  typedef nsRefPtr<PromiseType>(ThisType::*Type)(Arg1Type, Arg2Type);
  MethodCallWithTwoArgs(ThisType* aThisVal, Type aMethod, Arg1Type aArg1, Arg2Type aArg2)
    : mThisVal(aThisVal), mMethod(aMethod), mArg1(aArg1), mArg2(aArg2) {}
  nsRefPtr<PromiseType> Invoke() MOZ_OVERRIDE { return ((*mThisVal).*mMethod)(mArg1, mArg2); }
protected:
  nsRefPtr<ThisType> mThisVal;
  Type mMethod;
  Arg1Type mArg1;
  Arg2Type mArg2;
};

template<typename PromiseType>
class ProxyRunnable : public nsRunnable
{
public:
  ProxyRunnable(PromiseType* aProxyPromise, MethodCallBase<PromiseType>* aMethodCall)
    : mProxyPromise(aProxyPromise), mMethodCall(aMethodCall) {}

  NS_IMETHODIMP Run()
  {
    nsRefPtr<PromiseType> p = mMethodCall->Invoke();
    mMethodCall = nullptr;
    p->ChainTo(mProxyPromise.forget(), "<Proxy Promise>");
    return NS_OK;
  }

private:
  nsRefPtr<PromiseType> mProxyPromise;
  nsAutoPtr<MethodCallBase<PromiseType>> mMethodCall;
};

template<typename PromiseType, typename TargetType>
static nsRefPtr<PromiseType>
ProxyInternal(TargetType* aTarget, MethodCallBase<PromiseType>* aMethodCall, const char* aCallerName)
{
  nsRefPtr<PromiseType> p = new PromiseType(aCallerName);
  nsRefPtr<ProxyRunnable<PromiseType>> r = new ProxyRunnable<PromiseType>(p, aMethodCall);
  nsresult rv = detail::DispatchMediaPromiseRunnable(aTarget, r);
  MOZ_DIAGNOSTIC_ASSERT(NS_SUCCEEDED(rv));
  return p;
}

} 

template<typename PromiseType, typename TargetType, typename ThisType>
static nsRefPtr<PromiseType>
ProxyMediaCall(TargetType* aTarget, ThisType* aThisVal, const char* aCallerName,
               nsRefPtr<PromiseType>(ThisType::*aMethod)())
{
  typedef detail::MethodCallWithNoArgs<PromiseType, ThisType> MethodCallType;
  MethodCallType* methodCall = new MethodCallType(aThisVal, aMethod);
  return detail::ProxyInternal(aTarget, methodCall, aCallerName);
}



template<typename PromiseType, typename TargetType, typename ThisType,
         typename Arg1Type, typename Arg2Type>
static nsRefPtr<PromiseType>
ProxyMediaCall(TargetType* aTarget, ThisType* aThisVal, const char* aCallerName,
               nsRefPtr<PromiseType>(ThisType::*aMethod)(Arg1Type, Arg2Type), Arg1Type aArg1, Arg2Type aArg2)
{
  typedef detail::MethodCallWithTwoArgs<PromiseType, ThisType, Arg1Type, Arg2Type> MethodCallType;
  MethodCallType* methodCall = new MethodCallType(aThisVal, aMethod, aArg1, aArg2);
  return detail::ProxyInternal(aTarget, methodCall, aCallerName);
}

#undef PROMISE_LOG

} 

#endif
