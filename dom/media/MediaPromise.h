





#if !defined(MediaPromise_h_)
#define MediaPromise_h_

#include "mozilla/Logging.h"

#include "AbstractThread.h"

#include "nsTArray.h"
#include "nsThreadUtils.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/Maybe.h"
#include "mozilla/Mutex.h"
#include "mozilla/Monitor.h"
#include "mozilla/unused.h"


#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

namespace mozilla {

extern PRLogModuleInfo* gMediaPromiseLog;

#define PROMISE_LOG(x, ...) \
  MOZ_ASSERT(gMediaPromiseLog); \
  MOZ_LOG(gMediaPromiseLog, PR_LOG_DEBUG, (x, ##__VA_ARGS__))

namespace detail {
template<typename ThisType, typename Ret, typename ArgType>
static TrueType TakesArgumentHelper(Ret (ThisType::*)(ArgType));
template<typename ThisType, typename Ret, typename ArgType>
static TrueType TakesArgumentHelper(Ret (ThisType::*)(ArgType) const);
template<typename ThisType, typename Ret>
static FalseType TakesArgumentHelper(Ret (ThisType::*)());
template<typename ThisType, typename Ret>
static FalseType TakesArgumentHelper(Ret (ThisType::*)() const);
} 

template<typename MethodType>
struct TakesArgument {
  typedef decltype(detail::TakesArgumentHelper(DeclVal<MethodType>())) Type;
  static const bool value = Type::value;
};











class MediaPromiseBase
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaPromiseBase)
protected:
  virtual ~MediaPromiseBase() {}
};

template<typename T> class MediaPromiseHolder;
template<typename ResolveValueT, typename RejectValueT, bool IsExclusive>
class MediaPromise : public MediaPromiseBase
{
public:
  typedef ResolveValueT ResolveValueType;
  typedef RejectValueT RejectValueType;
  class ResolveOrRejectValue
  {
  public:
    void SetResolve(const ResolveValueType& aResolveValue)
    {
      MOZ_ASSERT(IsNothing());
      mResolveValue.emplace(aResolveValue);
    }

    void SetReject(const RejectValueType& aRejectValue)
    {
      MOZ_ASSERT(IsNothing());
      mRejectValue.emplace(aRejectValue);
    }

    static ResolveOrRejectValue MakeResolve(const ResolveValueType aResolveValue)
    {
      ResolveOrRejectValue val;
      val.SetResolve(aResolveValue);
      return val;
    }

    static ResolveOrRejectValue MakeReject(const RejectValueType aRejectValue)
    {
      ResolveOrRejectValue val;
      val.SetReject(aRejectValue);
      return val;
    }

    bool IsResolve() const { return mResolveValue.isSome(); }
    bool IsReject() const { return mRejectValue.isSome(); }
    bool IsNothing() const { return mResolveValue.isNothing() && mRejectValue.isNothing(); }
    ResolveValueType& ResolveValue() { return mResolveValue.ref(); }
    RejectValueType& RejectValue() { return mRejectValue.ref(); }

  private:
    Maybe<ResolveValueType> mResolveValue;
    Maybe<RejectValueType> mRejectValue;
  };

protected:
  
  
  explicit MediaPromise(const char* aCreationSite)
    : mCreationSite(aCreationSite)
    , mMutex("MediaPromise Mutex")
    , mHaveConsumer(false)
  {
    PROMISE_LOG("%s creating MediaPromise (%p)", mCreationSite, this);
  }

public:
  
  
  
  
  
  
  
  class Private;

  static nsRefPtr<MediaPromise>
  CreateAndResolve(ResolveValueType aResolveValue, const char* aResolveSite)
  {
    nsRefPtr<typename MediaPromise::Private> p = new MediaPromise::Private(aResolveSite);
    p->Resolve(aResolveValue, aResolveSite);
    return Move(p);
  }

  static nsRefPtr<MediaPromise>
  CreateAndReject(RejectValueType aRejectValue, const char* aRejectSite)
  {
    nsRefPtr<typename MediaPromise::Private> p = new MediaPromise::Private(aRejectSite);
    p->Reject(aRejectValue, aRejectSite);
    return Move(p);
  }

  class Consumer
  {
  public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Consumer)

    virtual void Disconnect() = 0;

    
    
