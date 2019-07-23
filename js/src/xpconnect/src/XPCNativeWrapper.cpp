







































#include "xpcprivate.h"
#include "XPCNativeWrapper.h"
#include "jsdbgapi.h"

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_Enumerate(JSContext *cx, JSObject *obj);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                  JSObject **objp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

JS_STATIC_DLL_CALLBACK(void)
XPC_NW_Finalize(JSContext *cx, JSObject *obj);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_CheckAccess(JSContext *cx, JSObject *obj, jsval id,
                   JSAccessMode mode, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
            jsval *rval);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

JS_STATIC_DLL_CALLBACK(uint32)
XPC_NW_Mark(JSContext *cx, JSObject *obj, void *arg);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_Equality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

static JSBool
RewrapIfDeepWrapper(JSContext *cx, JSObject *obj, jsval v, jsval *rval);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_FunctionWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                       jsval *rval);




JSExtendedClass XPCNativeWrapper::sXPC_NW_JSClass = {
  
  { "XPCNativeWrapper",
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS |
    
    JSCLASS_NEW_RESOLVE | JSCLASS_HAS_RESERVED_SLOTS(1) |
    JSCLASS_IS_EXTENDED,
    XPC_NW_AddProperty, XPC_NW_DelProperty,
    XPC_NW_GetProperty, XPC_NW_SetProperty,
    XPC_NW_Enumerate,   (JSResolveOp)XPC_NW_NewResolve,
    XPC_NW_Convert,     XPC_NW_Finalize,
    nsnull,             XPC_NW_CheckAccess,
    XPC_NW_Call,        XPC_NW_Construct,
    nsnull,             XPC_NW_HasInstance,
    XPC_NW_Mark,        nsnull
  },
  
  XPC_NW_Equality
};

#define FLAG_DEEP     0x1
#define FLAG_EXPLICIT 0x2


#define FLAG_RESOLVING 0x4

#define HAS_FLAGS(_val, _flags) \
  ((PRUint32(JSVAL_TO_INT(_val)) & (_flags)) != 0)

#define NATIVE_HAS_FLAG(_wn, _flag)                \
  ((_wn)->GetScriptableInfo() &&                   \
   (_wn)->GetScriptableInfo()->GetFlags()._flag())










#define XPC_NW_CALL_HOOK(cx, obj, hook, args)                                 \
  return JS_GET_CLASS(cx, obj)->hook args;

#define XPC_NW_CAST_HOOK(cx, obj, type, hook, args)                           \
  return ((type) JS_GET_CLASS(cx, obj)->hook) args;

static JSBool
ShouldBypassNativeWrapper(JSContext *cx, JSObject *obj)
{
  NS_ASSERTION(XPCNativeWrapper::IsNativeWrapper(cx, obj),
               "Unexpected object");
  jsval flags;

  ::JS_GetReservedSlot(cx, obj, 0, &flags);
  if (HAS_FLAGS(flags, FLAG_EXPLICIT))
    return JS_FALSE;

  
  JSScript *script = nsnull;
  JSStackFrame *fp = cx->fp;
  while(!script && fp) {
    script = fp->script;
    fp = fp->down;
  }

  
  
  return !script || !(::JS_GetScriptFilenameFlags(script) & JSFILENAME_SYSTEM);
}

#define XPC_NW_BYPASS_BASE(cx, obj, code)                                     \
  JS_BEGIN_MACRO                                                              \
    if (ShouldBypassNativeWrapper(cx, obj)) {                                 \
      XPCWrappedNative *wn_ = XPCNativeWrapper::GetWrappedNative(cx, obj);    \
      if (!wn_) {                                                             \
        return JS_TRUE;                                                       \
      }                                                                       \
      obj = wn_->GetFlatJSObject();                                           \
      code                                                                    \
    }                                                                         \
  JS_END_MACRO

#define XPC_NW_BYPASS(cx, obj, hook, args)                                    \
  XPC_NW_BYPASS_BASE(cx, obj, XPC_NW_CALL_HOOK(cx, obj, hook, args))

#define XPC_NW_BYPASS_CAST(cx, obj, type, hook, args)                         \
  XPC_NW_BYPASS_BASE(cx, obj, XPC_NW_CAST_HOOK(cx, obj, type, hook, args))

