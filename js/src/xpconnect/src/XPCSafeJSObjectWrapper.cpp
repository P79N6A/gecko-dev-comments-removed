






































#include "xpcprivate.h"
#include "jsdbgapi.h"
#include "jsscript.h" 
#include "XPCWrapper.h"

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_Enumerate(JSContext *cx, JSObject *obj);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                    JSObject **objp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

JS_STATIC_DLL_CALLBACK(void)
XPC_SJOW_Finalize(JSContext *cx, JSObject *obj);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_CheckAccess(JSContext *cx, JSObject *obj, jsval id, JSAccessMode mode,
                     jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval);

JSBool
XPC_SJOW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                   jsval *rval);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_Equality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

static inline
JSBool
ThrowException(nsresult ex, JSContext *cx)
{
  XPCThrower::Throw(ex, cx);

  return JS_FALSE;
}





static nsresult
FindPrincipals(JSContext *cx, JSObject *obj, nsIPrincipal **objectPrincipal,
               nsIPrincipal **subjectPrincipal,
               nsIScriptSecurityManager **secMgr)
{
  XPCCallContext ccx(JS_CALLER, cx);

  if (!ccx.IsValid()) {
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIXPCSecurityManager> sm = ccx.GetXPCContext()->
    GetAppropriateSecurityManager(nsIXPCSecurityManager::HOOK_CALL_METHOD);

  nsCOMPtr<nsIScriptSecurityManager> ssm(do_QueryInterface(sm));

  if (subjectPrincipal) {
    ssm->GetSubjectPrincipal(subjectPrincipal);

    if (!*subjectPrincipal) {
      return NS_ERROR_XPC_SECURITY_MANAGER_VETO;
    }
  }

  ssm->GetObjectPrincipal(cx, obj, objectPrincipal);

  if (secMgr) {
    *secMgr = nsnull;
    ssm.swap(*secMgr);
  }

  return *objectPrincipal ? NS_OK : NS_ERROR_XPC_SECURITY_MANAGER_VETO;
}

static PRBool
CanCallerAccess(JSContext *cx, JSObject *unsafeObj)
{
  nsCOMPtr<nsIPrincipal> subjPrincipal, objPrincipal;
  nsCOMPtr<nsIScriptSecurityManager> ssm;
  nsresult rv = FindPrincipals(cx, unsafeObj, getter_AddRefs(objPrincipal),
                               getter_AddRefs(subjPrincipal),
                               getter_AddRefs(ssm));
  if (NS_FAILED(rv)) {
    return ThrowException(rv, cx);
  }

  PRBool subsumes;
  rv = subjPrincipal->Subsumes(objPrincipal, &subsumes);

  if (NS_FAILED(rv) || !subsumes) {
    PRBool enabled = PR_FALSE;
    rv = ssm->IsCapabilityEnabled("UniversalXPConnect", &enabled);
    if (NS_FAILED(rv)) {
      return ThrowException(rv, cx);
    }

    if (!enabled) {
      return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
    }
  }

  return PR_TRUE;
}

static JSPrincipals *
FindObjectPrincipals(JSContext *cx, JSObject *obj)
{
  nsCOMPtr<nsIPrincipal> objPrincipal;
  nsresult rv = FindPrincipals(cx, obj, getter_AddRefs(objPrincipal), nsnull,
                               nsnull);
  if (NS_FAILED(rv)) {
    return nsnull;
  }

  JSPrincipals *jsprin;
  rv = objPrincipal->GetJSPrincipals(cx, &jsprin);
  if (NS_FAILED(rv)) {
    return nsnull;
  }

  return jsprin;
}





JSExtendedClass sXPC_SJOW_JSClass = {
  
  { "XPCSafeJSObjectWrapper",
    JSCLASS_NEW_RESOLVE | JSCLASS_IS_EXTENDED |
    JSCLASS_HAS_RESERVED_SLOTS(XPCWrapper::sNumSlots + 3),
    XPC_SJOW_AddProperty, XPC_SJOW_DelProperty,
    XPC_SJOW_GetProperty, XPC_SJOW_SetProperty,
    XPC_SJOW_Enumerate,   (JSResolveOp)XPC_SJOW_NewResolve,
    XPC_SJOW_Convert,     XPC_SJOW_Finalize,
    nsnull,               XPC_SJOW_CheckAccess,
    XPC_SJOW_Call,        XPC_SJOW_Construct,
    nsnull,               nsnull,
    nsnull,               nsnull
  },
  
  XPC_SJOW_Equality
};

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                  jsval *rval);

