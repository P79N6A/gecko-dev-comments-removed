





#ifndef mozilla_dom_Promise_h
#define mozilla_dom_Promise_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/TypeTraits.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/PromiseBinding.h"
#include "mozilla/dom/ToJSValue.h"
#include "mozilla/WeakPtr.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "js/TypeDecls.h"
#include "jspubtd.h"






#define DOM_PROMISE_DEPRECATED_REPORTING 1

#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
#include "mozilla/dom/workers/bindings/WorkerFeature.h"
#endif 

class nsIGlobalObject;

namespace mozilla {
namespace dom {

class AnyCallback;
class DOMError;
class MediaStreamError;
class PromiseCallback;
class PromiseInit;
class PromiseNativeHandler;
class PromiseDebugging;

class Promise;

#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
class PromiseReportRejectFeature : public workers::WorkerFeature
{
  
  Promise* mPromise;

public:
  explicit PromiseReportRejectFeature(Promise* aPromise)
    : mPromise(aPromise)
  {
    MOZ_ASSERT(mPromise);
  }

  virtual bool
  Notify(JSContext* aCx, workers::Status aStatus) override;
};
#endif 

#define NS_PROMISE_IID \
  { 0x1b8d6215, 0x3e67, 0x43ba, \
    { 0x8a, 0xf9, 0x31, 0x5e, 0x8f, 0xce, 0x75, 0x65 } }

class Promise : public nsISupports,
                public nsWrapperCache,
                public SupportsWeakPtr<Promise>
{
  friend class NativePromiseCallback;
  friend class PromiseCallbackTask;
  friend class PromiseResolverTask;
  friend class PromiseTask;
#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
  friend class PromiseReportRejectFeature;
#endif 
  friend class PromiseWorkerProxy;
  friend class PromiseWorkerProxyRunnable;
  friend class RejectPromiseCallback;
  friend class ResolvePromiseCallback;
  friend class ThenableResolverTask;
  friend class FastThenableResolverTask;
  friend class WrapperPromiseCallback;

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PROMISE_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(Promise)
  MOZ_DECLARE_REFCOUNTED_TYPENAME(Promise)

  
  
  
  
  static already_AddRefed<Promise>
  Create(nsIGlobalObject* aGlobal, ErrorResult& aRv);

  typedef void (Promise::*MaybeFunc)(JSContext* aCx,
                                     JS::Handle<JS::Value> aValue);

  void MaybeResolve(JSContext* aCx,
                    JS::Handle<JS::Value> aValue);
  void MaybeReject(JSContext* aCx,
                   JS::Handle<JS::Value> aValue);

  
  
  
  
  template <typename T>
  void MaybeResolve(const T& aArg) {
    MaybeSomething(aArg, &Promise::MaybeResolve);
  }

  inline void MaybeReject(nsresult aArg) {
    MOZ_ASSERT(NS_FAILED(aArg));
    MaybeSomething(aArg, &Promise::MaybeReject);
  }

  inline void MaybeReject(ErrorResult& aArg) {
    MOZ_ASSERT(aArg.Failed());
    MaybeSomething(aArg, &Promise::MaybeReject);
  }

  void MaybeReject(const nsRefPtr<MediaStreamError>& aArg);

  
  
  
  
  
  
  
  template<typename T>
  void MaybeRejectBrokenly(const T& aArg); 
                                           
                                           

  
  
  static bool PerformMicroTaskCheckpoint();

  

  nsIGlobalObject* GetParentObject() const
  {
    return mGlobal;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  static already_AddRefed<Promise>
  Constructor(const GlobalObject& aGlobal, PromiseInit& aInit,
              ErrorResult& aRv);

  static already_AddRefed<Promise>
  Resolve(const GlobalObject& aGlobal,
          JS::Handle<JS::Value> aValue, ErrorResult& aRv);

  static already_AddRefed<Promise>
  Resolve(nsIGlobalObject* aGlobal, JSContext* aCx,
          JS::Handle<JS::Value> aValue, ErrorResult& aRv);

  static already_AddRefed<Promise>
  Reject(const GlobalObject& aGlobal,
         JS::Handle<JS::Value> aValue, ErrorResult& aRv);

  static already_AddRefed<Promise>
  Reject(nsIGlobalObject* aGlobal, JSContext* aCx,
         JS::Handle<JS::Value> aValue, ErrorResult& aRv);

  already_AddRefed<Promise>
  Then(JSContext* aCx, AnyCallback* aResolveCallback,
       AnyCallback* aRejectCallback, ErrorResult& aRv);

  already_AddRefed<Promise>
  Catch(JSContext* aCx, AnyCallback* aRejectCallback, ErrorResult& aRv);

  static already_AddRefed<Promise>
  All(const GlobalObject& aGlobal,
      const Sequence<JS::Value>& aIterable, ErrorResult& aRv);

  static already_AddRefed<Promise>
  Race(const GlobalObject& aGlobal,
       const Sequence<JS::Value>& aIterable, ErrorResult& aRv);

  void AppendNativeHandler(PromiseNativeHandler* aRunnable);

  JSObject* GlobalJSObject() const;

  JSCompartment* Compartment() const;

  
  uint64_t GetID();

protected:
  
  
  explicit Promise(nsIGlobalObject* aGlobal);

  virtual ~Promise();

  
  static void
  DispatchToMicroTask(nsIRunnable* aRunnable);

  
  void CreateWrapper(ErrorResult& aRv);

  
  
  
  void CallInitFunction(const GlobalObject& aGlobal, PromiseInit& aInit,
                        ErrorResult& aRv);

  bool IsPending()
  {
    return mResolvePending;
  }

  void GetDependentPromises(nsTArray<nsRefPtr<Promise>>& aPromises);

  bool IsLastInChain() const
  {
    return mIsLastInChain;
  }

  void SetNotifiedAsUncaught()
  {
    mWasNotifiedAsUncaught = true;
  }

  bool WasNotifiedAsUncaught() const
  {
    return mWasNotifiedAsUncaught;
  }

private:
  friend class PromiseDebugging;

  enum PromiseState {
    Pending,
    Resolved,
    Rejected
  };

  void SetState(PromiseState aState)
  {
    MOZ_ASSERT(mState == Pending);
    MOZ_ASSERT(aState != Pending);
    mState = aState;
  }

  void SetResult(JS::Handle<JS::Value> aValue)
  {
    mResult = aValue;
  }

  
  
  
  
  void EnqueueCallbackTasks();

  void Settle(JS::Handle<JS::Value> aValue, Promise::PromiseState aState);
  void MaybeSettle(JS::Handle<JS::Value> aValue, Promise::PromiseState aState);

  void AppendCallbacks(PromiseCallback* aResolveCallback,
                       PromiseCallback* aRejectCallback);

#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
  
  
  
  void MaybeReportRejected();

  void MaybeReportRejectedOnce() {
    MaybeReportRejected();
    RemoveFeature();
    mResult.setUndefined();
  }
#endif 

  void MaybeResolveInternal(JSContext* aCx,
                            JS::Handle<JS::Value> aValue);
  void MaybeRejectInternal(JSContext* aCx,
                           JS::Handle<JS::Value> aValue);

  void ResolveInternal(JSContext* aCx,
                       JS::Handle<JS::Value> aValue);
  void RejectInternal(JSContext* aCx,
                      JS::Handle<JS::Value> aValue);

  template <typename T>
  void MaybeSomething(T& aArgument, MaybeFunc aFunc) {
    ThreadsafeAutoJSContext cx;
    JSObject* wrapper = GetWrapper();
    MOZ_ASSERT(wrapper); 

    JSAutoCompartment ac(cx, wrapper);
    JS::Rooted<JS::Value> val(cx);
    if (!ToJSValue(cx, aArgument, &val)) {
      HandleException(cx);
      return;
    }

    (this->*aFunc)(cx, val);
  }

  
  static bool
  JSCallback(JSContext *aCx, unsigned aArgc, JS::Value *aVp);

  static bool
  ThenableResolverCommon(JSContext* aCx, uint32_t  aTask,
                         unsigned aArgc, JS::Value* aVp);
  static bool
  JSCallbackThenableResolver(JSContext *aCx, unsigned aArgc, JS::Value *aVp);
  static bool
  JSCallbackThenableRejecter(JSContext *aCx, unsigned aArgc, JS::Value *aVp);

  static JSObject*
  CreateFunction(JSContext* aCx, Promise* aPromise, int32_t aTask);

  static JSObject*
  CreateThenableFunction(JSContext* aCx, Promise* aPromise, uint32_t aTask);

  void HandleException(JSContext* aCx);

#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
  void RemoveFeature();
#endif 

  
  
  bool CaptureStack(JSContext* aCx, JS::Heap<JSObject*>& aTarget);

  nsRefPtr<nsIGlobalObject> mGlobal;

  nsTArray<nsRefPtr<PromiseCallback> > mResolveCallbacks;
  nsTArray<nsRefPtr<PromiseCallback> > mRejectCallbacks;

  JS::Heap<JS::Value> mResult;
  
  
  JS::Heap<JSObject*> mAllocationStack;
  
  
  
  
  JS::Heap<JSObject*> mRejectionStack;
  
  
  
  
  JS::Heap<JSObject*> mFullfillmentStack;
  PromiseState mState;

#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
  bool mHadRejectCallback;

  
  
  
  
  nsAutoPtr<PromiseReportRejectFeature> mFeature;
#endif 

  bool mTaskPending;
  bool mResolvePending;

  
  
  
  bool mIsLastInChain;

  
  
  bool mWasNotifiedAsUncaught;

  
  TimeStamp mCreationTimestamp;

  
  TimeStamp mSettlementTimestamp;

  
  
  uint64_t mID;
};

NS_DEFINE_STATIC_IID_ACCESSOR(Promise, NS_PROMISE_IID)

} 
} 

#endif 
