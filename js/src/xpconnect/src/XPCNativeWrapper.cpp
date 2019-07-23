







































#include "xpcprivate.h"
#include "XPCNativeWrapper.h"
#include "XPCWrapper.h"
#include "jsdbgapi.h"

static JSBool
XPC_NW_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
XPC_NW_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
XPC_NW_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
XPC_NW_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
XPC_NW_Enumerate(JSContext *cx, JSObject *obj);

static JSBool
XPC_NW_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                  JSObject **objp);

static JSBool
XPC_NW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

static void
XPC_NW_Finalize(JSContext *cx, JSObject *obj);

static JSBool
XPC_NW_CheckAccess(JSContext *cx, JSObject *obj, jsval id,
                   JSAccessMode mode, jsval *vp);

static JSBool
XPC_NW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
            jsval *rval);

static JSBool
XPC_NW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval);

static JSBool
XPC_NW_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

static void
XPC_NW_Trace(JSTracer *trc, JSObject *obj);

static JSBool
XPC_NW_Equality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

static JSObject *
XPC_NW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly);

static JSBool
XPC_NW_FunctionWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                       jsval *rval);




JSExtendedClass XPCNativeWrapper::sXPC_NW_JSClass = {
  
  { "XPCNativeWrapper",
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS |
    
    JSCLASS_NEW_RESOLVE | JSCLASS_HAS_RESERVED_SLOTS(1) |
    JSCLASS_MARK_IS_TRACE | JSCLASS_IS_EXTENDED | JSCLASS_CONSTRUCT_PROTOTYPE,
    XPC_NW_AddProperty, XPC_NW_DelProperty,
    XPC_NW_GetProperty, XPC_NW_SetProperty,
    XPC_NW_Enumerate,   (JSResolveOp)XPC_NW_NewResolve,
    XPC_NW_Convert,     XPC_NW_Finalize,
    nsnull,             XPC_NW_CheckAccess,
    XPC_NW_Call,        XPC_NW_Construct,
    nsnull,             XPC_NW_HasInstance,
    JS_CLASS_TRACE(XPC_NW_Trace), nsnull
  },

  
  XPC_NW_Equality,
  nsnull,             
  nsnull,             
  XPC_NW_Iterator,
  nsnull,             
  JSCLASS_NO_RESERVED_MEMBERS
};










#define XPC_NW_CALL_HOOK(obj, hook, args)                                 \
  return STOBJ_GET_CLASS(obj)->hook args;

#define XPC_NW_CAST_HOOK(obj, type, hook, args)                           \
  return ((type) STOBJ_GET_CLASS(obj)->hook) args;

static JSBool
ShouldBypassNativeWrapper(JSContext *cx, JSObject *obj)
{
  NS_ASSERTION(XPCNativeWrapper::IsNativeWrapper(obj),
               "Unexpected object");
  jsval flags;

  ::JS_GetReservedSlot(cx, obj, 0, &flags);
  if (HAS_FLAGS(flags, FLAG_EXPLICIT))
    return JS_FALSE;

  
  JSStackFrame *fp = JS_GetScriptedCaller(cx, NULL);
  JSScript *script = fp ? fp->script : NULL;

  
  
  return !script || !(::JS_GetScriptFilenameFlags(script) & JSFILENAME_SYSTEM);
}

#define XPC_NW_BYPASS_BASE(cx, obj, code)                                     \
  JS_BEGIN_MACRO                                                              \
    if (ShouldBypassNativeWrapper(cx, obj)) {                                 \
      /* Use SafeGetWrappedNative since obj can't be an explicit native       \
         wrapper. */                                                          \
      XPCWrappedNative *wn_ = XPCNativeWrapper::SafeGetWrappedNative(obj);    \
      if (!wn_) {                                                             \
        return JS_TRUE;                                                       \
      }                                                                       \
      obj = wn_->GetFlatJSObject();                                           \
      code                                                                    \
    }                                                                         \
  JS_END_MACRO

#define XPC_NW_BYPASS(cx, obj, hook, args)                                    \
  XPC_NW_BYPASS_BASE(cx, obj, XPC_NW_CALL_HOOK(obj, hook, args))

#define XPC_NW_BYPASS_CAST(cx, obj, type, hook, args)                         \
  XPC_NW_BYPASS_BASE(cx, obj, XPC_NW_CAST_HOOK(obj, type, hook, args))

