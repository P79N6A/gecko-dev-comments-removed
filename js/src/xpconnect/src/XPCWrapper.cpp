









































#include "XPCWrapper.h"
#include "XPCNativeWrapper.h"

const PRUint32
XPCWrapper::sWrappedObjSlot = 1;

const PRUint32
XPCWrapper::sFlagsSlot = 0;

const PRUint32
XPCWrapper::sNumSlots = 2;

JSNative
XPCWrapper::sEvalNative = nsnull;

const PRUint32
XPCWrapper::sSecMgrSetProp = nsIXPCSecurityManager::ACCESS_SET_PROPERTY;
const PRUint32
XPCWrapper::sSecMgrGetProp = nsIXPCSecurityManager::ACCESS_GET_PROPERTY;


JSObject *
XPCWrapper::Unwrap(JSContext *cx, JSObject *wrapper)
{
  JSClass *clasp = STOBJ_GET_CLASS(wrapper);
  if (clasp == &sXPC_XOW_JSClass.base) {
    return UnwrapXOW(cx, wrapper);
  }

  if (XPCNativeWrapper::IsNativeWrapperClass(clasp)) {
    XPCWrappedNative *wrappedObj;
    if (!XPCNativeWrapper::GetWrappedNative(cx, wrapper, &wrappedObj) ||
        !wrappedObj) {
      return nsnull;
    }

    return wrappedObj->GetFlatJSObject();
  }

  if (clasp == &sXPC_SJOW_JSClass.base) {
    JSObject *wrappedObj = STOBJ_GET_PARENT(wrapper);

    if (NS_FAILED(CanAccessWrapper(cx, wrappedObj))) {
      JS_ClearPendingException(cx);

      return nsnull;
    }

    return wrappedObj;
  }

  if (clasp == &sXPC_SOW_JSClass.base) {
    return UnwrapSOW(cx, wrapper);
  }
  if (clasp == &sXPC_COW_JSClass.base) {
    return UnwrapCOW(cx, wrapper);
  }

  return nsnull;
}

static void
IteratorFinalize(JSContext *cx, JSObject *obj)
{
  jsval v;
  JS_GetReservedSlot(cx, obj, 0, &v);

  JSIdArray *ida = reinterpret_cast<JSIdArray *>(JSVAL_TO_PRIVATE(v));
  if (ida) {
    JS_DestroyIdArray(cx, ida);
  }
}

static JSBool
IteratorNext(JSContext *cx, uintN argc, jsval *vp)
{
  JSObject *obj;
  jsval v;

  obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;

  JS_GetReservedSlot(cx, obj, 0, &v);
  JSIdArray *ida = reinterpret_cast<JSIdArray *>(JSVAL_TO_PRIVATE(v));

  JS_GetReservedSlot(cx, obj, 1, &v);
  jsint idx = JSVAL_TO_INT(v);

  if (idx == ida->length) {
    return JS_ThrowStopIteration(cx);
  }

  JS_GetReservedSlot(cx, obj, 2, &v);
  jsid id = ida->vector[idx++];
  if (JSVAL_TO_BOOLEAN(v)) {
    JSString *str;
    if (!JS_IdToValue(cx, id, &v) ||
        !(str = JS_ValueToString(cx, v))) {
      return JS_FALSE;
    }

    *vp = STRING_TO_JSVAL(str);
  } else {
    
    if (!JS_GetPropertyById(cx, STOBJ_GET_PARENT(obj), id, &v)) {
      return JS_FALSE;
    }

    jsval name;
    JSString *str;
    if (!JS_IdToValue(cx, id, &name) ||
        !(str = JS_ValueToString(cx, name))) {
      return JS_FALSE;
    }

    jsval vec[2] = { STRING_TO_JSVAL(str), v };
    JSAutoTempValueRooter tvr(cx, 2, vec);
    JSObject *array = JS_NewArrayObject(cx, 2, vec);
    if (!array) {
      return JS_FALSE;
    }

    *vp = OBJECT_TO_JSVAL(array);
  }

  JS_SetReservedSlot(cx, obj, 1, INT_TO_JSVAL(idx));
  return JS_TRUE;
}

