







































#include "xpcprivate.h"
#include "XPCNativeWrapper.h"
#include "XPCWrapper.h"
#include "jsdbgapi.h"

static JSBool
XPC_NW_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_NW_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_NW_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_NW_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_NW_Enumerate(JSContext *cx, JSObject *obj);

static JSBool
XPC_NW_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                  JSObject **objp);

static JSBool
XPC_NW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

static void
XPC_NW_Finalize(JSContext *cx, JSObject *obj);

static JSBool
XPC_NW_CheckAccess(JSContext *cx, JSObject *obj, jsid id,
                   JSAccessMode mode, jsval *vp);

static JSBool
XPC_NW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
            jsval *rval);

static JSBool
XPC_NW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval);

static JSBool
XPC_NW_HasInstance(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp);

static void
XPC_NW_Trace(JSTracer *trc, JSObject *obj);

static JSBool
XPC_NW_Equality(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp);

static JSObject *
XPC_NW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly);

static JSBool
XPC_NW_FunctionWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                       jsval *rval);

using namespace XPCWrapper;





static const PRUint32 FLAG_EXPLICIT = XPCWrapper::LAST_FLAG << 1;

namespace XPCNativeWrapper { namespace internal {




js::Class NW_NoCall_Class = {
    "XPCNativeWrapper",
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS |
    
    JSCLASS_NEW_RESOLVE | JSCLASS_HAS_RESERVED_SLOTS(1) |
    JSCLASS_MARK_IS_TRACE | JSCLASS_CONSTRUCT_PROTOTYPE,
    js::Valueify(XPC_NW_AddProperty),
    js::Valueify(XPC_NW_DelProperty),
    js::Valueify(XPC_NW_GetProperty),
    js::Valueify(XPC_NW_SetProperty),
    XPC_NW_Enumerate,
    (JSResolveOp)XPC_NW_NewResolve,
    js::Valueify(XPC_NW_Convert),
    XPC_NW_Finalize,
    nsnull,   
    js::Valueify(XPC_NW_CheckAccess),
    nsnull,   
    js::Valueify(XPC_NW_Construct),
    nsnull,   
    js::Valueify(XPC_NW_HasInstance),
    JS_CLASS_TRACE(XPC_NW_Trace),

    
    {
      js::Valueify(XPC_NW_Equality),
      nsnull, 
      nsnull, 
      XPC_NW_Iterator,
      nsnull, 
    }
};

js::Class NW_Call_Class = {
    "XPCNativeWrapper",
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS |
    
    JSCLASS_NEW_RESOLVE | JSCLASS_HAS_RESERVED_SLOTS(1) |
    JSCLASS_MARK_IS_TRACE | JSCLASS_CONSTRUCT_PROTOTYPE,
    js::Valueify(XPC_NW_AddProperty),
    js::Valueify(XPC_NW_DelProperty),
    js::Valueify(XPC_NW_GetProperty),
    js::Valueify(XPC_NW_SetProperty),
    XPC_NW_Enumerate,
    (JSResolveOp)XPC_NW_NewResolve,
    js::Valueify(XPC_NW_Convert),
    XPC_NW_Finalize,
    nsnull,   
    js::Valueify(XPC_NW_CheckAccess),
    js::Valueify(XPC_NW_Call),
    js::Valueify(XPC_NW_Construct),
    nsnull,   
    js::Valueify(XPC_NW_HasInstance),
    JS_CLASS_TRACE(XPC_NW_Trace),

    
    {
      js::Valueify(XPC_NW_Equality),
      nsnull, 
      nsnull, 
      XPC_NW_Iterator,
      nsnull, 
    }
};

} 

JSBool
GetWrappedNative(JSContext *cx, JSObject *obj,
                 XPCWrappedNative **aWrappedNative)
{
  XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));
  *aWrappedNative = wn;
  if (!wn) {
    return JS_TRUE;
  }

  nsIScriptSecurityManager *ssm = GetSecurityManager();
  if (!ssm) {
    return JS_TRUE;
  }

  nsIPrincipal *subjectPrincipal = ssm->GetCxSubjectPrincipal(cx);
  if (!subjectPrincipal) {
    return JS_TRUE;
  }

  XPCWrappedNativeScope *scope = wn->GetScope();
  nsIPrincipal *objectPrincipal = scope->GetPrincipal();

  PRBool subsumes;
  nsresult rv = subjectPrincipal->Subsumes(objectPrincipal, &subsumes);
  if (NS_FAILED(rv) || !subsumes) {
    PRBool isPrivileged;
    rv = ssm->IsCapabilityEnabled("UniversalXPConnect", &isPrivileged);
    return NS_SUCCEEDED(rv) && isPrivileged;
  }

  return JS_TRUE;
}

