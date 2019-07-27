












#include "nsJSUtils.h"
#include "jsapi.h"
#include "js/OldDebugAPI.h"
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

bool
nsJSUtils::GetCallingLocation(JSContext* aContext, const char* *aFilename,
                              uint32_t* aLineno)
{
  JS::AutoFilename filename;
  unsigned lineno = 0;

  if (!JS::DescribeScriptedCaller(aContext, &filename, &lineno)) {
    return false;
  }

  *aFilename = filename.get();
  *aLineno = lineno;

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

nsIScriptGlobalObject *
nsJSUtils::GetDynamicScriptGlobal(JSContext* aContext)
{
  nsIScriptContext *scriptCX = GetDynamicScriptContext(aContext);
  if (!scriptCX)
    return nullptr;
  return scriptCX->GetGlobalObject();
}

nsIScriptContext *
nsJSUtils::GetDynamicScriptContext(JSContext *aContext)
{
  return GetScriptContextFromJSContext(aContext);
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
      scope = scx ? scx->GetWindowProxy()
                  : js::DefaultObjectForContextOrNull(aContext);
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
nsJSUtils::CompileFunction(JSContext* aCx,
                           JS::Handle<JSObject*> aTarget,
                           JS::CompileOptions& aOptions,
                           const nsACString& aName,
                           uint32_t aArgCount,
                           const char** aArgArray,
                           const nsAString& aBody,
                           JSObject** aFunctionObject)
{
  MOZ_ASSERT(js::GetEnterCompartmentDepth(aCx) > 0);
  MOZ_ASSERT_IF(aTarget, js::IsObjectInContextCompartment(aTarget, aCx));
  MOZ_ASSERT_IF(aOptions.versionSet, aOptions.version != JSVERSION_UNKNOWN);
  mozilla::DebugOnly<nsIScriptContext*> ctx = GetScriptContextFromJSContext(aCx);
  MOZ_ASSERT_IF(ctx, ctx->IsContextInitialized());

  
  if (aTarget) {
    JS::ExposeObjectToActiveJS(aTarget);
  }

  
  JS::Rooted<JSFunction*> fun(aCx);
  if (!JS::CompileFunction(aCx, aTarget, aOptions,
                           PromiseFlatCString(aName).get(),
                           aArgCount, aArgArray,
                           PromiseFlatString(aBody).get(),
                           aBody.Length(), &fun))
  {
    ReportPendingException(aCx);
    return NS_ERROR_FAILURE;
  }

  *aFunctionObject = JS_GetFunctionObject(fun);
  return NS_OK;
}

nsresult
nsJSUtils::EvaluateString(JSContext* aCx,
                          const nsAString& aScript,
                          JS::Handle<JSObject*> aScopeObject,
                          JS::CompileOptions& aCompileOptions,
                          const EvaluateOptions& aEvaluateOptions,
                          JS::MutableHandle<JS::Value> aRetValue,
                          void **aOffThreadToken)
{
  const nsPromiseFlatString& flatScript = PromiseFlatString(aScript);
  JS::SourceBufferHolder srcBuf(flatScript.get(), aScript.Length(),
                                JS::SourceBufferHolder::NoOwnership);
  return EvaluateString(aCx, srcBuf, aScopeObject, aCompileOptions,
                        aEvaluateOptions, aRetValue, aOffThreadToken);
}

nsresult
nsJSUtils::EvaluateString(JSContext* aCx,
                          JS::SourceBufferHolder& aSrcBuf,
                          JS::Handle<JSObject*> aScopeObject,
                          JS::CompileOptions& aCompileOptions,
                          const EvaluateOptions& aEvaluateOptions,
                          JS::MutableHandle<JS::Value> aRetValue,
                          void **aOffThreadToken)
{
  PROFILER_LABEL("nsJSUtils", "EvaluateString",
    js::ProfileEntry::Category::JS);

  MOZ_ASSERT_IF(aCompileOptions.versionSet,
                aCompileOptions.version != JSVERSION_UNKNOWN);
  MOZ_ASSERT_IF(aEvaluateOptions.coerceToString, aEvaluateOptions.needResult);
  MOZ_ASSERT_IF(!aEvaluateOptions.reportUncaught, aEvaluateOptions.needResult);
  MOZ_ASSERT(aCx == nsContentUtils::GetCurrentJSContext());
  MOZ_ASSERT(aSrcBuf.get());

  
  
  
  
  
  
  aRetValue.setUndefined();

  JS::ExposeObjectToActiveJS(aScopeObject);
  nsAutoMicroTask mt;
  nsresult rv = NS_OK;

  bool ok = false;
  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  NS_ENSURE_TRUE(ssm->ScriptAllowed(js::GetGlobalForObjectCrossCompartment(aScopeObject)), NS_OK);

  mozilla::Maybe<AutoDontReportUncaught> dontReport;
  if (!aEvaluateOptions.reportUncaught) {
    
    
    dontReport.emplace(aCx);
  }

  
  
  {
    JSAutoCompartment ac(aCx, aScopeObject);

    JS::Rooted<JSObject*> rootedScope(aCx, aScopeObject);
    if (aOffThreadToken) {
      JS::Rooted<JSScript*>
        script(aCx, JS::FinishOffThreadScript(aCx, JS_GetRuntime(aCx), *aOffThreadToken));
      *aOffThreadToken = nullptr; 
      if (script) {
        if (aEvaluateOptions.needResult) {
          ok = JS_ExecuteScript(aCx, rootedScope, script, aRetValue);
        } else {
          ok = JS_ExecuteScript(aCx, rootedScope, script);
        }
      } else {
        ok = false;
      }
    } else {
      if (aEvaluateOptions.needResult) {
        ok = JS::Evaluate(aCx, rootedScope, aCompileOptions,
                          aSrcBuf, aRetValue);
      } else {
        ok = JS::Evaluate(aCx, rootedScope, aCompileOptions,
                          aSrcBuf);
      }
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
      if (aEvaluateOptions.needResult) {
        aRetValue.setUndefined();
      }
    } else {
      rv = JS_IsExceptionPending(aCx) ? NS_ERROR_FAILURE
                                      : NS_ERROR_OUT_OF_MEMORY;
      JS::Rooted<JS::Value> exn(aCx);
      JS_GetPendingException(aCx, &exn);
      if (aEvaluateOptions.needResult) {
        aRetValue.set(exn);
      }
      JS_ClearPendingException(aCx);
    }
  }

  
  if (aEvaluateOptions.needResult) {
    JS::Rooted<JS::Value> v(aCx, aRetValue);
    if (!JS_WrapValue(aCx, &v)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    aRetValue.set(v);
  }
  return rv;
}

nsresult
nsJSUtils::EvaluateString(JSContext* aCx,
                          const nsAString& aScript,
                          JS::Handle<JSObject*> aScopeObject,
                          JS::CompileOptions& aCompileOptions,
                          void **aOffThreadToken)
{
  EvaluateOptions options;
  options.setNeedResult(false);
  JS::RootedValue unused(aCx);
  return EvaluateString(aCx, aScript, aScopeObject, aCompileOptions,
                        options, &unused, aOffThreadToken);
}

nsresult
nsJSUtils::EvaluateString(JSContext* aCx,
                          JS::SourceBufferHolder& aSrcBuf,
                          JS::Handle<JSObject*> aScopeObject,
                          JS::CompileOptions& aCompileOptions,
                          void **aOffThreadToken)
{
  EvaluateOptions options;
  options.setNeedResult(false);
  JS::RootedValue unused(aCx);
  return EvaluateString(aCx, aSrcBuf, aScopeObject, aCompileOptions,
                        options, &unused, aOffThreadToken);
}





JSObject* GetDefaultScopeFromJSContext(JSContext *cx)
{
  
  
  
  nsIScriptContext *scx = GetScriptContextFromJSContext(cx);
  if (scx) {
    return scx->GetWindowProxy();
  }
  return js::DefaultObjectForContextOrNull(cx);
}
