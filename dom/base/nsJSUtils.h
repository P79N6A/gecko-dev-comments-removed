




































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
                                   PRUint32* aLineno);

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
  



  explicit nsDependentJSString(jsid id)
    : nsDependentString(JS_GetInternedStringChars(JSID_TO_STRING(id)),
                        JS_GetStringLength(JSID_TO_STRING(id)))
  {
  }

  





  nsDependentJSString()
  {
  }

  JSBool init(JSContext* aContext, JSString* str)
  {
      size_t length;
      const jschar* chars = JS_GetStringCharsZAndLength(aContext, str, &length);
      if (!chars)
          return JS_FALSE;

      NS_ASSERTION(IsEmpty(), "init() on initialized string");
      nsDependentString* base = this;
      new(base) nsDependentString(chars, length);
      return JS_TRUE;
  }

  JSBool init(JSContext* aContext, const jsval &v)
  {
      return init(aContext, JSVAL_TO_STRING(v));
  }

  ~nsDependentJSString()
  {
  }
};

#endif 
