





#if !defined(MozPromise_h_)
#define MozPromise_h_

#include "mozilla/AbstractThread.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Logging.h"
#include "mozilla/Maybe.h"
#include "mozilla/Mutex.h"
#include "mozilla/Monitor.h"
#include "mozilla/unused.h"

#include "nsTArray.h"
#include "nsThreadUtils.h"


#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

namespace mozilla {

extern PRLogModuleInfo* gMozPromiseLog;

#define PROMISE_LOG(x, ...) \
  MOZ_ASSERT(gMozPromiseLog); \
  MOZ_LOG(gMozPromiseLog, mozilla::LogLevel::Debug, (x, ##__VA_ARGS__))

namespace detail {
template<typename ThisType, typename Ret, typename ArgType>
static TrueType TakesArgumentHelper(Ret (ThisType::*)(ArgType));
template<typename ThisType, typename Ret, typename ArgType>
static TrueType TakesArgumentHelper(Ret (ThisType::*)(ArgType) const);
template<typename ThisType, typename Ret>
static FalseType TakesArgumentHelper(Ret (ThisType::*)());
template<typename ThisType, typename Ret>
static FalseType TakesArgumentHelper(Ret (ThisType::*)() const);

template<typename ThisType, typename Ret, typename ArgType>
static Ret ReturnTypeHelper(Ret (ThisType::*)(ArgType));
template<typename ThisType, typename Ret, typename ArgType>
static Ret ReturnTypeHelper(Ret (ThisType::*)(ArgType) const);
template<typename ThisType, typename Ret>
static Ret ReturnTypeHelper(Ret (ThisType::*)());
template<typename ThisType, typename Ret>
static Ret ReturnTypeHelper(Ret (ThisType::*)() const);

template<typename MethodType>
struct ReturnType {
  typedef decltype(detail::ReturnTypeHelper(DeclVal<MethodType>())) Type;
};

} 

template<typename MethodType>
struct TakesArgument {
  static const bool value = decltype(detail::TakesArgumentHelper(DeclVal<MethodType>()))::value;
};

template<typename MethodType, typename TargetType>
struct ReturnTypeIs {
  static const bool value = IsConvertible<typename detail::ReturnType<MethodType>::Type, TargetType>::value;
};







































class MozPromiseRefcountable
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MozPromiseRefcountable)
protected:
  virtual ~MozPromiseRefcountable() {}
};

template<typename T> class MozPromiseHolder;
template<typename ResolveValueT, typename RejectValueT, bool IsExclusive>
class MozPromise : public MozPromiseRefcountable
{
public:
  typedef ResolveValueT ResolveValueType;
  typedef RejectValueT RejectValueType;
  class ResolveOrRejectValue
  {
  public:
    template<typename ResolveValueType_>
    void SetResolve(ResolveValueType_&& aResolveValue)
    {
      MOZ_ASSERT(IsNothing());
      mResolveValue.emplace(Forward<ResolveValueType_>(aResolveValue));
    }

    template<typename RejectValueType_>
    void SetReject(RejectValueType_&& aRejectValue)
    {
      MOZ_ASSERT(IsNothing());
      mRejectValue.emplace(Forward<RejectValueType_>(aRejectValue));
    }

    template<typename ResolveValueType_>
    static ResolveOrRejectValue MakeResolve(ResolveValueType_&& aResolveValue)
    {
      ResolveOrRejectValue val;
      val.SetResolve(Forward<ResolveValueType_>(aResolveValue));
      return val;
    }

    template<typename RejectValueType_>
    static ResolveOrRejectValue MakeReject(RejectValueType_&& aRejectValue)
    {
      ResolveOrRejectValue val;
      val.SetReject(Forward<RejectValueType_>(aRejectValue));
      return val;
    }

    bool IsResolve() const { return mResolveValue.isSome(); }
    bool IsReject() const { return mRejectValue.isSome(); }
    bool IsNothing() const { return mResolveValue.isNothing() && mRejectValue.isNothing(); }

    const ResolveValueType& ResolveValue() const { return mResolveValue.ref(); }
    const RejectValueType& RejectValue() const { return mRejectValue.ref(); }

  private:
    Maybe<ResolveValueType> mResolveValue;
    Maybe<RejectValueType> mRejectValue;
  };

protected:
  
  
  explicit MozPromise(const char* aCreationSite)
    : mCreationSite(aCreationSite)
    , mMutex("MozPromise Mutex")
    , mHaveRequest(false)
  {
    PROMISE_LOG("%s creating MozPromise (%p)", mCreationSite, this);
  }

public:
  
  
  
  
  
  
  
