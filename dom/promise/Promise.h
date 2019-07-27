





#ifndef mozilla_dom_Promise_h
#define mozilla_dom_Promise_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/TypeTraits.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/PromiseBinding.h"
#include "mozilla/dom/ToJSValue.h"
#include "mozilla/WeakPtr.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "js/TypeDecls.h"

#include "mozilla/dom/workers/bindings/WorkerFeature.h"

class nsIGlobalObject;

namespace mozilla {
namespace dom {

class AnyCallback;
class DOMError;
class PromiseCallback;
class PromiseInit;
class PromiseNativeHandler;
class PromiseDebugging;

class Promise;
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
  Notify(JSContext* aCx, workers::Status aStatus) MOZ_OVERRIDE;
};

class Promise MOZ_FINAL : public nsISupports,
                          public nsWrapperCache,
                          public SupportsWeakPtr<Promise>
{
  friend class NativePromiseCallback;
  friend class PromiseResolverTask;
  friend class PromiseTask;
  friend class PromiseReportRejectFeature;
  friend class PromiseWorkerProxy;
  friend class PromiseWorkerProxyRunnable;
  friend class RejectPromiseCallback;
  friend class ResolvePromiseCallback;
  friend class ThenableResolverTask;
  friend class WrapperPromiseCallback;

  ~Promise();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Promise)
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
  
  
  
  
  
  
  
  template<typename T>
  void MaybeRejectBrokenly(const T& aArg); 
                                           
                                           

  

  nsIGlobalObject* GetParentObject() const
  {
    return mGlobal;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

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

private:
  
  
  explicit Promise(nsIGlobalObject* aGlobal);

  friend class PromiseDebugging;

  enum PromiseState {
    Pending,
    Resolved,
    Rejected
  };

  enum PromiseTaskSync {
    SyncTask,
    AsyncTask
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

  
  static void
  DispatchToMainOrWorkerThread(nsIRunnable* aRunnable);

  
  
  
  
  void RunTask();

  void RunResolveTask(JS::Handle<JS::Value> aValue,
                      Promise::PromiseState aState,
                      PromiseTaskSync aAsynchronous);

  void AppendCallbacks(PromiseCallback* aResolveCallback,
                       PromiseCallback* aRejectCallback);

  
  
  
  void MaybeReportRejected();

  void MaybeReportRejectedOnce() {
    MaybeReportRejected();
    RemoveFeature();
    mResult = JS::UndefinedValue();
  }

  void MaybeResolveInternal(JSContext* aCx,
                            JS::Handle<JS::Value> aValue,
                            PromiseTaskSync aSync = AsyncTask);
  void MaybeRejectInternal(JSContext* aCx,
                           JS::Handle<JS::Value> aValue,
                           PromiseTaskSync aSync = AsyncTask);

  void ResolveInternal(JSContext* aCx,
                       JS::Handle<JS::Value> aValue,
                       PromiseTaskSync aSync = AsyncTask);

  void RejectInternal(JSContext* aCx,
                      JS::Handle<JS::Value> aValue,
                      PromiseTaskSync aSync = AsyncTask);

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
  CreateFunction(JSContext* aCx, JSObject* aParent, Promise* aPromise,
                int32_t aTask);

  static JSObject*
  CreateThenableFunction(JSContext* aCx, Promise* aPromise, uint32_t aTask);

  void HandleException(JSContext* aCx);

  void RemoveFeature();

  nsRefPtr<nsIGlobalObject> mGlobal;

  nsTArray<nsRefPtr<PromiseCallback> > mResolveCallbacks;
  nsTArray<nsRefPtr<PromiseCallback> > mRejectCallbacks;

  JS::Heap<JS::Value> mResult;
  PromiseState mState;
  bool mTaskPending;
  bool mHadRejectCallback;

  bool mResolvePending;

  
  
  
  
  nsAutoPtr<PromiseReportRejectFeature> mFeature;
};

} 
} 

#endif 
