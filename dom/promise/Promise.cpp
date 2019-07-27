





#include "mozilla/dom/Promise.h"

#include "jsfriendapi.h"
#include "js/Debug.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/OwningNonNull.h"
#include "mozilla/dom/PromiseBinding.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/MediaStreamError.h"
#include "mozilla/Atomics.h"
#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/Preferences.h"
#include "PromiseCallback.h"
#include "PromiseDebugging.h"
#include "PromiseNativeHandler.h"
#include "PromiseWorkerProxy.h"
#include "nsContentUtils.h"
#include "WorkerPrivate.h"
#include "WorkerRunnable.h"
#include "nsJSPrincipals.h"
#include "nsJSUtils.h"
#include "nsPIDOMWindow.h"
#include "nsJSEnvironment.h"
#include "nsIScriptObjectPrincipal.h"
#include "xpcpublic.h"
#include "nsGlobalWindow.h"

namespace mozilla {
namespace dom {

namespace {

Atomic<uintptr_t> gIDGenerator(0);
}

using namespace workers;

NS_IMPL_ISUPPORTS0(PromiseNativeHandler)


class PromiseCallbackTask final : public nsRunnable
{
public:
  PromiseCallbackTask(Promise* aPromise,
                      PromiseCallback* aCallback,
                      const JS::Value& aValue)
    : mPromise(aPromise)
    , mCallback(aCallback)
    , mValue(CycleCollectedJSRuntime::Get()->Runtime(), aValue)
  {
    MOZ_ASSERT(aPromise);
    MOZ_ASSERT(aCallback);
    MOZ_COUNT_CTOR(PromiseCallbackTask);
  }

  virtual
  ~PromiseCallbackTask()
  {
    NS_ASSERT_OWNINGTHREAD(PromiseCallbackTask);
    MOZ_COUNT_DTOR(PromiseCallbackTask);
  }

protected:
  NS_IMETHOD
  Run() override
  {
    NS_ASSERT_OWNINGTHREAD(PromiseCallbackTask);
    ThreadsafeAutoJSContext cx;
    JS::Rooted<JSObject*> wrapper(cx, mPromise->GetWrapper());
    MOZ_ASSERT(wrapper); 
    JSAutoCompartment ac(cx, wrapper);

    JS::Rooted<JS::Value> value(cx, mValue);
    if (!MaybeWrapValue(cx, &value)) {
      NS_WARNING("Failed to wrap value into the right compartment.");
      JS_ClearPendingException(cx);
      return NS_OK;
    }

    JS::Rooted<JSObject*> asyncStack(cx, mPromise->mAllocationStack);
    JS::Rooted<JSString*> asyncCause(cx, JS_NewStringCopyZ(cx, "Promise"));
    if (!asyncCause) {
      JS_ClearPendingException(cx);
      return NS_ERROR_OUT_OF_MEMORY;
    }

    {
      Maybe<JS::AutoSetAsyncStackForNewCalls> sas;
      if (asyncStack) {
        sas.emplace(cx, asyncStack, asyncCause);
      }
      mCallback->Call(cx, value);
    }

    return NS_OK;
  }

private:
  nsRefPtr<Promise> mPromise;
  nsRefPtr<PromiseCallback> mCallback;
  JS::PersistentRooted<JS::Value> mValue;
  NS_DECL_OWNINGTHREAD;
};

enum {
  SLOT_PROMISE = 0,
  SLOT_DATA
};










namespace {
void
LinkThenableCallables(JSContext* aCx, JS::Handle<JSObject*> aResolveFunc,
                      JS::Handle<JSObject*> aRejectFunc)
{
  js::SetFunctionNativeReserved(aResolveFunc, SLOT_DATA,
                                JS::ObjectValue(*aRejectFunc));
  js::SetFunctionNativeReserved(aRejectFunc, SLOT_DATA,
                                JS::ObjectValue(*aResolveFunc));
}





bool
MarkAsCalledIfNotCalledBefore(JSContext* aCx, JS::Handle<JSObject*> aFunc)
{
  JS::Value otherFuncVal = js::GetFunctionNativeReserved(aFunc, SLOT_DATA);

  if (!otherFuncVal.isObject()) {
    return false;
  }

  JSObject* otherFuncObj = &otherFuncVal.toObject();
  MOZ_ASSERT(js::GetFunctionNativeReserved(otherFuncObj, SLOT_DATA).isObject());

  
  js::SetFunctionNativeReserved(aFunc, SLOT_DATA, JS::UndefinedValue());
  js::SetFunctionNativeReserved(otherFuncObj, SLOT_DATA, JS::UndefinedValue());

  return true;
}

Promise*
GetPromise(JSContext* aCx, JS::Handle<JSObject*> aFunc)
{
  JS::Value promiseVal = js::GetFunctionNativeReserved(aFunc, SLOT_PROMISE);

  MOZ_ASSERT(promiseVal.isObject());

  Promise* promise;
  UNWRAP_OBJECT(Promise, &promiseVal.toObject(), promise);
  return promise;
}
};



class ThenableResolverTask final : public nsRunnable
{
public:
  ThenableResolverTask(Promise* aPromise,
                        JS::Handle<JSObject*> aThenable,
                        PromiseInit* aThen)
    : mPromise(aPromise)
    , mThenable(CycleCollectedJSRuntime::Get()->Runtime(), aThenable)
    , mThen(aThen)
  {
    MOZ_ASSERT(aPromise);
    MOZ_COUNT_CTOR(ThenableResolverTask);
  }

