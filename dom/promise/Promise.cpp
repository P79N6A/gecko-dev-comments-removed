





#include "mozilla/dom/Promise.h"

#include "jsfriendapi.h"
#include "mozilla/dom/OwningNonNull.h"
#include "mozilla/dom/PromiseBinding.h"
#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/Preferences.h"
#include "PromiseCallback.h"
#include "PromiseNativeHandler.h"
#include "nsContentUtils.h"
#include "WorkerPrivate.h"
#include "WorkerRunnable.h"
#include "nsJSPrincipals.h"
#include "nsJSUtils.h"
#include "nsPIDOMWindow.h"
#include "nsJSEnvironment.h"

namespace mozilla {
namespace dom {

using namespace workers;

NS_IMPL_ISUPPORTS0(PromiseNativeHandler)




class PromiseTask MOZ_FINAL : public nsRunnable
{
public:
  PromiseTask(Promise* aPromise)
    : mPromise(aPromise)
  {
    MOZ_ASSERT(aPromise);
    MOZ_COUNT_CTOR(PromiseTask);
  }

  ~PromiseTask()
  {
    MOZ_COUNT_DTOR(PromiseTask);
  }

  NS_IMETHOD Run()
  {
    mPromise->mTaskPending = false;
    mPromise->RunTask();
    return NS_OK;
  }

private:
  nsRefPtr<Promise> mPromise;
};

class WorkerPromiseTask MOZ_FINAL : public WorkerSameThreadRunnable
{
public:
  WorkerPromiseTask(WorkerPrivate* aWorkerPrivate, Promise* aPromise)
    : WorkerSameThreadRunnable(aWorkerPrivate)
    , mPromise(aPromise)
  {
    MOZ_ASSERT(aPromise);
    MOZ_COUNT_CTOR(WorkerPromiseTask);
  }

  ~WorkerPromiseTask()
  {
    MOZ_COUNT_DTOR(WorkerPromiseTask);
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    mPromise->mTaskPending = false;
    mPromise->RunTask();
    return true;
  }

private:
  nsRefPtr<Promise> mPromise;
};

class PromiseResolverMixin
{
public:
  PromiseResolverMixin(Promise* aPromise,
                       JS::Handle<JS::Value> aValue,
                       Promise::PromiseState aState)
    : mPromise(aPromise)
    , mValue(aValue)
    , mState(aState)
  {
    MOZ_ASSERT(aPromise);
    MOZ_ASSERT(mState != Promise::Pending);
    MOZ_COUNT_CTOR(PromiseResolverMixin);

    



    JS_AddNamedValueRootRT(CycleCollectedJSRuntime::Get()->Runtime(), mValue.unsafeGet(),
                           "PromiseResolverMixin.mValue");
  }