  class Private;

  template<typename ResolveValueType_>
  static nsRefPtr<MozPromise>
  CreateAndResolve(ResolveValueType_&& aResolveValue, const char* aResolveSite)
  {
    nsRefPtr<typename MozPromise::Private> p = new MozPromise::Private(aResolveSite);
    p->Resolve(Forward<ResolveValueType_>(aResolveValue), aResolveSite);
    return p.forget();
  }

  template<typename RejectValueType_>
  static nsRefPtr<MozPromise>
  CreateAndReject(RejectValueType_&& aRejectValue, const char* aRejectSite)
  {
    nsRefPtr<typename MozPromise::Private> p = new MozPromise::Private(aRejectSite);
    p->Reject(Forward<RejectValueType_>(aRejectValue), aRejectSite);
    return p.forget();
  }

  typedef MozPromise<nsTArray<ResolveValueType>, RejectValueType, IsExclusive> AllPromiseType;
private:
  class AllPromiseHolder : public MozPromiseRefcountable
  {
  public:
    explicit AllPromiseHolder(size_t aDependentPromises)
      : mPromise(new typename AllPromiseType::Private(__func__))
      , mOutstandingPromises(aDependentPromises)
    {
      mResolveValues.SetLength(aDependentPromises);
    }

    void Resolve(size_t aIndex, const ResolveValueType& aResolveValue)
    {
      if (!mPromise) {
        
        return;
      }

      mResolveValues[aIndex].emplace(aResolveValue);
      if (--mOutstandingPromises == 0) {
        nsTArray<ResolveValueType> resolveValues;
        resolveValues.SetCapacity(mResolveValues.Length());
        for (size_t i = 0; i < mResolveValues.Length(); ++i) {
          resolveValues.AppendElement(mResolveValues[i].ref());
        }

        mPromise->Resolve(resolveValues, __func__);
        mPromise = nullptr;
        mResolveValues.Clear();
      }
    }

    void Reject(const RejectValueType& aRejectValue)
    {
      mPromise->Reject(aRejectValue, __func__);
      mPromise = nullptr;
      mResolveValues.Clear();
    }

    AllPromiseType* Promise() { return mPromise; }

  private:
    nsTArray<Maybe<ResolveValueType>> mResolveValues;
    nsRefPtr<typename AllPromiseType::Private> mPromise;
    size_t mOutstandingPromises;
  };
public:

  static nsRefPtr<AllPromiseType> All(AbstractThread* aProcessingThread, nsTArray<nsRefPtr<MozPromise>>& aPromises)
  {
    nsRefPtr<AllPromiseHolder> holder = new AllPromiseHolder(aPromises.Length());
    for (size_t i = 0; i < aPromises.Length(); ++i) {
      aPromises[i]->Then(aProcessingThread, __func__,
        [holder, i] (ResolveValueType aResolveValue) -> void { holder->Resolve(i, aResolveValue); },
        [holder] (RejectValueType aRejectValue) -> void { holder->Reject(aRejectValue); }
      );
    }
    return holder->Promise();
  }

  class Request : public MozPromiseRefcountable
  {
  public:
    virtual void Disconnect() = 0;

    
    
    bool IsDisconnected() const { return mDisconnected; }

    virtual MozPromise* CompletionPromise() = 0;

  protected:
    Request() : mComplete(false), mDisconnected(false) {}
    virtual ~Request() {}

    bool mComplete;
    bool mDisconnected;
  };

protected:

  





  class ThenValueBase : public Request
  {
  public:
    class ResolveOrRejectRunnable : public nsRunnable
    {
    public:
      ResolveOrRejectRunnable(ThenValueBase* aThenValue, MozPromise* aPromise)
        : mThenValue(aThenValue)
        , mPromise(aPromise)
      {
        MOZ_DIAGNOSTIC_ASSERT(!mPromise->IsPending());
      }

      ~ResolveOrRejectRunnable()
      {
        MOZ_DIAGNOSTIC_ASSERT(!mThenValue || mThenValue->IsDisconnected());
      }