JSBool
WrapFunction(JSContext* cx, JSObject* funobj, jsval *rval)
{
  
  if (JS_GetFunctionNative(cx,
                           JS_ValueToFunction(cx, OBJECT_TO_JSVAL(funobj))) ==
      XPC_NW_FunctionWrapper) {
    *rval = OBJECT_TO_JSVAL(funobj);
    return JS_TRUE;
  }

  
  
  
  JSStackFrame *iterator = nsnull;
  if (!::JS_FrameIterator(cx, &iterator)) {
    ::JS_ReportError(cx, "XPCNativeWrappers must be used from script");
    return JS_FALSE;
  }

  
  
  
  
  
  JSFunction *funWrapper =
    ::JS_NewFunction(cx, XPC_NW_FunctionWrapper, 0, 0, nsnull,
                     "XPCNativeWrapper function wrapper");
  if (!funWrapper) {
    return JS_FALSE;
  }

  JSObject* funWrapperObj = ::JS_GetFunctionObject(funWrapper);
  ::JS_SetParent(cx, funWrapperObj, funobj);
  *rval = OBJECT_TO_JSVAL(funWrapperObj);

  JS_SetReservedSlot(cx, funWrapperObj, eAllAccessSlot, JSVAL_FALSE);

  return JS_TRUE;
}

JSBool
RewrapValue(JSContext *cx, JSObject *obj, jsval v, jsval *rval)
{
  NS_ASSERTION(XPCNativeWrapper::IsNativeWrapper(obj),
               "Unexpected object");

  if (JSVAL_IS_PRIMITIVE(v)) {
    *rval = v;
    return JS_TRUE;
  }

  JSObject* nativeObj = JSVAL_TO_OBJECT(v);

  
  if (JS_ObjectIsFunction(cx, nativeObj)) {
    return WrapFunction(cx, nativeObj, rval);
  }

  JSObject *scope = JS_GetScopeChain(cx);
  if (!scope) {
    return JS_FALSE;
  }

  jsval flags;
  ::JS_GetReservedSlot(cx, obj, 0, &flags);
  WrapperType type = HAS_FLAGS(flags, FLAG_EXPLICIT)
                     ? XPCNW_EXPLICIT : XPCNW_IMPLICIT;
  return RewrapObject(cx, JS_GetGlobalForObject(cx, scope),
                      nativeObj, type, rval);
}

} 

using namespace XPCNativeWrapper;

static JSBool
XPC_NW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval);

static inline
JSBool
ThrowException(nsresult ex, JSContext *cx)
{
  XPCThrower::Throw(ex, cx);

  return JS_FALSE;
}

static inline
JSBool
EnsureLegalActivity(JSContext *cx, JSObject *obj,
                    jsid id = JSID_VOID, PRUint32 accessType = 0)
{
  nsIScriptSecurityManager *ssm = GetSecurityManager();
  if (!ssm) {
    
    
    return JS_TRUE;
  }

  JSStackFrame *fp;
  nsIPrincipal *subjectPrincipal = ssm->GetCxSubjectPrincipalAndFrame(cx, &fp);
  if (!subjectPrincipal || !fp) {
    
    return JS_TRUE;
  }

  PRBool isSystem;
  if (NS_SUCCEEDED(ssm->IsSystemPrincipal(subjectPrincipal, &isSystem)) &&
      isSystem) {
    
    return JS_TRUE;
  }

  jsval flags;

  JS_GetReservedSlot(cx, obj, sFlagsSlot, &flags);
  if (HAS_FLAGS(flags, FLAG_SOW) && !SystemOnlyWrapper::CheckFilename(cx, id, fp)) {
    return JS_FALSE;
  }

  
  
  XPCWrappedNative *wn = XPCNativeWrapper::SafeGetWrappedNative(obj);
  if (wn) {
    nsIPrincipal *objectPrincipal = wn->GetScope()->GetPrincipal();
    PRBool subsumes;
    if (NS_FAILED(subjectPrincipal->Subsumes(objectPrincipal, &subsumes)) ||
        !subsumes) {
      
      PRBool isPrivileged = PR_FALSE;
      nsresult rv =
        ssm->IsCapabilityEnabled("UniversalXPConnect", &isPrivileged);
      if (NS_SUCCEEDED(rv) && isPrivileged) {
        return JS_TRUE;
      }

      JSObject* flatObj;
      if (!JSID_IS_VOID(id) &&
          (accessType & (sSecMgrSetProp | sSecMgrGetProp)) &&
          (flatObj = wn->GetFlatJSObject())) {
        rv = ssm->CheckPropertyAccess(cx, flatObj,
                                      flatObj->getClass()->name,
                                      id, accessType);
        return NS_SUCCEEDED(rv);
      }

      return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
    }
  }

#ifdef DEBUG
  
  

  if (HAS_FLAGS(flags, FLAG_EXPLICIT)) {
    
    return JS_TRUE;
  }

  JSScript *script = JS_GetFrameScript(cx, fp);
  if (!script) {
    
    
    return JS_TRUE;
  }

  uint32 fileFlags = JS_GetScriptFilenameFlags(script);
  if (fileFlags == JSFILENAME_NULL || (fileFlags & JSFILENAME_SYSTEM)) {
    
    return JS_TRUE;
  }

  
  
  NS_ERROR("Implicit native wrapper in content code");
  return JS_FALSE;
#else
  return JS_TRUE;
#endif

  
}