#define XPC_NW_BYPASS_TEST(cx, obj, hook, args)                               \
  XPC_NW_BYPASS_BASE(cx, obj,                                                 \
    JSClass *clasp_ = JS_GET_CLASS(cx, obj);                                  \
    return !clasp_->hook || clasp_->hook args;                                \
  )

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval);

static JSFunctionSpec sXPC_NW_JSClass_methods[] = {
  {"toString", XPC_NW_toString, 0, 0, 0},
  {0, 0, 0, 0, 0}
};

JS_STATIC_DLL_CALLBACK(JSBool)
XPCNativeWrapperCtor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                     jsval *rval);

static inline
JSBool
ThrowException(nsresult ex, JSContext *cx)
{
  XPCThrower::Throw(ex, cx);

  return JS_FALSE;
}

static JSBool
WrapFunction(JSContext* cx, JSObject* funobj, jsval *rval)
{
  
  if (JS_GetFunctionNative(cx,
                           JS_ValueToFunction(cx, OBJECT_TO_JSVAL(funobj))) ==
      XPC_NW_FunctionWrapper) {
    *rval = OBJECT_TO_JSVAL(funobj);
    return JS_TRUE;
  }
  
  
  
  
  JSFunction *funWrapper =
    ::JS_NewFunction(cx, XPC_NW_FunctionWrapper, 0, 0, funobj,
                     "XPCNativeWrapper function wrapper");
  if (!funWrapper) {
    return JS_FALSE;
  }

  *rval = OBJECT_TO_JSVAL(::JS_GetFunctionObject(funWrapper));
  return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  jsval flags;
  ::JS_GetReservedSlot(cx, obj, 0, &flags);
  if (!HAS_FLAGS(flags, FLAG_RESOLVING)) {
    return JS_TRUE;
  }

  
  
  return RewrapIfDeepWrapper(cx, obj, *vp, vp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  XPC_NW_BYPASS_BASE(cx, obj,
    
    
    
    {
      jsid interned_id;

      if (!::JS_ValueToId(cx, id, &interned_id)) {
        return JS_FALSE;
      }

      return OBJ_DELETE_PROPERTY(cx, obj, interned_id, vp);
    }
  );

  return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
}

static JSBool
RewrapIfDeepWrapper(JSContext *cx, JSObject *obj, jsval v, jsval *rval)
{
  NS_ASSERTION(XPCNativeWrapper::IsNativeWrapper(cx, obj),
               "Unexpected object");

  JSBool primitive = JSVAL_IS_PRIMITIVE(v);
  JSObject* nativeObj = primitive ? nsnull : JSVAL_TO_OBJECT(v);
  
  
  if (!primitive && JS_ObjectIsFunction(cx, nativeObj)) {
    return WrapFunction(cx, nativeObj, rval);
  }

  jsval flags;
  ::JS_GetReservedSlot(cx, obj, 0, &flags);

  
  
  if (HAS_FLAGS(flags, FLAG_DEEP) && !primitive) {

    XPCWrappedNative* wrappedNative =
      XPCWrappedNative::GetWrappedNativeOfJSObject(cx, nativeObj);
    if (!wrappedNative) {
      
      *rval = JSVAL_NULL;
      return JS_TRUE;
    }

    if (HAS_FLAGS(flags, FLAG_EXPLICIT)) {
#ifdef DEBUG_XPCNativeWrapper
      printf("Rewrapping for deep explicit wrapper\n");
#endif
      if (wrappedNative == XPCNativeWrapper::GetWrappedNative(cx, obj)) {
        
        *rval = OBJECT_TO_JSVAL(obj);
        return JS_TRUE;
      }

      
      
      
      

      return XPCNativeWrapperCtor(cx, nsnull, 1, &v, rval);
    }
    
#ifdef DEBUG_XPCNativeWrapper
    printf("Rewrapping for deep implicit wrapper\n");
#endif
    
    
    
    JSObject* wrapperObj = XPCNativeWrapper::GetNewOrUsed(cx, wrappedNative);
    if (!wrapperObj) {
      return JS_FALSE;
    }

    *rval = OBJECT_TO_JSVAL(wrapperObj);
  } else {
    *rval = v;
  }

  return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_FunctionWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                       jsval *rval)
{
  JSObject *funObj = JSVAL_TO_OBJECT(argv[-2]);
  if (!::JS_ObjectIsFunction(cx, funObj)) {
    obj = nsnull;
  }

  while (obj && !XPCNativeWrapper::IsNativeWrapper(cx, obj)) {
    obj = ::JS_GetPrototype(cx, obj);
  }

  if (!obj) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  
  
  JSObject *methodToCallObj = ::JS_GetParent(cx, funObj);
  XPCWrappedNative *wrappedNative =
    XPCNativeWrapper::GetWrappedNative(cx, obj);

  if (!::JS_ObjectIsFunction(cx, methodToCallObj) || !wrappedNative) {
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

  return RewrapIfDeepWrapper(cx, obj, v, rval);
}

static JSBool
XPC_NW_GetOrSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp,
                        JSBool aIsSet)
{
  
  if (id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_PROTOTYPE) ||
      id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    return JS_TRUE;
  }

  while (!XPCNativeWrapper::IsNativeWrapper(cx, obj)) {
    obj = ::JS_GetPrototype(cx, obj);
    if (!obj) {
      return ThrowException(NS_ERROR_UNEXPECTED, cx);
    }
  }

  XPCWrappedNative *wrappedNative =
    XPCNativeWrapper::GetWrappedNative(cx, obj);

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
           ? OBJ_SET_PROPERTY(cx, nativeObj, interned_id, vp)
           : OBJ_GET_PROPERTY(cx, nativeObj, interned_id, vp);
  }

  if (!aIsSet &&
      id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_WRAPPED_JSOBJECT)) {
    
    
    

    jsval nativeVal = OBJECT_TO_JSVAL(nativeObj);
    return XPC_SJOW_Construct(cx, nsnull, 1, &nativeVal, vp);
  }

  
  XPCCallContext ccx(JS_CALLER, cx, nativeObj, nsnull, id);

  if (aIsSet ? NATIVE_HAS_FLAG(wrappedNative, WantSetProperty) :
               NATIVE_HAS_FLAG(wrappedNative, WantGetProperty)) {

    jsval v = *vp;
    
    
    JSBool retval = JS_TRUE;
    nsresult rv;
    if (aIsSet) {
      rv = wrappedNative->GetScriptableCallback()->
        SetProperty(wrappedNative, cx, obj, id, &v, &retval);
    } else {
      rv = wrappedNative->GetScriptableCallback()->
        GetProperty(wrappedNative, cx, obj, id, &v, &retval);
    }
    
    if (NS_FAILED(rv)) {
      return ThrowException(rv, cx);
    }
    if (!retval) {
      return JS_FALSE;
    }

    if (rv == NS_SUCCESS_I_DID_SOMETHING) {
      
      AUTO_MARK_JSVAL(ccx, v);

#ifdef DEBUG_XPCNativeWrapper
      JSString* strId = ::JS_ValueToString(cx, id);
      if (strId) {
        NS_ConvertUTF16toUTF8 propName((PRUnichar*)::JS_GetStringChars(strId),
                                       ::JS_GetStringLength(strId));
        printf("%s via scriptable hooks for '%s'\n",
               aIsSet ? "Set" : "Got", propName.get());
      }
#endif

      return RewrapIfDeepWrapper(cx, obj, v, vp);
    }
  }
  
  if (!JSVAL_IS_STRING(id)) {
    
    return JS_TRUE;
  }

  
  XPCWrappedNative* wrapper = ccx.GetWrapper();
  if (wrapper != wrappedNative || !wrapper->IsValid()) {
    NS_ASSERTION(wrapper == wrappedNative, "Uh, how did this happen!");
    return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
  }

  
  
  XPCNativeInterface* iface = ccx.GetInterface();
  if (!iface) {

    return JS_TRUE;
  }

  
  XPCNativeMember* member = ccx.GetMember();
  NS_ASSERTION(member, "not doing IDispatch, how'd this happen?");
  if (!member) {
    

    return JS_TRUE;
  }

  
  
  jsval memberval;
  if (!member->GetValue(ccx, iface, &memberval)) {
    return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
  }

  if (member->IsConstant()) {
    
    
    if (aIsSet) {
      return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
    }

    *vp = memberval;

    return JS_TRUE;
  }

  if (!member->IsAttribute()) {
    
    

    return JS_TRUE;
  }

  
  
  AUTO_MARK_JSVAL(ccx, memberval);

  
  JSObject* funobj = xpc_CloneJSFunction(ccx, JSVAL_TO_OBJECT(memberval),
                                         wrapper->GetFlatJSObject());
  if (!funobj) {
    return JS_FALSE;
  }

  jsval *argv = nsnull;
  uintN argc = 0;

  if (aIsSet) {
    if (member->IsReadOnlyAttribute()) {
      
      return ThrowException(NS_ERROR_NOT_AVAILABLE, cx);
    }

#ifdef DEBUG_XPCNativeWrapper
    printf("Calling setter for %s\n",
           ::JS_GetStringBytes(JSVAL_TO_STRING(id)));
#endif

    argv = vp;
    argc = 1;
  } else {
#ifdef DEBUG_XPCNativeWrapper
    printf("Calling getter for %s\n",
           ::JS_GetStringBytes(JSVAL_TO_STRING(id)));
#endif
  }

  
  jsval v;
  if (!::JS_CallFunctionValue(cx, wrapper->GetFlatJSObject(),
                              OBJECT_TO_JSVAL(funobj), argc, argv, &v)) {
    return JS_FALSE;
  }

  if (aIsSet) {
    return JS_TRUE;
  }

  {
    
    AUTO_MARK_JSVAL(ccx, v);

    return RewrapIfDeepWrapper(cx, obj, v, vp);
  }
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  return XPC_NW_GetOrSetProperty(cx, obj, id, vp, PR_FALSE);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  return XPC_NW_GetOrSetProperty(cx, obj, id, vp, PR_TRUE);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_Enumerate(JSContext *cx, JSObject *obj)
{
  
  
  
  
  

  XPCWrappedNative *wn = XPCNativeWrapper::GetWrappedNative(cx, obj);
  if (!wn) {
    return JS_TRUE;
  }

  JSIdArray *ida = JS_Enumerate(cx, wn->GetFlatJSObject());
  if (!ida) {
    return JS_FALSE;
  }

  JSBool ok = JS_TRUE;

  for (jsint i = 0, n = ida->length; i < n; i++) {
    JSObject *pobj;
    JSProperty *prop;

    
    
    ok = OBJ_LOOKUP_PROPERTY(cx, obj, ida->vector[i], &pobj, &prop);
    if (!ok) {
      break;
    }
    if (prop) {
      OBJ_DROP_PROPERTY(cx, pobj, prop);
    }
  }

  JS_DestroyIdArray(cx, ida);
  return ok;
}