    bool IsDisconnected() const { return mDisconnected; }

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
    class ResolveOrRejectRunnable : public nsRunnable
    {
    public:
      ResolveOrRejectRunnable(ThenValueBase* aThenValue, ResolveOrRejectValue& aValue)
        : mThenValue(aThenValue)
        , mValue(aValue) {}

      ~ResolveOrRejectRunnable()
      {
        MOZ_DIAGNOSTIC_ASSERT(!mThenValue || mThenValue->IsDisconnected());
      }

      NS_IMETHODIMP Run()
      {
        PROMISE_LOG("ResolveOrRejectRunnable::Run() [this=%p]", this);
        mThenValue->DoResolveOrReject(mValue);
        mThenValue = nullptr;
        return NS_OK;
      }

    private:
      nsRefPtr<ThenValueBase> mThenValue;
      ResolveOrRejectValue mValue;
    };

    explicit ThenValueBase(AbstractThread* aResponseTarget, const char* aCallSite)
      : mResponseTarget(aResponseTarget), mCallSite(aCallSite) {}

    void Dispatch(MediaPromise *aPromise)
    {
      aPromise->mMutex.AssertCurrentThreadOwns();
      MOZ_ASSERT(!aPromise->IsPending());

      nsRefPtr<nsRunnable> runnable =
        static_cast<nsRunnable*>(new (typename ThenValueBase::ResolveOrRejectRunnable)(this, aPromise->mValue));
      PROMISE_LOG("%s Then() call made from %s [Runnable=%p, Promise=%p, ThenValue=%p]",
                  aPromise->mValue.IsResolve() ? "Resolving" : "Rejecting", ThenValueBase::mCallSite,
                  runnable.get(), aPromise, this);

      
      
      
      
      mResponseTarget->Dispatch(runnable.forget(), AbstractThread::DontAssertDispatchSuccess);
    }

    virtual void Disconnect() override
    {
      MOZ_ASSERT(ThenValueBase::mResponseTarget->IsCurrentThreadIn());
      MOZ_DIAGNOSTIC_ASSERT(!Consumer::mComplete);
      Consumer::mDisconnected = true;
    }

  protected:
    virtual void DoResolveOrRejectInternal(ResolveOrRejectValue& aValue) = 0;

    void DoResolveOrReject(ResolveOrRejectValue& aValue)
    {
      Consumer::mComplete = true;
      if (Consumer::mDisconnected) {
        PROMISE_LOG("ThenValue::DoResolveOrReject disconnected - bailing out [this=%p]", this);
        return;
      }

      DoResolveOrRejectInternal(aValue);
    }

    nsRefPtr<AbstractThread> mResponseTarget; 
    const char* mCallSite;
  };

  




  template<typename ThisType, typename MethodType, typename ValueType>
  static typename EnableIf<TakesArgument<MethodType>::value, void>::Type
  InvokeCallbackMethod(ThisType* aThisVal, MethodType aMethod, ValueType aValue)
  {
      ((*aThisVal).*aMethod)(aValue);
  }

  template<typename ThisType, typename MethodType, typename ValueType>
  static typename EnableIf<!TakesArgument<MethodType>::value, void>::Type
  InvokeCallbackMethod(ThisType* aThisVal, MethodType aMethod, ValueType aValue)
  {
      ((*aThisVal).*aMethod)();
  }

  template<typename ThisType, typename ResolveMethodType, typename RejectMethodType>
  class MethodThenValue : public ThenValueBase
  {
  public:
    MethodThenValue(AbstractThread* aResponseTarget, ThisType* aThisVal,
                    ResolveMethodType aResolveMethod, RejectMethodType aRejectMethod,
                    const char* aCallSite)
      : ThenValueBase(aResponseTarget, aCallSite)
      , mThisVal(aThisVal)
      , mResolveMethod(aResolveMethod)
      , mRejectMethod(aRejectMethod) {}

  virtual void Disconnect() override
  {
    ThenValueBase::Disconnect();

    
    
    
    mThisVal = nullptr;
  }

  protected:
    virtual void DoResolveOrRejectInternal(ResolveOrRejectValue& aValue) override
    {
      if (aValue.IsResolve()) {
        InvokeCallbackMethod(mThisVal.get(), mResolveMethod, aValue.ResolveValue());
      } else {
        InvokeCallbackMethod(mThisVal.get(), mRejectMethod, aValue.RejectValue());
      }

      
      
      
      
      mThisVal = nullptr;
    }

