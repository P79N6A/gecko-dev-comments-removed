





#include "mozilla/dom/CallbackObject.h"
#include "jsfriendapi.h"
#include "nsIScriptGlobalObject.h"
#include "nsIXPConnect.h"
#include "nsIScriptContext.h"
#include "nsPIDOMWindow.h"
#include "nsJSUtils.h"
#include "nsIScriptSecurityManager.h"

namespace mozilla {
namespace dom {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CallbackObject)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(CallbackObject)
NS_IMPL_CYCLE_COLLECTING_RELEASE(CallbackObject)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(CallbackObject)
  tmp->DropCallback();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(CallbackObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(CallbackObject)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mCallback)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

CallbackObject::CallSetup::CallSetup(JSObject* const aCallback,
                                     ErrorResult& aRv,
                                     ExceptionHandling aExceptionHandling)
  : mCx(nullptr)
  , mErrorResult(aRv)
  , mExceptionHandling(aExceptionHandling)
{
  xpc_UnmarkGrayObject(aCallback);

  
  
  
  
  

  
  JSObject* realCallback = js::UnwrapObject(aCallback);

  
  JSContext* cx = nullptr;
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

  
  mAr.construct(cx);

  
  if (!mCxPusher.Push(cx)) {
    return;
  }

  
  
  
  
  
  mCtx = ctx;

  
  
  
  
  nsresult rv = nsContentUtils::GetSecurityManager()->
    CheckFunctionAccess(cx, js::UnwrapObject(aCallback), nullptr);

  
  
  
  
  if (ctx) {
    mTerminationFuncHolder.construct(static_cast<nsJSContext*>(ctx));
  }

  if (NS_FAILED(rv)) {
    
    return;
  }

  
  mAc.construct(cx, aCallback);

  
  mCx = cx;

  
  if (mExceptionHandling == eRethrowExceptions) {
    mSavedJSContextOptions = JS_GetOptions(cx);
    JS_SetOptions(cx, mSavedJSContextOptions | JSOPTION_DONT_REPORT_UNCAUGHT);
  }
}

CallbackObject::CallSetup::~CallSetup()
{
  
  
  if (mCx) {
    bool dealtWithPendingException = false;
    if (mExceptionHandling == eRethrowExceptions) {
      
      JS_SetOptions(mCx, mSavedJSContextOptions);
      mErrorResult.MightThrowJSException();
      if (JS_IsExceptionPending(mCx)) {
        JS::Value exn;
        if (JS_GetPendingException(mCx, &exn)) {
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

  if (mCtx) {
    mCtx->ScriptEvaluated(true);
  }
}

} 
} 