static JSBool
XPC_NW_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  JSPropertyDescriptor desc;

  if (!JS_GetPropertyDescriptorById(cx, obj, id, JSRESOLVE_QUALIFIED,
                                    &desc)) {
    return JS_FALSE;
  }

  
  if (desc.attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  jsval flags = JSVAL_VOID;
  JS_GetReservedSlot(cx, obj, 0, &flags);
  
  
  
  
  if (!HAS_FLAGS(flags, FLAG_RESOLVING)) {
    return JS_TRUE;
  }

  
  
  return EnsureLegalActivity(cx, obj, id, sSecMgrSetProp) &&
         RewrapValue(cx, obj, *vp, vp);
}

static JSBool
XPC_NW_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  return EnsureLegalActivity(cx, obj);
}

static JSBool
XPC_NW_FunctionWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                       jsval *rval)
{
  JSObject *funObj = JSVAL_TO_OBJECT(argv[-2]);
  if (!::JS_ObjectIsFunction(cx, funObj)) {
    obj = nsnull;
  }

  while (obj && !XPCNativeWrapper::IsNativeWrapper(obj)) {
    obj = obj->getProto();
  }

  if (!obj) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  
  
  JSObject *methodToCallObj = funObj->getParent();
  XPCWrappedNative* wrappedNative = nsnull;

  jsval isAllAccess;
  if (::JS_GetReservedSlot(cx, funObj, eAllAccessSlot, &isAllAccess) &&
      JSVAL_TO_BOOLEAN(isAllAccess)) {
    wrappedNative = XPCNativeWrapper::SafeGetWrappedNative(obj);
  } else if (!XPCNativeWrapper::GetWrappedNative(cx, obj, &wrappedNative)) {
    wrappedNative = nsnull;
  }

  if (!wrappedNative || !::JS_ObjectIsFunction(cx, methodToCallObj)) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  jsval v;
  if (!::JS_CallFunctionValue(cx, wrappedNative->GetFlatJSObject(),
                              OBJECT_TO_JSVAL(methodToCallObj), argc, argv,
                              &v)) {
    return JS_FALSE;
  }

  XPCCallContext ccx(JS_CALLER, cx, obj);

  
  AUTO_MARK_JSVAL(ccx, v);

  return RewrapValue(cx, obj, v, rval);
}

static JSBool
GetwrappedJSObject(JSContext *cx, JSObject *obj, jsval *vp)
{
  
  
  

  nsIScriptSecurityManager *ssm = GetSecurityManager();
  nsCOMPtr<nsIPrincipal> prin;
  nsresult rv = ssm->GetObjectPrincipal(cx, obj, getter_AddRefs(prin));
  if (NS_FAILED(rv)) {
    return ThrowException(rv, cx);
  }

  jsval v = OBJECT_TO_JSVAL(obj);

  PRBool isSystem;
  if (NS_SUCCEEDED(ssm->IsSystemPrincipal(prin, &isSystem)) && isSystem) {
    *vp = v;
    return JS_TRUE;
  }

  return XPCSafeJSObjectWrapper::WrapObject(cx, JS_GetScopeChain(cx), v, vp);
}

