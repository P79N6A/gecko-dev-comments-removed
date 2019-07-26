












#include "nsJSUtils.h"
#include "jsapi.h"
#include "jsdbgapi.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
#include "nsCOMPtr.h"
#include "nsIScriptSecurityManager.h"
#include "nsPIDOMWindow.h"
#include "GeckoProfiler.h"
#include "nsDOMJSUtils.h" 
#include "nsJSPrincipals.h"
#include "mozilla/dom/BindingUtils.h"

bool
nsJSUtils::GetCallingLocation(JSContext* aContext, const char* *aFilename,
                              uint32_t* aLineno)
{
  JSScript* script = nullptr;
  unsigned lineno = 0;

  if (!JS_DescribeScriptedCaller(aContext, &script, &lineno)) {
    return false;
  }

  *aFilename = ::JS_GetScriptFilename(aContext, script);
  *aLineno = lineno;

  return true;
}

nsIScriptGlobalObject *
nsJSUtils::GetStaticScriptGlobal(JSObject* aObj)
{
  JSClass* clazz;
  JSObject* glob = aObj; 

  if (!glob)
    return nullptr;

  glob = js::GetGlobalForObjectCrossCompartment(glob);
  NS_ABORT_IF_FALSE(glob, "Infallible returns null");

  clazz = JS_GetClass(glob);

  
  
  
  MOZ_ASSERT(!(clazz->flags & JSCLASS_IS_DOMJSCLASS));
  nsISupports* supports;
  if (!(clazz->flags & JSCLASS_HAS_PRIVATE) ||
      !(clazz->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS) ||
      !(supports = (nsISupports*)::JS_GetPrivate(glob))) {
    return nullptr;
  }

  
  
  
  
  
  
  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(supports));
  if (!sgo) {
    nsCOMPtr<nsIXPConnectWrappedNative> wrapper(do_QueryInterface(supports));
    if (!wrapper) {
      return nullptr;
    }
    sgo = do_QueryWrappedNative(wrapper);
  }

  
  
  return sgo;
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
      JSAutoCompartment ac(aContext, js::DefaultObjectForContextOrNull(aContext));
      JS_ReportPendingException(aContext);
    }
    if (saved) {
      JS_RestoreFrameChain(aContext);
    }
  }
}

nsresult
nsJSUtils::CompileFunction(JSContext* aCx,
                           JS::HandleObject aTarget,
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

  
  
  
  JSPrincipals* p = JS_GetCompartmentPrincipals(js::GetContextCompartment(aCx));
  aOptions.setPrincipals(p);

  
  xpc_UnmarkGrayObject(aTarget);

  
  JSFunction* fun = JS::CompileFunction(aCx, aTarget, aOptions,
                                        PromiseFlatCString(aName).get(),
                                        aArgCount, aArgArray,
                                        PromiseFlatString(aBody).get(),
                                        aBody.Length());
  if (!fun) {
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
                          EvaluateOptions& aEvaluateOptions,
                          JS::Value* aRetValue)
{
  PROFILER_LABEL("JS", "EvaluateString");
  MOZ_ASSERT_IF(aCompileOptions.versionSet,
                aCompileOptions.version != JSVERSION_UNKNOWN);
  MOZ_ASSERT_IF(aEvaluateOptions.coerceToString, aRetValue);
  MOZ_ASSERT_IF(!aEvaluateOptions.reportUncaught, aRetValue);
  MOZ_ASSERT(aCx == nsContentUtils::GetCurrentJSContext());

  
  
  
  
  if (aRetValue) {
    *aRetValue = JSVAL_VOID;
  }

  xpc_UnmarkGrayObject(aScopeObject);
  nsAutoMicroTask mt;

  JSPrincipals* p = JS_GetCompartmentPrincipals(js::GetObjectCompartment(aScopeObject));
  aCompileOptions.setPrincipals(p);

  bool ok = false;
  nsresult rv = nsContentUtils::GetSecurityManager()->
                  CanExecuteScripts(aCx, nsJSPrincipals::get(p), &ok);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(ok, NS_OK);

  
  
  {
    JSAutoCompartment ac(aCx, aScopeObject);

    JS::RootedObject rootedScope(aCx, aScopeObject);
    ok = JS::Evaluate(aCx, rootedScope, aCompileOptions,
                      PromiseFlatString(aScript).get(),
                      aScript.Length(), aRetValue);
    if (ok && aEvaluateOptions.coerceToString && !aRetValue->isUndefined()) {
      JSString* str = JS_ValueToString(aCx, *aRetValue);
      ok = !!str;
      *aRetValue = ok ? JS::StringValue(str) : JS::UndefinedValue();
    }
  }

  if (!ok) {
    if (aEvaluateOptions.reportUncaught) {
      ReportPendingException(aCx);
      if (aRetValue) {
        *aRetValue = JS::UndefinedValue();
      }
    } else {
      rv = JS_IsExceptionPending(aCx) ? NS_ERROR_FAILURE
                                      : NS_ERROR_OUT_OF_MEMORY;
      JS_GetPendingException(aCx, aRetValue);
      JS_ClearPendingException(aCx);
    }
  }

  
  if (aRetValue && !JS_WrapValue(aCx, aRetValue))
    return NS_ERROR_OUT_OF_MEMORY;
  return rv;
}
