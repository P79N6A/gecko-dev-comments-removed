





#include "PromiseCallback.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseNativeHandler.h"

#include "js/OldDebugAPI.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTING_ADDREF(PromiseCallback)
NS_IMPL_CYCLE_COLLECTING_RELEASE(PromiseCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(PromiseCallback)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_CLASS(PromiseCallback)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(PromiseCallback)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(PromiseCallback)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

PromiseCallback::PromiseCallback()
{
  MOZ_COUNT_CTOR(PromiseCallback);
}

PromiseCallback::~PromiseCallback()
{
  MOZ_COUNT_DTOR(PromiseCallback);
}



NS_IMPL_CYCLE_COLLECTION_CLASS(ResolvePromiseCallback)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(ResolvePromiseCallback,
                                                PromiseCallback)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPromise)
  tmp->mGlobal = nullptr;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(ResolvePromiseCallback,
                                                  PromiseCallback)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPromise)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(ResolvePromiseCallback)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mGlobal)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ResolvePromiseCallback)
NS_INTERFACE_MAP_END_INHERITING(PromiseCallback)

NS_IMPL_ADDREF_INHERITED(ResolvePromiseCallback, PromiseCallback)
NS_IMPL_RELEASE_INHERITED(ResolvePromiseCallback, PromiseCallback)

ResolvePromiseCallback::ResolvePromiseCallback(Promise* aPromise,
                                               JS::Handle<JSObject*> aGlobal)
  : mPromise(aPromise)
  , mGlobal(aGlobal)
{
  MOZ_ASSERT(aPromise);
  MOZ_ASSERT(aGlobal);
  MOZ_COUNT_CTOR(ResolvePromiseCallback);
  HoldJSObjects(this);
}

ResolvePromiseCallback::~ResolvePromiseCallback()
{
  MOZ_COUNT_DTOR(ResolvePromiseCallback);
  DropJSObjects(this);
}

void
ResolvePromiseCallback::Call(JS::Handle<JS::Value> aValue)
{
  
  ThreadsafeAutoSafeJSContext cx;

  JSAutoCompartment ac(cx, mGlobal);
  JS::Rooted<JS::Value> value(cx, aValue);
  if (!JS_WrapValue(cx, &value)) {
    NS_WARNING("Failed to wrap value into the right compartment.");
    return;
  }

  mPromise->ResolveInternal(cx, value, Promise::SyncTask);
}



NS_IMPL_CYCLE_COLLECTION_CLASS(RejectPromiseCallback)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(RejectPromiseCallback,
                                                PromiseCallback)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPromise)
  tmp->mGlobal = nullptr;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(RejectPromiseCallback,
                                                  PromiseCallback)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPromise)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(RejectPromiseCallback)
NS_INTERFACE_MAP_END_INHERITING(PromiseCallback)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(RejectPromiseCallback,
                                               PromiseCallback)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mGlobal)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_ADDREF_INHERITED(RejectPromiseCallback, PromiseCallback)
NS_IMPL_RELEASE_INHERITED(RejectPromiseCallback, PromiseCallback)

RejectPromiseCallback::RejectPromiseCallback(Promise* aPromise,
                                             JS::Handle<JSObject*> aGlobal)
  : mPromise(aPromise)
  , mGlobal(aGlobal)
{
  MOZ_ASSERT(aPromise);
  MOZ_ASSERT(mGlobal);
  MOZ_COUNT_CTOR(RejectPromiseCallback);
  HoldJSObjects(this);
}

RejectPromiseCallback::~RejectPromiseCallback()
{
  MOZ_COUNT_DTOR(RejectPromiseCallback);
  DropJSObjects(this);
}

void
RejectPromiseCallback::Call(JS::Handle<JS::Value> aValue)
{
  
  ThreadsafeAutoSafeJSContext cx;

  JSAutoCompartment ac(cx, mGlobal);
  JS::Rooted<JS::Value> value(cx, aValue);
  if (!JS_WrapValue(cx, &value)) {
    NS_WARNING("Failed to wrap value into the right compartment.");
    return;
  }


  mPromise->RejectInternal(cx, value, Promise::SyncTask);
}


NS_IMPL_CYCLE_COLLECTION_CLASS(WrapperPromiseCallback)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(WrapperPromiseCallback,
                                                PromiseCallback)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mNextPromise)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mCallback)
  tmp->mGlobal = nullptr;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(WrapperPromiseCallback,
                                                  PromiseCallback)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNextPromise)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCallback)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(WrapperPromiseCallback)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mGlobal)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(WrapperPromiseCallback)
NS_INTERFACE_MAP_END_INHERITING(PromiseCallback)

NS_IMPL_ADDREF_INHERITED(WrapperPromiseCallback, PromiseCallback)
NS_IMPL_RELEASE_INHERITED(WrapperPromiseCallback, PromiseCallback)

WrapperPromiseCallback::WrapperPromiseCallback(Promise* aNextPromise,
                                               JS::Handle<JSObject*> aGlobal,
                                               AnyCallback* aCallback)
  : mNextPromise(aNextPromise)
  , mGlobal(aGlobal)
  , mCallback(aCallback)
{
  MOZ_ASSERT(aNextPromise);
  MOZ_ASSERT(aGlobal);
  MOZ_COUNT_CTOR(WrapperPromiseCallback);
  HoldJSObjects(this);
}