static JSBool
XPC_NW_GetOrSetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp,
                        JSBool aIsSet)
{
  
  if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_PROTOTYPE) ||
      id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    return JS_TRUE;
  }

  while (!XPCNativeWrapper::IsNativeWrapper(obj)) {
    obj = obj->getProto();
    if (!obj) {
      return ThrowException(NS_ERROR_UNEXPECTED, cx);
    }
  }

  if (!EnsureLegalActivity(cx, obj, id,
                           aIsSet ? sSecMgrSetProp : sSecMgrGetProp)) {
    return JS_FALSE;
  }

  
  XPCWrappedNative *wrappedNative = XPCNativeWrapper::SafeGetWrappedNative(obj);

  if (!wrappedNative) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  JSObject *nativeObj = wrappedNative->GetFlatJSObject();

  if (!aIsSet &&
      id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_WRAPPED_JSOBJECT)) {
    return GetwrappedJSObject(cx, nativeObj, vp);
  }

  return GetOrSetNativeProperty(cx, obj, wrappedNative, id, vp, aIsSet,
                                JS_TRUE);
}

static JSBool
XPC_NW_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  return XPC_NW_GetOrSetProperty(cx, obj, id, vp, PR_FALSE);
}

static JSBool
XPC_NW_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  return XPC_NW_GetOrSetProperty(cx, obj, id, vp, PR_TRUE);
}

static JSBool
XPC_NW_Enumerate(JSContext *cx, JSObject *obj)
{
  
  
  
  
  

  if (!EnsureLegalActivity(cx, obj)) {
    return JS_FALSE;
  }

  
  XPCWrappedNative *wn = XPCNativeWrapper::SafeGetWrappedNative(obj);
  if (!wn) {
    return JS_TRUE;
  }

  return Enumerate(cx, obj, wn->GetFlatJSObject());
}

static JSBool
XPC_NW_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                  JSObject **objp)
{
  
  
  
  
  if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_WRAPPED_JSOBJECT)) {
    return JS_TRUE;
  }

  if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    *objp = obj;

    
    
    JSFunction *fun = JS_NewFunction(cx, XPC_NW_toString, 0, 0, nsnull,
                                     "toString");
    if (!fun) {
      return JS_FALSE;
    }

    JSObject *funobj = JS_GetFunctionObject(fun);
    funobj->setParent(obj);

    return JS_DefineProperty(cx, obj, "toString", OBJECT_TO_JSVAL(funobj),
                             nsnull, nsnull, 0);
  }

  PRUint32 accessType =
    (flags & JSRESOLVE_ASSIGNING) ? sSecMgrSetProp : sSecMgrGetProp;
  if (!EnsureLegalActivity(cx, obj, id, accessType)) {
    return JS_FALSE;
  }

  while (!XPCNativeWrapper::IsNativeWrapper(obj)) {
    obj = obj->getProto();
    if (!obj) {
      return ThrowException(NS_ERROR_UNEXPECTED, cx);
    }
  }

  
  XPCWrappedNative *wrappedNative = XPCNativeWrapper::SafeGetWrappedNative(obj);

  if (!wrappedNative) {
    

    return JS_TRUE;
  }

  return ResolveNativeProperty(cx, obj, wrappedNative->GetFlatJSObject(),
                               wrappedNative, id, flags, objp, JS_TRUE);
}

static JSBool
XPC_NW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
  return EnsureLegalActivity(cx, obj);
}

static void
XPC_NW_Finalize(JSContext *cx, JSObject *obj)
{
  
  
  XPCJSRuntime *rt = nsXPConnect::GetRuntimeInstance();

  {
    
    XPCAutoLock lock(rt->GetMapLock());
    rt->GetExplicitNativeWrapperMap()->Remove(obj);
  }
}

static JSBool
XPC_NW_CheckAccess(JSContext *cx, JSObject *obj, jsid id,
                   JSAccessMode mode, jsval *vp)
{
  
  if ((mode & JSACC_WATCH) == JSACC_PROTO && (mode & JSACC_WRITE)) {
    return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
  }

  
  JSSecurityCallbacks *callbacks = JS_GetSecurityCallbacks(cx);
  if (callbacks && callbacks->checkObjectAccess &&
      !callbacks->checkObjectAccess(cx, obj, id, mode, vp)) {
    return JS_FALSE;
  }

  
  XPCWrappedNative *wrappedNative = XPCNativeWrapper::SafeGetWrappedNative(obj);
  if (!wrappedNative) {
    return JS_TRUE;
  }

  JSObject *wrapperJSObject = wrappedNative->GetFlatJSObject();

  JSClass *clazz = wrapperJSObject->getJSClass();
  return !clazz->checkAccess ||
    clazz->checkAccess(cx, wrapperJSObject, id, mode, vp);
}