  virtual
  ~ThenableResolverTask()
  {
    NS_ASSERT_OWNINGTHREAD(ThenableResolverTask);
    MOZ_COUNT_DTOR(ThenableResolverTask);
  }

protected:
  NS_IMETHOD
  Run() override
  {
    NS_ASSERT_OWNINGTHREAD(ThenableResolverTask);
    ThreadsafeAutoJSContext cx;
    JS::Rooted<JSObject*> wrapper(cx, mPromise->GetWrapper());
    MOZ_ASSERT(wrapper); 
    
    
    JSAutoCompartment ac(cx, wrapper);

    JS::Rooted<JSObject*> resolveFunc(cx,
      mPromise->CreateThenableFunction(cx, mPromise, PromiseCallback::Resolve));

    if (!resolveFunc) {
      mPromise->HandleException(cx);
      return NS_OK;
    }

    JS::Rooted<JSObject*> rejectFunc(cx,
      mPromise->CreateThenableFunction(cx, mPromise, PromiseCallback::Reject));
    if (!rejectFunc) {
      mPromise->HandleException(cx);
      return NS_OK;
    }

    LinkThenableCallables(cx, resolveFunc, rejectFunc);

    ErrorResult rv;

    JS::Rooted<JSObject*> rootedThenable(cx, mThenable);

    mThen->Call(rootedThenable, resolveFunc, rejectFunc, rv,
                "promise thenable", CallbackObject::eRethrowExceptions,
                mPromise->Compartment());

    rv.WouldReportJSException();
    if (rv.Failed()) {
      JS::Rooted<JS::Value> exn(cx);
      if (rv.IsJSException()) {
        rv.StealJSException(cx, &exn);
      } else {
        
        
        
        
        JSAutoCompartment ac(cx, mPromise->GlobalJSObject());
        DebugOnly<bool> conversionResult = ToJSValue(cx, rv, &exn);
        MOZ_ASSERT(conversionResult);
      }

      bool couldMarkAsCalled = MarkAsCalledIfNotCalledBefore(cx, resolveFunc);

      
      
      if (couldMarkAsCalled) {
        bool ok = JS_WrapValue(cx, &exn);
        MOZ_ASSERT(ok);
        if (!ok) {
          NS_WARNING("Failed to wrap value into the right compartment.");
        }

        mPromise->RejectInternal(cx, exn);
      }
      
      
      
    }

    return rv.ErrorCode();
  }

private:
  nsRefPtr<Promise> mPromise;
  JS::PersistentRooted<JSObject*> mThenable;
  nsRefPtr<PromiseInit> mThen;
  NS_DECL_OWNINGTHREAD;
};





class FastThenableResolverTask final : public nsRunnable
{
public:
  FastThenableResolverTask(PromiseCallback* aResolveCallback,
                           PromiseCallback* aRejectCallback,
                           Promise* aNextPromise)
    : mResolveCallback(aResolveCallback)
    , mRejectCallback(aRejectCallback)
    , mNextPromise(aNextPromise)
  {
    MOZ_ASSERT(aResolveCallback);
    MOZ_ASSERT(aRejectCallback);
    MOZ_ASSERT(aNextPromise);
    MOZ_COUNT_CTOR(FastThenableResolverTask);
  }

  virtual
  ~FastThenableResolverTask()
  {
    NS_ASSERT_OWNINGTHREAD(FastThenableResolverTask);
    MOZ_COUNT_DTOR(FastThenableResolverTask);
  }

protected:
  NS_IMETHOD
  Run() override
  {
    NS_ASSERT_OWNINGTHREAD(FastThenableResolverTask);
    mNextPromise->AppendCallbacks(mResolveCallback, mRejectCallback);
    return NS_OK;
  }

private:
  nsRefPtr<PromiseCallback> mResolveCallback;
  nsRefPtr<PromiseCallback> mRejectCallback;
  nsRefPtr<Promise> mNextPromise;
};



NS_IMPL_CYCLE_COLLECTION_CLASS(Promise)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(Promise)
#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
  tmp->MaybeReportRejectedOnce();
#else
  tmp->mResult = JS::UndefinedValue();
#endif 
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mGlobal)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mResolveCallbacks)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRejectCallbacks)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(Promise)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mGlobal)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mResolveCallbacks)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRejectCallbacks)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(Promise)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(mResult)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mAllocationStack)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mRejectionStack)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mFullfillmentStack)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(Promise)
  if (tmp->IsBlack()) {
    if (tmp->mResult.isObject()) {
      JS::ExposeObjectToActiveJS(&(tmp->mResult.toObject()));
    }
    if (tmp->mAllocationStack) {
      JS::ExposeObjectToActiveJS(tmp->mAllocationStack);
    }
    if (tmp->mRejectionStack) {
      JS::ExposeObjectToActiveJS(tmp->mRejectionStack);
    }
    if (tmp->mFullfillmentStack) {
      JS::ExposeObjectToActiveJS(tmp->mFullfillmentStack);
    }
    return true;
  }
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(Promise)
  return tmp->IsBlackAndDoesNotNeedTracing(tmp);
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(Promise)
  return tmp->IsBlack();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(Promise)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Promise)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Promise)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(Promise)
NS_INTERFACE_MAP_END

Promise::Promise(nsIGlobalObject* aGlobal)
  : mGlobal(aGlobal)
  , mResult(JS::UndefinedValue())
  , mAllocationStack(nullptr)
  , mRejectionStack(nullptr)
  , mFullfillmentStack(nullptr)
  , mState(Pending)
#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
  , mHadRejectCallback(false)
#endif 
  , mTaskPending(false)
  , mResolvePending(false)
  , mIsLastInChain(true)
  , mWasNotifiedAsUncaught(false)
  , mID(0)
{
  MOZ_ASSERT(mGlobal);

  mozilla::HoldJSObjects(this);

  mCreationTimestamp = TimeStamp::Now();
}

Promise::~Promise()
{
#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
  MaybeReportRejectedOnce();
#endif 
  mozilla::DropJSObjects(this);
}

JSObject*
Promise::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return PromiseBinding::Wrap(aCx, this, aGivenProto);
}

