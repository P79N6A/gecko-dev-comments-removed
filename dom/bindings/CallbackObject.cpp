





#include "mozilla/dom/CallbackObject.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/DOMErrorBinding.h"
#include "jsfriendapi.h"
#include "nsIScriptGlobalObject.h"
#include "nsIXPConnect.h"
#include "nsIScriptContext.h"
#include "nsPIDOMWindow.h"
#include "nsJSUtils.h"
#include "nsCxPusher.h"
#include "nsIScriptSecurityManager.h"
#include "xpcprivate.h"
#include "WorkerPrivate.h"
#include "nsGlobalWindow.h"
#include "WorkerScope.h"

namespace mozilla {
namespace dom {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CallbackObject)
  NS_INTERFACE_MAP_ENTRY(mozilla::dom::CallbackObject)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(CallbackObject)
NS_IMPL_CYCLE_COLLECTING_RELEASE(CallbackObject)

NS_IMPL_CYCLE_COLLECTION_CLASS(CallbackObject)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(CallbackObject)
  tmp->DropCallback();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mIncumbentGlobal)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(CallbackObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mIncumbentGlobal)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(CallbackObject)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mCallback)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

CallbackObject::CallSetup::CallSetup(CallbackObject* aCallback,
                                     ErrorResult& aRv,
                                     ExceptionHandling aExceptionHandling,
                                     JSCompartment* aCompartment)
  : mCx(nullptr)
  , mCompartment(aCompartment)
  , mErrorResult(aRv)
  , mExceptionHandling(aExceptionHandling)
  , mIsMainThread(NS_IsMainThread())
{
  if (mIsMainThread) {
    nsContentUtils::EnterMicroTask();
  }
  
  
  
  
  

  
  JSObject* realCallback = js::UncheckedUnwrap(aCallback->CallbackPreserveColor());
  JSContext* cx = nullptr;
  nsIGlobalObject* globalObject = nullptr;

  if (mIsMainThread) {
    
    nsGlobalWindow* win = xpc::WindowGlobalOrNull(realCallback);
    if (win) {
      
      
      
      
      
      MOZ_ASSERT(win->IsInnerWindow());
      nsPIDOMWindow* outer = win->GetOuterWindow();
      if (!outer || win != outer->GetCurrentInnerWindow()) {
        
        return;
      }
      cx = win->GetContext() ? win->GetContext()->GetNativeContext()
                             
                             
                             : nsContentUtils::GetSafeJSContext();
      globalObject = win;
    } else {
      
      JSObject* glob = js::GetGlobalForObjectCrossCompartment(realCallback);
      globalObject = xpc::GetNativeForGlobal(glob);
      MOZ_ASSERT(globalObject);
      cx = nsContentUtils::GetSafeJSContext();
    }
  } else {
    cx = workers::GetCurrentThreadJSContext();
    globalObject = workers::GetCurrentThreadWorkerPrivate()->GlobalScope();
  }

  
  
  
  if (!globalObject->GetGlobalJSObject()) {
    return;
  }

  mAutoEntryScript.construct(globalObject, mIsMainThread, cx);
  if (aCallback->IncumbentGlobalOrNull()) {
    mAutoIncumbentScript.construct(aCallback->IncumbentGlobalOrNull());
  }

  
  
  
  
  
  
  
  
  
  mRootedCallable.construct(cx, aCallback->Callback());

  if (mIsMainThread) {
    
    
    
    bool allowed = nsContentUtils::GetSecurityManager()->
      ScriptAllowed(js::GetGlobalForObjectCrossCompartment(realCallback));

    if (!allowed) {
      return;
    }
  }

  
  
  
  
  
  mAc.construct(cx, mRootedCallable.ref());

  
  mCx = cx;

  
  if (mExceptionHandling == eRethrowContentExceptions ||
      mExceptionHandling == eRethrowExceptions) {
    mSavedJSContextOptions = JS::ContextOptionsRef(cx);
    JS::ContextOptionsRef(cx).setDontReportUncaught(true);
  }
}

bool
CallbackObject::CallSetup::ShouldRethrowException(JS::Handle<JS::Value> aException)
{
  if (mExceptionHandling == eRethrowExceptions) {
    return true;
  }

  MOZ_ASSERT(mExceptionHandling == eRethrowContentExceptions);

  
  
  

  if (!aException.isObject()) {
    return false;
  }

  JS::Rooted<JSObject*> obj(mCx, &aException.toObject());
  obj = js::UncheckedUnwrap(obj,  false);
  if (js::GetObjectCompartment(obj) != mCompartment) {
    return false;
  }

  DOMError* domError;
  return NS_SUCCEEDED(UNWRAP_OBJECT(DOMError, obj, domError));
}

CallbackObject::CallSetup::~CallSetup()
{
  
  
  if (mCx) {
    bool dealtWithPendingException = false;
    if ((mCompartment && mExceptionHandling == eRethrowContentExceptions) ||
        mExceptionHandling == eRethrowExceptions) {
      
      JS::ContextOptionsRef(mCx) = mSavedJSContextOptions;
      mErrorResult.MightThrowJSException();
      if (JS_IsExceptionPending(mCx)) {
        JS::Rooted<JS::Value> exn(mCx);
        if (JS_GetPendingException(mCx, &exn) &&
            ShouldRethrowException(exn)) {
          mErrorResult.ThrowJSException(mCx, exn);
          JS_ClearPendingException(mCx);
          dealtWithPendingException = true;
        }
      }
    }

    if (!dealtWithPendingException) {
      
      
      
      nsJSUtils::ReportPendingException(mCx);
    }
  }

  
  
  mAc.destroyIfConstructed();

  mAutoIncumbentScript.destroyIfConstructed();
  mAutoEntryScript.destroyIfConstructed();

  
  
  if (mIsMainThread) {
    nsContentUtils::LeaveMicroTask();
  }
}

already_AddRefed<nsISupports>
CallbackObjectHolderBase::ToXPCOMCallback(CallbackObject* aCallback,
                                          const nsIID& aIID) const
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!aCallback) {
    return nullptr;
  }

  AutoSafeJSContext cx;

  JS::Rooted<JSObject*> callback(cx, aCallback->Callback());

  JSAutoCompartment ac(cx, callback);
  nsRefPtr<nsXPCWrappedJS> wrappedJS;
  nsresult rv =
    nsXPCWrappedJS::GetNewOrUsed(callback, aIID,
                                 nullptr, getter_AddRefs(wrappedJS));
  if (NS_FAILED(rv) || !wrappedJS) {
    return nullptr;
  }

  nsCOMPtr<nsISupports> retval;
  rv = wrappedJS->QueryInterface(aIID, getter_AddRefs(retval));
  if (NS_FAILED(rv)) {
    return nullptr;
  }

  return retval.forget();
}

} 
} 
