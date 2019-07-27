












#include "nsJSUtils.h"
#include "jsapi.h"
#include "jsfriendapi.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIXPConnect.h"
#include "nsCOMPtr.h"
#include "nsIScriptSecurityManager.h"
#include "nsPIDOMWindow.h"
#include "GeckoProfiler.h"
#include "nsDOMJSUtils.h" 
#include "nsJSPrincipals.h"
#include "xpcpublic.h"
#include "nsContentUtils.h"
#include "nsGlobalWindow.h"

#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/ScriptSettings.h"

using namespace mozilla::dom;

bool
nsJSUtils::GetCallingLocation(JSContext* aContext, nsACString& aFilename,
                              uint32_t* aLineno)
{
  JS::AutoFilename filename;
  if (!JS::DescribeScriptedCaller(aContext, &filename, aLineno)) {
    return false;
  }

  aFilename.Assign(filename.get());
  return true;
}

bool
nsJSUtils::GetCallingLocation(JSContext* aContext, nsAString& aFilename,
                              uint32_t* aLineno)
{
  JS::AutoFilename filename;
  if (!JS::DescribeScriptedCaller(aContext, &filename, aLineno)) {
    return false;
  }

  aFilename.Assign(NS_ConvertUTF8toUTF16(filename.get()));
  return true;
}

nsIScriptGlobalObject *
nsJSUtils::GetStaticScriptGlobal(JSObject* aObj)
{
  if (!aObj)
    return nullptr;
  return xpc::WindowGlobalOrNull(aObj);
}

nsIScriptContext *
nsJSUtils::GetStaticScriptContext(JSObject* aObj)
{
  nsIScriptGlobalObject *nativeGlobal = GetStaticScriptGlobal(aObj);
  if (!nativeGlobal)
    return nullptr;

  return nativeGlobal->GetScriptContext();
}

uint64_t
nsJSUtils::GetCurrentlyRunningCodeInnerWindowID(JSContext *aContext)
{
  if (!aContext)
    return 0;

  uint64_t innerWindowID = 0;

  JSObject *jsGlobal = JS::CurrentGlobalOrNull(aContext);
  if (jsGlobal) {
    nsIScriptGlobalObject *scriptGlobal = GetStaticScriptGlobal(jsGlobal);
    if (scriptGlobal) {
      nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(scriptGlobal);
      if (win)
        innerWindowID = win->WindowID();
    }
  }

  return innerWindowID;
}

void
nsJSUtils::ReportPendingException(JSContext *aContext)
{
  if (JS_IsExceptionPending(aContext)) {
    bool saved = JS_SaveFrameChain(aContext);
    {
      
      
      
      
      
      
      
      
      
      
      
      nsIScriptContext* scx = GetScriptContextFromJSContext(aContext);
      JS::Rooted<JSObject*> scope(aContext);
      scope = scx ? scx->GetWindowProxy() : nullptr;
      if (!scope) {
        
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(aContext == nsContentUtils::GetSafeJSContext());
        scope = xpc::UnprivilegedJunkScope(); 
      }
      JSAutoCompartment ac(aContext, scope);
      JS_ReportPendingException(aContext);
    }
    if (saved) {
      JS_RestoreFrameChain(aContext);
    }
  }
}

nsresult
nsJSUtils::CompileFunction(AutoJSAPI& jsapi,
                           JS::AutoObjectVector& aScopeChain,
                           JS::CompileOptions& aOptions,
                           const nsACString& aName,
                           uint32_t aArgCount,
                           const char** aArgArray,
                           const nsAString& aBody,
                           JSObject** aFunctionObject)
{
  MOZ_ASSERT(jsapi.OwnsErrorReporting());
  JSContext* cx = jsapi.cx();
  MOZ_ASSERT(js::GetEnterCompartmentDepth(cx) > 0);
  MOZ_ASSERT_IF(aScopeChain.length() != 0,
                js::IsObjectInContextCompartment(aScopeChain[0], cx));
  MOZ_ASSERT_IF(aOptions.versionSet, aOptions.version != JSVERSION_UNKNOWN);
  mozilla::DebugOnly<nsIScriptContext*> ctx = GetScriptContextFromJSContext(cx);
  MOZ_ASSERT_IF(ctx, ctx->IsContextInitialized());

  
  for (size_t i = 0; i < aScopeChain.length(); ++i) {
    JS::ExposeObjectToActiveJS(aScopeChain[i]);
  }

  
  JS::Rooted<JSFunction*> fun(cx);
  if (!JS::CompileFunction(cx, aScopeChain, aOptions,
                           PromiseFlatCString(aName).get(),
                           aArgCount, aArgArray,
                           PromiseFlatString(aBody).get(),
                           aBody.Length(), &fun))
  {
    return NS_ERROR_FAILURE;
  }

  *aFunctionObject = JS_GetFunctionObject(fun);
  return NS_OK;
}

