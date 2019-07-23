




































#ifndef nsWWJSUtils_h__
#define nsWWJSUtils_h__





#include "nsISupports.h"
#include "jsapi.h"

class nsIScriptContext;
class nsIScriptGlobalObject;

class nsWWJSUtils {
public:
  static nsIScriptGlobalObject *GetStaticScriptGlobal(JSContext* aContext,
                                                      JSObject* aObj);

  static nsIScriptContext *GetStaticScriptContext(JSContext* aContext,
                                                  JSObject* aObj);

  static nsIScriptGlobalObject *GetDynamicScriptGlobal(JSContext *aContext);

  static nsIScriptContext *GetDynamicScriptContext(JSContext *aContext);
};

#endif 