static JSFunctionSpec sXPC_SJOW_JSClass_methods[] = {
  {"toString", XPC_SJOW_toString, 0, 0, 0},
  {0, 0, 0, 0, 0}
};





#define XPC_SJOW_SLOT_IS_RESOLVING           0



#define XPC_SJOW_SLOT_SCRIPTED_GETSET        1



#define XPC_SJOW_SLOT_SCRIPTED_FUN           2



#define XPC_SJOW_SLOT_SCRIPTED_TOSTRING      3




#define XPC_SJOW_SLOT_PRINCIPAL              4





static JSBool
WrapJSValue(JSContext *cx, JSObject *obj, jsval val, jsval *rval)
{
  JSBool ok = JS_TRUE;

  if (JSVAL_IS_PRIMITIVE(val)) {
    *rval = val;
  } else {
    
    
    
    JSObject *safeObj =
      ::JS_ConstructObjectWithArguments(cx, &sXPC_SJOW_JSClass.base, nsnull,
                                        nsnull, 1, &val);
    if (!safeObj) {
      return JS_FALSE;
    }

    
    
    *rval = OBJECT_TO_JSVAL(safeObj);

    
    
    if (JS_GetGlobalForObject(cx, obj) == JS_GetGlobalForObject(cx, safeObj)) {
      jsval rsval;
      if (!::JS_GetReservedSlot(cx, obj, XPC_SJOW_SLOT_SCRIPTED_GETSET,
                                &rsval) ||
          !::JS_SetReservedSlot(cx, safeObj, XPC_SJOW_SLOT_SCRIPTED_GETSET,
                                rsval) ||
          !::JS_GetReservedSlot(cx, obj, XPC_SJOW_SLOT_SCRIPTED_FUN,
                                &rsval) ||
          !::JS_SetReservedSlot(cx, safeObj, XPC_SJOW_SLOT_SCRIPTED_FUN,
                                rsval)) {
        return JS_FALSE;
      }
    } else {
      
      
      
      
      nsCOMPtr<nsIPrincipal> srcObjPrincipal;
      nsCOMPtr<nsIPrincipal> valObjPrincipal;

      nsresult rv = FindPrincipals(cx, obj, getter_AddRefs(srcObjPrincipal),
                                   nsnull, nsnull);
      if (NS_FAILED(rv)) {
        return ThrowException(rv, cx);
      }

      rv = FindPrincipals(cx, JSVAL_TO_OBJECT(val),
                          getter_AddRefs(valObjPrincipal), nsnull, nsnull);
      if (NS_FAILED(rv)) {
        return ThrowException(rv, cx);
      }

      PRBool subsumes = PR_FALSE;
      rv = srcObjPrincipal->Subsumes(valObjPrincipal, &subsumes);
      if (NS_FAILED(rv)) {
        return ThrowException(rv, cx);
      }

      if (!subsumes) {
        
        
        
        
        
        if (!::JS_SetReservedSlot(cx, safeObj, XPC_SJOW_SLOT_PRINCIPAL,
                                  PRIVATE_TO_JSVAL(srcObjPrincipal.get()))) {
          return JS_FALSE;
        }

        
        
        nsIPrincipal *tmp = nsnull;
        srcObjPrincipal.swap(tmp);
      }
    }
  }

  return ok;
}

static inline JSObject *
FindSafeObject(JSContext *cx, JSObject *obj)
{
  while (JS_GET_CLASS(cx, obj) != &sXPC_SJOW_JSClass.base) {
    obj = ::JS_GetPrototype(cx, obj);

    if (!obj) {
      break;
    }
  }

  return obj;
}

PRBool
IsXPCSafeJSObjectWrapper(JSContext *cx, JSObject *obj)
{
  return FindSafeObject(cx, obj) != nsnull;
}