nsresult
nsJSUtils::EvaluateString(JSContext* aCx,
                          const nsAString& aScript,
                          JS::Handle<JSObject*> aEvaluationGlobal,
                          JS::CompileOptions& aCompileOptions,
                          const EvaluateOptions& aEvaluateOptions,
                          JS::MutableHandle<JS::Value> aRetValue)
{
  const nsPromiseFlatString& flatScript = PromiseFlatString(aScript);
  JS::SourceBufferHolder srcBuf(flatScript.get(), aScript.Length(),
                                JS::SourceBufferHolder::NoOwnership);
  return EvaluateString(aCx, srcBuf, aEvaluationGlobal, aCompileOptions,
                        aEvaluateOptions, aRetValue, nullptr);
}

nsresult
nsJSUtils::EvaluateString(JSContext* aCx,
                          JS::SourceBufferHolder& aSrcBuf,
                          JS::Handle<JSObject*> aEvaluationGlobal,
                          JS::CompileOptions& aCompileOptions,
                          const EvaluateOptions& aEvaluateOptions,
                          JS::MutableHandle<JS::Value> aRetValue,
                          void **aOffThreadToken)
{
  PROFILER_LABEL("nsJSUtils", "EvaluateString",
    js::ProfileEntry::Category::JS);

  MOZ_ASSERT_IF(aCompileOptions.versionSet,
                aCompileOptions.version != JSVERSION_UNKNOWN);
  MOZ_ASSERT_IF(aEvaluateOptions.coerceToString, !aCompileOptions.noScriptRval);
  MOZ_ASSERT_IF(!aEvaluateOptions.reportUncaught, !aCompileOptions.noScriptRval);
  
  
  MOZ_ASSERT(aCx == nsContentUtils::GetCurrentJSContext());
  MOZ_ASSERT(aSrcBuf.get());
  MOZ_ASSERT(js::GetGlobalForObjectCrossCompartment(aEvaluationGlobal) ==
             aEvaluationGlobal);
  MOZ_ASSERT_IF(aOffThreadToken, aCompileOptions.noScriptRval);

  
  
  
  
  
  
  aRetValue.setUndefined();

  nsresult rv = NS_OK;

  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  NS_ENSURE_TRUE(ssm->ScriptAllowed(aEvaluationGlobal), NS_OK);

  mozilla::Maybe<AutoDontReportUncaught> dontReport;
  if (!aEvaluateOptions.reportUncaught) {
    
    
    dontReport.emplace(aCx);
  }

  bool ok = true;
  
  
  {
    JSAutoCompartment ac(aCx, aEvaluationGlobal);

    
    JS::AutoObjectVector scopeChain(aCx);
    if (!scopeChain.reserve(aEvaluateOptions.scopeChain.length())) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    for (size_t i = 0; i < aEvaluateOptions.scopeChain.length(); ++i) {
      JS::ExposeObjectToActiveJS(aEvaluateOptions.scopeChain[i]);
      scopeChain.infallibleAppend(aEvaluateOptions.scopeChain[i]);
      if (!JS_WrapObject(aCx, scopeChain[i])) {
        ok = false;
        break;
      }
    }

    if (ok && aOffThreadToken) {
      JS::Rooted<JSScript*>
        script(aCx, JS::FinishOffThreadScript(aCx, JS_GetRuntime(aCx), *aOffThreadToken));
      *aOffThreadToken = nullptr; 
      if (script) {
        ok = JS_ExecuteScript(aCx, scopeChain, script);
      } else {
        ok = false;
      }
    } else if (ok) {
      ok = JS::Evaluate(aCx, scopeChain, aCompileOptions, aSrcBuf, aRetValue);
    }

    if (ok && aEvaluateOptions.coerceToString && !aRetValue.isUndefined()) {
      JS::Rooted<JS::Value> value(aCx, aRetValue);
      JSString* str = JS::ToString(aCx, value);
      ok = !!str;
      aRetValue.set(ok ? JS::StringValue(str) : JS::UndefinedValue());
    }
  }

  if (!ok) {
    if (aEvaluateOptions.reportUncaught) {
      ReportPendingException(aCx);
      if (!aCompileOptions.noScriptRval) {
        aRetValue.setUndefined();
      }
    } else {
      rv = JS_IsExceptionPending(aCx) ? NS_ERROR_FAILURE
                                      : NS_ERROR_OUT_OF_MEMORY;
      JS::Rooted<JS::Value> exn(aCx);
      JS_GetPendingException(aCx, &exn);
      MOZ_ASSERT(!aCompileOptions.noScriptRval); 
      aRetValue.set(exn);
      JS_ClearPendingException(aCx);
    }
  }

  
  if (!aCompileOptions.noScriptRval) {
    if (!JS_WrapValue(aCx, aRetValue)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return rv;
}

nsresult
nsJSUtils::EvaluateString(JSContext* aCx,
                          JS::SourceBufferHolder& aSrcBuf,
                          JS::Handle<JSObject*> aEvaluationGlobal,
                          JS::CompileOptions& aCompileOptions,
                          const EvaluateOptions& aEvaluateOptions,
                          JS::MutableHandle<JS::Value> aRetValue)
{
  return EvaluateString(aCx, aSrcBuf, aEvaluationGlobal, aCompileOptions,
                        aEvaluateOptions, aRetValue, nullptr);
}

nsresult
nsJSUtils::EvaluateString(JSContext* aCx,
                          const nsAString& aScript,
                          JS::Handle<JSObject*> aEvaluationGlobal,
                          JS::CompileOptions& aCompileOptions)
{
  EvaluateOptions options(aCx);
  aCompileOptions.setNoScriptRval(true);
  JS::RootedValue unused(aCx);
  return EvaluateString(aCx, aScript, aEvaluationGlobal, aCompileOptions,
                        options, &unused);
}

nsresult
nsJSUtils::EvaluateString(JSContext* aCx,
                          JS::SourceBufferHolder& aSrcBuf,
                          JS::Handle<JSObject*> aEvaluationGlobal,
                          JS::CompileOptions& aCompileOptions,
                          void **aOffThreadToken)
{
  EvaluateOptions options(aCx);
  aCompileOptions.setNoScriptRval(true);
  JS::RootedValue unused(aCx);
  return EvaluateString(aCx, aSrcBuf, aEvaluationGlobal, aCompileOptions,
                        options, &unused, aOffThreadToken);
}


bool
nsJSUtils::GetScopeChainForElement(JSContext* aCx,
                                   mozilla::dom::Element* aElement,
                                   JS::AutoObjectVector& aScopeChain)
{
  for (nsINode* cur = aElement; cur; cur = cur->GetScopeChainParent()) {
    JS::RootedValue val(aCx);
    if (!GetOrCreateDOMReflector(aCx, cur, &val)) {
      return false;
    }

    if (!aScopeChain.append(&val.toObject())) {
      return false;
    }
  }

  return true;
}






JSObject* GetDefaultScopeFromJSContext(JSContext *cx)
{
  
  
  
  nsIScriptContext *scx = GetScriptContextFromJSContext(cx);
  return  scx ? scx->GetWindowProxy() : nullptr;
}

bool nsAutoJSString::init(const JS::Value &v)
{
  return init(nsContentUtils::RootingCxForThread(), v);
}

