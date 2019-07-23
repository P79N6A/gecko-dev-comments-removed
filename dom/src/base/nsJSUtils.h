




































#ifndef nsJSUtils_h__
#define nsJSUtils_h__








#include "nsISupports.h"
#include "jsapi.h"
#include "nsString.h"

class nsIDOMEventListener;
class nsIScriptContext;
class nsIScriptGlobalObject;

class nsJSUtils
{
public:
  static JSBool GetCallingLocation(JSContext* aContext, const char* *aFilename,
                                   PRUint32 *aLineno);

  static jsval ConvertStringToJSVal(const nsString& aProp,
                                    JSContext* aContext);

  static void ConvertJSValToString(nsAString& aString,
                                   JSContext* aContext, jsval aValue);

  static PRBool ConvertJSValToUint32(PRUint32* aProp, JSContext* aContext,
                                     jsval aValue);

  static nsIScriptGlobalObject *GetStaticScriptGlobal(JSContext* aContext,
                                                      JSObject* aObj);

  static nsIScriptContext *GetStaticScriptContext(JSContext* aContext,
                                                  JSObject* aObj);

  static nsIScriptGlobalObject *GetDynamicScriptGlobal(JSContext *aContext);

  static nsIScriptContext *GetDynamicScriptContext(JSContext *aContext);
};


class nsDependentJSString : public nsDependentString
{
public:
  explicit nsDependentJSString(jsval v)
    : nsDependentString((PRUnichar *)::JS_GetStringChars(JSVAL_TO_STRING(v)),
                        ::JS_GetStringLength(JSVAL_TO_STRING(v)))
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