static JSClass IteratorClass = {
  "XOW iterator", JSCLASS_HAS_RESERVED_SLOTS(3),
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub,
  JS_ConvertStub, IteratorFinalize,

  JSCLASS_NO_OPTIONAL_MEMBERS
};


JSObject *
XPCWrapper::CreateIteratorObj(JSContext *cx, JSObject *tempWrapper,
                              JSObject *wrapperObj, JSObject *innerObj,
                              JSBool keysonly)
{
  
  
  
  
  

  JSObject *iterObj = JS_NewObject(cx, &IteratorClass, tempWrapper, wrapperObj);
  if (!iterObj) {
    return nsnull;
  }

  JSAutoTempValueRooter tvr(cx, OBJECT_TO_JSVAL(iterObj));

  
  
  if (!JS_SetReservedSlot(cx, iterObj, 0, PRIVATE_TO_JSVAL(nsnull))) {
    return nsnull;
  }

  
  if (!JS_DefineFunction(cx, iterObj, "next", (JSNative)IteratorNext, 0,
                         JSFUN_FAST_NATIVE)) {
    return nsnull;
  }

  if (XPCNativeWrapper::IsNativeWrapper(wrapperObj)) {
    
    
    
    
    

    JSAutoTempValueRooter tvr(cx, tempWrapper);
    if (!JS_SetPrototype(cx, iterObj, wrapperObj) ||
        !XPCWrapper::Enumerate(cx, iterObj, wrapperObj) ||
        !JS_SetPrototype(cx, iterObj, tempWrapper)) {
      return nsnull;
    }
  }

  
  do {
    if (!XPCWrapper::Enumerate(cx, iterObj, innerObj)) {
      return nsnull;
    }
  } while ((innerObj = STOBJ_GET_PROTO(innerObj)) != nsnull);

  JSIdArray *ida = JS_Enumerate(cx, iterObj);
  if (!ida) {
    return nsnull;
  }

  if (!JS_SetReservedSlot(cx, iterObj, 0, PRIVATE_TO_JSVAL(ida)) ||
      !JS_SetReservedSlot(cx, iterObj, 1, JSVAL_ZERO) ||
      !JS_SetReservedSlot(cx, iterObj, 2, BOOLEAN_TO_JSVAL(keysonly))) {
    return nsnull;
  }

  if (!JS_SetPrototype(cx, iterObj, nsnull)) {
    return nsnull;
  }

  return iterObj;
}


JSBool
XPCWrapper::AddProperty(JSContext *cx, JSObject *wrapperObj,
                        JSBool wantGetterSetter, JSObject *innerObj, jsval id,
                        jsval *vp)
{
  jsid interned_id;
  if (!::JS_ValueToId(cx, id, &interned_id)) {
    return JS_FALSE;
  }

  JSPropertyDescriptor desc;
  if (!GetPropertyAttrs(cx, wrapperObj, interned_id, JSRESOLVE_QUALIFIED,
                        wantGetterSetter, &desc)) {
    return JS_FALSE;
  }

  NS_ASSERTION(desc.obj == wrapperObj,
               "What weird wrapper are we using?");

  return JS_DefinePropertyById(cx, innerObj, interned_id, desc.value,
                               desc.getter, desc.setter, desc.attrs);
}


JSBool
XPCWrapper::DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  if (JSVAL_IS_STRING(id)) {
    JSString *str = JSVAL_TO_STRING(id);
    jschar *chars = ::JS_GetStringChars(str);
    size_t length = ::JS_GetStringLength(str);

    return ::JS_DeleteUCProperty2(cx, obj, chars, length, vp);
  }

  if (!JSVAL_IS_INT(id)) {
    return ThrowException(NS_ERROR_NOT_IMPLEMENTED, cx);
  }

  return ::JS_DeleteElement2(cx, obj, JSVAL_TO_INT(id), vp);
}


