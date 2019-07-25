




































#ifndef nsJSUtils_h__
#define nsJSUtils_h__








#include "nsISupports.h"
#include "jsapi.h"
#include "nsString.h"

class nsIDOMEventListener;
class nsIScriptContext;
class nsIScriptGlobalObject;
class nsIPrincipal;

class nsJSUtils
{
public:
  static JSBool GetCallingLocation(JSContext* aContext, const char* *aFilename,
                                   PRUint32* aLineno, nsIPrincipal* aPrincipal);

  static nsIScriptGlobalObject *GetStaticScriptGlobal(JSContext* aContext,
                                                      JSObject* aObj);

  static nsIScriptContext *GetStaticScriptContext(JSContext* aContext,
                                                  JSObject* aObj);

  static nsIScriptGlobalObject *GetDynamicScriptGlobal(JSContext *aContext);

  static nsIScriptContext *GetDynamicScriptContext(JSContext *aContext);

  







  static PRUint64 GetCurrentlyRunningCodeWindowID(JSContext *aContext);
};


class nsDependentJSString : public nsDependentString
{
public:
  explicit nsDependentJSString(jsval v)
    : nsDependentString((PRUnichar *)::JS_GetStringChars(JSVAL_TO_STRING(v)),
                        ::JS_GetStringLength(JSVAL_TO_STRING(v)))
  {
  }

  explicit nsDependentJSString(jsid id)
    : nsDependentString((PRUnichar *)::JS_GetStringChars(JSID_TO_STRING(id)),
                        ::JS_GetStringLength(JSID_TO_STRING(id)))
  {
  }

  explicit nsDependentJSString(JSString *str)
    : nsDependentString((PRUnichar *)::JS_GetStringChars(str), ::JS_GetStringLength(str))
  {
  }

  ~nsDependentJSString()
  {
  }
};

#endif 
