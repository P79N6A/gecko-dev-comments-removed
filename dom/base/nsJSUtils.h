




#ifndef nsJSUtils_h__
#define nsJSUtils_h__








#include "mozilla/Assertions.h"

#include "jsapi.h"
#include "jsfriendapi.h"
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
    bool needResult;

    explicit EvaluateOptions() : coerceToString(false)
                               , reportUncaught(true)
                               , needResult(true)
    {}

    EvaluateOptions& setCoerceToString(bool aCoerce) {
      coerceToString = aCoerce;
      return *this;
    }

    EvaluateOptions& setReportUncaught(bool aReport) {
      reportUncaught = aReport;
      return *this;
    }

    EvaluateOptions& setNeedResult(bool aNeedResult) {
      needResult = aNeedResult;
      return *this;
    }
  };

  static nsresult EvaluateString(JSContext* aCx,
                                 const nsAString& aScript,
                                 JS::Handle<JSObject*> aScopeObject,
                                 JS::CompileOptions &aCompileOptions,
                                 const EvaluateOptions& aEvaluateOptions,
                                 JS::MutableHandle<JS::Value> aRetValue,
                                 void **aOffThreadToken = nullptr);

  static nsresult EvaluateString(JSContext* aCx,
                                 JS::SourceBufferHolder& aSrcBuf,
                                 JS::Handle<JSObject*> aScopeObject,
                                 JS::CompileOptions &aCompileOptions,
                                 const EvaluateOptions& aEvaluateOptions,
                                 JS::MutableHandle<JS::Value> aRetValue,
                                 void **aOffThreadToken = nullptr);


  static nsresult EvaluateString(JSContext* aCx,
                                 const nsAString& aScript,
                                 JS::Handle<JSObject*> aScopeObject,
                                 JS::CompileOptions &aCompileOptions,
                                 void **aOffThreadToken = nullptr);

  static nsresult EvaluateString(JSContext* aCx,
                                 JS::SourceBufferHolder& aSrcBuf,
                                 JS::Handle<JSObject*> aScopeObject,
                                 JS::CompileOptions &aCompileOptions,
                                 void **aOffThreadToken = nullptr);

};

class MOZ_STACK_CLASS AutoDontReportUncaught {
  JSContext* mContext;
  bool mWasSet;

public:
  explicit AutoDontReportUncaught(JSContext* aContext) : mContext(aContext) {
    MOZ_ASSERT(aContext);
    mWasSet = JS::ContextOptionsRef(mContext).dontReportUncaught();
    if (!mWasSet) {
      JS::ContextOptionsRef(mContext).setDontReportUncaught(true);
    }
  }
  ~AutoDontReportUncaught() {
    if (!mWasSet) {
      JS::ContextOptionsRef(mContext).setDontReportUncaught(false);
    }
  }
};

template<typename T>
inline bool
AssignJSString(JSContext *cx, T &dest, JSString *s)
{
  size_t len = js::GetStringLength(s);
  static_assert(js::MaxStringLength < (1 << 28),
                "Shouldn't overflow here or in SetCapacity");
  if (MOZ_UNLIKELY(!dest.SetLength(len, mozilla::fallible_t()))) {
    JS_ReportOutOfMemory(cx);
    return false;
  }
  return js::CopyStringChars(cx, dest.BeginWriting(), s, len);
}

inline void
AssignJSFlatString(nsAString &dest, JSFlatString *s)
{
  size_t len = js::GetFlatStringLength(s);
  static_assert(js::MaxStringLength < (1 << 28),
                "Shouldn't overflow here or in SetCapacity");
  dest.SetLength(len);
  js::CopyFlatStringChars(dest.BeginWriting(), s, len);
}

class nsAutoJSString : public nsAutoString
{
public:

  



  nsAutoJSString() {}

  bool init(JSContext* aContext, JSString* str)
  {
    return AssignJSString(aContext, *this, str);
  }

  bool init(JSContext* aContext, const JS::Value &v)
  {
    if (v.isString()) {
      return init(aContext, v.toString());
    }

    
    JS::Rooted<JSString*> str(aContext);
    if (v.isObject()) {
      str = JS_NewStringCopyZ(aContext, "[Object]");
    } else {
      JS::Rooted<JS::Value> rootedVal(aContext, v);
      str = JS::ToString(aContext, rootedVal);
    }

    return str && init(aContext, str);
  }

  bool init(JSContext* aContext, jsid id)
  {
    JS::Rooted<JS::Value> v(aContext);
    return JS_IdToValue(aContext, id, &v) && init(aContext, v);
  }

  ~nsAutoJSString() {}
};

#endif 