  virtual ~PromiseResolverMixin()
  {
    NS_ASSERT_OWNINGTHREAD(PromiseResolverMixin);
    MOZ_COUNT_DTOR(PromiseResolverMixin);

    



    JS_RemoveValueRootRT(CycleCollectedJSRuntime::Get()->Runtime(), mValue.unsafeGet());
  }

protected:
  void
  RunInternal()
  {
    NS_ASSERT_OWNINGTHREAD(PromiseResolverMixin);
    mPromise->RunResolveTask(
      JS::Handle<JS::Value>::fromMarkedLocation(mValue.address()),
      mState, Promise::SyncTask);
  }

private:
  nsRefPtr<Promise> mPromise;
  JS::Heap<JS::Value> mValue;
  Promise::PromiseState mState;
  NS_DECL_OWNINGTHREAD;
};


class PromiseResolverTask MOZ_FINAL : public nsRunnable,
                                      public PromiseResolverMixin
{
public:
  PromiseResolverTask(Promise* aPromise,
                      JS::Handle<JS::Value> aValue,
                      Promise::PromiseState aState)
    : PromiseResolverMixin(aPromise, aValue, aState)
  {}

  ~PromiseResolverTask()
  {}

  NS_IMETHOD Run()
  {
    RunInternal();
    return NS_OK;
  }
};

class WorkerPromiseResolverTask MOZ_FINAL : public WorkerSameThreadRunnable,
                                            public PromiseResolverMixin
{
public:
  WorkerPromiseResolverTask(WorkerPrivate* aWorkerPrivate,
                            Promise* aPromise,
                            JS::Handle<JS::Value> aValue,
                            Promise::PromiseState aState)
    : WorkerSameThreadRunnable(aWorkerPrivate),
      PromiseResolverMixin(aPromise, aValue, aState)
  {}

  ~WorkerPromiseResolverTask()
  {}

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    RunInternal();
    return true;
  }
};



NS_IMPL_CYCLE_COLLECTION_CLASS(Promise)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(Promise)
  tmp->MaybeReportRejectedOnce();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mGlobal)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mResolveCallbacks);
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRejectCallbacks);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(Promise)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mGlobal)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mResolveCallbacks);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRejectCallbacks);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(Promise)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(mResult)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(Promise)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Promise)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Promise)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Promise::Promise(nsIGlobalObject* aGlobal)
  : mGlobal(aGlobal)
  , mResult(JS::UndefinedValue())
  , mState(Pending)
  , mTaskPending(false)
  , mHadRejectCallback(false)
  , mResolvePending(false)
{
  MOZ_ASSERT(mGlobal);

  mozilla::HoldJSObjects(this);
  SetIsDOMBinding();
}

Promise::~Promise()
{
  MaybeReportRejectedOnce();
  mozilla::DropJSObjects(this);
}

JSObject*
Promise::WrapObject(JSContext* aCx)
{
  return PromiseBinding::Wrap(aCx, this);
}

JSObject*
Promise::GetOrCreateWrapper(JSContext* aCx)
{
  if (JSObject* wrapper = GetWrapper()) {
    return wrapper;
  }

  nsIGlobalObject* global = GetParentObject();
  MOZ_ASSERT(global);

  JS::Rooted<JSObject*> scope(aCx, global->GetGlobalJSObject());
  if (!scope) {
    JS_ReportError(aCx, "can't get scope");
    return nullptr;
  }

  JSAutoCompartment ac(aCx, scope);

  JS::Rooted<JS::Value> val(aCx);
  if (!WrapNewBindingObject(aCx, this, &val)) {
    MOZ_ASSERT(JS_IsExceptionPending(aCx));
    return nullptr;
  }

  return GetWrapper();
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

static void
EnterCompartment(Maybe<JSAutoCompartment>& aAc, JSContext* aCx,
                 JS::Handle<JS::Value> aValue)
{
  
  if (aValue.isObject()) {
    JS::Rooted<JSObject*> rooted(aCx, &aValue.toObject());
    aAc.construct(aCx, rooted);
  }
}

enum {
  SLOT_PROMISE = 0,
  SLOT_DATA
};

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
  } else {
    promise->MaybeRejectInternal(aCx, args.get(0));
  }

  return true;
}










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






 bool