JSBool
XPCWrapper::Enumerate(JSContext *cx, JSObject *wrapperObj, JSObject *innerObj)
{
  
  
  
  
  

  JSBool ok = JS_TRUE;

  JSIdArray *ida = JS_Enumerate(cx, innerObj);
  if (!ida) {
    return JS_FALSE;
  }

  for (jsint i = 0, n = ida->length; i < n; i++) {
    JSObject *pobj;

    
    
    jsval v = JSVAL_VOID;

    
    ok = JS_LookupPropertyWithFlagsById(cx, wrapperObj, ida->vector[i],
                                        JSRESOLVE_QUALIFIED, &pobj, &v);
    if (!ok) {
      break;
    }

    if (pobj && pobj != wrapperObj) {
      
      
      ok = JS_DefinePropertyById(cx, wrapperObj, ida->vector[i], JSVAL_VOID,
                                 nsnull, nsnull, JSPROP_ENUMERATE | JSPROP_SHARED);
    }

    if (!ok) {
      break;
    }
  }

  JS_DestroyIdArray(cx, ida);

  return ok;
}


JSBool
XPCWrapper::NewResolve(JSContext *cx, JSObject *wrapperObj,
                       JSBool wantDetails, JSObject *innerObj, jsval id,
                       uintN flags, JSObject **objp)
{
  jsid interned_id;
  if (!::JS_ValueToId(cx, id, &interned_id)) {
    return JS_FALSE;
  }

  JSPropertyDescriptor desc;
  if (!GetPropertyAttrs(cx, innerObj, interned_id, flags, wantDetails, &desc)) {
    return JS_FALSE;
  }

  if (!desc.obj) {
    
    return JS_TRUE;
  }

  desc.value = JSVAL_VOID;

  jsval oldFlags;
  if (!::JS_GetReservedSlot(cx, wrapperObj, sFlagsSlot, &oldFlags) ||
      !::JS_SetReservedSlot(cx, wrapperObj, sFlagsSlot,
                            INT_TO_JSVAL(JSVAL_TO_INT(oldFlags) |
                                         FLAG_RESOLVING))) {
    return JS_FALSE;
  }

  JSBool ok = JS_DefinePropertyById(cx, wrapperObj, interned_id, desc.value,
                                    desc.getter, desc.setter, desc.attrs);

  JS_SetReservedSlot(cx, wrapperObj, sFlagsSlot, oldFlags);

  if (ok) {
    *objp = wrapperObj;
  }

  return ok;
}