      NS_IMETHODIMP Run()
      {
        PROMISE_LOG("ResolveOrRejectRunnable::Run() [this=%p]", this);
        mThenValue->DoResolveOrReject(mPromise->Value());
        mThenValue = nullptr;
        mPromise = nullptr;
        return NS_OK;
      }

    private:
      nsRefPtr<ThenValueBase> mThenValue;
      nsRefPtr<MozPromise> mPromise;
    };

    explicit ThenValueBase(AbstractThread* aResponseTarget, const char* aCallSite)
      : mResponseTarget(aResponseTarget), mCallSite(aCallSite) {}

    MozPromise* CompletionPromise() override
    {
      MOZ_DIAGNOSTIC_ASSERT(mResponseTarget->IsCurrentThreadIn());
      MOZ_DIAGNOSTIC_ASSERT(!Request::mComplete);
      if (!mCompletionPromise) {
        mCompletionPromise = new MozPromise::Private("<completion promise>");
      }
      return mCompletionPromise;
    }

    void Dispatch(MozPromise *aPromise)
    {
      aPromise->mMutex.AssertCurrentThreadOwns();
      MOZ_ASSERT(!aPromise->IsPending());

      nsRefPtr<nsRunnable> runnable =
        static_cast<nsRunnable*>(new (typename ThenValueBase::ResolveOrRejectRunnable)(this, aPromise));
      PROMISE_LOG("%s Then() call made from %s [Runnable=%p, Promise=%p, ThenValue=%p]",
                  aPromise->mValue.IsResolve() ? "Resolving" : "Rejecting", ThenValueBase::mCallSite,
                  runnable.get(), aPromise, this);

      
      
      
      
      mResponseTarget->Dispatch(runnable.forget(), AbstractThread::DontAssertDispatchSuccess);
    }

    virtual void Disconnect() override
    {
      MOZ_ASSERT(ThenValueBase::mResponseTarget->IsCurrentThreadIn());
      MOZ_DIAGNOSTIC_ASSERT(!Request::mComplete);
      Request::mDisconnected = true;

      
      
      
      
      MOZ_DIAGNOSTIC_ASSERT(!mCompletionPromise);
    }

  protected:
    virtual already_AddRefed<MozPromise> DoResolveOrRejectInternal(const ResolveOrRejectValue& aValue) = 0;

    void DoResolveOrReject(const ResolveOrRejectValue& aValue)
    {
      Request::mComplete = true;
      if (Request::mDisconnected) {
        PROMISE_LOG("ThenValue::DoResolveOrReject disconnected - bailing out [this=%p]", this);
        return;
      }

      
      nsRefPtr<MozPromise> p = DoResolveOrRejectInternal(aValue);

      
      
      
      
      
      
      nsRefPtr<MozPromise::Private> completionPromise =
        dont_AddRef(static_cast<MozPromise::Private*>(mCompletionPromise.forget().take()));
      if (completionPromise) {
        if (p) {
          p->ChainTo(completionPromise.forget(), "<chained completion promise>");
        } else {
          completionPromise->ResolveOrReject(aValue, "<completion of non-promise-returning method>");
        }
      }
    }

    nsRefPtr<AbstractThread> mResponseTarget; 

    
    
    
    
    nsRefPtr<MozPromise> mCompletionPromise;

    const char* mCallSite;
  };

  




  template<typename ThisType, typename MethodType, typename ValueType>
  static typename EnableIf<ReturnTypeIs<MethodType, nsRefPtr<MozPromise>>::value &&
                           TakesArgument<MethodType>::value,
                           already_AddRefed<MozPromise>>::Type
  InvokeCallbackMethod(ThisType* aThisVal, MethodType aMethod, ValueType&& aValue)
  {
    return ((*aThisVal).*aMethod)(Forward<ValueType>(aValue)).forget();
  }

  template<typename ThisType, typename MethodType, typename ValueType>
  static typename EnableIf<ReturnTypeIs<MethodType, void>::value &&
                           TakesArgument<MethodType>::value,
                           already_AddRefed<MozPromise>>::Type
  InvokeCallbackMethod(ThisType* aThisVal, MethodType aMethod, ValueType&& aValue)
  {
    ((*aThisVal).*aMethod)(Forward<ValueType>(aValue));
    return nullptr;
  }

  template<typename ThisType, typename MethodType, typename ValueType>
  static typename EnableIf<ReturnTypeIs<MethodType, nsRefPtr<MozPromise>>::value &&
                           !TakesArgument<MethodType>::value,
                           already_AddRefed<MozPromise>>::Type
  InvokeCallbackMethod(ThisType* aThisVal, MethodType aMethod, ValueType&& aValue)
  {
    return ((*aThisVal).*aMethod)().forget();
  }