#define XPC_NW_BYPASS_TEST(cx, obj, hook, args)                               \
  XPC_NW_BYPASS_BASE(cx, obj,                                                 \
    JSClass *clasp_ = STOBJ_GET_CLASS(obj);                                  \
    return !clasp_->hook || clasp_->hook args;                                \
  )

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
                    jsval id = JSVAL_VOID, PRUint32 accessType = 0)
{
  nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
  if (!ssm) {
    
    
    return JS_TRUE;
  }

  JSStackFrame *fp;
  nsIPrincipal *subjectPrincipal = ssm->GetCxSubjectPrincipalAndFrame(cx, &fp);
  if (!subjectPrincipal || !fp) {
    
    return JS_TRUE;
  }

  
  void *annotation = JS_GetFrameAnnotation(cx, fp);
  PRBool isPrivileged = PR_FALSE;
  nsresult rv = subjectPrincipal->IsCapabilityEnabled("UniversalXPConnect",
                                                      annotation,
                                                      &isPrivileged);
  if (NS_SUCCEEDED(rv) && isPrivileged) {
    return JS_TRUE;
  }

  
  
  XPCWrappedNative *wn = XPCNativeWrapper::SafeGetWrappedNative(obj);
  if (wn) {
    nsIPrincipal *objectPrincipal = wn->GetScope()->GetPrincipal();
    PRBool subsumes;
    if (NS_FAILED(subjectPrincipal->Subsumes(objectPrincipal, &subsumes)) ||
        !subsumes) {

      JSObject* flatObj;
      if (!JSVAL_IS_VOID(id) &&
          (accessType & (XPCWrapper::sSecMgrSetProp |
                         XPCWrapper::sSecMgrGetProp)) &&
          (flatObj = wn->GetFlatJSObject())) {
        rv = ssm->CheckPropertyAccess(cx, flatObj,
                                      STOBJ_GET_CLASS(flatObj)->name,
                                      id, accessType);
        return NS_SUCCEEDED(rv);
      }

      return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
    }
  }

  
  
  
  jsval flags;

  ::JS_GetReservedSlot(cx, obj, 0, &flags);
  if (HAS_FLAGS(flags, FLAG_EXPLICIT)) {
    
    return JS_TRUE;
  }

  JSScript *script = JS_GetFrameScript(cx, fp);
  uint32 fileFlags = JS_GetScriptFilenameFlags(script);
  if (fileFlags == JSFILENAME_NULL || (fileFlags & JSFILENAME_SYSTEM)) {
    
    return JS_TRUE;
  }

  
  
  return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
}


JSBool
XPCNativeWrapper::GetWrappedNative(JSContext *cx, JSObject *obj,
                                   XPCWrappedNative **aWrappedNative)
{
  XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));
  *aWrappedNative = wn;
  if (!wn) {
    return JS_TRUE;
  }

  nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
  if (!ssm) {
    return JS_TRUE;
  }

  JSStackFrame *fp;
  nsIPrincipal *subjectPrincipal = ssm->GetCxSubjectPrincipalAndFrame(cx, &fp);
  if (!subjectPrincipal) {
    return JS_TRUE;
  }

  if (fp) {
    void *annotation = JS_GetFrameAnnotation(cx, fp);

    PRBool isPrivileged;
    nsresult rv =
      subjectPrincipal->IsCapabilityEnabled("UniversalXPConnect",
                                            annotation,
                                            &isPrivileged);
    if (NS_SUCCEEDED(rv) && isPrivileged) {
      return JS_TRUE;
    }
  }

  XPCWrappedNativeScope *scope = wn->GetScope();
  nsIPrincipal *objectPrincipal = scope->GetPrincipal();

  PRBool subsumes;
  nsresult rv = subjectPrincipal->Subsumes(objectPrincipal, &subsumes);
  if (NS_FAILED(rv) || !subsumes) {
    return JS_FALSE;
  }

  return JS_TRUE;
}

JSBool
XPC_NW_WrapFunction(JSContext* cx, JSObject* funobj, jsval *rval)
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

  JS_SetReservedSlot(cx, funWrapperObj, XPCWrapper::eAllAccessSlot, JSVAL_FALSE);

  return JS_TRUE;
}