already_AddRefed<Promise>
Promise::Create(nsIGlobalObject* aGlobal, ErrorResult& aRv)
{
  nsRefPtr<Promise> p = new Promise(aGlobal);
  p->CreateWrapper(aRv);
  if (aRv.Failed()) {
    return nullptr;
  }
  return p.forget();
}

void
Promise::CreateWrapper(ErrorResult& aRv)
{
  AutoJSAPI jsapi;
  if (!jsapi.Init(mGlobal)) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }
  JSContext* cx = jsapi.cx();

  JS::Rooted<JS::Value> wrapper(cx);
  if (!GetOrCreateDOMReflector(cx, this, &wrapper)) {
    JS_ClearPendingException(cx);
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  dom::PreserveWrapper(this);

  
  if (!CaptureStack(cx, mAllocationStack)) {
    JS_ClearPendingException(cx);
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  JS::RootedObject obj(cx, &wrapper.toObject());
  JS::dbg::onNewPromise(cx, obj);
}

void
Promise::MaybeResolve(JSContext* aCx,
                      JS::Handle<JS::Value> aValue)
{
  MaybeResolveInternal(aCx, aValue);
}

void
Promise::MaybeReject(JSContext* aCx,
                     JS::Handle<JS::Value> aValue)
{
  MaybeRejectInternal(aCx, aValue);
}

void
Promise::MaybeReject(const nsRefPtr<MediaStreamError>& aArg) {
  MaybeSomething(aArg, &Promise::MaybeReject);
}

bool
Promise::PerformMicroTaskCheckpoint()
{
  CycleCollectedJSRuntime* runtime = CycleCollectedJSRuntime::Get();
  nsTArray<nsCOMPtr<nsIRunnable>>& microtaskQueue =
    runtime->GetPromiseMicroTaskQueue();

  if (microtaskQueue.IsEmpty()) {
    return false;
  }

  Maybe<AutoSafeJSContext> cx;
  if (NS_IsMainThread()) {
    cx.emplace();
  }

  do {
    nsCOMPtr<nsIRunnable> runnable = microtaskQueue.ElementAt(0);
    MOZ_ASSERT(runnable);

    
    microtaskQueue.RemoveElementAt(0);
    nsresult rv = runnable->Run();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return false;
    }
    if (cx.isSome()) {
      JS::CheckForJSInterrupt(cx.ref());
    }
  } while (!microtaskQueue.IsEmpty());

  return true;
}

 bool
Promise::JSCallback(JSContext* aCx, unsigned aArgc, JS::Value* aVp)
{
  JS::CallArgs args = CallArgsFromVp(aArgc, aVp);

  JS::Rooted<JS::Value> v(aCx,
                          js::GetFunctionNativeReserved(&args.callee(),
                                                        SLOT_PROMISE));
  MOZ_ASSERT(v.isObject());

  Promise* promise;
  if (NS_FAILED(UNWRAP_OBJECT(Promise, &v.toObject(), promise))) {
    return Throw(aCx, NS_ERROR_UNEXPECTED);
  }

  v = js::GetFunctionNativeReserved(&args.callee(), SLOT_DATA);
  PromiseCallback::Task task = static_cast<PromiseCallback::Task>(v.toInt32());

  if (task == PromiseCallback::Resolve) {
    promise->MaybeResolveInternal(aCx, args.get(0));
    if (!promise->CaptureStack(aCx, promise->mFullfillmentStack)) {
      return false;
    }
  } else {
    promise->MaybeRejectInternal(aCx, args.get(0));
    if (!promise->CaptureStack(aCx, promise->mRejectionStack)) {
      return false;
    }
  }

  args.rval().setUndefined();
  return true;
}






 bool
Promise::ThenableResolverCommon(JSContext* aCx, uint32_t aTask,
                                unsigned aArgc, JS::Value* aVp)
{
  JS::CallArgs args = CallArgsFromVp(aArgc, aVp);
  JS::Rooted<JSObject*> thisFunc(aCx, &args.callee());
  if (!MarkAsCalledIfNotCalledBefore(aCx, thisFunc)) {
    
    args.rval().setUndefined();
    return true;
  }

  Promise* promise = GetPromise(aCx, thisFunc);
  MOZ_ASSERT(promise);

  if (aTask == PromiseCallback::Resolve) {
    promise->ResolveInternal(aCx, args.get(0));
  } else {
    promise->RejectInternal(aCx, args.get(0));
  }

  args.rval().setUndefined();
  return true;
}

 bool
Promise::JSCallbackThenableResolver(JSContext* aCx,
                                    unsigned aArgc, JS::Value* aVp)
{
  return ThenableResolverCommon(aCx, PromiseCallback::Resolve, aArgc, aVp);
}

 bool
Promise::JSCallbackThenableRejecter(JSContext* aCx,
                                    unsigned aArgc, JS::Value* aVp)
{
  return ThenableResolverCommon(aCx, PromiseCallback::Reject, aArgc, aVp);
}

 JSObject*
Promise::CreateFunction(JSContext* aCx, Promise* aPromise, int32_t aTask)
{
  JSFunction* func = js::NewFunctionWithReserved(aCx, JSCallback,
                                                 1 , 0 ,
                                                 nullptr);
  if (!func) {
    return nullptr;
  }

  JS::Rooted<JSObject*> obj(aCx, JS_GetFunctionObject(func));

  JS::Rooted<JS::Value> promiseObj(aCx);
  if (!dom::GetOrCreateDOMReflector(aCx, aPromise, &promiseObj)) {
    return nullptr;
  }

  js::SetFunctionNativeReserved(obj, SLOT_PROMISE, promiseObj);
  js::SetFunctionNativeReserved(obj, SLOT_DATA, JS::Int32Value(aTask));

  return obj;
}

 JSObject*
