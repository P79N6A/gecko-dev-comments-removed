





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
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(CallbackObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(CallbackObject)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mCallback)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

CallbackObject::CallSetup::CallSetup(JS::Handle<JSObject*> aCallback,
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
  
  
  
  
  

  
  JSObject* realCallback = js::UncheckedUnwrap(aCallback);
  JSContext* cx = nullptr;

  if (mIsMainThread) {
    
    nsIScriptContext* ctx = nullptr;
    nsIScriptGlobalObject* sgo = nsJSUtils::GetStaticScriptGlobal(realCallback);
    if (sgo) {
      
      
      
      
      
      nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(sgo);
      if (win) {
        MOZ_ASSERT(win->IsInnerWindow());
        nsPIDOMWindow* outer = win->GetOuterWindow();
        if (!outer || win != outer->GetCurrentInnerWindow()) {
          
          return;
        }
      }
      

      ctx = sgo->GetContext();
      if (ctx) {
        
        
        
        cx = ctx->GetNativeContext();
      }
    }

    if (!cx) {
      
      
      cx = nsContentUtils::GetSafeJSContext();
    }

    
    mCxPusher.Push(cx);
  } else {
    cx = workers::GetCurrentThreadJSContext();
  }

  
  
  
  
  
  
  
  
  JS::ExposeObjectToActiveJS(aCallback);
  mRootedCallable.construct(cx, aCallback);

  if (mIsMainThread) {
    
    
    
    
    nsresult rv = nsContentUtils::GetSecurityManager()->
      CheckFunctionAccess(cx, js::UncheckedUnwrap(aCallback), nullptr);

    if (NS_FAILED(rv)) {
      
      return;
    }
  }

  
  mAc.construct(cx, aCallback);

  
  mCx = cx;

  
  if (mExceptionHandling == eRethrowContentExceptions ||
      mExceptionHandling == eRethrowExceptions) {
    mSavedJSContextOptions = JS_GetOptions(cx);
    JS_SetOptions(cx, mSavedJSContextOptions | JSOPTION_DONT_REPORT_UNCAUGHT);
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
  return NS_SUCCEEDED(UNWRAP_OBJECT(DOMError, mCx, obj, domError));
}

CallbackObject::CallSetup::~CallSetup()
{
  
  
  if (mCx) {
    bool dealtWithPendingException = false;
    if (mExceptionHandling == eRethrowContentExceptions ||
        mExceptionHandling == eRethrowExceptions) {
      
      JS_SetOptions(mCx, mSavedJSContextOptions);
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

  
  
  
  
  

  
  mCxPusher.Pop();

  
  
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