static JSBool
XPC_NW_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  jsid idAsId;
  JSPropertyDescriptor desc;

  if (!JS_ValueToId(cx, id, &idAsId) ||
      !JS_GetPropertyDescriptorById(cx, obj, idAsId, JSRESOLVE_QUALIFIED,
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

  
  
  return EnsureLegalActivity(cx, obj, id, XPCWrapper::sSecMgrSetProp) &&
         XPC_NW_RewrapIfDeepWrapper(cx, obj, *vp, vp);
}

static JSBool
XPC_NW_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  if (!EnsureLegalActivity(cx, obj)) {
    return JS_FALSE;
  }

  XPC_NW_BYPASS_BASE(cx, obj,
    
    
    
    {
      jsid interned_id;

      if (!JS_ValueToId(cx, id, &interned_id)) {
        return JS_FALSE;
      }

      return JS_DeletePropertyById(cx, obj, interned_id);
    }
  );

  return JS_TRUE;
}

JSBool
XPC_NW_RewrapIfDeepWrapper(JSContext *cx, JSObject *obj, jsval v, jsval *rval)
{
  NS_ASSERTION(XPCNativeWrapper::IsNativeWrapper(obj),
               "Unexpected object");

  JSBool primitive = JSVAL_IS_PRIMITIVE(v);
  JSObject* nativeObj = primitive ? nsnull : JSVAL_TO_OBJECT(v);
  
  
  if (!primitive && JS_ObjectIsFunction(cx, nativeObj)) {
    return XPC_NW_WrapFunction(cx, nativeObj, rval);
  }

  jsval flags;
  ::JS_GetReservedSlot(cx, obj, 0, &flags);

  
  
  if (HAS_FLAGS(flags, FLAG_DEEP) && !primitive) {
    
    if (STOBJ_GET_CLASS(nativeObj) == &sXPC_XOW_JSClass.base) {
      if (!::JS_GetReservedSlot(cx, nativeObj, XPCWrapper::sWrappedObjSlot,
                                &v)) {
        return JS_FALSE;
      }

      
      
      if (!JSVAL_IS_PRIMITIVE(v)) {
        nativeObj = JSVAL_TO_OBJECT(v);
      }
    }

    XPCWrappedNative* wrappedNative =
      XPCWrappedNative::GetAndMorphWrappedNativeOfJSObject(cx, nativeObj);
    if (!wrappedNative)
      return XPC_SJOW_Construct(cx, nsnull, 1, &v, rval);

    if (HAS_FLAGS(flags, FLAG_EXPLICIT)) {
#ifdef DEBUG_XPCNativeWrapper
      printf("Rewrapping for deep explicit wrapper\n");
#endif
      if (wrappedNative == XPCNativeWrapper::SafeGetWrappedNative(obj)) {
        
        *rval = OBJECT_TO_JSVAL(obj);
        return JS_TRUE;
      }

      
      

      return XPCNativeWrapper::CreateExplicitWrapper(cx, wrappedNative,
                                                     JS_TRUE, rval);
    }

#ifdef DEBUG_XPCNativeWrapper
    printf("Rewrapping for deep implicit wrapper\n");
#endif
    
    
    
    JSObject* wrapperObj = XPCNativeWrapper::GetNewOrUsed(cx, wrappedNative,
                                                          nsnull);
    if (!wrapperObj) {
      return JS_FALSE;
    }

    *rval = OBJECT_TO_JSVAL(wrapperObj);
  } else {
    *rval = v;
  }

  return JS_TRUE;
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
    obj = STOBJ_GET_PROTO(obj);
  }

  if (!obj) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  
  
  JSObject *methodToCallObj = STOBJ_GET_PARENT(funObj);
  XPCWrappedNative* wrappedNative = nsnull;

  jsval isAllAccess;
  if (::JS_GetReservedSlot(cx, funObj,
                           XPCWrapper::eAllAccessSlot,
                           &isAllAccess) &&
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

  return XPC_NW_RewrapIfDeepWrapper(cx, obj, v, rval);
}