static
JSBool MaybePreserveWrapper(JSContext* cx, XPCWrappedNative *wn, uintN flags)
{
  if ((flags & JSRESOLVE_ASSIGNING) &&
      (::JS_GetOptions(cx) & JSOPTION_PRIVATE_IS_NSISUPPORTS)) {
    nsCOMPtr<nsIXPCScriptNotify> scriptNotify = 
      do_QueryInterface(NS_STATIC_CAST(nsISupports*,
                                       JS_GetContextPrivate(cx)));
    if (scriptNotify) {
      return NS_SUCCEEDED(scriptNotify->PreserveWrapper(wn));
    }
  }
  return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                  JSObject **objp)
{
  
  
  
  
  if (id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_WRAPPED_JSOBJECT) ||
      id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    return JS_TRUE;
  }

  
  
  
  
  

  if (ShouldBypassNativeWrapper(cx, obj)) {
    XPCWrappedNative *wn = XPCNativeWrapper::GetWrappedNative(cx, obj);
    if (!wn) {
      return JS_TRUE;
    }

    JSAutoRequest ar(cx);

    jsid interned_id;
    JSObject *pobj;
    JSProperty *prop;

    if (!::JS_ValueToId(cx, id, &interned_id) ||
        !OBJ_LOOKUP_PROPERTY(cx, wn->GetFlatJSObject(), interned_id,
                             &pobj, &prop)) {
      return JS_FALSE;
    }

    if (prop) {
      OBJ_DROP_PROPERTY(cx, pobj, prop);

      if (!OBJ_DEFINE_PROPERTY(cx, obj, interned_id, JSVAL_VOID,
                               nsnull, nsnull, 0, nsnull)) {
        return JS_FALSE;
      }

      *objp = obj;
    }
    return JS_TRUE;
  }

  while (!XPCNativeWrapper::IsNativeWrapper(cx, obj)) {
    obj = ::JS_GetPrototype(cx, obj);
    if (!obj) {
      return ThrowException(NS_ERROR_UNEXPECTED, cx);
    }
  }

  XPCWrappedNative *wrappedNative =
    XPCNativeWrapper::GetWrappedNative(cx, obj);

  if (!wrappedNative) {
    

    return JS_TRUE;
  }

  JSObject *nativeObj = wrappedNative->GetFlatJSObject();

  
  XPCCallContext ccx(JS_CALLER, cx, nativeObj, nsnull, id);

  
  
  if (NATIVE_HAS_FLAG(wrappedNative, WantNewResolve) &&
      id != GetRTStringByIndex(cx, XPCJSRuntime::IDX_CONSTRUCTOR)) {

    
    
    jsval oldFlags;
    ::JS_GetReservedSlot(cx, obj, 0, &oldFlags);
    if (!::JS_SetReservedSlot(cx, obj, 0,
                              INT_TO_JSVAL(JSVAL_TO_INT(oldFlags) |
                                           FLAG_RESOLVING))) {
      return JS_FALSE;
    }        
    
    XPCWrappedNative* oldResolvingWrapper = nsnull;
    JSBool allowPropMods =
      NATIVE_HAS_FLAG(wrappedNative, AllowPropModsDuringResolve);
    if (allowPropMods) {
      oldResolvingWrapper = ccx.SetResolvingWrapper(wrappedNative);
    }
      
    JSBool retval = JS_TRUE;
    JSObject* newObj = nsnull;
    nsresult rv = wrappedNative->GetScriptableInfo()->
      GetCallback()->NewResolve(wrappedNative, cx, obj, id, flags,
                                &newObj, &retval);

    if (allowPropMods) {
      ccx.SetResolvingWrapper(oldResolvingWrapper);
    }

    if (!::JS_SetReservedSlot(cx, obj, 0, oldFlags)) {
      return JS_FALSE;
    }
    
    if (NS_FAILED(rv)) {
      return ThrowException(rv, cx);
    }

    if (newObj) {
#ifdef DEBUG_XPCNativeWrapper
      JSString* strId = ::JS_ValueToString(cx, id);
      if (strId) {
        NS_ConvertUTF16toUTF8 propName((PRUnichar*)::JS_GetStringChars(strId),
                                       ::JS_GetStringLength(strId));
        printf("Resolved via scriptable hooks for '%s'\n", propName.get());
      }
#endif
      
      
      *objp = newObj;
      return retval;
    }      
  }

  if (!JSVAL_IS_STRING(id)) {
    
    

    return MaybePreserveWrapper(cx, wrappedNative, flags);
  }

  
  XPCWrappedNative* wrapper = ccx.GetWrapper();
  if (wrapper != wrappedNative || !wrapper->IsValid()) {
    NS_ASSERTION(wrapper == wrappedNative, "Uh, how did this happen!");
    return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
  }

  
  
  XPCNativeInterface* iface = ccx.GetInterface();
  if (!iface) {
    

    return MaybePreserveWrapper(cx, wrappedNative, flags);
  }

  
  XPCNativeMember* member = ccx.GetMember();
  NS_ASSERTION(member, "not doing IDispatch, how'd this happen?");
  if (!member) {
    

    return MaybePreserveWrapper(cx, wrappedNative, flags);
  }

  
  
  jsval memberval;
  if (!member->GetValue(ccx, iface, &memberval)) {
    return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
  }

  
  AUTO_MARK_JSVAL(ccx, memberval);
  
  JSString *str = JSVAL_TO_STRING(id);
  if (!str) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  jsval v;
  uintN attrs = JSPROP_ENUMERATE;

  if (member->IsConstant()) {
    v = memberval;
  } else if (member->IsAttribute()) {
    
    
    
    

    v = JSVAL_VOID;
    attrs |= JSPROP_SHARED;
  } else {
    
    
    

    JSObject* funobj = xpc_CloneJSFunction(ccx, JSVAL_TO_OBJECT(memberval),
                                           wrapper->GetFlatJSObject());
    if (!funobj) {
      return JS_FALSE;
    }

    AUTO_MARK_JSVAL(ccx, OBJECT_TO_JSVAL(funobj));

#ifdef DEBUG_XPCNativeWrapper
    printf("Wrapping function object for %s\n",
           ::JS_GetStringBytes(JSVAL_TO_STRING(id)));
#endif

    if (!WrapFunction(cx, funobj, &v)) {
      return JS_FALSE;
    }
  }

  if (!::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                            ::JS_GetStringLength(str), v, nsnull, nsnull,
                            attrs)) {
    return JS_FALSE;
  }

  *objp = obj;

  return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
  XPC_NW_BYPASS(cx, obj, convert, (cx, obj, type, vp));

  return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(void)