static JSBool
XPC_NW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
#ifdef DEBUG
  if (!XPCNativeWrapper::IsNativeWrapper(obj) &&
      !JS_ObjectIsFunction(cx, obj)) {
    NS_WARNING("Ignoring a call for a weird object");
  }
#endif
  return JS_TRUE;
}

static JSBool
XPC_NW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval)
{
  
  
  
  
  obj = JSVAL_TO_OBJECT(argv[-2]);

  if (!EnsureLegalActivity(cx, obj)) {
    return JS_FALSE;
  }

  
  XPCWrappedNative *wrappedNative = XPCNativeWrapper::SafeGetWrappedNative(obj);
  if (!wrappedNative) {
    return JS_TRUE;
  }

  JSBool retval = JS_TRUE;

  if (!NATIVE_HAS_FLAG(wrappedNative, WantConstruct)) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  nsresult rv = wrappedNative->GetScriptableInfo()->
    GetCallback()->Construct(wrappedNative, cx, obj, argc, argv, rval,
                             &retval);
  if (NS_FAILED(rv)) {
    return ThrowException(rv, cx);
  }

  if (!retval) {
    return JS_FALSE;
  }

  if (JSVAL_IS_PRIMITIVE(*rval)) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  return RewrapValue(cx, obj, *rval, rval);
}

static JSBool
XPC_NW_HasInstance(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp)
{
  return JS_TRUE;
}

static JSBool
MirrorWrappedNativeParent(JSContext *cx, XPCWrappedNative *wrapper,
                          JSObject **result NS_OUTPARAM)
{
  JSObject *wn_parent = wrapper->GetFlatJSObject()->getParent();
  if (!wn_parent) {
    *result = nsnull;
  } else {
    XPCWrappedNative *parent_wrapper =
      XPCWrappedNative::GetAndMorphWrappedNativeOfJSObject(cx, wn_parent);

    
    
    
    if (parent_wrapper) {
      *result = XPCNativeWrapper::GetNewOrUsed(cx, parent_wrapper, nsnull,
                                               nsnull);
      if (!*result)
        return JS_FALSE;
    } else {
      *result = nsnull;
    }
  }
  return JS_TRUE;
}

static JSBool
XPCNativeWrapperCtor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                     jsval *rval)
{
  JSStackFrame *fp = nsnull;
  JSBool constructing = JS_FALSE;
  if (JS_FrameIterator(cx, &fp) && JS_IsConstructorFrame(cx, fp)) {
    constructing = JS_TRUE;

    JSObject *proto = obj->getProto();
    if (proto && !XPCNativeWrapper::IsNativeWrapper(proto)) {
      

      JS_ASSERT(XPCNativeWrapper::IsNativeWrapper(obj));
      return JS_SetPrivate(cx, obj, nsnull) &&
             JS_SetReservedSlot(cx, obj, 0, JSVAL_ZERO);
    }
  }

  if (argc < 1) {
    return ThrowException(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx);
  }

  
  
  obj = nsnull;

  jsval native = argv[0];

  if (JSVAL_IS_PRIMITIVE(native)) {
    if (constructing) {
      return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
    }

    *rval = native;
    return JS_TRUE;
  }

  JSObject *nativeObj = JSVAL_TO_OBJECT(native);

  
  
  nativeObj = UnsafeUnwrapSecurityWrapper(cx, nativeObj);
  if (!nativeObj) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }
  native = OBJECT_TO_JSVAL(nativeObj);

  
  JSObject *scope = JS_GetScopeChain(cx);
  if (!scope) {
    return JS_FALSE;
  }

  XPCWrappedNativeScope *xpcscope =
    XPCWrappedNativeScope::FindInJSObjectScope(cx, scope);
  NS_ASSERTION(xpcscope, "what crazy scope are we in?");

  XPCWrappedNative *wrappedNative;
  WrapperType type = xpcscope->GetWrapperFor(cx, nativeObj, XPCNW_EXPLICIT,
                                             &wrappedNative);

  if (type != NONE && !(type & XPCNW_EXPLICIT)) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  
  if (!wrappedNative) {
    wrappedNative =
      XPCWrappedNative::GetAndMorphWrappedNativeOfJSObject(cx, nativeObj);

    if (!wrappedNative) {
      return ThrowException(NS_ERROR_INVALID_ARG, cx);
    }
  }

  
  
  nsCOMPtr<nsIXPConnectWrappedJS> xpcwrappedjs =
    do_QueryWrappedNative(wrappedNative);

  if (xpcwrappedjs) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  if (argc == 2 && !JSVAL_IS_PRIMITIVE(argv[1])) {
    
    
    
    
    JSBool hasInstance;
    if (!JS_HasInstance(cx, JSVAL_TO_OBJECT(argv[1]), native, &hasInstance)) {
      return ThrowException(NS_ERROR_UNEXPECTED, cx);
    }

    if (!hasInstance) {
      return ThrowException(NS_ERROR_INVALID_ARG, cx);
    }
  }

  if (!XPCNativeWrapper::CreateExplicitWrapper(cx, wrappedNative, rval)) {
    return JS_FALSE;
  }

  if (!(type & SOW)) {
    return JS_TRUE;
  }

  return SystemOnlyWrapper::MakeSOW(cx, JSVAL_TO_OBJECT(*rval));
}

