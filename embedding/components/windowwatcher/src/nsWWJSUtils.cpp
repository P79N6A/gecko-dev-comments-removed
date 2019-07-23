








































#include "nsCOMPtr.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsWWJSUtils.h"
#include "nsIXPConnect.h"
#include "nsDOMJSUtils.h"

nsIScriptGlobalObject *
nsWWJSUtils::GetStaticScriptGlobal(JSContext* aContext, JSObject* aObj)
{
  nsISupports* supports;
  JSClass* clazz;
  JSObject* parent;
  JSObject* glob = aObj; 

  if (!glob)
    return nsnull;

  while (nsnull != (parent = JS_GetParent(aContext, glob)))
    glob = parent;

#ifdef JS_THREADSAFE
  clazz = JS_GetClass(aContext, glob);
#else
  clazz = JS_GetClass(glob);
#endif

  if (!clazz ||
      !(clazz->flags & JSCLASS_HAS_PRIVATE) ||
      !(clazz->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS) ||
      !(supports = (nsISupports*) JS_GetPrivate(aContext, glob))) {
    return nsnull;
  }
 
  nsCOMPtr<nsIXPConnectWrappedNative> wrapper(do_QueryInterface(supports));
  NS_ENSURE_TRUE(wrapper, nsnull);

  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryWrappedNative(wrapper));

  
  
  return sgo;
}

nsIScriptContext *
nsWWJSUtils::GetDynamicScriptContext(JSContext *aContext)
{
  return GetScriptContextFromJSContext(aContext);
}

nsIScriptGlobalObject *
nsWWJSUtils::GetDynamicScriptGlobal(JSContext* aContext)
{
  nsIScriptContext *scriptCX = GetDynamicScriptContext(aContext);
  if (!scriptCX)
    return nsnull;
  return scriptCX->GetGlobalObject();
}

nsIScriptContext *
nsWWJSUtils::GetStaticScriptContext(JSContext* aContext,
                                    JSObject* aObj)
{
  nsIScriptGlobalObject *nativeGlobal = GetStaticScriptGlobal(aContext, aObj);
  if (!nativeGlobal)    
    return nsnull;
  return nativeGlobal->GetContext();
}  

