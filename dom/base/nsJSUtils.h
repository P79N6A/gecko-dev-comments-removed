




#ifndef nsJSUtils_h__
#define nsJSUtils_h__








#include "mozilla/Assertions.h"

#include "jsapi.h"
#include "nsString.h"

class nsIScriptContext;
class nsIScriptGlobalObject;

class nsJSUtils
{
public:
  static bool GetCallingLocation(JSContext* aContext, const char* *aFilename,
                                 uint32_t* aLineno);

  static nsIScriptGlobalObject *GetStaticScriptGlobal(JSObject* aObj);

  static nsIScriptContext *GetStaticScriptContext(JSObject* aObj);

  static nsIScriptGlobalObject *GetDynamicScriptGlobal(JSContext *aContext);

  static nsIScriptContext *GetDynamicScriptContext(JSContext *aContext);

  







  static uint64_t GetCurrentlyRunningCodeInnerWindowID(JSContext *aContext);

  




  static void ReportPendingException(JSContext *aContext);

  static nsresult CompileFunction(JSContext* aCx,
                                  JS::Handle<JSObject*> aTarget,
                                  JS::CompileOptions& aOptions,
                                  const nsACString& aName,
                                  uint32_t aArgCount,
                                  const char** aArgArray,
                                  const nsAString& aBody,
                                  JSObject** aFunctionObject);

  struct EvaluateOptions {
    bool coerceToString;
    bool reportUncaught;

    explicit EvaluateOptions() : coerceToString(false)
                               , reportUncaught(true)
    {}

    EvaluateOptions& setCoerceToString(bool aCoerce) {
      coerceToString = aCoerce;
      return *this;
    }

    EvaluateOptions& setReportUncaught(bool aReport) {
      reportUncaught = aReport;
      return *this;
    }
  };

  static nsresult EvaluateString(JSContext* aCx,
                                 const nsAString& aScript,
                                 JS::Handle<JSObject*> aScopeObject,
                                 JS::CompileOptions &aCompileOptions,
                                 EvaluateOptions& aEvaluateOptions,
                                 JS::Value* aRetValue,
                                 void **aOffThreadToken = nullptr);

};


class nsDependentJSString : public nsDependentString
{
public:
  



  explicit nsDependentJSString(JS::Handle<jsid> id)
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

  bool init(JSContext* aContext, JSString* str)
  {
      size_t length;
      const jschar* chars = JS_GetStringCharsZAndLength(aContext, str, &length);
      if (!chars)
          return false;

      NS_ASSERTION(IsEmpty(), "init() on initialized string");
      nsDependentString* base = this;
      new(base) nsDependentString(chars, length);
      return true;
  }

  bool init(JSContext* aContext, const JS::Value &v)
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