static void
XPC_NW_Trace(JSTracer *trc, JSObject *obj)
{
  
  XPCWrappedNative *wrappedNative = XPCNativeWrapper::SafeGetWrappedNative(obj);

  if (wrappedNative && wrappedNative->IsValid()) {
    JS_CALL_OBJECT_TRACER(trc, wrappedNative->GetFlatJSObject(),
                          "wrappedNative.flatJSObject");
  }
}

static JSBool
XPC_NW_Equality(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp)
{
  NS_ASSERTION(XPCNativeWrapper::IsNativeWrapper(obj),
               "Uh, we should only ever be called for XPCNativeWrapper "
               "objects!");

  if (!EnsureLegalActivity(cx, obj)) {
    return JS_FALSE;
  }

  jsval v = *valp;
  if (JSVAL_IS_PRIMITIVE(v)) {
    *bp = JS_FALSE;

    return JS_TRUE;
  }

  
  XPCWrappedNative *wrappedNative = XPCNativeWrapper::SafeGetWrappedNative(obj);

  if (wrappedNative && wrappedNative->IsValid() &&
      NATIVE_HAS_FLAG(wrappedNative, WantEquality)) {
    
    nsresult rv = wrappedNative->GetScriptableCallback()->
      Equality(wrappedNative, cx, obj, v, bp);

    if (NS_FAILED(rv)) {
      return ThrowException(rv, cx);
    }
  } else {
    JSObject *other = JSVAL_TO_OBJECT(v);

    *bp = (obj == other ||
           XPC_GetIdentityObject(cx, obj) == XPC_GetIdentityObject(cx, other));
  }

  return JS_TRUE;
}

static JSObject *
XPC_NW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly)
{
  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    ThrowException(NS_ERROR_FAILURE, cx);
    return nsnull;
  }

  JSObject *wrapperIter =
    JS_NewObjectWithGivenProto(cx, XPCNativeWrapper::GetJSClass(false), nsnull,
                               obj->getParent());
  if (!wrapperIter) {
    return nsnull;
  }

  js::AutoObjectRooter tvr(cx, wrapperIter);

  
  XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(JS_GetPrivate(cx, obj));
  JS_SetPrivate(cx, wrapperIter, wn);
  if (!JS_SetReservedSlot(cx, wrapperIter, 0, INT_TO_JSVAL(FLAG_EXPLICIT))) {
    return nsnull;
  }

  return CreateIteratorObj(cx, wrapperIter, obj, wn->GetFlatJSObject(),
                           keysonly);
}

static JSBool
XPC_NW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
  while (!XPCNativeWrapper::IsNativeWrapper(obj)) {
    obj = obj->getProto();
    if (!obj) {
      return ThrowException(NS_ERROR_UNEXPECTED, cx);
    }
  }

  if (!EnsureLegalActivity(cx, obj,
                           GetRTIdByIndex(cx, XPCJSRuntime::IDX_TO_STRING),
                           sSecMgrGetProp)) {
    return JS_FALSE;
  }

  
  XPCWrappedNative *wrappedNative = XPCNativeWrapper::SafeGetWrappedNative(obj);

  if (!wrappedNative) {
    
    NS_NAMED_LITERAL_STRING(protoString, "[object XPCNativeWrapper]");
    JSString *str =
      ::JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar*>
                                                (protoString.get()),
                            protoString.Length());
    NS_ENSURE_TRUE(str, JS_FALSE);
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
  }

  return NativeToString(cx, wrappedNative, argc, argv, rval, JS_TRUE);
}