WrapperPromiseCallback::~WrapperPromiseCallback()
{
  MOZ_COUNT_DTOR(WrapperPromiseCallback);
  DropJSObjects(this);
}

void
WrapperPromiseCallback::Call(JS::Handle<JS::Value> aValue)
{
  ThreadsafeAutoSafeJSContext cx;

  JSAutoCompartment ac(cx, mGlobal);
  JS::Rooted<JS::Value> value(cx, aValue);
  if (!JS_WrapValue(cx, &value)) {
    NS_WARNING("Failed to wrap value into the right compartment.");
    return;
  }

  ErrorResult rv;

  
  
  JS::Rooted<JS::Value> retValue(cx,
    mCallback->Call(value, rv, CallbackObject::eRethrowExceptions));

  rv.WouldReportJSException();

  if (rv.Failed() && rv.IsJSException()) {
    JS::Rooted<JS::Value> value(cx);
    rv.StealJSException(cx, &value);

    if (!JS_WrapValue(cx, &value)) {
      NS_WARNING("Failed to wrap value into the right compartment.");
      return;
    }

    mNextPromise->RejectInternal(cx, value, Promise::SyncTask);
    return;
  }

  
  if (retValue.isObject()) {
    JS::Rooted<JSObject*> valueObj(cx, &retValue.toObject());
    Promise* returnedPromise;
    nsresult r = UNWRAP_OBJECT(Promise, valueObj, returnedPromise);

    if (NS_SUCCEEDED(r) && returnedPromise == mNextPromise) {
      const char* fileName = nullptr;
      uint32_t lineNumber = 0;

      
      
      JS::Rooted<JSObject*> unwrapped(cx,
        js::CheckedUnwrap(mCallback->Callback()));

      if (unwrapped) {
        JSAutoCompartment ac(cx, unwrapped);
        if (JS_ObjectIsFunction(cx, unwrapped)) {
          JS::Rooted<JS::Value> asValue(cx, JS::ObjectValue(*unwrapped));
          JS::Rooted<JSFunction*> func(cx, JS_ValueToFunction(cx, asValue));

          MOZ_ASSERT(func);
          JSScript* script = JS_GetFunctionScript(cx, func);
          if (script) {
            fileName = JS_GetScriptFilename(script);
            lineNumber = JS_GetScriptBaseLineNumber(cx, script);
          }
        }
      }

      
      JS::Rooted<JSString*> stack(cx, JS_GetEmptyString(JS_GetRuntime(cx)));
      JS::Rooted<JSString*> fn(cx, JS_NewStringCopyZ(cx, fileName));
      if (!fn) {
        
        JS_ClearPendingException(cx);
        return;
      }

      JS::Rooted<JSString*> message(cx,
        JS_NewStringCopyZ(cx,
          "then() cannot return same Promise that it resolves."));
      if (!message) {
        
        JS_ClearPendingException(cx);
        return;
      }

      JS::Rooted<JS::Value> typeError(cx);
      if (!JS::CreateTypeError(cx, stack, fn, lineNumber, 0,
                               nullptr, message, &typeError)) {
        
        JS_ClearPendingException(cx);
        return;
      }

      mNextPromise->RejectInternal(cx, typeError, Promise::SyncTask);
      return;
    }
  }

  
  
  if (!JS_WrapValue(cx, &retValue)) {
    NS_WARNING("Failed to wrap value into the right compartment.");
    return;
  }

  mNextPromise->ResolveInternal(cx, retValue, Promise::SyncTask);
}



NS_IMPL_CYCLE_COLLECTION_INHERITED_1(NativePromiseCallback,
                                     PromiseCallback, mHandler)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(NativePromiseCallback)
NS_INTERFACE_MAP_END_INHERITING(PromiseCallback)

NS_IMPL_ADDREF_INHERITED(NativePromiseCallback, PromiseCallback)
NS_IMPL_RELEASE_INHERITED(NativePromiseCallback, PromiseCallback)

NativePromiseCallback::NativePromiseCallback(PromiseNativeHandler* aHandler,
                                             Promise::PromiseState aState)
  : mHandler(aHandler)
  , mState(aState)
{
  MOZ_ASSERT(aHandler);
  MOZ_COUNT_CTOR(NativePromiseCallback);
}

NativePromiseCallback::~NativePromiseCallback()
{
  MOZ_COUNT_DTOR(NativePromiseCallback);
}

void
NativePromiseCallback::Call(JS::Handle<JS::Value> aValue)
{
  if (mState == Promise::Resolved) {
    mHandler->ResolvedCallback(aValue);
    return;
  }

  if (mState == Promise::Rejected) {
    mHandler->RejectedCallback(aValue);
    return;
  }

  NS_NOTREACHED("huh?");
}

 PromiseCallback*
PromiseCallback::Factory(Promise* aNextPromise, JS::Handle<JSObject*> aGlobal,
                         AnyCallback* aCallback, Task aTask)
{
  MOZ_ASSERT(aNextPromise);

  
  
  if (aCallback) {
    return new WrapperPromiseCallback(aNextPromise, aGlobal, aCallback);
  }

  if (aTask == Resolve) {
    return new ResolvePromiseCallback(aNextPromise, aGlobal);
  }

  if (aTask == Reject) {
    return new RejectPromiseCallback(aNextPromise, aGlobal);
  }

  MOZ_ASSERT(false, "This should not happen");
  return nullptr;
}

} 
} 