static JSBool
XPC_NW_GetOrSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp,
                        JSBool aIsSet)
{
  
  if (id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_PROTOTYPE) ||
      id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    return JS_TRUE;
  }

  while (!XPCNativeWrapper::IsNativeWrapper(obj)) {
    obj = STOBJ_GET_PROTO(obj);
    if (!obj) {
      return ThrowException(NS_ERROR_UNEXPECTED, cx);
    }
  }

  if (!EnsureLegalActivity(cx, obj, id,
                           aIsSet ? XPCWrapper::sSecMgrSetProp
                                  : XPCWrapper::sSecMgrGetProp)) {
    return JS_FALSE;
  }

  
  XPCWrappedNative *wrappedNative = XPCNativeWrapper::SafeGetWrappedNative(obj);

  if (!wrappedNative) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  JSObject *nativeObj = wrappedNative->GetFlatJSObject();

  
  
  
  

  if (ShouldBypassNativeWrapper(cx, obj)) {
    jsid interned_id;

    if (!::JS_ValueToId(cx, id, &interned_id)) {
      return JS_FALSE;
    }

    return aIsSet
           ? JS_SetPropertyById(cx, nativeObj, interned_id, vp)
           : JS_GetPropertyById(cx, nativeObj, interned_id, vp);
  }

  if (!aIsSet &&
      id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_WRAPPED_JSOBJECT)) {
    
    
    

    jsval nativeVal = OBJECT_TO_JSVAL(nativeObj);

    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    nsCOMPtr<nsIPrincipal> prin;
    nsresult rv = ssm->GetObjectPrincipal(cx, nativeObj,
                                          getter_AddRefs(prin));
    if (NS_FAILED(rv)) {
      return ThrowException(rv, cx);
    }

    PRBool isSystem;
    if (NS_SUCCEEDED(ssm->IsSystemPrincipal(prin, &isSystem)) && isSystem) {
      *vp = nativeVal;
      return JS_TRUE;
    }

    return XPC_SJOW_Construct(cx, nsnull, 1, &nativeVal, vp);
  }

  return XPCWrapper::GetOrSetNativeProperty(cx, obj, wrappedNative, id, vp,
                                            aIsSet, JS_TRUE);
}

static JSBool
XPC_NW_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  return XPC_NW_GetOrSetProperty(cx, obj, id, vp, PR_FALSE);
}

static JSBool
XPC_NW_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
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

  return XPCWrapper::Enumerate(cx, obj, wn->GetFlatJSObject());
}

static JSBool
XPC_NW_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                  JSObject **objp)
{
  
  
  
  
  if (id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_WRAPPED_JSOBJECT)) {
    return JS_TRUE;
  }

  if (id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    *objp = obj;

    
    
    JSFunction *fun = JS_NewFunction(cx, XPC_NW_toString, 0, 0, nsnull,
                                     "toString");
    if (!fun) {
      return JS_FALSE;
    }

    JSObject *funobj = JS_GetFunctionObject(fun);
    STOBJ_SET_PARENT(funobj, obj);

    return JS_DefineProperty(cx, obj, "toString", OBJECT_TO_JSVAL(funobj),
                             nsnull, nsnull, 0);
  }

  PRUint32 accessType =
    (flags & JSRESOLVE_ASSIGNING) ? XPCWrapper::sSecMgrSetProp
                                  : XPCWrapper::sSecMgrGetProp;
  if (!EnsureLegalActivity(cx, obj, id, accessType)) {
    return JS_FALSE;
  }

  
  
  
  
  

  if (ShouldBypassNativeWrapper(cx, obj)) {
    
    XPCWrappedNative *wn = XPCNativeWrapper::SafeGetWrappedNative(obj);
    if (!wn) {
      return JS_TRUE;
    }

    JSAutoRequest ar(cx);

    jsid interned_id;
    JSObject *pobj;
    jsval val;
    if (!JS_ValueToId(cx, id, &interned_id) ||
        !JS_LookupPropertyWithFlagsById(cx, wn->GetFlatJSObject(), interned_id,
                                        JSRESOLVE_QUALIFIED, &pobj, &val)) {
      return JS_FALSE;
    }

    if (pobj) {
      if (!JS_DefinePropertyById(cx, obj, interned_id, JSVAL_VOID, nsnull,
                                 nsnull, 0)) {
        return JS_FALSE;
      }

      *objp = obj;
    }
    return JS_TRUE;
  }

  while (!XPCNativeWrapper::IsNativeWrapper(obj)) {
    obj = STOBJ_GET_PROTO(obj);
    if (!obj) {
      return ThrowException(NS_ERROR_UNEXPECTED, cx);
    }
  }

  
  XPCWrappedNative *wrappedNative = XPCNativeWrapper::SafeGetWrappedNative(obj);

  if (!wrappedNative) {
    

    return JS_TRUE;
  }

  return XPCWrapper::ResolveNativeProperty(cx, obj,
                                           wrappedNative->GetFlatJSObject(),
                                           wrappedNative, id, flags, objp,
                                           JS_TRUE);
}