Promise::CreateThenableFunction(JSContext* aCx, Promise* aPromise, uint32_t aTask)
{
  JSNative whichFunc =
    aTask == PromiseCallback::Resolve ? JSCallbackThenableResolver :
                                        JSCallbackThenableRejecter ;

  JSFunction* func = js::NewFunctionWithReserved(aCx, whichFunc,
                                                 1 , 0 ,
                                                 nullptr);
  if (!func) {
    return nullptr;
  }

  JS::Rooted<JSObject*> obj(aCx, JS_GetFunctionObject(func));

  JS::Rooted<JS::Value> promiseObj(aCx);
  if (!dom::GetOrCreateDOMReflector(aCx, aPromise, &promiseObj)) {
    return nullptr;
  }

  js::SetFunctionNativeReserved(obj, SLOT_PROMISE, promiseObj);

  return obj;
}

 already_AddRefed<Promise>
Promise::Constructor(const GlobalObject& aGlobal,
                     PromiseInit& aInit, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global;
  global = do_QueryInterface(aGlobal.GetAsSupports());
  if (!global) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Create(global, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  promise->CallInitFunction(aGlobal, aInit, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  return promise.forget();
}

void
Promise::CallInitFunction(const GlobalObject& aGlobal,
                          PromiseInit& aInit, ErrorResult& aRv)
{
  JSContext* cx = aGlobal.Context();

  JS::Rooted<JSObject*> resolveFunc(cx,
                                    CreateFunction(cx, this,
                                                   PromiseCallback::Resolve));
  if (!resolveFunc) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  JS::Rooted<JSObject*> rejectFunc(cx,
                                   CreateFunction(cx, this,
                                                  PromiseCallback::Reject));
  if (!rejectFunc) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  aInit.Call(resolveFunc, rejectFunc, aRv, "promise initializer",
             CallbackObject::eRethrowExceptions, Compartment());
  aRv.WouldReportJSException();

  if (aRv.IsJSException()) {
    JS::Rooted<JS::Value> value(cx);
    aRv.StealJSException(cx, &value);

    
    
    if (!JS_WrapValue(cx, &value)) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return;
    }

    MaybeRejectInternal(cx, value);
  }

  
  
  
  
  
}

 already_AddRefed<Promise>
Promise::Resolve(const GlobalObject& aGlobal,
                 JS::Handle<JS::Value> aValue, ErrorResult& aRv)
{
  
  if (aValue.isObject()) {
    JS::Rooted<JSObject*> valueObj(aGlobal.Context(), &aValue.toObject());
    Promise* nextPromise;
    nsresult rv = UNWRAP_OBJECT(Promise, valueObj, nextPromise);

    if (NS_SUCCEEDED(rv)) {
      nsRefPtr<Promise> addRefed = nextPromise;
      return addRefed.forget();
    }
  }

  nsCOMPtr<nsIGlobalObject> global =
    do_QueryInterface(aGlobal.GetAsSupports());
  if (!global) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<Promise> p = Resolve(global, aGlobal.Context(), aValue, aRv);
  if (p) {
    p->mFullfillmentStack = p->mAllocationStack;
  }
  return p.forget();
}

 already_AddRefed<Promise>
Promise::Resolve(nsIGlobalObject* aGlobal, JSContext* aCx,
                 JS::Handle<JS::Value> aValue, ErrorResult& aRv)
{
  nsRefPtr<Promise> promise = Create(aGlobal, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  promise->MaybeResolveInternal(aCx, aValue);
  return promise.forget();
}

 already_AddRefed<Promise>
Promise::Reject(const GlobalObject& aGlobal,
                JS::Handle<JS::Value> aValue, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global =
    do_QueryInterface(aGlobal.GetAsSupports());
  if (!global) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<Promise> p = Reject(global, aGlobal.Context(), aValue, aRv);
  if (p) {
    p->mRejectionStack = p->mAllocationStack;
  }
  return p.forget();
}

 already_AddRefed<Promise>
Promise::Reject(nsIGlobalObject* aGlobal, JSContext* aCx,
                JS::Handle<JS::Value> aValue, ErrorResult& aRv)
{
  nsRefPtr<Promise> promise = Create(aGlobal, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  promise->MaybeRejectInternal(aCx, aValue);
  return promise.forget();
}

already_AddRefed<Promise>
Promise::Then(JSContext* aCx, AnyCallback* aResolveCallback,
              AnyCallback* aRejectCallback, ErrorResult& aRv)
{
  nsRefPtr<Promise> promise = Create(GetParentObject(), aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  JS::Rooted<JSObject*> global(aCx, JS::CurrentGlobalOrNull(aCx));

  nsRefPtr<PromiseCallback> resolveCb =
    PromiseCallback::Factory(promise, global, aResolveCallback,
                             PromiseCallback::Resolve);

  nsRefPtr<PromiseCallback> rejectCb =
    PromiseCallback::Factory(promise, global, aRejectCallback,
                             PromiseCallback::Reject);

  AppendCallbacks(resolveCb, rejectCb);

  return promise.forget();
}

already_AddRefed<Promise>
Promise::Catch(JSContext* aCx, AnyCallback* aRejectCallback, ErrorResult& aRv)
{
  nsRefPtr<AnyCallback> resolveCb;
  return Then(aCx, resolveCb, aRejectCallback, aRv);
}






class CountdownHolder final : public nsISupports
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CountdownHolder)

  CountdownHolder(const GlobalObject& aGlobal, Promise* aPromise,
                  uint32_t aCountdown)
    : mPromise(aPromise), mCountdown(aCountdown)
  {
    MOZ_ASSERT(aCountdown != 0);
    JSContext* cx = aGlobal.Context();

    
    
    

    JSAutoCompartment ac(cx, aGlobal.Get());
    mValues = JS_NewArrayObject(cx, aCountdown);
    mozilla::HoldJSObjects(this);
  }

private:
  ~CountdownHolder()
  {
    mozilla::DropJSObjects(this);
  }

public:
  void SetValue(uint32_t index, const JS::Handle<JS::Value> aValue)
  {
    MOZ_ASSERT(mCountdown > 0);

    ThreadsafeAutoSafeJSContext cx;
    JSAutoCompartment ac(cx, mValues);
    {

      AutoDontReportUncaught silenceReporting(cx);
      JS::Rooted<JS::Value> value(cx, aValue);
      JS::Rooted<JSObject*> values(cx, mValues);
      if (!JS_WrapValue(cx, &value) ||
          !JS_DefineElement(cx, values, index, value, JSPROP_ENUMERATE)) {
        MOZ_ASSERT(JS_IsExceptionPending(cx));
        JS::Rooted<JS::Value> exn(cx);
        JS_GetPendingException(cx, &exn);

        mPromise->MaybeReject(cx, exn);
      }
    }

    --mCountdown;
    if (mCountdown == 0) {
      JS::Rooted<JS::Value> result(cx, JS::ObjectValue(*mValues));
      mPromise->MaybeResolve(cx, result);
    }
  }

private:
  nsRefPtr<Promise> mPromise;
  uint32_t mCountdown;
  JS::Heap<JSObject*> mValues;
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(CountdownHolder)
NS_IMPL_CYCLE_COLLECTING_RELEASE(CountdownHolder)
NS_IMPL_CYCLE_COLLECTION_CLASS(CountdownHolder)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CountdownHolder)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(CountdownHolder)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mValues)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(CountdownHolder)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPromise)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(CountdownHolder)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPromise)
  tmp->mValues = nullptr;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END






