





#ifndef nsDOMJSUtils_h__
#define nsDOMJSUtils_h__

#include "nsIScriptContext.h"
#include "jsapi.h"

class nsIJSArgArray;



inline nsIScriptContext *
GetScriptContextFromJSContext(JSContext *cx)
{
  if (!(JS::ContextOptionsRef(cx).privateIsNSISupports())) {
    return nullptr;
  }

  nsCOMPtr<nsIScriptContext> scx =
    do_QueryInterface(static_cast<nsISupports *>
                                 (::JS_GetContextPrivate(cx)));

  
  
  return scx;
}

JSObject* GetDefaultScopeFromJSContext(JSContext *cx);








nsresult NS_CreateJSArgv(JSContext *aContext, uint32_t aArgc,
                         const JS::Value* aArgv, nsIJSArgArray **aArray);

#endif 