PRBool
IsXPCSafeJSObjectWrapperClass(JSClass *clazz)
{
  return clazz == &sXPC_SJOW_JSClass.base;
}

static inline JSObject *
GetUnsafeObject(JSContext *cx, JSObject *obj)
{
  obj = FindSafeObject(cx, obj);

  if (!obj) {
    return nsnull;
  }

  return ::JS_GetParent(cx, obj);
}

JSObject *
XPC_SJOW_GetUnsafeObject(JSContext *cx, JSObject *obj)
{
  return GetUnsafeObject(cx, obj);
}

static jsval
UnwrapJSValue(JSContext *cx, jsval val)
{
  if (JSVAL_IS_PRIMITIVE(val)) {
    return val;
  }

  JSObject *unsafeObj = GetUnsafeObject(cx, JSVAL_TO_OBJECT(val));
  if (unsafeObj) {
    return OBJECT_TO_JSVAL(unsafeObj);
  }

  return val;
}







static JSBool
GetScriptedFunction(JSContext *cx, JSObject *obj, JSObject *unsafeObj,
                    uint32 slotIndex, const nsAFlatCString& funScript,
                    jsval *scriptedFunVal)
{
  if (!::JS_GetReservedSlot(cx, obj, slotIndex, scriptedFunVal)) {
    return JS_FALSE;
  }

  
  
  
  
  if (JSVAL_IS_VOID(*scriptedFunVal) ||
      JS_GetGlobalForObject(cx, unsafeObj) !=
      JS_GetGlobalForObject(cx, JSVAL_TO_OBJECT(*scriptedFunVal))) {
    
    jsval pv;
    if (!::JS_GetReservedSlot(cx, obj, XPC_SJOW_SLOT_PRINCIPAL, &pv)) {
      return JS_FALSE;
    }

    JSPrincipals *jsprin = nsnull;

    if (!JSVAL_IS_VOID(pv)) {
      nsIPrincipal *principal = (nsIPrincipal *)JSVAL_TO_PRIVATE(pv);

      
      
      principal->GetJSPrincipals(cx, &jsprin);
    } else {
      
      
      jsprin = FindObjectPrincipals(cx, unsafeObj);
    }

    if (!jsprin) {
      return ThrowException(NS_ERROR_UNEXPECTED, cx);
    }

    JSFunction *scriptedFun =
      ::JS_CompileFunctionForPrincipals(cx,
                                        JS_GetGlobalForObject(cx, unsafeObj),
                                        jsprin, nsnull, 0, nsnull,
                                        funScript.get(), funScript.Length(),
                                        "XPCSafeJSObjectWrapper.cpp",
                                        __LINE__);

    JSPRINCIPALS_DROP(cx, jsprin);

    if (!scriptedFun) {
      return ThrowException(NS_ERROR_FAILURE, cx);
    }

    *scriptedFunVal = OBJECT_TO_JSVAL(::JS_GetFunctionObject(scriptedFun));

    if (*scriptedFunVal == JSVAL_NULL ||
        !::JS_SetReservedSlot(cx, obj, slotIndex, *scriptedFunVal)) {
      return JS_FALSE;
    }
  }

  return JS_TRUE;
}


JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  
  
  if (id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_CONSTRUCTOR) ||
      id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    return JS_TRUE;
  }

  obj = FindSafeObject(cx, obj);
  NS_ASSERTION(obj != nsnull, "FindSafeObject() returned null in class hook!");

  
  
  jsval isResolving;
  JSBool ok = ::JS_GetReservedSlot(cx, obj, XPC_SJOW_SLOT_IS_RESOLVING,
                                   &isResolving);
  if (!ok || JSVAL_TO_BOOLEAN(isResolving)) {
    return ok;
  }

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  
  if (!CanCallerAccess(cx, unsafeObj)) {
    
    return JS_FALSE;
  }

  return XPCWrapper::AddProperty(cx, unsafeObj, id, vp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  
  if (!CanCallerAccess(cx, unsafeObj)) {
    
    return JS_FALSE;
  }

  return XPCWrapper::DelProperty(cx, unsafeObj, id, vp);
}





JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_CallWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                     jsval *rval)
{
  
  
  if (argc < 1) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  return ::JS_CallFunctionValue(cx, obj, argv[0], argc - 1, argv + 1, rval);
}

static JSBool
XPC_SJOW_GetOrSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp,
                          JSBool aIsSet)
{
  
  if (id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_PROTOTYPE) ||
      id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    return JS_TRUE;
  }

  obj = FindSafeObject(cx, obj);
  NS_ASSERTION(obj != nsnull, "FindSafeObject() returned null in class hook!");

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  
  if (!CanCallerAccess(cx, unsafeObj)) {
    
    return JS_FALSE;
  }

  
  
  
  
  
  
  NS_NAMED_LITERAL_CSTRING(funScript,
    "if (arguments.length == 1) return this[arguments[0]];"
    "return this[arguments[0]] = arguments[1];");

  jsval scriptedFunVal;
  if (!GetScriptedFunction(cx, obj, unsafeObj, XPC_SJOW_SLOT_SCRIPTED_GETSET,
                           funScript, &scriptedFunVal)) {
    return JS_FALSE;
  }

  
  jsval args[2];

  args[0] = id;

  if (aIsSet) {
    args[1] = UnwrapJSValue(cx, *vp);
  }

  jsval val;
  JSBool ok = ::JS_CallFunctionValue(cx, unsafeObj, scriptedFunVal,
                                     aIsSet ? 2 : 1, args, &val);

  return ok && WrapJSValue(cx, obj, val, vp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  return XPC_SJOW_GetOrSetProperty(cx, obj, id, vp, PR_FALSE);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  return XPC_SJOW_GetOrSetProperty(cx, obj, id, vp, PR_TRUE);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_Enumerate(JSContext *cx, JSObject *obj)
{
  obj = FindSafeObject(cx, obj);
  NS_ASSERTION(obj != nsnull, "FindSafeObject() returned null in class hook!");

  
  
  
  
  
  

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    return JS_TRUE;
  }

  
  
  
  

  return XPCWrapper::Enumerate(cx, obj, unsafeObj);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                    JSObject **objp)
{
  
  if (id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    return JS_TRUE;
  }

  obj = FindSafeObject(cx, obj);
  NS_ASSERTION(obj != nsnull, "FindSafeObject() returned null in class hook!");

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    

    return JS_TRUE;
  }

  
  if (!CanCallerAccess(cx, unsafeObj)) {
    
    return JS_FALSE;
  }

  return XPCWrapper::NewResolve(cx, obj, unsafeObj, id, flags, objp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
  NS_ASSERTION(type != JSTYPE_STRING, "toString failed us");
  return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(void)
XPC_SJOW_Finalize(JSContext *cx, JSObject *obj)
{
  
  jsval v;
  if (::JS_GetReservedSlot(cx, obj, XPC_SJOW_SLOT_PRINCIPAL, &v) &&
      !JSVAL_IS_VOID(v)) {
    nsIPrincipal *principal = (nsIPrincipal *)JSVAL_TO_PRIVATE(v);

    NS_RELEASE(principal);
  }
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_CheckAccess(JSContext *cx, JSObject *obj, jsval id,
                     JSAccessMode mode, jsval *vp)
{
  
  if ((mode & JSACC_WATCH) == JSACC_PROTO && (mode & JSACC_WRITE)) {
    return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
  }

  
  if (cx->runtime->checkObjectAccess &&
      !cx->runtime->checkObjectAccess(cx, obj, id, mode, vp)) {
    return JS_FALSE;
  }

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    return JS_TRUE;
  }

  
  
  if (cx->runtime->checkObjectAccess &&
      !cx->runtime->checkObjectAccess(cx, unsafeObj, id, mode, vp)) {
    return JS_FALSE;
  }

  JSClass *clazz = JS_GET_CLASS(cx, unsafeObj);
  return !clazz->checkAccess ||
    clazz->checkAccess(cx, unsafeObj, id, mode, vp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
  JSObject *tmp = FindSafeObject(cx, obj);
  JSObject *unsafeObj, *callThisObj = nsnull;

  if (tmp) {
    
    
    
    
    
    obj = tmp;
  } else {
    
    
    
    
    
    callThisObj = obj;

    
    
    if (!CanCallerAccess(cx, callThisObj)) {
      
      return JS_FALSE;
    }

    obj = FindSafeObject(cx, JSVAL_TO_OBJECT(argv[-2]));

    if (!obj) {
      return ThrowException(NS_ERROR_INVALID_ARG, cx);
    }
  }

  unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  if (!callThisObj) {
    callThisObj = unsafeObj;
  }

  JSObject *funToCall = GetUnsafeObject(cx, JSVAL_TO_OBJECT(argv[-2]));

  if (!funToCall) {
    
    
    
    return JS_TRUE;
  }

  
  
  if (!CanCallerAccess(cx, unsafeObj) || !CanCallerAccess(cx, funToCall)) {
    
    return JS_FALSE;
  }

  
  
  
  
  
  NS_NAMED_LITERAL_CSTRING(funScript,
                           "var args = [];"
                           "for (var i = 1; i < arguments.length; i++)"
                           "args.push(arguments[i]);"
                           "return arguments[0].apply(this, args);");

  
  jsval scriptedFunVal;
  if (!GetScriptedFunction(cx, obj, unsafeObj, XPC_SJOW_SLOT_SCRIPTED_FUN,
                           funScript, &scriptedFunVal)) {
    return JS_FALSE;
  }

  JSFunction *callWrapper;
  jsval cwval;

  
  
  if (!::JS_GetReservedSlot(cx, JSVAL_TO_OBJECT(scriptedFunVal), 0, &cwval)) {
    return JS_FALSE;
  }

  if (JSVAL_IS_PRIMITIVE(cwval)) {
    
    callWrapper =
      ::JS_NewFunction(cx, XPC_SJOW_CallWrapper, 0, 0, callThisObj,
                       "XPC_SJOW_CallWrapper");
    if (!callWrapper) {
      return JS_FALSE;
    }

    
    
    
    
    
    JSObject *callWrapperObj = ::JS_GetFunctionObject(callWrapper);
    if (!::JS_SetReservedSlot(cx, JSVAL_TO_OBJECT(scriptedFunVal), 0,
                              OBJECT_TO_JSVAL(callWrapperObj))) {
      return JS_FALSE;
    }
  } else {
    
    callWrapper = ::JS_ValueToFunction(cx, cwval);

    if (!callWrapper) {
      return ThrowException(NS_ERROR_UNEXPECTED, cx);
    }
  }

  
  jsval argsBuf[8];
  jsval *args = argsBuf;

  if (argc > 7) {
    args = (jsval *)nsMemory::Alloc((argc + 2) * sizeof(jsval *));
    if (!args) {
      return ThrowException(NS_ERROR_OUT_OF_MEMORY, cx);
    }
  }

  args[0] = OBJECT_TO_JSVAL(::JS_GetFunctionObject(callWrapper));
  args[1] = OBJECT_TO_JSVAL(funToCall);

  if (args[0] == JSVAL_NULL) {
    return JS_FALSE;
  }

  for (uintN i = 0; i < argc; ++i) {
    args[i + 2] = UnwrapJSValue(cx, argv[i]);
  }

  jsval val;
  JSBool ok = ::JS_CallFunctionValue(cx, callThisObj, scriptedFunVal, argc + 2,
                                     args, &val);

  if (args != argsBuf) {
    nsMemory::Free(args);
  }

  return ok && WrapJSValue(cx, obj, val, rval);
}

JSBool
XPC_SJOW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                   jsval *rval)
{
  if (argc < 1) {
    return ThrowException(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx);
  }

  
  
  obj = nsnull;

  if (JSVAL_IS_PRIMITIVE(argv[0])) {
    JSStackFrame *fp = nsnull;
    if (JS_FrameIterator(cx, &fp) && JS_IsConstructorFrame(cx, fp)) {
      return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
    }

    *rval = argv[0];
    return JS_TRUE;
  }

  JSObject *objToWrap = JSVAL_TO_OBJECT(argv[0]);

  
  
  

  if (JS_GET_CLASS(cx, objToWrap) == &js_ScriptClass ||
      (::JS_ObjectIsFunction(cx, objToWrap) &&
       ::JS_GetFunctionNative(cx, ::JS_ValueToFunction(cx, argv[0])) ==
       XPCWrapper::sEvalNative)) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  
  if (!CanCallerAccess(cx, objToWrap)) {
    
    return JS_FALSE;
  }

  JSObject *unsafeObj = GetUnsafeObject(cx, objToWrap);

  if (unsafeObj) {
    
    

    objToWrap = unsafeObj;
  }

  
  
  JSObject *wrapperObj = ::JS_NewObject(cx, &sXPC_SJOW_JSClass.base, nsnull,
                                        objToWrap);

  if (!wrapperObj) {
    
    return JS_FALSE;
  }

  if (!::JS_SetReservedSlot(cx, wrapperObj, XPC_SJOW_SLOT_IS_RESOLVING,
                            BOOLEAN_TO_JSVAL(JS_FALSE))) {
    return JS_FALSE;
  }

  *rval = OBJECT_TO_JSVAL(wrapperObj);

  return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_Equality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
  if (JSVAL_IS_PRIMITIVE(v)) {
    *bp = JS_FALSE;
  } else {
    JSObject *unsafeObj = GetUnsafeObject(cx, obj);

    JSObject *other = JSVAL_TO_OBJECT(v);
    JSObject *otherUnsafe = GetUnsafeObject(cx, other);

    *bp = (obj == other || unsafeObj == other ||
           (unsafeObj && unsafeObj == otherUnsafe) ||
           XPC_GetIdentityObject(cx, obj) == XPC_GetIdentityObject(cx, other));
  }

  return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_SJOW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                  jsval *rval)
{
  obj = FindSafeObject(cx, obj);
  if (!obj) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);

  if (!unsafeObj) {
    
    
    

    JSString *str = JS_NewStringCopyZ(cx, "[object XPCSafeJSObjectWrapper]");
    if (!str) {
      return JS_FALSE;
    }

    *rval = STRING_TO_JSVAL(str);

    return JS_TRUE;
  }

  
  if (!CanCallerAccess(cx, unsafeObj)) {
    
    return JS_FALSE;
  }

  
  NS_NAMED_LITERAL_CSTRING(funScript, "return '' + this;");

  jsval scriptedFunVal;
  if (!GetScriptedFunction(cx, obj, unsafeObj, XPC_SJOW_SLOT_SCRIPTED_TOSTRING,
                           funScript, &scriptedFunVal)) {
    return JS_FALSE;
  }

  jsval val;
  JSBool ok = ::JS_CallFunctionValue(cx, unsafeObj, scriptedFunVal, 0, nsnull,
                                     &val);

  return ok && WrapJSValue(cx, obj, val, rval);
}