  template<typename ThisType, typename MethodType, typename ValueType>
  static typename EnableIf<ReturnTypeIs<MethodType, void>::value &&
                           !TakesArgument<MethodType>::value,
                           already_AddRefed<MozPromise>>::Type
  InvokeCallbackMethod(ThisType* aThisVal, MethodType aMethod, ValueType&& aValue)
  {
    ((*aThisVal).*aMethod)();
    return nullptr;
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
    virtual already_AddRefed<MozPromise> DoResolveOrRejectInternal(const ResolveOrRejectValue& aValue) override
    {
      nsRefPtr<MozPromise> completion;
      if (aValue.IsResolve()) {
        completion = InvokeCallbackMethod(mThisVal.get(), mResolveMethod, aValue.ResolveValue());
      } else {
        completion = InvokeCallbackMethod(mThisVal.get(), mRejectMethod, aValue.RejectValue());
      }

      
      
      
      
      mThisVal = nullptr;

      return completion.forget();
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
    virtual already_AddRefed<MozPromise> DoResolveOrRejectInternal(const ResolveOrRejectValue& aValue) override
    {
      
      
      
      
      
      nsRefPtr<MozPromise> completion;
      if (aValue.IsResolve()) {
        completion = InvokeCallbackMethod(mResolveFunction.ptr(), &ResolveFunction::operator(), aValue.ResolveValue());
      } else {
        completion = InvokeCallbackMethod(mRejectFunction.ptr(), &RejectFunction::operator(), aValue.RejectValue());
      }

      
      
      
      
      mResolveFunction.reset();
      mRejectFunction.reset();

      return completion.forget();
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
    MOZ_DIAGNOSTIC_ASSERT(!IsExclusive || !mHaveRequest);
    mHaveRequest = true;
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
  nsRefPtr<Request> Then(AbstractThread* aResponseThread, const char* aCallSite, ThisType* aThisVal,
                         ResolveMethodType aResolveMethod, RejectMethodType aRejectMethod)
  {
    nsRefPtr<ThenValueBase> thenValue = new MethodThenValue<ThisType, ResolveMethodType, RejectMethodType>(
                                              aResponseThread, aThisVal, aResolveMethod, aRejectMethod, aCallSite);
    ThenInternal(aResponseThread, thenValue, aCallSite);
    return thenValue.forget(); 
  }

  template<typename ResolveFunction, typename RejectFunction>
  nsRefPtr<Request> Then(AbstractThread* aResponseThread, const char* aCallSite,
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
    MOZ_DIAGNOSTIC_ASSERT(!IsExclusive || !mHaveRequest);
    mHaveRequest = true;
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
  bool IsPending() const { return mValue.IsNothing(); }
  const ResolveOrRejectValue& Value() const
  {
    
    
    MOZ_DIAGNOSTIC_ASSERT(!IsPending());
    return mValue;
  }

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

  virtual ~MozPromise()
  {
    PROMISE_LOG("MozPromise::~MozPromise [this=%p]", this);
    MOZ_ASSERT(!IsPending());
    MOZ_ASSERT(mThenValues.IsEmpty());
    MOZ_ASSERT(mChainedPromises.IsEmpty());
  };

  const char* mCreationSite; 
  Mutex mMutex;
  ResolveOrRejectValue mValue;
  nsTArray<nsRefPtr<ThenValueBase>> mThenValues;
  nsTArray<nsRefPtr<Private>> mChainedPromises;
  bool mHaveRequest;
};

template<typename ResolveValueT, typename RejectValueT, bool IsExclusive>
class MozPromise<ResolveValueT, RejectValueT, IsExclusive>::Private
  : public MozPromise<ResolveValueT, RejectValueT, IsExclusive>
{
public:
  explicit Private(const char* aCreationSite) : MozPromise(aCreationSite) {}

  template<typename ResolveValueT_>
  void Resolve(ResolveValueT_&& aResolveValue, const char* aResolveSite)
  {
    MutexAutoLock lock(mMutex);
    MOZ_ASSERT(IsPending());
    PROMISE_LOG("%s resolving MozPromise (%p created at %s)", aResolveSite, this, mCreationSite);
    mValue.SetResolve(Forward<ResolveValueT_>(aResolveValue));
    DispatchAll();
  }

  template<typename RejectValueT_>
  void Reject(RejectValueT_&& aRejectValue, const char* aRejectSite)
  {
    MutexAutoLock lock(mMutex);
    MOZ_ASSERT(IsPending());
    PROMISE_LOG("%s rejecting MozPromise (%p created at %s)", aRejectSite, this, mCreationSite);
    mValue.SetReject(Forward<RejectValueT_>(aRejectValue));
    DispatchAll();
  }

  template<typename ResolveOrRejectValue_>
  void ResolveOrReject(ResolveOrRejectValue_&& aValue, const char* aSite)
  {
    MutexAutoLock lock(mMutex);
    MOZ_ASSERT(IsPending());
    PROMISE_LOG("%s resolveOrRejecting MozPromise (%p created at %s)", aSite, this, mCreationSite);
    mValue = Forward<ResolveOrRejectValue_>(aValue);
    DispatchAll();
  }
};


typedef MozPromise<bool, nsresult,  false> GenericPromise;





template<typename PromiseType>
class MozPromiseHolder
{
public:
  MozPromiseHolder()
    : mMonitor(nullptr) {}

  
  MozPromiseHolder& operator=(MozPromiseHolder&& aOther)
  {
    MOZ_ASSERT(!mMonitor && !aOther.mMonitor);
    MOZ_DIAGNOSTIC_ASSERT(!mPromise);
    mPromise = aOther.mPromise;
    aOther.mPromise = nullptr;
    return *this;
  }

  ~MozPromiseHolder() { MOZ_ASSERT(!mPromise); }

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
class MozPromiseRequestHolder
{
public:
  MozPromiseRequestHolder() {}
  ~MozPromiseRequestHolder() { MOZ_ASSERT(!mRequest); }

  void Begin(typename PromiseType::Request* aRequest)
  {
    MOZ_DIAGNOSTIC_ASSERT(!Exists());
    mRequest = aRequest;
  }

  void Complete()
  {
    MOZ_DIAGNOSTIC_ASSERT(Exists());
    mRequest = nullptr;
  }

  
  
  void Disconnect() {
    MOZ_ASSERT(Exists());
    mRequest->Disconnect();
    mRequest = nullptr;
  }

  void DisconnectIfExists() {
    if (Exists()) {
      Disconnect();
    }
  }

  bool Exists() { return !!mRequest; }

private:
  nsRefPtr<typename PromiseType::Request> mRequest;
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

template<typename PromiseType, typename ThisType, typename Arg1Type, typename Arg2Type, typename Arg3Type>
class MethodCallWithThreeArgs : public MethodCallBase<PromiseType>
{
public:
  typedef nsRefPtr<PromiseType>(ThisType::*Type)(Arg1Type, Arg2Type, Arg3Type);
  MethodCallWithThreeArgs(ThisType* aThisVal, Type aMethod, Arg1Type aArg1, Arg2Type aArg2, Arg3Type aArg3)
    : mThisVal(aThisVal), mMethod(aMethod), mArg1(aArg1), mArg2(aArg2), mArg3(aArg3) {}
  nsRefPtr<PromiseType> Invoke() override { return ((*mThisVal).*mMethod)(mArg1, mArg2, mArg3); }
protected:
  nsRefPtr<ThisType> mThisVal;
  Type mMethod;
  Arg1Type mArg1;
  Arg2Type mArg2;
  Arg3Type mArg3;
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
  return p.forget();
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

template<typename PromiseType, typename ThisType, typename Arg1Type, typename Arg2Type, typename Arg3Type>
static nsRefPtr<PromiseType>
ProxyMediaCall(AbstractThread* aTarget, ThisType* aThisVal, const char* aCallerName,
               nsRefPtr<PromiseType>(ThisType::*aMethod)(Arg1Type, Arg2Type, Arg3Type), Arg1Type aArg1, Arg2Type aArg2, Arg3Type aArg3)
{
  typedef detail::MethodCallWithThreeArgs<PromiseType, ThisType, Arg1Type, Arg2Type, Arg3Type> MethodCallType;
  MethodCallType* methodCall = new MethodCallType(aThisVal, aMethod, aArg1, aArg2, aArg3);
  return detail::ProxyInternal(aTarget, methodCall, aCallerName);
}

#undef PROMISE_LOG

} 

#endif
