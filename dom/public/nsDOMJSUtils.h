
#ifndef nsDOMJSUtils_h__
#define nsDOMJSUtils_h__

#include "jsapi.h"
#include "nsIScriptContext.h"



inline nsIScriptContext *
GetScriptContextFromJSContext(JSContext *cx)
{
  if (!(::JS_GetOptions(cx) & JSOPTION_PRIVATE_IS_NSISUPPORTS)) {
    return nsnull;
  }

  nsCOMPtr<nsIScriptContext> scx =
    do_QueryInterface(NS_STATIC_CAST(nsISupports *,
                                     ::JS_GetContextPrivate(cx)));

  
  
  return scx;
}











nsresult NS_CreateJSArgv(JSContext *aContext, PRUint32 aArgc, void *aArgv,
                         nsIArray **aArray);

#endif 
