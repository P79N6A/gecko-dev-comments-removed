












#include "nsJSUtils.h"
#include "jsapi.h"
#include "jsdbgapi.h"
#include "prprf.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
#include "nsCOMPtr.h"
#include "nsIScriptSecurityManager.h"
#include "nsPIDOMWindow.h"

#include "nsDOMJSUtils.h" 

#include "mozilla/dom/BindingUtils.h"

JSBool
nsJSUtils::GetCallingLocation(JSContext* aContext, const char* *aFilename,
                              uint32_t* aLineno)
{
  JSScript* script = nullptr;
  unsigned lineno = 0;

  if (!JS_DescribeScriptedCaller(aContext, &script, &lineno)) {
    return JS_FALSE;
  }

  *aFilename = ::JS_GetScriptFilename(aContext, script);
  *aLineno = lineno;

  return JS_TRUE;
}

nsIScriptGlobalObject *
nsJSUtils::GetStaticScriptGlobal(JSContext* aContext, JSObject* aObj)
{
  JSClass* clazz;
  JSObject* glob = aObj; 

  if (!glob)
    return nullptr;

  glob = JS_GetGlobalForObject(aContext, glob);
  NS_ABORT_IF_FALSE(glob, "Infallible returns null");

  clazz = JS_GetClass(glob);

  
  
  
  MOZ_ASSERT(!(clazz->flags & JSCLASS_IS_DOMJSCLASS));
  nsISupports* supports;
  if (!(clazz->flags & JSCLASS_HAS_PRIVATE) ||
      !(clazz->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS) ||
      !(supports = (nsISupports*)::JS_GetPrivate(glob))) {
    return nullptr;
  }

  
  
  
  
  
  
  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(supports));
  if (!sgo) {
    nsCOMPtr<nsIXPConnectWrappedNative> wrapper(do_QueryInterface(supports));
    NS_ENSURE_TRUE(wrapper, nullptr);
    sgo = do_QueryWrappedNative(wrapper);
  }

  
  
  return sgo;
}

nsIScriptContext *
nsJSUtils::GetStaticScriptContext(JSContext* aContext, JSObject* aObj)
{
  nsIScriptGlobalObject *nativeGlobal = GetStaticScriptGlobal(aContext, aObj);
  if (!nativeGlobal)
    return nullptr;

  return nativeGlobal->GetScriptContext();
}

nsIScriptGlobalObject *
nsJSUtils::GetDynamicScriptGlobal(JSContext* aContext)
{
  nsIScriptContext *scriptCX = GetDynamicScriptContext(aContext);
  if (!scriptCX)
    return nullptr;
  return scriptCX->GetGlobalObject();
}

nsIScriptContext *
nsJSUtils::GetDynamicScriptContext(JSContext *aContext)
{
  return GetScriptContextFromJSContext(aContext);
}

uint64_t
nsJSUtils::GetCurrentlyRunningCodeInnerWindowID(JSContext *aContext)
{
  if (!aContext)
    return 0;

  uint64_t innerWindowID = 0;

  JSObject *jsGlobal = JS_GetGlobalForScopeChain(aContext);
  if (jsGlobal) {
    nsIScriptGlobalObject *scriptGlobal = GetStaticScriptGlobal(aContext,
                                                                jsGlobal);
    if (scriptGlobal) {
      nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(scriptGlobal);
      if (win)
        innerWindowID = win->WindowID();
    }
  }

  return innerWindowID;
}