static JSBool
XPC_NW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
  if (!EnsureLegalActivity(cx, obj)) {
    return JS_FALSE;
  }

  XPC_NW_BYPASS(cx, obj, convert, (cx, obj, type, vp));
  return JS_TRUE;
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
XPC_NW_CheckAccess(JSContext *cx, JSObject *obj, jsval id,
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

  JSClass *clazz = STOBJ_GET_CLASS(wrapperJSObject);
  return !clazz->checkAccess ||
    clazz->checkAccess(cx, wrapperJSObject, id, mode, vp);
}

static JSBool
XPC_NW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  if (!XPCNativeWrapper::IsNativeWrapper(obj)) {
    
    
    

#ifdef DEBUG
    if (!JS_ObjectIsFunction(cx, obj)) {
      NS_WARNING("Ignoring a call for a weird object");
    }
#endif
    return JS_TRUE;
  }

  XPC_NW_BYPASS_TEST(cx, obj, call, (cx, obj, argc, argv, rval));

  return JS_TRUE;
}

static JSBool
XPC_NW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval)
{
  
  
  
  
  obj = JSVAL_TO_OBJECT(argv[-2]);

  XPC_NW_BYPASS_TEST(cx, obj, construct, (cx, obj, argc, argv, rval));

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

  return XPC_NW_RewrapIfDeepWrapper(cx, obj, *rval, rval);
}

static JSBool
XPC_NW_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
  XPC_NW_BYPASS_TEST(cx, obj, hasInstance, (cx, obj, v, bp));

  return JS_TRUE;
}