class AllResolveHandler final : public PromiseNativeHandler
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(AllResolveHandler)

  AllResolveHandler(CountdownHolder* aHolder, uint32_t aIndex)
    : mCountdownHolder(aHolder), mIndex(aIndex)
  {
    MOZ_ASSERT(aHolder);
  }

  void
  ResolvedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) override
  {
    mCountdownHolder->SetValue(mIndex, aValue);
  }

  void
  RejectedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) override
  {
    
    MOZ_ASSERT(false, "AllResolveHandler should never be attached to a Promise's reject handler!");
  }

private:
  ~AllResolveHandler()
  {
  }

  nsRefPtr<CountdownHolder> mCountdownHolder;
  uint32_t mIndex;
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(AllResolveHandler)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AllResolveHandler)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AllResolveHandler)
NS_INTERFACE_MAP_END_INHERITING(PromiseNativeHandler)

NS_IMPL_CYCLE_COLLECTION(AllResolveHandler, mCountdownHolder)

 already_AddRefed<Promise>
Promise::All(const GlobalObject& aGlobal,
             const Sequence<JS::Value>& aIterable, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global =
    do_QueryInterface(aGlobal.GetAsSupports());
  if (!global) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  JSContext* cx = aGlobal.Context();

  if (aIterable.Length() == 0) {
    JS::Rooted<JSObject*> empty(cx, JS_NewArrayObject(cx, 0));
    if (!empty) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return nullptr;
    }
    JS::Rooted<JS::Value> value(cx, JS::ObjectValue(*empty));
    
    
    return Promise::Resolve(global, cx, value, aRv);
  }

  nsRefPtr<Promise> promise = Create(global, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }
  nsRefPtr<CountdownHolder> holder =
    new CountdownHolder(aGlobal, promise, aIterable.Length());

  JS::Rooted<JSObject*> obj(cx, JS::CurrentGlobalOrNull(cx));
  if (!obj) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<PromiseCallback> rejectCb = new RejectPromiseCallback(promise, obj);

  for (uint32_t i = 0; i < aIterable.Length(); ++i) {
    JS::Rooted<JS::Value> value(cx, aIterable.ElementAt(i));
    nsRefPtr<Promise> nextPromise = Promise::Resolve(aGlobal, value, aRv);

    MOZ_ASSERT(!aRv.Failed());

    nsRefPtr<PromiseNativeHandler> resolveHandler =
      new AllResolveHandler(holder, i);

    nsRefPtr<PromiseCallback> resolveCb =
      new NativePromiseCallback(resolveHandler, Resolved);
    
    
    nextPromise->AppendCallbacks(resolveCb, rejectCb);
  }

  return promise.forget();
}

 already_AddRefed<Promise>
