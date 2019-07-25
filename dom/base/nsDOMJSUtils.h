




#ifndef nsDOMJSUtils_h__
#define nsDOMJSUtils_h__

#include "jsapi.h"
#include "nsIScriptContext.h"

class nsIJSArgArray;



inline nsIScriptContext *
GetScriptContextFromJSContext(JSContext *cx)
{
  if (!(::JS_GetOptions(cx) & JSOPTION_PRIVATE_IS_NSISUPPORTS)) {
    return nullptr;
  }

  nsCOMPtr<nsIScriptContext> scx =
    do_QueryInterface(static_cast<nsISupports *>
                                 (::JS_GetContextPrivate(cx)));

  
  
  return scx;
}

inline nsIScriptContextPrincipal*
GetScriptContextPrincipalFromJSContext(JSContext *cx)
{
  if (!(::JS_GetOptions(cx) & JSOPTION_PRIVATE_IS_NSISUPPORTS)) {
    return nullptr;
  }

  nsCOMPtr<nsIScriptContextPrincipal> scx =
    do_QueryInterface(static_cast<nsISupports *>
                                 (::JS_GetContextPrivate(cx)));

  
  
  return scx;
}











nsresult NS_CreateJSArgv(JSContext *aContext, uint32_t aArgc, void *aArgv,
                         nsIJSArgArray **aArray);

#endif 