XPC_NW_Finalize(JSContext *cx, JSObject *obj)
{
  
  
  XPCJSRuntime *rt = nsXPConnect::GetRuntime();

  {
    
    XPCAutoLock lock(rt->GetMapLock());
    rt->GetExplicitNativeWrapperMap()->Remove(obj);
  }
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_CheckAccess(JSContext *cx, JSObject *obj, jsval id,
                   JSAccessMode mode, jsval *vp)
{
  
  if ((mode & JSACC_WATCH) == JSACC_PROTO && (mode & JSACC_WRITE)) {
    return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
  }

  
  if (cx->runtime->checkObjectAccess &&
      !cx->runtime->checkObjectAccess(cx, obj, id, mode, vp)) {
    return JS_FALSE;
  }

  XPCWrappedNative *wrappedNative =
    XPCNativeWrapper::GetWrappedNative(cx, obj);
  if (!wrappedNative) {
    return JS_TRUE;
  }

  JSObject *wrapperJSObject = wrappedNative->GetFlatJSObject();

  JSClass *clazz = JS_GET_CLASS(cx, wrapperJSObject);
  return !clazz->checkAccess ||
    clazz->checkAccess(cx, wrapperJSObject, id, mode, vp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  if (!XPCNativeWrapper::IsNativeWrapper(cx, obj)) {
    
    
    

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

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval)
{
  
  
  
  
  obj = JSVAL_TO_OBJECT(argv[-2]);

  XPC_NW_BYPASS_TEST(cx, obj, construct, (cx, obj, argc, argv, rval));

  XPCWrappedNative *wrappedNative =
    XPCNativeWrapper::GetWrappedNative(cx, obj);
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

  return RewrapIfDeepWrapper(cx, obj, *rval, rval);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
  XPC_NW_BYPASS_TEST(cx, obj, hasInstance, (cx, obj, v, bp));

  return JS_TRUE;
}

static JSBool
MirrorWrappedNativeParent(JSContext *cx, XPCWrappedNative *wrapper,
                          JSObject **result)
{
  JSObject *wn_parent = ::JS_GetParent(cx, wrapper->GetFlatJSObject());
  if (!wn_parent) {
    *result = nsnull;
  } else {
    XPCWrappedNative *parent_wrapper =
      XPCWrappedNative::GetWrappedNativeOfJSObject(cx, wn_parent);

    *result = XPCNativeWrapper::GetNewOrUsed(cx, parent_wrapper);
    if (!*result)
      return JS_FALSE;
  }
  return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPCNativeWrapperCtor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                     jsval *rval)
{
  if (argc < 1) {
    return ThrowException(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx);
  }

  
  
  obj = nsnull;
  
  jsval native = argv[0];

  if (JSVAL_IS_PRIMITIVE(native)) {
    return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
  }

  JSObject *nativeObj = JSVAL_TO_OBJECT(native);

  XPCWrappedNative *wrappedNative;

  if (XPCNativeWrapper::IsNativeWrapper(cx, nativeObj)) {
    
    

#ifdef DEBUG_XPCNativeWrapper
    printf("Wrapping already wrapped object\n");
#endif

    wrappedNative = XPCNativeWrapper::GetWrappedNative(cx, nativeObj);

    if (!wrappedNative) {
      return ThrowException(NS_ERROR_INVALID_ARG, cx);
    }

    nativeObj = wrappedNative->GetFlatJSObject();
    native = OBJECT_TO_JSVAL(nativeObj);
  } else {
    wrappedNative
      = XPCWrappedNative::GetWrappedNativeOfJSObject(cx, nativeObj);

    if (!wrappedNative) {
      return ThrowException(NS_ERROR_INVALID_ARG, cx);
    }

    
    
    nsCOMPtr<nsIXPConnectWrappedJS> xpcwrappedjs =
      do_QueryWrappedNative(wrappedNative);

    if (xpcwrappedjs) {
      return ThrowException(NS_ERROR_INVALID_ARG, cx);
    }
  }

  JSObject *wrapperObj;

  
  
#ifdef DEBUG_XPCNativeWrapper
  printf("Creating new JSObject\n");
#endif
  wrapperObj = ::JS_NewObject(cx, XPCNativeWrapper::GetJSClass(), nsnull,
                              wrappedNative->GetScope()->GetGlobalJSObject());

  if (!wrapperObj) {
    
    return JS_FALSE;
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

  JSBool isDeep = !hasStringArgs;
  jsuint flags = isDeep ? FLAG_DEEP | FLAG_EXPLICIT : FLAG_EXPLICIT;
  if (!::JS_SetReservedSlot(cx, wrapperObj, 0, INT_TO_JSVAL(flags))) {
    return JS_FALSE;
  }

  JSObject *parent = nsnull;

  if (isDeep) {
    
    
    ::JS_LockGCThing(cx, wrapperObj);

    
    
    if (!MirrorWrappedNativeParent(cx, wrappedNative, &parent))
      return JS_FALSE;

    ::JS_UnlockGCThing(cx, wrapperObj);

    if (argc == 2 && !JSVAL_IS_PRIMITIVE(argv[1])) {
      
      
      
      
      JSBool hasInstance;
      if (!::JS_HasInstance(cx, JSVAL_TO_OBJECT(argv[1]), native,
                            &hasInstance)) {
        return ThrowException(NS_ERROR_UNEXPECTED, cx);
      }

      if (!hasInstance) {
        return ThrowException(NS_ERROR_INVALID_ARG, cx);
      }
    }
  }

  if (!parent) {
    parent = wrappedNative->GetScope()->GetGlobalJSObject();
  }
    
  if (!::JS_SetParent(cx, wrapperObj, parent))
    return JS_FALSE;

  
  if (!::JS_SetPrivate(cx, wrapperObj, wrappedNative)) {
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

JS_STATIC_DLL_CALLBACK(uint32)
XPC_NW_Mark(JSContext *cx, JSObject *obj, void *arg)
{
  XPCWrappedNative *wrappedNative =
    XPCNativeWrapper::GetWrappedNative(cx, obj);

  if (wrappedNative && wrappedNative->IsValid()) {
    ::JS_MarkGCThing(cx, wrappedNative->GetFlatJSObject(),
                     "XPCNativeWrapper wrapped native", arg);
  }

  return 0;
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_Equality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
  NS_ASSERTION(XPCNativeWrapper::IsNativeWrapper(cx, obj),
               "Uh, we should only ever be called for XPCNativeWrapper "
               "objects!");

  if (JSVAL_IS_PRIMITIVE(v)) {
    *bp = JS_FALSE;

    return JS_TRUE;
  }

  XPCWrappedNative *wrappedNative =
    XPCNativeWrapper::GetWrappedNative(cx, obj);

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

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_NW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
  while (!XPCNativeWrapper::IsNativeWrapper(cx, obj)) {
    obj = ::JS_GetPrototype(cx, obj);
    if (!obj) {
      return ThrowException(NS_ERROR_UNEXPECTED, cx);
    }
  }

  
  
  XPCJSRuntime *rt = nsXPConnect::GetRuntime();
  if (!rt)
    return JS_FALSE;

  jsid id = rt->GetStringID(XPCJSRuntime::IDX_TO_STRING);
  jsval idAsVal;
  if (!::JS_IdToValue(cx, id, &idAsVal)) {
    return JS_FALSE;
  }

  XPCWrappedNative *wrappedNative =
    XPCNativeWrapper::GetWrappedNative(cx, obj);

  if (!wrappedNative) {
    
    NS_NAMED_LITERAL_STRING(protoString, "[object XPCNativeWrapper]");
    JSString *str =
      ::JS_NewUCStringCopyN(cx, NS_REINTERPRET_CAST(const jschar*,
                                                    protoString.get()),
                            protoString.Length());
    NS_ENSURE_TRUE(str, JS_FALSE);
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
  }

  
  JSObject *wn_obj = wrappedNative->GetFlatJSObject();
  XPCCallContext ccx(JS_CALLER, cx, wn_obj, nsnull, idAsVal);
  if (!ccx.IsValid()) {
    
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  XPCNativeInterface *iface = ccx.GetInterface();
  XPCNativeMember *member = ccx.GetMember();
  JSBool overridden = JS_FALSE;
  jsval toStringVal;

  
  
  if (iface && member) {
    if (!member->GetValue(ccx, iface, &toStringVal)) {
      return JS_FALSE;
    }

    overridden = member->IsMethod();
  }

  JSString* str = nsnull;
  if (overridden) {
    

    AUTO_MARK_JSVAL(ccx, toStringVal);

    JSObject *funobj = xpc_CloneJSFunction(ccx, JSVAL_TO_OBJECT(toStringVal),
                                           wn_obj);
    if (!funobj) {
      return JS_FALSE;
    }

    jsval v;
    if (!::JS_CallFunctionValue(cx, wn_obj, OBJECT_TO_JSVAL(funobj), argc, argv,
                                &v)) {
      return JS_FALSE;
    }

    if (JSVAL_IS_STRING(v)) {
      str = JSVAL_TO_STRING(v);
    }
  }

  if (!str) {
    
    
    
    
    

    nsAutoString resultString;
    resultString.AppendLiteral("[object XPCNativeWrapper");

    char *wrapperStr = wrappedNative->ToString(ccx);
    if (!wrapperStr) {
      return JS_FALSE;
    }

    resultString.Append(' ');
    resultString.AppendASCII(wrapperStr);
    JS_smprintf_free(wrapperStr);

    resultString.Append(']');

    str = ::JS_NewUCStringCopyN(cx, NS_REINTERPRET_CAST(const jschar *,
                                                        resultString.get()),
                                resultString.Length());
  }

  NS_ENSURE_TRUE(str, JS_FALSE);

  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}


PRBool
XPCNativeWrapper::AttachNewConstructorObject(XPCCallContext &ccx,
                                             JSObject *aGlobalObject)
{
  JSObject *class_obj =
    ::JS_InitClass(ccx, aGlobalObject, nsnull, &sXPC_NW_JSClass.base,
                   XPCNativeWrapperCtor, 0, nsnull, sXPC_NW_JSClass_methods,
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
XPCNativeWrapper::GetNewOrUsed(JSContext *cx, XPCWrappedNative *wrapper)
{
  
  
  nsCOMPtr<nsIXPConnectWrappedJS> xpcwrappedjs(do_QueryWrappedNative(wrapper));

  if (xpcwrappedjs) {
    XPCThrower::Throw(NS_ERROR_INVALID_ARG, cx);

    return nsnull;
  }

  JSObject *obj = wrapper->GetNativeWrapper();
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

  obj = ::JS_NewObject(cx, GetJSClass(), nsnull, nw_parent);

  if (lock) {
    ::JS_UnlockGCThing(cx, nw_parent);
  }

  if (!obj ||
      !::JS_SetPrivate(cx, obj, wrapper) ||
      !::JS_SetReservedSlot(cx, obj, 0, INT_TO_JSVAL(FLAG_DEEP))) {
    return nsnull;
  }

  wrapper->SetNativeWrapper(obj);

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

struct WrapperAndCxHolder
{
    XPCWrappedNative* wrapper;
    JSContext* cx;
};

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
ClearNativeWrapperScope(JSDHashTable *table, JSDHashEntryHdr *hdr,
                        uint32 number, void *arg)
{
    JSDHashEntryStub* entry = (JSDHashEntryStub*)hdr;
    WrapperAndCxHolder* d = (WrapperAndCxHolder*)arg;

    if (d->wrapper->GetNativeWrapper() == (JSObject*)entry->key)
    {
        ::JS_ClearScope(d->cx, (JSObject*)entry->key);
    }

    return JS_DHASH_NEXT;
}


void
XPCNativeWrapper::ClearWrappedNativeScopes(JSContext* cx,
                                           XPCWrappedNative* wrapper)
{
  JSObject *nativeWrapper = wrapper->GetNativeWrapper();

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