JSBool
XPCWrapper::ResolveNativeProperty(JSContext *cx, JSObject *wrapperObj,
                                  JSObject *innerObj, XPCWrappedNative *wn,
                                  jsval id, uintN flags, JSObject **objp,
                                  JSBool isNativeWrapper)
{
  
  XPCCallContext ccx(JS_CALLER, cx, innerObj, nsnull, id);

  
  
  if (NATIVE_HAS_FLAG(wn, WantNewResolve) &&
      id != GetRTStringByIndex(cx, XPCJSRuntime::IDX_CONSTRUCTOR)) {

    
    
    jsval oldFlags;
    if (!::JS_GetReservedSlot(cx, wrapperObj, sFlagsSlot, &oldFlags) ||
        !::JS_SetReservedSlot(cx, wrapperObj, sFlagsSlot,
                              INT_TO_JSVAL(JSVAL_TO_INT(oldFlags) |
                                           FLAG_RESOLVING))) {
      return JS_FALSE;
    }

    XPCWrappedNative* oldResolvingWrapper = nsnull;
    JSBool allowPropMods =
      NATIVE_HAS_FLAG(wn, AllowPropModsDuringResolve);
    if (allowPropMods) {
      oldResolvingWrapper = ccx.SetResolvingWrapper(wn);
    }

    JSBool retval = JS_TRUE;
    JSObject* newObj = nsnull;
    nsresult rv = wn->GetScriptableInfo()->
      GetCallback()->NewResolve(wn, cx, wrapperObj, id, flags,
                                &newObj, &retval);

    if (allowPropMods) {
      ccx.SetResolvingWrapper(oldResolvingWrapper);
    }

    if (!::JS_SetReservedSlot(cx, wrapperObj, sFlagsSlot, oldFlags)) {
      return JS_FALSE;
    }

    if (NS_FAILED(rv)) {
      return ThrowException(rv, cx);
    }

    if (newObj) {
      if (isNativeWrapper || newObj == wrapperObj) {
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

      
      
      
      
      
      
      return ThrowException(NS_ERROR_NOT_IMPLEMENTED, cx);
    }
  }

  if (!JSVAL_IS_STRING(id)) {
    
    

    return MaybePreserveWrapper(cx, wn, flags);
  }

  
  XPCWrappedNative* wrapper = ccx.GetWrapper();
  if (wrapper != wn || !wrapper->IsValid()) {
    NS_ASSERTION(wrapper == wn, "Uh, how did this happen!");
    return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
  }

  
  
  XPCNativeInterface* iface = ccx.GetInterface();
  if (!iface) {
    

    return MaybePreserveWrapper(cx, wn, flags);
  }

  
  XPCNativeMember* member = ccx.GetMember();
  if (!member) {
    

    return MaybePreserveWrapper(cx, wn, flags);
  }

  JSString *str = JSVAL_TO_STRING(id);
  if (!str) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  
  
  jsval v;
  uintN attrs = JSPROP_ENUMERATE;
  JSPropertyOp getter = nsnull;
  JSPropertyOp setter = nsnull;

  if (member->IsConstant()) {
    if (!member->GetConstantValue(ccx, iface, &v)) {
      return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
    }
  } else if (member->IsAttribute()) {
    
    
    
    

    v = JSVAL_VOID;
    attrs |= JSPROP_SHARED;
  } else {
    
    
    

    jsval funval;
    if (!member->NewFunctionObject(ccx, iface, wrapper->GetFlatJSObject(),
                                   &funval)) {
      return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
    }

    AUTO_MARK_JSVAL(ccx, funval);

#ifdef DEBUG_XPCNativeWrapper
    printf("Wrapping function object for %s\n",
           ::JS_GetStringBytes(JSVAL_TO_STRING(id)));
#endif

    if (!WrapFunction(cx, wrapperObj, JSVAL_TO_OBJECT(funval), &v,
                      isNativeWrapper)) {
      return JS_FALSE;
    }

    
    
    
    
    
    getter = setter = JS_PropertyStub;

    
    
    
    
    JS_SetReservedSlot(cx, JSVAL_TO_OBJECT(v), eAllAccessSlot, JSVAL_TRUE);
  }

  
  AUTO_MARK_JSVAL(ccx, v);

  
  jsval oldFlags;
  if (!isNativeWrapper &&
      (!::JS_GetReservedSlot(cx, wrapperObj, sFlagsSlot, &oldFlags) ||
       !::JS_SetReservedSlot(cx, wrapperObj, sFlagsSlot,
                             INT_TO_JSVAL(JSVAL_TO_INT(oldFlags) |
                                          FLAG_RESOLVING)))) {
    return JS_FALSE;
  }

  if (!::JS_DefineUCProperty(cx, wrapperObj, ::JS_GetStringChars(str),
                            ::JS_GetStringLength(str), v, getter, setter,
                            attrs)) {
    return JS_FALSE;
  }

  if (!isNativeWrapper &&
      !::JS_SetReservedSlot(cx, wrapperObj, sFlagsSlot, oldFlags)) {
    return JS_FALSE;
  }

  *objp = wrapperObj;

  return JS_TRUE;
}


JSBool
XPCWrapper::GetOrSetNativeProperty(JSContext *cx, JSObject *obj,
                                   XPCWrappedNative *wrappedNative,
                                   jsval id, jsval *vp, JSBool aIsSet,
                                   JSBool isNativeWrapper)
{
  
  JSObject *nativeObj = wrappedNative->GetFlatJSObject();
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

      return RewrapIfDeepWrapper(cx, obj, v, vp, isNativeWrapper);
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
  if (!member) {
    

    return JS_TRUE;
  }

  if (member->IsConstant()) {
    jsval memberval;
    if (!member->GetConstantValue(ccx, iface, &memberval)) {
      return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
    }

    
    
    if (aIsSet) {
      return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
    }

    *vp = memberval;

    return JS_TRUE;
  }

  if (!member->IsAttribute()) {
    
    

    return JS_TRUE;
  }

  jsval funval;
  if (!member->NewFunctionObject(ccx, iface, wrapper->GetFlatJSObject(),
                                 &funval)) {
    return ThrowException(NS_ERROR_XPC_BAD_CONVERT_JS, cx);
  }

  AUTO_MARK_JSVAL(ccx, funval);

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
  if (!::JS_CallFunctionValue(cx, wrapper->GetFlatJSObject(), funval, argc,
                              argv, &v)) {
    return JS_FALSE;
  }

  if (aIsSet) {
    return JS_TRUE;
  }

  {
    
    AUTO_MARK_JSVAL(ccx, v);

    return RewrapIfDeepWrapper(cx, obj, v, vp, isNativeWrapper);
  }
}


JSBool
XPCWrapper::NativeToString(JSContext *cx, XPCWrappedNative *wrappedNative,
                           uintN argc, jsval *argv, jsval *rval,
                           JSBool isNativeWrapper)
{
  
  
  XPCJSRuntime *rt = nsXPConnect::GetRuntimeInstance();

  jsid id = rt->GetStringID(XPCJSRuntime::IDX_TO_STRING);
  jsval idAsVal;
  if (!::JS_IdToValue(cx, id, &idAsVal)) {
    return JS_FALSE;
  }

  
  JSObject *wn_obj = wrappedNative->GetFlatJSObject();
  XPCCallContext ccx(JS_CALLER, cx, wn_obj, nsnull, idAsVal);
  if (!ccx.IsValid()) {
    
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  XPCNativeInterface *iface = ccx.GetInterface();
  XPCNativeMember *member = ccx.GetMember();
  JSString* str = nsnull;

  
  
  if (iface && member && member->IsMethod()) {
    jsval toStringVal;
    if (!member->NewFunctionObject(ccx, iface, wn_obj, &toStringVal)) {
      return JS_FALSE;
    }

    

    AUTO_MARK_JSVAL(ccx, toStringVal);

    jsval v;
    if (!::JS_CallFunctionValue(cx, wn_obj, toStringVal, argc, argv, &v)) {
      return JS_FALSE;
    }

    if (JSVAL_IS_STRING(v)) {
      str = JSVAL_TO_STRING(v);
    }
  }

  if (!str) {
    
    
    
    
    

    char *wrapperStr = nsnull;
    nsAutoString resultString;
    if (isNativeWrapper) {
      resultString.AppendLiteral("[object XPCNativeWrapper ");

      wrapperStr = wrappedNative->ToString(ccx);
      if (!wrapperStr) {
        return JS_FALSE;
      }
    } else {
      wrapperStr = wrappedNative->ToString(ccx);
      if (!wrapperStr) {
        return JS_FALSE;
      }
    }

    resultString.AppendASCII(wrapperStr);
    JS_smprintf_free(wrapperStr);

    if (isNativeWrapper) {
      resultString.Append(']');
    }

    str = ::JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar *>
                                                    (resultString.get()),
                                resultString.Length());
  }

  NS_ENSURE_TRUE(str, JS_FALSE);

  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}


JSBool
XPCWrapper::GetPropertyAttrs(JSContext *cx, JSObject *obj, jsid interned_id,
                             uintN flags, JSBool wantDetails,
                             JSPropertyDescriptor *desc)
{
  if (!JS_GetPropertyDescriptorById(cx, obj, interned_id, flags, desc)) {
    return JS_FALSE;
  }

  const uintN interesting_attrs = wantDetails
                                  ? (JSPROP_ENUMERATE |
                                     JSPROP_READONLY  |
                                     JSPROP_PERMANENT |
                                     JSPROP_SHARED    |
                                     JSPROP_GETTER    |
                                     JSPROP_SETTER)
                                  : JSPROP_ENUMERATE;
  desc->attrs &= interesting_attrs;

  if (wantDetails) {
    
    
    if (!(desc->attrs & JSPROP_GETTER)) {
      desc->getter = nsnull;
    }
    if (!(desc->attrs & JSPROP_SETTER)) {
      desc->setter = nsnull;
    }
  } else {
    
    desc->getter = desc->setter = nsnull;
    desc->value = JSVAL_VOID;
  }

  return JS_TRUE;
}