Promise::Race(const GlobalObject& aGlobal,
              const Sequence<JS::Value>& aIterable, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global =
    do_QueryInterface(aGlobal.GetAsSupports());
  if (!global) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  JSContext* cx = aGlobal.Context();

  JS::Rooted<JSObject*> obj(cx, JS::CurrentGlobalOrNull(cx));
  if (!obj) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Create(global, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  nsRefPtr<PromiseCallback> resolveCb =
    new ResolvePromiseCallback(promise, obj);

  nsRefPtr<PromiseCallback> rejectCb = new RejectPromiseCallback(promise, obj);

  for (uint32_t i = 0; i < aIterable.Length(); ++i) {
    JS::Rooted<JS::Value> value(cx, aIterable.ElementAt(i));
    nsRefPtr<Promise> nextPromise = Promise::Resolve(aGlobal, value, aRv);
    
    
    
    
    MOZ_ASSERT(!aRv.Failed());
    nextPromise->AppendCallbacks(resolveCb, rejectCb);
  }

  return promise.forget();
}

void
Promise::AppendNativeHandler(PromiseNativeHandler* aRunnable)
{
  nsRefPtr<PromiseCallback> resolveCb =
    new NativePromiseCallback(aRunnable, Resolved);

  nsRefPtr<PromiseCallback> rejectCb =
    new NativePromiseCallback(aRunnable, Rejected);

  AppendCallbacks(resolveCb, rejectCb);
}

JSObject*
Promise::GlobalJSObject() const
{
  return mGlobal->GetGlobalJSObject();
}

JSCompartment*
Promise::Compartment() const
{
  return js::GetObjectCompartment(GlobalJSObject());
}

void
Promise::AppendCallbacks(PromiseCallback* aResolveCallback,
                         PromiseCallback* aRejectCallback)
{
  if (mGlobal->IsDying()) {
    return;
  }

  MOZ_ASSERT(aResolveCallback);
  MOZ_ASSERT(aRejectCallback);

  if (mIsLastInChain && mState == PromiseState::Rejected) {
    
    PromiseDebugging::AddConsumedRejection(*this);
    
    
    
  }
  mIsLastInChain = false;

#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
  
  mHadRejectCallback = true;
  RemoveFeature();
#endif 

  mResolveCallbacks.AppendElement(aResolveCallback);
  mRejectCallbacks.AppendElement(aRejectCallback);

  
  
  
  if (mState != Pending) {
    EnqueueCallbackTasks();
  }
}

class WrappedWorkerRunnable final : public WorkerSameThreadRunnable
{
public:
  WrappedWorkerRunnable(workers::WorkerPrivate* aWorkerPrivate, nsIRunnable* aRunnable)
    : WorkerSameThreadRunnable(aWorkerPrivate)
    , mRunnable(aRunnable)
  {
    MOZ_ASSERT(aRunnable);
    MOZ_COUNT_CTOR(WrappedWorkerRunnable);
  }

  bool
  WorkerRun(JSContext* aCx, workers::WorkerPrivate* aWorkerPrivate) override
  {
    NS_ASSERT_OWNINGTHREAD(WrappedWorkerRunnable);
    mRunnable->Run();
    return true;
  }

private:
  virtual
  ~WrappedWorkerRunnable()
  {
    MOZ_COUNT_DTOR(WrappedWorkerRunnable);
    NS_ASSERT_OWNINGTHREAD(WrappedWorkerRunnable);
  }

  nsCOMPtr<nsIRunnable> mRunnable;
  NS_DECL_OWNINGTHREAD
};

 void
Promise::DispatchToMicroTask(nsIRunnable* aRunnable)
{
  MOZ_ASSERT(aRunnable);

  CycleCollectedJSRuntime* runtime = CycleCollectedJSRuntime::Get();
  nsTArray<nsCOMPtr<nsIRunnable>>& microtaskQueue =
    runtime->GetPromiseMicroTaskQueue();

  microtaskQueue.AppendElement(aRunnable);
}

#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
void
Promise::MaybeReportRejected()
{
  if (mState != Rejected || mHadRejectCallback || mResult.isUndefined()) {
    return;
  }

  AutoJSAPI jsapi;
  
  
  jsapi.Init();
  JSContext* cx = jsapi.cx();
  JS::Rooted<JSObject*> obj(cx, GetWrapper());
  MOZ_ASSERT(obj); 
  JS::Rooted<JS::Value> val(cx, mResult);
  JS::ExposeValueToActiveJS(val);

  JSAutoCompartment ac(cx, obj);
  if (!JS_WrapValue(cx, &val)) {
    JS_ClearPendingException(cx);
    return;
  }

  js::ErrorReport report(cx);
  if (!report.init(cx, val)) {
    JS_ClearPendingException(cx);
    return;
  }

  nsRefPtr<xpc::ErrorReport> xpcReport = new xpc::ErrorReport();
  bool isMainThread = MOZ_LIKELY(NS_IsMainThread());
  bool isChrome = isMainThread ? nsContentUtils::IsSystemPrincipal(nsContentUtils::ObjectPrincipal(obj))
                               : GetCurrentThreadWorkerPrivate()->IsChromeWorker();
  nsPIDOMWindow* win = isMainThread ? xpc::WindowGlobalOrNull(obj) : nullptr;
  xpcReport->Init(report.report(), report.message(), isChrome, win ? win->WindowID() : 0);

  
  
  
  
  nsRefPtr<AsyncErrorReporter> r =
    new AsyncErrorReporter(CycleCollectedJSRuntime::Get()->Runtime(), xpcReport);
  NS_DispatchToMainThread(r);
}
#endif 

void
Promise::MaybeResolveInternal(JSContext* aCx,
                              JS::Handle<JS::Value> aValue)
{
  if (mResolvePending) {
    return;
  }

  ResolveInternal(aCx, aValue);
}

void
Promise::MaybeRejectInternal(JSContext* aCx,
                             JS::Handle<JS::Value> aValue)
{
  if (mResolvePending) {
    return;
  }

  RejectInternal(aCx, aValue);
}

void
Promise::HandleException(JSContext* aCx)
{
  JS::Rooted<JS::Value> exn(aCx);
  if (JS_GetPendingException(aCx, &exn)) {
    JS_ClearPendingException(aCx);
    RejectInternal(aCx, exn);
  }
}

void
Promise::ResolveInternal(JSContext* aCx,
                         JS::Handle<JS::Value> aValue)
{
  mResolvePending = true;

  if (aValue.isObject()) {
    AutoDontReportUncaught silenceReporting(aCx);
    JS::Rooted<JSObject*> valueObj(aCx, &aValue.toObject());

    
    JS::Rooted<JS::Value> then(aCx);
    if (!JS_GetProperty(aCx, valueObj, "then", &then)) {
      HandleException(aCx);
      return;
    }

    if (then.isObject() && JS::IsCallable(&then.toObject())) {
      
      JS::Rooted<JSObject*> thenObj(aCx, &then.toObject());

      
      
      
      
      
      
      
      
      
      
      Promise* nextPromise;
      if (PromiseBinding::IsThenMethod(thenObj) &&
          NS_SUCCEEDED(UNWRAP_OBJECT(Promise, valueObj, nextPromise))) {
        
        
        
        
        
        
        
        
        JS::Rooted<JSObject*> glob(aCx, GlobalJSObject());
        nsRefPtr<PromiseCallback> resolveCb = new ResolvePromiseCallback(this, glob);
        nsRefPtr<PromiseCallback> rejectCb = new RejectPromiseCallback(this, glob);
        nsRefPtr<FastThenableResolverTask> task =
          new FastThenableResolverTask(resolveCb, rejectCb, nextPromise);
        DispatchToMicroTask(task);
        return;
      }

      nsRefPtr<PromiseInit> thenCallback =
        new PromiseInit(thenObj, mozilla::dom::GetIncumbentGlobal());
      nsRefPtr<ThenableResolverTask> task =
        new ThenableResolverTask(this, valueObj, thenCallback);
      DispatchToMicroTask(task);
      return;
    }
  }

  MaybeSettle(aValue, Resolved);
}

void
Promise::RejectInternal(JSContext* aCx,
                        JS::Handle<JS::Value> aValue)
{
  mResolvePending = true;

  MaybeSettle(aValue, Rejected);
}

void
Promise::Settle(JS::Handle<JS::Value> aValue, PromiseState aState)
{
  if (mGlobal->IsDying()) {
    return;
  }

  mSettlementTimestamp = TimeStamp::Now();

  SetResult(aValue);
  SetState(aState);

  AutoJSAPI jsapi;
  jsapi.Init();
  JSContext* cx = jsapi.cx();
  JS::RootedObject wrapper(cx, GetWrapper());
  MOZ_ASSERT(wrapper); 
  JSAutoCompartment ac(cx, wrapper);
  JS::dbg::onPromiseSettled(cx, wrapper);

  if (aState == PromiseState::Rejected &&
      mIsLastInChain) {
    
    
    
    
    PromiseDebugging::AddUncaughtRejection(*this);
  }

#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
  
  
  if (aState == PromiseState::Rejected &&
      !mHadRejectCallback &&
      !NS_IsMainThread()) {
    workers::WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    worker->AssertIsOnWorkerThread();

    mFeature = new PromiseReportRejectFeature(this);
    if (NS_WARN_IF(!worker->AddFeature(worker->GetJSContext(), mFeature))) {
      
      mFeature = nullptr;
      
      
      MaybeReportRejectedOnce();
    }
  }
#endif 

  EnqueueCallbackTasks();
}

void
Promise::MaybeSettle(JS::Handle<JS::Value> aValue,
                     PromiseState aState)
{
  
  
  
  if (mState != Pending) {
    return;
  }

  Settle(aValue, aState);
}

void
Promise::EnqueueCallbackTasks()
{
  nsTArray<nsRefPtr<PromiseCallback>> callbacks;
  callbacks.SwapElements(mState == Resolved ? mResolveCallbacks
                                            : mRejectCallbacks);
  mResolveCallbacks.Clear();
  mRejectCallbacks.Clear();

  for (uint32_t i = 0; i < callbacks.Length(); ++i) {
    nsRefPtr<PromiseCallbackTask> task =
      new PromiseCallbackTask(this, callbacks[i], mResult);
    DispatchToMicroTask(task);
  }
}

#if defined(DOM_PROMISE_DEPRECATED_REPORTING)
void
Promise::RemoveFeature()
{
  if (mFeature) {
    workers::WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    worker->RemoveFeature(worker->GetJSContext(), mFeature);
    mFeature = nullptr;
  }
}

bool
PromiseReportRejectFeature::Notify(JSContext* aCx, workers::Status aStatus)
{
  MOZ_ASSERT(aStatus > workers::Running);
  mPromise->MaybeReportRejectedOnce();
  
  return true;
}
#endif 

bool
Promise::CaptureStack(JSContext* aCx, JS::Heap<JSObject*>& aTarget)
{
  JS::Rooted<JSObject*> stack(aCx);
  if (!JS::CaptureCurrentStack(aCx, &stack)) {
    return false;
  }
  aTarget = stack;
  return true;
}

void
Promise::GetDependentPromises(nsTArray<nsRefPtr<Promise>>& aPromises)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  for (size_t i = 0; i < mRejectCallbacks.Length(); ++i) {
    Promise* p = mRejectCallbacks[i]->GetDependentPromise();
    if (p) {
      aPromises.AppendElement(p);
    }
  }
}