PRBool
XPC_SJOW_AttachNewConstructorObject(XPCCallContext &ccx,
                                    JSObject *aGlobalObject)
{
  
  
  
  if (!XPCWrapper::FindEval(ccx, aGlobalObject)) {
    return PR_FALSE;
  }

  JSObject *class_obj =
    ::JS_InitClass(ccx, aGlobalObject, nsnull, &sXPC_SJOW_JSClass.base,
                   XPC_SJOW_Construct, 0, nsnull, sXPC_SJOW_JSClass_methods,
                   nsnull, nsnull);
  if (!class_obj) {
    NS_WARNING("can't initialize the XPCSafeJSObjectWrapper class");
    return PR_FALSE;
  }

  
  
  
  ::JS_SetParent(ccx, class_obj, nsnull);

  
  
  ::JS_SetPrototype(ccx, class_obj, nsnull);
  if (!::JS_SealObject(ccx, class_obj, JS_FALSE)) {
    NS_WARNING("Failed to seal XPCSafeJSObjectWrapper.prototype");
    return PR_FALSE;
  }

  JSBool found;
  return ::JS_SetPropertyAttributes(ccx, aGlobalObject,
                                    sXPC_SJOW_JSClass.base.name,
                                    JSPROP_READONLY | JSPROP_PERMANENT,
                                    &found);
}