static JSBool
UnwrapNW(JSContext *cx, uintN argc, jsval *vp)
{
  if (argc != 1) {
    return ThrowException(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx);
  }

  jsval v = JS_ARGV(cx, vp)[0];
  if (JSVAL_IS_PRIMITIVE(v)) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  if (!IsNativeWrapper(JSVAL_TO_OBJECT(v))) {
    JS_SET_RVAL(cx, vp, v);
    return JS_TRUE;
  }

  XPCWrappedNative *wn;
  if (!XPCNativeWrapper::GetWrappedNative(cx, JSVAL_TO_OBJECT(v), &wn)) {
    return JS_FALSE;
  }

  if (!wn) {
    JS_SET_RVAL(cx, vp, JSVAL_NULL);
    return JS_TRUE;
  }

  return GetwrappedJSObject(cx, wn->GetFlatJSObject(), vp);
}

static JSFunctionSpec static_functions[] = {
  JS_FN("unwrap", UnwrapNW, 1, 0),
  JS_FS_END
};


PRBool
XPCNativeWrapper::AttachNewConstructorObject(XPCCallContext &ccx,
                                             JSObject *aGlobalObject)
{
  JSObject *class_obj =
    ::JS_InitClass(ccx, aGlobalObject, nsnull,
                   js::Jsvalify(&internal::NW_Call_Class),
                   XPCNativeWrapperCtor, 0, nsnull, nsnull,
                   nsnull, static_functions);
  if (!class_obj) {
    NS_WARNING("can't initialize the XPCNativeWrapper class");
    return PR_FALSE;
  }
  
  
  
  ::JS_SetPrototype(ccx, class_obj, nsnull);
  if (!::JS_SealObject(ccx, class_obj, JS_FALSE)) {
    NS_WARNING("Failed to seal XPCNativeWrapper.prototype");
    return PR_FALSE;
  }

  JSBool found;
  return ::JS_SetPropertyAttributes(ccx, aGlobalObject,
                                    internal::NW_Call_Class.name,
                                    JSPROP_READONLY | JSPROP_PERMANENT,
                                    &found);
}


JSObject *
XPCNativeWrapper::GetNewOrUsed(JSContext *cx, XPCWrappedNative *wrapper,
                               JSObject *scope, nsIPrincipal *aObjectPrincipal)
{
  if (aObjectPrincipal) {
    nsIScriptSecurityManager *ssm = GetSecurityManager();

    PRBool isSystem;
    nsresult rv = ssm->IsSystemPrincipal(aObjectPrincipal, &isSystem);
    if (NS_SUCCEEDED(rv) && !isSystem) {
      jsval v = OBJECT_TO_JSVAL(wrapper->GetFlatJSObject());
      if (!CreateExplicitWrapper(cx, wrapper, &v)) {
        return nsnull;
      }
      return JSVAL_TO_OBJECT(v);
    }
  }

  
  
  nsCOMPtr<nsIXPConnectWrappedJS> xpcwrappedjs(do_QueryWrappedNative(wrapper));

  if (xpcwrappedjs) {
    JSObject *flat = wrapper->GetFlatJSObject();
    jsval v = OBJECT_TO_JSVAL(flat);

    XPCCallContext ccx(JS_CALLER, cx);

    
    AUTO_MARK_JSVAL(ccx, v);

    if (XPCSafeJSObjectWrapper::WrapObject(cx, scope, v, &v))
        return JSVAL_TO_OBJECT(v);

    return nsnull;
  }

  JSObject *obj = wrapper->GetWrapper();
  if (obj) {
    return obj;
  }

  JSObject *nw_parent;
  if (!MirrorWrappedNativeParent(cx, wrapper, &nw_parent)) {
    return nsnull;
  }

  PRBool lock;

  if (!nw_parent) {
    nw_parent = wrapper->GetScope()->GetGlobalJSObject();

    lock = PR_FALSE;
  } else {
    lock = PR_TRUE;
  }

  if (lock) {
    
    
    ::JS_LockGCThing(cx, nw_parent);
  }

  bool call = NATIVE_HAS_FLAG(wrapper, WantCall) ||
              NATIVE_HAS_FLAG(wrapper, WantConstruct);
  obj = ::JS_NewObjectWithGivenProto(cx, GetJSClass(call), nsnull, nw_parent);

  if (lock) {
    ::JS_UnlockGCThing(cx, nw_parent);
  }

  if (!obj ||
      !::JS_SetPrivate(cx, obj, wrapper) ||
      !::JS_SetReservedSlot(cx, obj, 0, JSVAL_ZERO)) {
    return nsnull;
  }

  wrapper->SetWrapper(obj);

#if defined(DEBUG_XPCNativeWrapper) || defined(DEBUG_xpc_leaks)
  {
    XPCCallContext ccx(NATIVE_CALLER, cx);

    
    AUTO_MARK_JSVAL(ccx, OBJECT_TO_JSVAL(obj));

    char *s = wrapper->ToString(ccx);
    printf("Created new XPCNativeWrapper %p for wrapped native %s\n",
           (void*)obj, s);
    if (s)
      JS_smprintf_free(s);
  }
#endif
  
  return obj;
}