  private:
    nsRefPtr<ThisType> mThisVal; 
    ResolveMethodType mResolveMethod;
    RejectMethodType mRejectMethod;
  };

  
  template<typename ResolveFunction, typename RejectFunction>
  class FunctionThenValue : public ThenValueBase
  {
  public:
    FunctionThenValue(AbstractThread* aResponseTarget,
                      ResolveFunction&& aResolveFunction,
                      RejectFunction&& aRejectFunction,
                      const char* aCallSite)
      : ThenValueBase(aResponseTarget, aCallSite)
    {
      mResolveFunction.emplace(Move(aResolveFunction));
      mRejectFunction.emplace(Move(aRejectFunction));
    }

  virtual void Disconnect() override
  {
    ThenValueBase::Disconnect();

    
    
    
    
    mResolveFunction.reset();
    mRejectFunction.reset();
  }

  protected:
    virtual void DoResolveOrRejectInternal(ResolveOrRejectValue& aValue) override
    {
      
      
      
      
      
      if (aValue.IsResolve()) {
        InvokeCallbackMethod(mResolveFunction.ptr(), &ResolveFunction::operator(), aValue.ResolveValue());
      } else {
        InvokeCallbackMethod(mRejectFunction.ptr(), &RejectFunction::operator(), aValue.RejectValue());
      }

      
      
      
      
      mResolveFunction.reset();
      mRejectFunction.reset();
    }

  private:
    Maybe<ResolveFunction> mResolveFunction; 
    Maybe<RejectFunction> mRejectFunction; 
  };

public:
  void ThenInternal(AbstractThread* aResponseThread, ThenValueBase* aThenValue,
                    const char* aCallSite)
  {
    MutexAutoLock lock(mMutex);
    MOZ_ASSERT(aResponseThread->IsDispatchReliable());
    MOZ_DIAGNOSTIC_ASSERT(!IsExclusive || !mHaveConsumer);
    mHaveConsumer = true;
    PROMISE_LOG("%s invoking Then() [this=%p, aThenValue=%p, isPending=%d]",
                aCallSite, this, aThenValue, (int) IsPending());
    if (!IsPending()) {
      aThenValue->Dispatch(this);
    } else {
      mThenValues.AppendElement(aThenValue);
    }
  }

public:

  template<typename ThisType, typename ResolveMethodType, typename RejectMethodType>
  nsRefPtr<Consumer> Then(AbstractThread* aResponseThread, const char* aCallSite, ThisType* aThisVal,
                          ResolveMethodType aResolveMethod, RejectMethodType aRejectMethod)
  {
    nsRefPtr<ThenValueBase> thenValue = new MethodThenValue<ThisType, ResolveMethodType, RejectMethodType>(
                                              aResponseThread, aThisVal, aResolveMethod, aRejectMethod, aCallSite);
    ThenInternal(aResponseThread, thenValue, aCallSite);
    return thenValue.forget(); 
  }

  template<typename ResolveFunction, typename RejectFunction>
  nsRefPtr<Consumer> Then(AbstractThread* aResponseThread, const char* aCallSite,
                          ResolveFunction&& aResolveFunction, RejectFunction&& aRejectFunction)
  {
    nsRefPtr<ThenValueBase> thenValue = new FunctionThenValue<ResolveFunction, RejectFunction>(aResponseThread,
                                              Move(aResolveFunction), Move(aRejectFunction), aCallSite);
    ThenInternal(aResponseThread, thenValue, aCallSite);
    return thenValue.forget(); 
  }

  void ChainTo(already_AddRefed<Private> aChainedPromise, const char* aCallSite)
  {
    MutexAutoLock lock(mMutex);
    MOZ_DIAGNOSTIC_ASSERT(!IsExclusive || !mHaveConsumer);
    mHaveConsumer = true;
    nsRefPtr<Private> chainedPromise = aChainedPromise;
    PROMISE_LOG("%s invoking Chain() [this=%p, chainedPromise=%p, isPending=%d]",
                aCallSite, this, chainedPromise.get(), (int) IsPending());
    if (!IsPending()) {
      ForwardTo(chainedPromise);
    } else {
      mChainedPromises.AppendElement(chainedPromise);
    }
  }

protected:
  bool IsPending() { return mValue.IsNothing(); }
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

