




#ifndef nsJSUtils_h__
#define nsJSUtils_h__








#include "mozilla/Assertions.h"

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
                                   uint32_t* aLineno);

  static nsIScriptGlobalObject *GetStaticScriptGlobal(JSContext* aContext,
                                                      JSObject* aObj);

  static nsIScriptContext *GetStaticScriptContext(JSContext* aContext,
                                                  JSObject* aObj);

  static nsIScriptGlobalObject *GetDynamicScriptGlobal(JSContext *aContext);

  static nsIScriptContext *GetDynamicScriptContext(JSContext *aContext);

  







  static uint64_t GetCurrentlyRunningCodeInnerWindowID(JSContext *aContext);
};


class nsDependentJSString : public nsDependentString
{
public:
  



  explicit nsDependentJSString(jsid id)
    : nsDependentString(JS_GetInternedStringChars(JSID_TO_STRING(id)),
                        JS_GetStringLength(JSID_TO_STRING(id)))
  {
  }

  


  explicit nsDependentJSString(JSFlatString* fstr)
    : nsDependentString(JS_GetFlatStringChars(fstr),
                        JS_GetStringLength(JS_FORGET_STRING_FLATNESS(fstr)))
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

  void init(JSFlatString* fstr)
  {
      MOZ_ASSERT(IsEmpty(), "init() on initialized string");
      new(this) nsDependentJSString(fstr);
  }

  ~nsDependentJSString()
  {
  }
};

#endif 