JSBool
XPCNativeWrapper::CreateExplicitWrapper(JSContext *cx,
                                        XPCWrappedNative *wrappedNative,
                                        jsval *rval)
{
#ifdef DEBUG_XPCNativeWrapper
  printf("Creating new JSObject\n");
#endif

  bool call = NATIVE_HAS_FLAG(wrappedNative, WantCall) ||
              NATIVE_HAS_FLAG(wrappedNative, WantConstruct);
  JSObject *wrapperObj =
    JS_NewObjectWithGivenProto(cx, XPCNativeWrapper::GetJSClass(call), nsnull,
                               wrappedNative->GetScope()->GetGlobalJSObject());

  if (!wrapperObj) {
    
    return JS_FALSE;
  }

  if (!::JS_SetReservedSlot(cx, wrapperObj, 0, INT_TO_JSVAL(FLAG_EXPLICIT))) {
    return JS_FALSE;
  }

  JSObject *parent = nsnull;

  
  
  JS_LockGCThing(cx, wrapperObj);

  
  
  if (!MirrorWrappedNativeParent(cx, wrappedNative, &parent))
    return JS_FALSE;

  JS_UnlockGCThing(cx, wrapperObj);

  if (!parent) {
    parent = wrappedNative->GetScope()->GetGlobalJSObject();
  }

  if (!JS_SetParent(cx, wrapperObj, parent))
    return JS_FALSE;

  
  if (!JS_SetPrivate(cx, wrapperObj, wrappedNative)) {
    return JS_FALSE;
  }

#if defined(DEBUG_XPCNativeWrapper) || defined(DEBUG_xpc_leaks)
  {
    XPCCallContext ccx(JS_CALLER, cx);

    
    AUTO_MARK_JSVAL(ccx, OBJECT_TO_JSVAL(wrapperObj));

    char *s = wrappedNative->ToString(ccx);
    printf("Created new XPCNativeWrapper %p for wrapped native %s\n",
           (void*)wrapperObj, s);
    if (s)
      JS_smprintf_free(s);
  }
#endif

  *rval = OBJECT_TO_JSVAL(wrapperObj);

  {
    XPCJSRuntime *rt = wrappedNative->GetRuntime();

    
    XPCAutoLock lock(rt->GetMapLock());
    rt->GetExplicitNativeWrapperMap()->Add(wrapperObj);
  }

  return JS_TRUE;
}

struct WrapperAndCxHolder
{
    XPCWrappedNative* wrapper;
    JSContext* cx;
};

static JSDHashOperator
ClearNativeWrapperScope(JSDHashTable *table, JSDHashEntryHdr *hdr,
                        uint32 number, void *arg)
{
    JSDHashEntryStub* entry = (JSDHashEntryStub*)hdr;
    WrapperAndCxHolder* d = (WrapperAndCxHolder*)arg;

    if (d->wrapper->GetWrapper() == (JSObject*)entry->key)
    {
        ::JS_ClearScope(d->cx, (JSObject*)entry->key);
    }

    return JS_DHASH_NEXT;
}


void
XPCNativeWrapper::ClearWrappedNativeScopes(JSContext* cx,
                                           XPCWrappedNative* wrapper)
{
  JSObject *nativeWrapper = wrapper->GetWrapper();

  if (nativeWrapper) {
    ::JS_ClearScope(cx, nativeWrapper);
  }

  WrapperAndCxHolder d =
    {
      wrapper,
      cx
    };

  wrapper->GetRuntime()->GetExplicitNativeWrapperMap()->
    Enumerate(ClearNativeWrapperScope, &d);
}