  void ForwardTo(Private* aOther)
  {
    MOZ_ASSERT(!IsPending());
    if (mValue.IsResolve()) {
      aOther->Resolve(mValue.ResolveValue(), "<chained promise>");
    } else {
      aOther->Reject(mValue.RejectValue(), "<chained promise>");
    }
  }

  virtual ~MediaPromise()
  {
    PROMISE_LOG("MediaPromise::~MediaPromise [this=%p]", this);
    MOZ_ASSERT(!IsPending());
    MOZ_ASSERT(mThenValues.IsEmpty());
    MOZ_ASSERT(mChainedPromises.IsEmpty());
  };

  const char* mCreationSite; 
  Mutex mMutex;
  ResolveOrRejectValue mValue;
  nsTArray<nsRefPtr<ThenValueBase>> mThenValues;
  nsTArray<nsRefPtr<Private>> mChainedPromises;
  bool mHaveConsumer;
};

template<typename ResolveValueT, typename RejectValueT, bool IsExclusive>
class MediaPromise<ResolveValueT, RejectValueT, IsExclusive>::Private
  : public MediaPromise<ResolveValueT, RejectValueT, IsExclusive>
{
public:
  explicit Private(const char* aCreationSite) : MediaPromise(aCreationSite) {}

  void Resolve(ResolveValueT aResolveValue, const char* aResolveSite)
  {
    MutexAutoLock lock(mMutex);
    MOZ_ASSERT(IsPending());
    PROMISE_LOG("%s resolving MediaPromise (%p created at %s)", aResolveSite, this, mCreationSite);
    mValue.SetResolve(aResolveValue);
    DispatchAll();
  }

  void Reject(RejectValueT aRejectValue, const char* aRejectSite)
  {
    MutexAutoLock lock(mMutex);
    MOZ_ASSERT(IsPending());
    PROMISE_LOG("%s rejecting MediaPromise (%p created at %s)", aRejectSite, this, mCreationSite);
    mValue.SetReject(aRejectValue);
    DispatchAll();
  }
};





template<typename PromiseType>
class MediaPromiseHolder
{
public:
  MediaPromiseHolder()
    : mMonitor(nullptr) {}

  
  MediaPromiseHolder& operator=(MediaPromiseHolder&& aOther)
  {
    MOZ_ASSERT(!mMonitor && !aOther.mMonitor);
    MOZ_DIAGNOSTIC_ASSERT(!mPromise);
    mPromise = aOther.mPromise;
    aOther.mPromise = nullptr;
    return *this;
  }

  ~MediaPromiseHolder() { MOZ_ASSERT(!mPromise); }

