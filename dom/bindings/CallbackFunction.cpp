





#include "mozilla/dom/CallbackFunction.h"
#include "jsfriendapi.h"
#include "nsIScriptGlobalObject.h"
#include "nsIXPConnect.h"
#include "nsIScriptContext.h"
#include "nsPIDOMWindow.h"
#include "nsJSUtils.h"
#include "nsIScriptSecurityManager.h"

namespace mozilla {
namespace dom {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CallbackFunction)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(CallbackFunction)
NS_IMPL_CYCLE_COLLECTING_RELEASE(CallbackFunction)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(CallbackFunction)
  tmp->DropCallback();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(CallbackFunction)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(CallbackFunction)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mCallable)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

CallbackFunction::CallSetup::CallSetup(JSObject* const aCallable)
  : mCx(nullptr)
{
  xpc_UnmarkGrayObject(aCallable);

  
  
  

  
  JSObject* realCallable = js::UnwrapObject(aCallable);

  
  JSContext* cx = nullptr;
  nsIScriptContext* ctx = nullptr;
  nsIScriptGlobalObject* sgo = nsJSUtils::GetStaticScriptGlobal(realCallable);
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

  
  if (!mCxPusher.Push(cx, false)) {
    return;
  }

  
  
  
  
  
  mCtx = ctx;

  
  
  
  
  nsresult rv = nsContentUtils::GetSecurityManager()->
    CheckFunctionAccess(cx, js::UnwrapObject(aCallable), nullptr);

  
  
  
  
  if (ctx) {
    mTerminationFuncHolder.construct(static_cast<nsJSContext*>(ctx));
  }

  if (NS_FAILED(rv)) {
    
    return;
  }

  
  mAc.construct(cx, aCallable);

  
  mCx = cx;
}

CallbackFunction::CallSetup::~CallSetup()
{
  
  
  if (mCx) {
    nsJSUtils::ReportPendingException(mCx);
  }

  
  
  
  
  mAc.destroyIfConstructed();

  
  
  
  
  

  
  mCxPusher.Pop();

  if (mCtx) {
    mCtx->ScriptEvaluated(true);
  }
}

} 
} 
