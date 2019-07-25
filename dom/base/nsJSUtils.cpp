














































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

#include "nsDOMJSUtils.h" 

JSBool
nsJSUtils::GetCallingLocation(JSContext* aContext, const char* *aFilename,
                              PRUint32* aLineno)
{
  
  JSStackFrame* frame = nsnull;
  JSScript* script = nsnull;
  do {
    frame = ::JS_FrameIterator(aContext, &frame);

    if (frame) {
      script = ::JS_GetFrameScript(aContext, frame);
    }
  } while (frame && !script);

  if (script) {
    const char* filename = ::JS_GetScriptFilename(aContext, script);

    if (filename) {
      PRUint32 lineno = 0;
      jsbytecode* bytecode = ::JS_GetFramePC(aContext, frame);

      if (bytecode) {
        lineno = ::JS_PCToLineNumber(aContext, script, bytecode);
      }

      *aFilename = filename;
      *aLineno = lineno;
      return JS_TRUE;
    }
  }

  return JS_FALSE;
}

nsIScriptGlobalObject *
nsJSUtils::GetStaticScriptGlobal(JSContext* aContext, JSObject* aObj)
{
  nsISupports* supports;
  JSClass* clazz;
  JSObject* parent;
  JSObject* glob = aObj; 

  if (!glob)
    return nsnull;

  while ((parent = ::JS_GetParent(aContext, glob)))
    glob = parent;

  clazz = JS_GET_CLASS(aContext, glob);

  if (!clazz ||
      !(clazz->flags & JSCLASS_HAS_PRIVATE) ||
      !(clazz->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS) ||
      !(supports = (nsISupports*)::JS_GetPrivate(aContext, glob))) {
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

  return nativeGlobal->GetScriptContext(nsIProgrammingLanguage::JAVASCRIPT);
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
nsJSUtils::GetCurrentlyRunningCodeWindowID(JSContext *aContext)
{
  if (!aContext)
    return 0;

  PRUint64 windowID = 0;

  JSObject *jsGlobal = JS_GetGlobalForScopeChain(aContext);
  if (jsGlobal) {
    nsIScriptGlobalObject *scriptGlobal = GetStaticScriptGlobal(aContext,
                                                                jsGlobal);
    if (scriptGlobal) {
      nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(scriptGlobal);
      if (win)
        windowID = win->GetOuterWindow()->WindowID();
    }
  }

  return windowID;
}