class PromiseWorkerProxyRunnable : public workers::WorkerRunnable
{
public:
  PromiseWorkerProxyRunnable(PromiseWorkerProxy* aPromiseWorkerProxy,
                             const JSStructuredCloneCallbacks* aCallbacks,
                             JSAutoStructuredCloneBuffer&& aBuffer,
                             PromiseWorkerProxy::RunCallbackFunc aFunc)
    : WorkerRunnable(aPromiseWorkerProxy->GetWorkerPrivate(),
                     WorkerThreadUnchangedBusyCount)
    , mPromiseWorkerProxy(aPromiseWorkerProxy)
    , mCallbacks(aCallbacks)
    , mBuffer(Move(aBuffer))
    , mFunc(aFunc)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mPromiseWorkerProxy);
  }

  virtual bool
  WorkerRun(JSContext* aCx, workers::WorkerPrivate* aWorkerPrivate)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
    MOZ_ASSERT(aWorkerPrivate == mWorkerPrivate);

    MOZ_ASSERT(mPromiseWorkerProxy);
    nsRefPtr<Promise> workerPromise = mPromiseWorkerProxy->GetWorkerPromise();
    MOZ_ASSERT(workerPromise);

    
    JS::Rooted<JS::Value> value(aCx);
    if (!mBuffer.read(aCx, &value, mCallbacks, mPromiseWorkerProxy)) {
      JS_ClearPendingException(aCx);
      return false;
    }

    
    (workerPromise.get()->*mFunc)(aCx,
                                  value);

    
    mPromiseWorkerProxy->CleanUp(aCx);
    return true;
  }