  already_AddRefed<PromiseType> Ensure(const char* aMethodName) {
    if (mMonitor) {
      mMonitor->AssertCurrentThreadOwns();
    }
    if (!mPromise) {
      mPromise = new (typename PromiseType::Private)(aMethodName);
    }
    nsRefPtr<PromiseType> p = mPromise.get();
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

  already_AddRefed<typename PromiseType::Private> Steal()
  {
    if (mMonitor) {
      mMonitor->AssertCurrentThreadOwns();
    }

    nsRefPtr<typename PromiseType::Private> p = mPromise;
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
  nsRefPtr<typename PromiseType::Private> mPromise;
};





template<typename PromiseType>
class MediaPromiseConsumerHolder
{
public:
  MediaPromiseConsumerHolder() {}
  ~MediaPromiseConsumerHolder() { MOZ_ASSERT(!mConsumer); }

  void Begin(typename PromiseType::Consumer* aConsumer)
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
  nsRefPtr<PromiseType> Invoke() override { return ((*mThisVal).*mMethod)(); }
protected:
  nsRefPtr<ThisType> mThisVal;
  Type mMethod;
};

template<typename PromiseType, typename ThisType, typename Arg1Type>
class MethodCallWithOneArg : public MethodCallBase<PromiseType>
{
public:
  typedef nsRefPtr<PromiseType>(ThisType::*Type)(Arg1Type);
  MethodCallWithOneArg(ThisType* aThisVal, Type aMethod, Arg1Type aArg1)
    : mThisVal(aThisVal), mMethod(aMethod), mArg1(aArg1) {}
  nsRefPtr<PromiseType> Invoke() override { return ((*mThisVal).*mMethod)(mArg1); }
protected:
  nsRefPtr<ThisType> mThisVal;
  Type mMethod;
  Arg1Type mArg1;
};

template<typename PromiseType, typename ThisType, typename Arg1Type, typename Arg2Type>
class MethodCallWithTwoArgs : public MethodCallBase<PromiseType>
{
public:
  typedef nsRefPtr<PromiseType>(ThisType::*Type)(Arg1Type, Arg2Type);
  MethodCallWithTwoArgs(ThisType* aThisVal, Type aMethod, Arg1Type aArg1, Arg2Type aArg2)
    : mThisVal(aThisVal), mMethod(aMethod), mArg1(aArg1), mArg2(aArg2) {}
  nsRefPtr<PromiseType> Invoke() override { return ((*mThisVal).*mMethod)(mArg1, mArg2); }
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
  ProxyRunnable(typename PromiseType::Private* aProxyPromise, MethodCallBase<PromiseType>* aMethodCall)
    : mProxyPromise(aProxyPromise), mMethodCall(aMethodCall) {}

  NS_IMETHODIMP Run()
  {
    nsRefPtr<PromiseType> p = mMethodCall->Invoke();
    mMethodCall = nullptr;
    p->ChainTo(mProxyPromise.forget(), "<Proxy Promise>");
    return NS_OK;
  }

private:
  nsRefPtr<typename PromiseType::Private> mProxyPromise;
  nsAutoPtr<MethodCallBase<PromiseType>> mMethodCall;
};

template<typename PromiseType>
static nsRefPtr<PromiseType>
ProxyInternal(AbstractThread* aTarget, MethodCallBase<PromiseType>* aMethodCall, const char* aCallerName)
{
  nsRefPtr<typename PromiseType::Private> p = new (typename PromiseType::Private)(aCallerName);
  nsRefPtr<ProxyRunnable<PromiseType>> r = new ProxyRunnable<PromiseType>(p, aMethodCall);
  MOZ_ASSERT(aTarget->IsDispatchReliable());
  aTarget->Dispatch(r.forget());
  return Move(p);
}

} 

template<typename PromiseType, typename ThisType>
static nsRefPtr<PromiseType>
ProxyMediaCall(AbstractThread* aTarget, ThisType* aThisVal, const char* aCallerName,
               nsRefPtr<PromiseType>(ThisType::*aMethod)())
{
  typedef detail::MethodCallWithNoArgs<PromiseType, ThisType> MethodCallType;
  MethodCallType* methodCall = new MethodCallType(aThisVal, aMethod);
  return detail::ProxyInternal(aTarget, methodCall, aCallerName);
}

template<typename PromiseType, typename ThisType, typename Arg1Type>
static nsRefPtr<PromiseType>
ProxyMediaCall(AbstractThread* aTarget, ThisType* aThisVal, const char* aCallerName,
               nsRefPtr<PromiseType>(ThisType::*aMethod)(Arg1Type), Arg1Type aArg1)
{
  typedef detail::MethodCallWithOneArg<PromiseType, ThisType, Arg1Type> MethodCallType;
  MethodCallType* methodCall = new MethodCallType(aThisVal, aMethod, aArg1);
  return detail::ProxyInternal(aTarget, methodCall, aCallerName);
}

template<typename PromiseType, typename ThisType, typename Arg1Type, typename Arg2Type>
static nsRefPtr<PromiseType>
ProxyMediaCall(AbstractThread* aTarget, ThisType* aThisVal, const char* aCallerName,
               nsRefPtr<PromiseType>(ThisType::*aMethod)(Arg1Type, Arg2Type), Arg1Type aArg1, Arg2Type aArg2)
{
  typedef detail::MethodCallWithTwoArgs<PromiseType, ThisType, Arg1Type, Arg2Type> MethodCallType;
  MethodCallType* methodCall = new MethodCallType(aThisVal, aMethod, aArg1, aArg2);
  return detail::ProxyInternal(aTarget, methodCall, aCallerName);
}

#undef PROMISE_LOG

} 

#endif
