





#ifndef mozilla_dom_Promise_h
#define mozilla_dom_Promise_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/TypeTraits.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/PromiseBinding.h"
#include "mozilla/dom/TypedArray.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "js/TypeDecls.h"

#include "mozilla/dom/workers/bindings/WorkerFeature.h"

class nsIGlobalObject;

namespace mozilla {
namespace dom {

class AnyCallback;
class PromiseCallback;
class PromiseInit;
class PromiseNativeHandler;

class Promise;
class PromiseReportRejectFeature : public workers::WorkerFeature
{
  
  Promise* mPromise;

public:
  PromiseReportRejectFeature(Promise* aPromise)
    : mPromise(aPromise)
  {
    MOZ_ASSERT(mPromise);
  }

  virtual bool
  Notify(JSContext* aCx, workers::Status aStatus) MOZ_OVERRIDE;
};

class Promise MOZ_FINAL : public nsISupports,
                          public nsWrapperCache
{
  friend class NativePromiseCallback;
  friend class PromiseResolverMixin;
  friend class PromiseResolverTask;
  friend class PromiseTask;
  friend class PromiseReportRejectFeature;
  friend class RejectPromiseCallback;
  friend class ResolvePromiseCallback;
  friend class WorkerPromiseResolverTask;
  friend class WorkerPromiseTask;
  friend class WrapperPromiseCallback;

  ~Promise();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Promise)

  Promise(nsIGlobalObject* aGlobal);

  typedef void (Promise::*MaybeFunc)(JSContext* aCx,
                                     JS::Handle<JS::Value> aValue);

  void MaybeResolve(JSContext* aCx,
                    JS::Handle<JS::Value> aValue);
  void MaybeReject(JSContext* aCx,
                   JS::Handle<JS::Value> aValue);

  
  
  
  template <typename T>
  void MaybeResolve(T& aArg) {
    MaybeSomething(aArg, &Promise::MaybeResolve);
  }

  template <typename T>
  void MaybeReject(T& aArg) {
    MaybeSomething(aArg, &Promise::MaybeReject);
  }

  

  nsIGlobalObject* GetParentObject() const
  {
    return mGlobal;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  static already_AddRefed<Promise>
  Constructor(const GlobalObject& aGlobal, PromiseInit& aInit,
              ErrorResult& aRv);

  static already_AddRefed<Promise>
  Resolve(const GlobalObject& aGlobal, JSContext* aCx,
          JS::Handle<JS::Value> aValue, ErrorResult& aRv);

  static already_AddRefed<Promise>
  Resolve(nsIGlobalObject* aGlobal, JSContext* aCx,
          JS::Handle<JS::Value> aValue, ErrorResult& aRv);

  static already_AddRefed<Promise>
  Reject(const GlobalObject& aGlobal, JSContext* aCx,
         JS::Handle<JS::Value> aValue, ErrorResult& aRv);

  static already_AddRefed<Promise>
  Reject(nsIGlobalObject* aGlobal, JSContext* aCx,
         JS::Handle<JS::Value> aValue, ErrorResult& aRv);

  already_AddRefed<Promise>
  Then(AnyCallback* aResolveCallback, AnyCallback* aRejectCallback);

  already_AddRefed<Promise>
  Catch(AnyCallback* aRejectCallback);

  
  static already_AddRefed<Promise>
  All(const GlobalObject& aGlobal, JSContext* aCx,
      const Sequence<JS::Value>& aIterable, ErrorResult& aRv);

  
  static already_AddRefed<Promise>
  Race(const GlobalObject& aGlobal, JSContext* aCx,
       const Sequence<JS::Value>& aIterable, ErrorResult& aRv);

  void AppendNativeHandler(PromiseNativeHandler* aRunnable);

private:
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

  
  JSObject* GetOrCreateWrapper(JSContext* aCx);

  
  

  
  bool
  ArgumentToJSValue(const nsAString& aArgument,
                    JSContext* aCx,
                    JSObject* aScope,
                    JS::MutableHandle<JS::Value> aValue);

  
  bool
  ArgumentToJSValue(bool aArgument,
                    JSContext* aCx,
                    JSObject* aScope,
                    JS::MutableHandle<JS::Value> aValue);

  
  
  template <class T>
  typename EnableIf<IsBaseOf<nsWrapperCache, T>::value &&
                    IsBaseOf<nsISupports, T>::value, bool>::Type
  ArgumentToJSValue(T& aArgument,
                    JSContext* aCx,
                    JSObject* aScope,
                    JS::MutableHandle<JS::Value> aValue)
  {
    JS::Rooted<JSObject*> scope(aCx, aScope);

    return WrapNewBindingObject(aCx, scope, aArgument, aValue);
  }

  
  template<typename T>
  typename EnableIf<IsBaseOf<AllTypedArraysBase, T>::value, bool>::Type
  ArgumentToJSValue(const TypedArrayCreator<T>& aArgument,
                    JSContext* aCx,
                    JSObject* aScope,
                    JS::MutableHandle<JS::Value> aValue)
  {
    JS::RootedObject scope(aCx, aScope);

    JSObject* abv = aArgument.Create(aCx, scope);
    if (!abv) {
      return false;
    }
    aValue.setObject(*abv);
    return true;
  }

  
  
  template <class T>
  typename EnableIf<!IsBaseOf<nsWrapperCache, T>::value &&
                    IsBaseOf<nsISupports, T>::value, bool>::Type
  ArgumentToJSValue(T& aArgument,
                    JSContext* aCx,
                    JSObject* aScope,
                    JS::MutableHandle<JS::Value> aValue)
  {
    JS::Rooted<JSObject*> scope(aCx, aScope);

    nsresult rv = nsContentUtils::WrapNative(aCx, scope, &aArgument, aValue);
    return NS_SUCCEEDED(rv);
  }

  template <template <typename> class SmartPtr, typename T>
  bool
  ArgumentToJSValue(const SmartPtr<T>& aArgument,
                    JSContext* aCx,
                    JSObject* aScope,
                    JS::MutableHandle<JS::Value> aValue)
  {
    return ArgumentToJSValue(*aArgument.get(), aCx, aScope, aValue);
  }

  template <typename T>
  void MaybeSomething(T& aArgument, MaybeFunc aFunc) {
    ThreadsafeAutoJSContext cx;

    JSObject* wrapper = GetOrCreateWrapper(cx);
    if (!wrapper) {
      HandleException(cx);
      return;
    }

    JSAutoCompartment ac(cx, wrapper);
    JS::Rooted<JS::Value> val(cx);
    if (!ArgumentToJSValue(aArgument, cx, wrapper, &val)) {
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
