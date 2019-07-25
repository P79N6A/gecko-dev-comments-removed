














































#include "nsJSUtils.h"
#include "jsapi.h"
#include "jsdbgapi.h"
#include "prprf.h"
#include "nsIScriptContext.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsPIDOMWindow.h"

#include "nsDOMJSUtils.h" 

#include "mozilla/dom/bindings/Utils.h"

JSBool
nsJSUtils::GetCallingLocation(JSContext* aContext, const char* *aFilename,
                              PRUint32* aLineno)
{
  JSScript* script = nsnull;
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
    return nsnull;

  glob = JS_GetGlobalForObject(aContext, glob);
  NS_ABORT_IF_FALSE(glob, "Infallible returns null");

  clazz = JS_GetClass(glob);

  
  
  
  MOZ_ASSERT(!(clazz->flags & JSCLASS_IS_DOMJSCLASS));
  nsISupports* supports;
  if (!(clazz->flags & JSCLASS_HAS_PRIVATE) ||
      !(clazz->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS) ||
      !(supports = (nsISupports*)::JS_GetPrivate(glob))) {
    return nsnull;
  }

  
  
  
  
  
  
  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(supports));
  if (!sgo) {
    nsCOMPtr<nsIXPConnectWrappedNative> wrapper(do_QueryInterface(supports));
    NS_ENSURE_TRUE(wrapper, nsnull);
    sgo = do_QueryWrappedNative(wrapper);
  }

  
  
  return sgo;
}

nsIScriptContext *
nsJSUtils::GetStaticScriptContext(JSContext* aContext, JSObject* aObj)
{
  nsIScriptGlobalObject *nativeGlobal = GetStaticScriptGlobal(aContext, aObj);
  if (!nativeGlobal)
    return nsnull;

  return nativeGlobal->GetScriptContext();
}

nsIScriptGlobalObject *
nsJSUtils::GetDynamicScriptGlobal(JSContext* aContext)
{
  nsIScriptContext *scriptCX = GetDynamicScriptContext(aContext);
  if (!scriptCX)
    return nsnull;
  return scriptCX->GetGlobalObject();
}

nsIScriptContext *
nsJSUtils::GetDynamicScriptContext(JSContext *aContext)
{
  return GetScriptContextFromJSContext(aContext);
}

PRUint64
nsJSUtils::GetCurrentlyRunningCodeInnerWindowID(JSContext *aContext)
{
  if (!aContext)
    return 0;

  PRUint64 innerWindowID = 0;

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