Promise::ThenableResolverCommon(JSContext* aCx, uint32_t aTask,
                                unsigned aArgc, JS::Value* aVp)
{
  JS::CallArgs args = CallArgsFromVp(aArgc, aVp);
  JS::Rooted<JSObject*> thisFunc(aCx, &args.callee());
  if (!MarkAsCalledIfNotCalledBefore(aCx, thisFunc)) {
    
    return true;
  }

  Promise* promise = GetPromise(aCx, thisFunc);
  MOZ_ASSERT(promise);

  if (aTask == PromiseCallback::Resolve) {
    promise->ResolveInternal(aCx, args.get(0), SyncTask);
  } else {
    promise->RejectInternal(aCx, args.get(0), SyncTask);
  }
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
Promise::CreateFunction(JSContext* aCx, JSObject* aParent, Promise* aPromise,
                        int32_t aTask)
{
  JSFunction* func = js::NewFunctionWithReserved(aCx, JSCallback,
                                                 1 , 0 ,
                                                 aParent, nullptr);
  if (!func) {
    return nullptr;
  }

  JS::Rooted<JSObject*> obj(aCx, JS_GetFunctionObject(func));

  JS::Rooted<JS::Value> promiseObj(aCx);
  if (!dom::WrapNewBindingObject(aCx, aPromise, &promiseObj)) {
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
                                                 nullptr, nullptr);
  if (!func) {
    return nullptr;
  }

  JS::Rooted<JSObject*> obj(aCx, JS_GetFunctionObject(func));

  JS::Rooted<JS::Value> promiseObj(aCx);
  if (!dom::WrapNewBindingObject(aCx, aPromise, &promiseObj)) {
    return nullptr;
  }

  js::SetFunctionNativeReserved(obj, SLOT_PROMISE, promiseObj);

  return obj;
}

 already_AddRefed<Promise>
Promise::Constructor(const GlobalObject& aGlobal,
                     PromiseInit& aInit, ErrorResult& aRv)
{
  JSContext* cx = aGlobal.GetContext();

  nsCOMPtr<nsIGlobalObject> global;
  global = do_QueryInterface(aGlobal.GetAsSupports());
  if (!global) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<Promise> promise = new Promise(global);

  JS::Rooted<JSObject*> resolveFunc(cx,
                                    CreateFunction(cx, aGlobal.Get(), promise,
                                                   PromiseCallback::Resolve));
  if (!resolveFunc) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  JS::Rooted<JSObject*> rejectFunc(cx,
                                   CreateFunction(cx, aGlobal.Get(), promise,
                                                  PromiseCallback::Reject));
  if (!rejectFunc) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  aInit.Call(resolveFunc, rejectFunc, aRv, CallbackObject::eRethrowExceptions);
  aRv.WouldReportJSException();

  if (aRv.IsJSException()) {
    JS::Rooted<JS::Value> value(cx);
    aRv.StealJSException(cx, &value);

    Maybe<JSAutoCompartment> ac;
    EnterCompartment(ac, cx, value);
    promise->MaybeRejectInternal(cx, value);
  }

  return promise.forget();
}

 already_AddRefed<Promise>
Promise::Resolve(const GlobalObject& aGlobal, JSContext* aCx,
                 JS::Handle<JS::Value> aValue, ErrorResult& aRv)
{
  
  if (aValue.isObject()) {
    JS::Rooted<JSObject*> valueObj(aCx, &aValue.toObject());
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

  return Resolve(global, aCx, aValue, aRv);
}

 already_AddRefed<Promise>
Promise::Resolve(nsIGlobalObject* aGlobal, JSContext* aCx,
                 JS::Handle<JS::Value> aValue, ErrorResult& aRv)
{
  nsRefPtr<Promise> promise = new Promise(aGlobal);

  promise->MaybeResolveInternal(aCx, aValue);
  return promise.forget();
}

 already_AddRefed<Promise>
Promise::Reject(const GlobalObject& aGlobal, JSContext* aCx,
                JS::Handle<JS::Value> aValue, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global =
    do_QueryInterface(aGlobal.GetAsSupports());
  if (!global) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  return Reject(global, aCx, aValue, aRv);
}

 already_AddRefed<Promise>
Promise::Reject(nsIGlobalObject* aGlobal, JSContext* aCx,
                JS::Handle<JS::Value> aValue, ErrorResult& aRv)
{
  nsRefPtr<Promise> promise = new Promise(aGlobal);

  promise->MaybeRejectInternal(aCx, aValue);
  return promise.forget();
}

already_AddRefed<Promise>
Promise::Then(AnyCallback* aResolveCallback, AnyCallback* aRejectCallback)
{
  nsRefPtr<Promise> promise = new Promise(GetParentObject());

  nsRefPtr<PromiseCallback> resolveCb =
    PromiseCallback::Factory(promise, aResolveCallback, PromiseCallback::Resolve);

  nsRefPtr<PromiseCallback> rejectCb =
    PromiseCallback::Factory(promise, aRejectCallback, PromiseCallback::Reject);

  AppendCallbacks(resolveCb, rejectCb);

  return promise.forget();
}

already_AddRefed<Promise>
Promise::Catch(AnyCallback* aRejectCallback)
{
  nsRefPtr<AnyCallback> resolveCb;
  return Then(resolveCb, aRejectCallback);
}






class CountdownHolder MOZ_FINAL : public nsISupports
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CountdownHolder)

  CountdownHolder(const GlobalObject& aGlobal, Promise* aPromise, uint32_t aCountdown)
    : mPromise(aPromise), mCountdown(aCountdown)
  {
    MOZ_ASSERT(aCountdown != 0);
    JSContext* cx = aGlobal.GetContext();
    JSAutoCompartment ac(cx, aGlobal.Get());
    mValues = JS_NewArrayObject(cx, aCountdown);
    mozilla::HoldJSObjects(this);
  }

  ~CountdownHolder()
  {
    mozilla::DropJSObjects(this);
  }

  void SetValue(uint32_t index, const JS::Handle<JS::Value> aValue)
  {
    MOZ_ASSERT(mCountdown > 0);

    ThreadsafeAutoSafeJSContext cx;
    JSAutoCompartment ac(cx, mValues);

    {
      AutoDontReportUncaught silenceReporting(cx);
      if (!JS_DefineElement(cx, mValues, index, aValue, nullptr, nullptr, JSPROP_ENUMERATE)) {
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






class AllResolveHandler MOZ_FINAL : public PromiseNativeHandler
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(AllResolveHandler)

  AllResolveHandler(CountdownHolder* aHolder, uint32_t aIndex)
    : mCountdownHolder(aHolder), mIndex(aIndex)
  {
    MOZ_ASSERT(aHolder);
  }

  ~AllResolveHandler()
  {
  }

  void
  ResolvedCallback(JS::Handle<JS::Value> aValue)
  {
    mCountdownHolder->SetValue(mIndex, aValue);
  }

  void
  RejectedCallback(JS::Handle<JS::Value> aValue)
  {
    
    MOZ_ASSERT(false, "AllResolveHandler should never be attached to a Promise's reject handler!");
  }

private:
  nsRefPtr<CountdownHolder> mCountdownHolder;
  uint32_t mIndex;
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(AllResolveHandler)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AllResolveHandler)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AllResolveHandler)
NS_INTERFACE_MAP_END_INHERITING(PromiseNativeHandler)

NS_IMPL_CYCLE_COLLECTION_1(AllResolveHandler, mCountdownHolder)

 already_AddRefed<Promise>
Promise::All(const GlobalObject& aGlobal, JSContext* aCx,
             const Sequence<JS::Value>& aIterable, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global =
    do_QueryInterface(aGlobal.GetAsSupports());
  if (!global) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  if (aIterable.Length() == 0) {
    JS::Rooted<JSObject*> empty(aCx, JS_NewArrayObject(aCx, 0));
    if (!empty) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return nullptr;
    }
    JS::Rooted<JS::Value> value(aCx, JS::ObjectValue(*empty));
    return Promise::Resolve(aGlobal, aCx, value, aRv);
  }

  nsRefPtr<Promise> promise = new Promise(global);
  nsRefPtr<CountdownHolder> holder =
    new CountdownHolder(aGlobal, promise, aIterable.Length());

  nsRefPtr<PromiseCallback> rejectCb = new RejectPromiseCallback(promise);

  for (uint32_t i = 0; i < aIterable.Length(); ++i) {
    JS::Rooted<JS::Value> value(aCx, aIterable.ElementAt(i));
    nsRefPtr<Promise> nextPromise = Promise::Resolve(aGlobal, aCx, value, aRv);

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
Promise::Race(const GlobalObject& aGlobal, JSContext* aCx,
              const Sequence<JS::Value>& aIterable, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global =
    do_QueryInterface(aGlobal.GetAsSupports());
  if (!global) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<Promise> promise = new Promise(global);
  nsRefPtr<PromiseCallback> resolveCb = new ResolvePromiseCallback(promise);
  nsRefPtr<PromiseCallback> rejectCb = new RejectPromiseCallback(promise);

  for (uint32_t i = 0; i < aIterable.Length(); ++i) {
    JS::Rooted<JS::Value> value(aCx, aIterable.ElementAt(i));
    nsRefPtr<Promise> nextPromise = Promise::Resolve(aGlobal, aCx, value, aRv);
    
    
    
    
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

void
Promise::AppendCallbacks(PromiseCallback* aResolveCallback,
                         PromiseCallback* aRejectCallback)
{
  if (aResolveCallback) {
    mResolveCallbacks.AppendElement(aResolveCallback);
  }

  if (aRejectCallback) {
    mHadRejectCallback = true;
    mRejectCallbacks.AppendElement(aRejectCallback);

    
    RemoveFeature();
  }

  
  
  
  if (mState != Pending && !mTaskPending) {
    if (MOZ_LIKELY(NS_IsMainThread())) {
      nsRefPtr<PromiseTask> task = new PromiseTask(this);
      NS_DispatchToCurrentThread(task);
    } else {
      WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
      MOZ_ASSERT(worker);
      nsRefPtr<WorkerPromiseTask> task = new WorkerPromiseTask(worker, this);
      task->Dispatch(worker->GetJSContext());
    }
    mTaskPending = true;
  }
}

void
Promise::RunTask()
{
  MOZ_ASSERT(mState != Pending);

  nsTArray<nsRefPtr<PromiseCallback>> callbacks;
  callbacks.SwapElements(mState == Resolved ? mResolveCallbacks
                                            : mRejectCallbacks);
  mResolveCallbacks.Clear();
  mRejectCallbacks.Clear();

  ThreadsafeAutoJSContext cx; 
  JS::Rooted<JS::Value> value(cx, mResult);

  for (uint32_t i = 0; i < callbacks.Length(); ++i) {
    callbacks[i]->Call(value);
  }
}

void
Promise::MaybeReportRejected()
{
  if (mState != Rejected || mHadRejectCallback || mResult.isUndefined()) {
    return;
  }

  if (!mResult.isObject()) {
    return;
  }
  ThreadsafeAutoJSContext cx;
  JS::Rooted<JSObject*> obj(cx, &mResult.toObject());
  JSAutoCompartment ac(cx, obj);
  JSErrorReport* report = JS_ErrorFromException(cx, obj);
  if (!report) {
    return;
  }

  
  nsCOMPtr<nsPIDOMWindow> win;
  bool isChromeError = false;

  if (MOZ_LIKELY(NS_IsMainThread())) {
    win =
      do_QueryInterface(nsJSUtils::GetStaticScriptGlobal(obj));
    nsIPrincipal* principal = nsContentUtils::GetObjectPrincipal(obj);
    isChromeError = nsContentUtils::IsSystemPrincipal(principal);
  } else {
    WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    isChromeError = worker->IsChromeWorker();
  }

  
  
  
  
  nsRefPtr<AsyncErrorReporter> r =
    new AsyncErrorReporter(JS_GetObjectRuntime(obj),
                           report,
                           nullptr,
                           isChromeError,
                           win);
  NS_DispatchToMainThread(r);
}

void
Promise::MaybeResolveInternal(JSContext* aCx,
                              JS::Handle<JS::Value> aValue,
                              PromiseTaskSync aAsynchronous)
{
  if (mResolvePending) {
    return;
  }

  ResolveInternal(aCx, aValue, aAsynchronous);
}

void
Promise::MaybeRejectInternal(JSContext* aCx,
                             JS::Handle<JS::Value> aValue,
                             PromiseTaskSync aAsynchronous)
{
  if (mResolvePending) {
    return;
  }

  RejectInternal(aCx, aValue, aAsynchronous);
}

void
Promise::HandleException(JSContext* aCx)
{
  JS::Rooted<JS::Value> exn(aCx);
  if (JS_GetPendingException(aCx, &exn)) {
    JS_ClearPendingException(aCx);
    RejectInternal(aCx, exn, SyncTask);
  }
}

void
Promise::ResolveInternal(JSContext* aCx,
                         JS::Handle<JS::Value> aValue,
                         PromiseTaskSync aAsynchronous)
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

    if (then.isObject() && JS_ObjectIsCallable(aCx, &then.toObject())) {
      JS::Rooted<JSObject*> resolveFunc(aCx,
        CreateThenableFunction(aCx, this, PromiseCallback::Resolve));

      if (!resolveFunc) {
        HandleException(aCx);
        return;
      }

      JS::Rooted<JSObject*> rejectFunc(aCx,
        CreateThenableFunction(aCx, this, PromiseCallback::Reject));
      if (!rejectFunc) {
        HandleException(aCx);
        return;
      }

      LinkThenableCallables(aCx, resolveFunc, rejectFunc);

      JS::Rooted<JSObject*> thenObj(aCx, &then.toObject());
      nsRefPtr<PromiseInit> thenCallback =
        new PromiseInit(thenObj, mozilla::dom::GetIncumbentGlobal());

      ErrorResult rv;
      thenCallback->Call(valueObj, resolveFunc, rejectFunc,
                         rv, CallbackObject::eRethrowExceptions);
      rv.WouldReportJSException();

      if (rv.IsJSException()) {
        JS::Rooted<JS::Value> exn(aCx);
        rv.StealJSException(aCx, &exn);

        bool couldMarkAsCalled = MarkAsCalledIfNotCalledBefore(aCx, resolveFunc);

        
        
        if (couldMarkAsCalled) {
          Maybe<JSAutoCompartment> ac;
          EnterCompartment(ac, aCx, exn);
          RejectInternal(aCx, exn, Promise::SyncTask);
        }
        
        
        
      }

      return;
    }
  }

  
  
  
  
  RunResolveTask(aValue, Resolved, aAsynchronous);
}

void
Promise::RejectInternal(JSContext* aCx,
                        JS::Handle<JS::Value> aValue,
                        PromiseTaskSync aAsynchronous)
{
  mResolvePending = true;

  
  
  
  RunResolveTask(aValue, Rejected, aAsynchronous);
}

void
Promise::RunResolveTask(JS::Handle<JS::Value> aValue,
                        PromiseState aState,
                        PromiseTaskSync aAsynchronous)
{
  
  
  if (aAsynchronous == AsyncTask) {
    if (MOZ_LIKELY(NS_IsMainThread())) {
      nsRefPtr<PromiseResolverTask> task =
        new PromiseResolverTask(this, aValue, aState);
      NS_DispatchToCurrentThread(task);
    } else {
      WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
      MOZ_ASSERT(worker);
      nsRefPtr<WorkerPromiseResolverTask> task =
        new WorkerPromiseResolverTask(worker, this, aValue, aState);
      task->Dispatch(worker->GetJSContext());
    }
    return;
  }

  
  
  
  if (mState != Pending) {
    return;
  }

  SetResult(aValue);
  SetState(aState);

  
  
  if (aState == PromiseState::Rejected &&
      !mHadRejectCallback &&
      !NS_IsMainThread()) {
    WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    worker->AssertIsOnWorkerThread();

    mFeature = new PromiseReportRejectFeature(this);
    if (NS_WARN_IF(!worker->AddFeature(worker->GetJSContext(), mFeature))) {
      
      
      MaybeReportRejected();
    }
  }

  RunTask();
}

void
Promise::RemoveFeature()
{
  if (mFeature) {
    WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
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

bool
Promise::ArgumentToJSValue(const nsAString& aArgument,
                           JSContext* aCx,
                           JS::MutableHandle<JS::Value> aValue)
{
  
  
  nsStringBuffer* sharedBuffer;
  if (!XPCStringConvert::ReadableToJSVal(aCx, aArgument, &sharedBuffer,
                                         aValue)) {
    return false;
  }

  if (sharedBuffer) {
    NS_ADDREF(sharedBuffer);
  }

  return true;
}

bool
Promise::ArgumentToJSValue(bool aArgument,
                           JSContext* aCx,
                           JS::MutableHandle<JS::Value> aValue)
{
  aValue.setBoolean(aArgument);
  return true;
}

} 
} 