static JSBool
MirrorWrappedNativeParent(JSContext *cx, XPCWrappedNative *wrapper,
                          JSObject **result NS_OUTPARAM)
{
  JSObject *wn_parent = STOBJ_GET_PARENT(wrapper->GetFlatJSObject());
  if (!wn_parent) {
    *result = nsnull;
  } else {
    XPCWrappedNative *parent_wrapper =
      XPCWrappedNative::GetAndMorphWrappedNativeOfJSObject(cx, wn_parent);

    
    
    
    if (parent_wrapper) {
      *result = XPCNativeWrapper::GetNewOrUsed(cx, parent_wrapper, nsnull);
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

    JSObject *proto = STOBJ_GET_PROTO(obj);
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

  
  if (STOBJ_GET_CLASS(nativeObj) == &sXPC_XOW_JSClass.base) {
    jsval v;
    if (!::JS_GetReservedSlot(cx, nativeObj, XPCWrapper::sWrappedObjSlot, &v)) {
      return JS_FALSE;
    }
    
    
    if (!JSVAL_IS_PRIMITIVE(v)) {
      nativeObj = JSVAL_TO_OBJECT(v);
    }
  } else if (STOBJ_GET_CLASS(nativeObj) == &sXPC_SJOW_JSClass.base) {
    
    nativeObj = JS_GetParent(cx, nativeObj);
    if (!nativeObj) {
      return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
    }
  }

  XPCWrappedNative *wrappedNative;

  if (XPCNativeWrapper::IsNativeWrapper(nativeObj)) {
    
    

#ifdef DEBUG_XPCNativeWrapper
    printf("Wrapping already wrapped object\n");
#endif

    
    wrappedNative = XPCNativeWrapper::SafeGetWrappedNative(nativeObj);

    if (!wrappedNative) {
      return ThrowException(NS_ERROR_INVALID_ARG, cx);
    }

    nativeObj = wrappedNative->GetFlatJSObject();
    native = OBJECT_TO_JSVAL(nativeObj);
  } else {
    wrappedNative =
      XPCWrappedNative::GetAndMorphWrappedNativeOfJSObject(cx, nativeObj);

    if (!wrappedNative) {
      return ThrowException(NS_ERROR_INVALID_ARG, cx);
    }

    
    
    nsCOMPtr<nsIXPConnectWrappedJS> xpcwrappedjs =
      do_QueryWrappedNative(wrappedNative);

    if (xpcwrappedjs) {
      return ThrowException(NS_ERROR_INVALID_ARG, cx);
    }
  }

  PRBool hasStringArgs = PR_FALSE;
  for (uintN i = 1; i < argc; ++i) {
    if (!JSVAL_IS_STRING(argv[i])) {
      hasStringArgs = PR_FALSE;

      break;
    }

    if (i == 1) {
#ifdef DEBUG_XPCNativeWrapper
      printf("Constructing XPCNativeWrapper() with string args\n");
#endif
    }

#ifdef DEBUG_XPCNativeWrapper
    printf("  %s\n", ::JS_GetStringBytes(JSVAL_TO_STRING(argv[i])));
#endif

    hasStringArgs = PR_TRUE;
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

  return XPCNativeWrapper::CreateExplicitWrapper(cx, wrappedNative,
                                                 !hasStringArgs, rval);
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
XPC_NW_Equality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
  NS_ASSERTION(XPCNativeWrapper::IsNativeWrapper(obj),
               "Uh, we should only ever be called for XPCNativeWrapper "
               "objects!");

  if (!EnsureLegalActivity(cx, obj)) {
    return JS_FALSE;
  }

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
    JS_NewObjectWithGivenProto(cx, XPCNativeWrapper::GetJSClass(), nsnull,
                               STOBJ_GET_PARENT(obj));
  if (!wrapperIter) {
    return nsnull;
  }

  JSAutoTempValueRooter tvr(cx, OBJECT_TO_JSVAL(wrapperIter));

  
  XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(JS_GetPrivate(cx, obj));
  JS_SetPrivate(cx, wrapperIter, wn);
  if (!JS_SetReservedSlot(cx, wrapperIter, 0,
                          INT_TO_JSVAL(FLAG_DEEP | FLAG_EXPLICIT))) {
    return nsnull;
  }

  return XPCWrapper::CreateIteratorObj(cx, wrapperIter, obj,
                                       wn->GetFlatJSObject(), keysonly);
}

static JSBool
XPC_NW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
  while (!XPCNativeWrapper::IsNativeWrapper(obj)) {
    obj = STOBJ_GET_PROTO(obj);
    if (!obj) {
      return ThrowException(NS_ERROR_UNEXPECTED, cx);
    }
  }

  if (!EnsureLegalActivity(cx, obj,
                           GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING),
                           XPCWrapper::sSecMgrGetProp)) {
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

  return XPCWrapper::NativeToString(cx, wrappedNative, argc, argv, rval,
                                    JS_TRUE);
}


PRBool
XPCNativeWrapper::AttachNewConstructorObject(XPCCallContext &ccx,
                                             JSObject *aGlobalObject)
{
  JSObject *class_obj =
    ::JS_InitClass(ccx, aGlobalObject, nsnull, &sXPC_NW_JSClass.base,
                   XPCNativeWrapperCtor, 0, nsnull, nsnull,
                   nsnull, nsnull);
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
                                    sXPC_NW_JSClass.base.name,
                                    JSPROP_READONLY | JSPROP_PERMANENT,
                                    &found);
}


JSObject *
XPCNativeWrapper::GetNewOrUsed(JSContext *cx, XPCWrappedNative *wrapper,
                               nsIPrincipal *aObjectPrincipal)
{
  if (aObjectPrincipal) {
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();

    PRBool isSystem;
    nsresult rv = ssm->IsSystemPrincipal(aObjectPrincipal, &isSystem);
    if (NS_SUCCEEDED(rv) && !isSystem) {
      jsval v = OBJECT_TO_JSVAL(wrapper->GetFlatJSObject());
      if (!CreateExplicitWrapper(cx, wrapper, true, &v)) {
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

    if (XPC_SJOW_Construct(cx, nsnull, 1, &v, &v))
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

  obj = ::JS_NewObjectWithGivenProto(cx, GetJSClass(), nsnull, nw_parent);

  if (lock) {
    ::JS_UnlockGCThing(cx, nw_parent);
  }

  if (!obj ||
      !::JS_SetPrivate(cx, obj, wrapper) ||
      !::JS_SetReservedSlot(cx, obj, 0, INT_TO_JSVAL(FLAG_DEEP))) {
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
                                        JSBool deep,
                                        jsval *rval)
{
#ifdef DEBUG_XPCNativeWrapper
  printf("Creating new JSObject\n");
#endif

  JSObject *wrapperObj =
    JS_NewObjectWithGivenProto(cx, XPCNativeWrapper::GetJSClass(), nsnull,
                               wrappedNative->GetScope()->GetGlobalJSObject());

  if (!wrapperObj) {
    
    return JS_FALSE;
  }

  jsuint flags = deep ? FLAG_DEEP | FLAG_EXPLICIT : FLAG_EXPLICIT;
  if (!::JS_SetReservedSlot(cx, wrapperObj, 0, INT_TO_JSVAL(flags))) {
    return JS_FALSE;
  }

  JSObject *parent = nsnull;

  if (deep) {
    
    
    JS_LockGCThing(cx, wrapperObj);

    
    
    if (!MirrorWrappedNativeParent(cx, wrappedNative, &parent))
      return JS_FALSE;

    JS_UnlockGCThing(cx, wrapperObj);
  }

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
