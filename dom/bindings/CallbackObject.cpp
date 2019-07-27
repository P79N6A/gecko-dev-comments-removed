





#include "mozilla/dom/CallbackObject.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/DOMErrorBinding.h"
#include "mozilla/dom/DOMException.h"
#include "mozilla/dom/DOMExceptionBinding.h"
#include "jsfriendapi.h"
#include "nsIScriptGlobalObject.h"
#include "nsIXPConnect.h"
#include "nsIScriptContext.h"
#include "nsPIDOMWindow.h"
#include "nsJSUtils.h"
#include "nsIScriptSecurityManager.h"
#include "xpcprivate.h"
#include "WorkerPrivate.h"
#include "nsGlobalWindow.h"
#include "WorkerScope.h"
#include "jsapi.h"
#include "nsJSPrincipals.h"

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
  tmp->DropJSObjects();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mIncumbentGlobal)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(CallbackObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mIncumbentGlobal)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(CallbackObject)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mCallback)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mIncumbentJSGlobal)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

CallbackObject::CallSetup::CallSetup(CallbackObject* aCallback,
                                     ErrorResult& aRv,
                                     const char* aExecutionReason,
                                     ExceptionHandling aExceptionHandling,
                                     JSCompartment* aCompartment,
                                     bool aIsJSImplementedWebIDL)
  : mCx(nullptr)
  , mCompartment(aCompartment)
  , mErrorResult(aRv)
  , mExceptionHandling(aExceptionHandling)
  , mIsMainThread(NS_IsMainThread())
{
  
  
  nsIPrincipal* webIDLCallerPrincipal = nullptr;
  if (aIsJSImplementedWebIDL) {
    webIDLCallerPrincipal = nsContentUtils::SubjectPrincipal();
  }

  
  
  
  
  

  
  JSObject* realCallback = js::UncheckedUnwrap(aCallback->CallbackPreserveColor());
  JSContext* cx = nullptr;
  nsIGlobalObject* globalObject = nullptr;

  {
    
    
    
    JS::AutoSuppressGCAnalysis nogc;
    if (mIsMainThread) {
      
      
      nsGlobalWindow* win =
        aIsJSImplementedWebIDL ? nullptr : xpc::WindowGlobalOrNull(realCallback);
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
        globalObject = xpc::NativeGlobal(glob);
        MOZ_ASSERT(globalObject);
        cx = nsContentUtils::GetSafeJSContext();
      }
    } else {
      cx = workers::GetCurrentThreadJSContext();
      JSObject *global = js::GetGlobalForObjectCrossCompartment(realCallback);
      globalObject = workers::GetGlobalObjectForGlobal(global);
      MOZ_ASSERT(globalObject);
    }

    
    
    
    if (!globalObject->GetGlobalJSObject()) {
      return;
    }

    mAutoEntryScript.emplace(globalObject, aExecutionReason,
                             mIsMainThread, cx);
    mAutoEntryScript->SetWebIDLCallerPrincipal(webIDLCallerPrincipal);
    nsIGlobalObject* incumbent = aCallback->IncumbentGlobalOrNull();
    if (incumbent) {
      
      
      
      
      
      if (!incumbent->GetGlobalJSObject()) {
        return;
      }
      mAutoIncumbentScript.emplace(incumbent);
    }

    
    
    
    
    
    
    
    
    
    mRootedCallable.emplace(cx, aCallback->Callback());
  }

  
  
  if (mIsMainThread && !aIsJSImplementedWebIDL) {
    
    
    
    bool allowed = nsContentUtils::GetSecurityManager()->
      ScriptAllowed(js::GetGlobalForObjectCrossCompartment(realCallback));

    if (!allowed) {
      return;
    }
  }

  
  
  
  
  
  mAc.emplace(cx, *mRootedCallable);

  
  mCx = cx;

  
  if ((mCompartment && mExceptionHandling == eRethrowContentExceptions) ||
      mExceptionHandling == eRethrowExceptions) {
    mSavedJSContextOptions = JS::ContextOptionsRef(cx);
    JS::ContextOptionsRef(cx).setDontReportUncaught(true);
  }
}

bool
CallbackObject::CallSetup::ShouldRethrowException(JS::Handle<JS::Value> aException)
{
  if (mExceptionHandling == eRethrowExceptions) {
    if (!mCompartment) {
      
      return true;
    }

    
    
    
    if (mCompartment == js::GetContextCompartment(mCx)) {
      return true;
    }

    MOZ_ASSERT(NS_IsMainThread());

    
    
    
    nsIPrincipal* callerPrincipal =
      nsJSPrincipals::get(JS_GetCompartmentPrincipals(mCompartment));
    nsIPrincipal* calleePrincipal = nsContentUtils::SubjectPrincipal();
    if (callerPrincipal->SubsumesConsideringDomain(calleePrincipal)) {
      return true;
    }
  }

  MOZ_ASSERT(mCompartment);

  
  
  

  if (!aException.isObject()) {
    return false;
  }

  JS::Rooted<JSObject*> obj(mCx, &aException.toObject());
  obj = js::UncheckedUnwrap(obj,  false);
  if (js::GetObjectCompartment(obj) != mCompartment) {
    return false;
  }

  DOMError* domError;
  DOMException* domException;
  return NS_SUCCEEDED(UNWRAP_OBJECT(DOMError, obj, domError)) ||
         NS_SUCCEEDED(UNWRAP_OBJECT(DOMException, obj, domException));
}

CallbackObject::CallSetup::~CallSetup()
{
  
  
  
  
  
  mAc.reset();

  
  
  if (mCx) {
    bool needToDealWithException = JS_IsExceptionPending(mCx);
    if ((mCompartment && mExceptionHandling == eRethrowContentExceptions) ||
        mExceptionHandling == eRethrowExceptions) {
      
      JS::ContextOptionsRef(mCx) = mSavedJSContextOptions;
      mErrorResult.MightThrowJSException();
      if (needToDealWithException) {
        JS::Rooted<JS::Value> exn(mCx);
        if (JS_GetPendingException(mCx, &exn) &&
            ShouldRethrowException(exn)) {
          mErrorResult.ThrowJSException(mCx, exn);
          JS_ClearPendingException(mCx);
          needToDealWithException = false;
        }
      }
    }

    if (needToDealWithException) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      JS::Rooted<JSObject*> oldGlobal(mCx, JS::CurrentGlobalOrNull(mCx));
      MOZ_ASSERT(oldGlobal, "How can we not have a global here??");
      bool saved = JS_SaveFrameChain(mCx);
      
      
      {
        JSAutoCompartment ac(mCx, oldGlobal);
        MOZ_ASSERT(!JS::DescribeScriptedCaller(mCx),
                   "Our comment above about JS_SaveFrameChain having been "
                   "called is a lie?");
        JS_ReportPendingException(mCx);
      }
      if (saved) {
        JS_RestoreFrameChain(mCx);
      }
    }
  }

  mAutoIncumbentScript.reset();
  mAutoEntryScript.reset();
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
    nsXPCWrappedJS::GetNewOrUsed(callback, aIID, getter_AddRefs(wrappedJS));
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