protected:
  ~PromiseWorkerProxyRunnable() {}

private:
  nsRefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
  const JSStructuredCloneCallbacks* mCallbacks;
  JSAutoStructuredCloneBuffer mBuffer;

  
  PromiseWorkerProxy::RunCallbackFunc mFunc;
};


already_AddRefed<PromiseWorkerProxy>
PromiseWorkerProxy::Create(workers::WorkerPrivate* aWorkerPrivate,
                           Promise* aWorkerPromise,
                           const JSStructuredCloneCallbacks* aCb)
{
  MOZ_ASSERT(aWorkerPrivate);
  aWorkerPrivate->AssertIsOnWorkerThread();
  MOZ_ASSERT(aWorkerPromise);

  nsRefPtr<PromiseWorkerProxy> proxy =
    new PromiseWorkerProxy(aWorkerPrivate, aWorkerPromise, aCb);

  
  
  if (!aWorkerPrivate->AddFeature(aWorkerPrivate->GetJSContext(), proxy)) {
    
    
    proxy->mCleanedUp = true;
    proxy->mWorkerPromise = nullptr;
    return nullptr;
  }

  return proxy.forget();
}

PromiseWorkerProxy::PromiseWorkerProxy(workers::WorkerPrivate* aWorkerPrivate,
                                       Promise* aWorkerPromise,
                                       const JSStructuredCloneCallbacks* aCallbacks)
  : mWorkerPrivate(aWorkerPrivate)
  , mWorkerPromise(aWorkerPromise)
  , mCleanedUp(false)
  , mCallbacks(aCallbacks)
  , mCleanUpLock("cleanUpLock")
{
}

PromiseWorkerProxy::~PromiseWorkerProxy()
{
  MOZ_ASSERT(mCleanedUp);
  MOZ_ASSERT(!mWorkerPromise);
}

workers::WorkerPrivate*
PromiseWorkerProxy::GetWorkerPrivate() const
{
  
  
  MOZ_ASSERT(!mCleanedUp);

  return mWorkerPrivate;
}

Promise*
PromiseWorkerProxy::GetWorkerPromise() const
{
  return mWorkerPromise;
}

void
PromiseWorkerProxy::StoreISupports(nsISupports* aSupports)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsMainThreadPtrHandle<nsISupports> supports(
    new nsMainThreadPtrHolder<nsISupports>(aSupports));
  mSupportsArray.AppendElement(supports);
}

bool
PromiseWorkerProxyControlRunnable::WorkerRun(JSContext* aCx,
                                             workers::WorkerPrivate* aWorkerPrivate)
{
  mProxy->CleanUp(aCx);
  return true;
}

void
PromiseWorkerProxy::RunCallback(JSContext* aCx,
                                JS::Handle<JS::Value> aValue,
                                RunCallbackFunc aFunc)
{
  MOZ_ASSERT(NS_IsMainThread());

  MutexAutoLock lock(mCleanUpLock);
  
  if (mCleanedUp) {
    return;
  }

  
  
  
  JSAutoStructuredCloneBuffer buffer;
  if (!buffer.write(aCx, aValue, mCallbacks, this)) {
    JS_ClearPendingException(aCx);
    MOZ_ASSERT(false, "cannot write the JSAutoStructuredCloneBuffer!");
  }

  nsRefPtr<PromiseWorkerProxyRunnable> runnable =
    new PromiseWorkerProxyRunnable(this,
                                   mCallbacks,
                                   Move(buffer),
                                   aFunc);

  if (!runnable->Dispatch(aCx)) {
    nsRefPtr<WorkerControlRunnable> runnable =
      new PromiseWorkerProxyControlRunnable(mWorkerPrivate, this);
    mWorkerPrivate->DispatchControlRunnable(runnable);
  }
}

void
PromiseWorkerProxy::ResolvedCallback(JSContext* aCx,
                                     JS::Handle<JS::Value> aValue)
{
  RunCallback(aCx, aValue, &Promise::ResolveInternal);
}

void
PromiseWorkerProxy::RejectedCallback(JSContext* aCx,
                                     JS::Handle<JS::Value> aValue)
{
  RunCallback(aCx, aValue, &Promise::RejectInternal);
}

bool
PromiseWorkerProxy::Notify(JSContext* aCx, Status aStatus)
{
  MOZ_ASSERT(mWorkerPrivate);
  mWorkerPrivate->AssertIsOnWorkerThread();
  MOZ_ASSERT(mWorkerPrivate->GetJSContext() == aCx);

  if (aStatus >= Canceling) {
    CleanUp(aCx);
  }

  return true;
}

void
PromiseWorkerProxy::CleanUp(JSContext* aCx)
{
  MutexAutoLock lock(mCleanUpLock);

  
  
  if (mCleanedUp) {
    return;
  }

  MOZ_ASSERT(mWorkerPrivate);
  mWorkerPrivate->AssertIsOnWorkerThread();
  MOZ_ASSERT(mWorkerPrivate->GetJSContext() == aCx);

  
  
  
  mWorkerPromise = nullptr;
  mWorkerPrivate->RemoveFeature(aCx, this);
  mCleanedUp = true;
}


template<>
void Promise::MaybeRejectBrokenly(const nsRefPtr<DOMError>& aArg) {
  MaybeSomething(aArg, &Promise::MaybeReject);
}
template<>
void Promise::MaybeRejectBrokenly(const nsAString& aArg) {
  MaybeSomething(aArg, &Promise::MaybeReject);
}

uint64_t
Promise::GetID() {
  if (mID != 0) {
    return mID;
  }
  return mID = ++gIDGenerator;
}

} 
} 
