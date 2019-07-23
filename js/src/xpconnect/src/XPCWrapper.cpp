









































#include "XPCWrapper.h"

const PRUint32
XPCWrapper::sWrappedObjSlot = 1;

const PRUint32
XPCWrapper::sResolvingSlot = 0;

const PRUint32
XPCWrapper::sNumSlots = 2;

JSNative
XPCWrapper::sEvalNative = nsnull;


JSBool
XPCWrapper::AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  if (JSVAL_IS_STRING(id)) {
    JSString *str = JSVAL_TO_STRING(id);
    jschar *chars = ::JS_GetStringChars(str);
    size_t length = ::JS_GetStringLength(str);

    return ::JS_DefineUCProperty(cx, obj, chars, length, *vp, nsnull,
                                 nsnull, JSPROP_ENUMERATE);
  }

  if (!JSVAL_IS_INT(id)) {
    return ThrowException(NS_ERROR_NOT_IMPLEMENTED, cx);
  }

  return ::JS_DefineElement(cx, obj, JSVAL_TO_INT(id), *vp, nsnull,
                            nsnull, JSPROP_ENUMERATE);
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
  
  
  
  
  

  JSIdArray *ida = JS_Enumerate(cx, innerObj);
  if (!ida) {
    return JS_FALSE;
  }

  JSBool ok = JS_TRUE;

  for (jsint i = 0, n = ida->length; i < n; i++) {
    JSObject *pobj;
    JSProperty *prop;

    
    
    ok = OBJ_LOOKUP_PROPERTY(cx, wrapperObj, ida->vector[i], &pobj, &prop);
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


JSBool
XPCWrapper::NewResolve(JSContext *cx, JSObject *wrapperObj,
                       JSObject *innerObj, jsval id, uintN flags,
                       JSObject **objp, JSBool preserveVal)
{
  jschar *chars = nsnull;
  size_t length;
  JSBool hasProp, ok;
  jsval v = JSVAL_VOID;

  if (JSVAL_IS_STRING(id)) {
    JSString *str = JSVAL_TO_STRING(id);

    chars = ::JS_GetStringChars(str);
    length = ::JS_GetStringLength(str);

    ok = ::JS_HasUCProperty(cx, innerObj, chars, length, &hasProp);
    if (preserveVal && ok && hasProp) {
      ok = ::JS_LookupUCProperty(cx, innerObj, chars, length, &v);
    }
  } else if (JSVAL_IS_INT(id)) {
    ok = ::JS_HasElement(cx, innerObj, JSVAL_TO_INT(id), &hasProp);
    if (preserveVal && ok && hasProp) {
      ok = ::JS_LookupElement(cx, innerObj, JSVAL_TO_INT(id), &v);
    }
  } else {
    
    
    

    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  if (!ok || !hasProp) {
    
    
    
    
    

    return ok;
  }

  jsval oldSlotVal;
  if (!::JS_GetReservedSlot(cx, wrapperObj, sResolvingSlot, &oldSlotVal) ||
      !::JS_SetReservedSlot(cx, wrapperObj, sResolvingSlot,
                            BOOLEAN_TO_JSVAL(JS_TRUE))) {
    return JS_FALSE;
  }

  if (chars) {
    ok = ::JS_DefineUCProperty(cx, wrapperObj, chars, length, v,
                               nsnull, nsnull, JSPROP_ENUMERATE);
  } else {
    ok = ::JS_DefineElement(cx, wrapperObj, JSVAL_TO_INT(id), v,
                            nsnull, nsnull, JSPROP_ENUMERATE);
  }

  if (ok && (ok = ::JS_SetReservedSlot(cx, wrapperObj, sResolvingSlot,
                                       oldSlotVal))) {
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
    if (isNativeWrapper) {
      if (!::JS_GetReservedSlot(cx, wrapperObj, 0, &oldFlags) ||
          !::JS_SetReservedSlot(cx, wrapperObj, 0,
                                INT_TO_JSVAL(JSVAL_TO_INT(oldFlags) |
                                             FLAG_RESOLVING))) {
        return JS_FALSE;
      }
    } else {
      if (!::JS_GetReservedSlot(cx, wrapperObj, sResolvingSlot, &oldFlags) ||
          !::JS_SetReservedSlot(cx, wrapperObj, sResolvingSlot, JSVAL_TRUE)) {
        return JS_FALSE;
      }
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

    if (!::JS_SetReservedSlot(cx, wrapperObj,
                              isNativeWrapper ? 0 : sResolvingSlot,
                              oldFlags)) {
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

      return NewResolve(cx, wrapperObj, innerObj, id, flags, objp, JS_TRUE);
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
  NS_ASSERTION(member, "not doing IDispatch, how'd this happen?");
  if (!member) {
    

    return MaybePreserveWrapper(cx, wn, flags);
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

    if (!WrapFunction(cx, wrapperObj, funobj, &v, isNativeWrapper)) {
      return JS_FALSE;
    }
  }

  
  jsval oldFlags;
  if (!isNativeWrapper &&
      (!::JS_GetReservedSlot(cx, wrapperObj, sResolvingSlot, &oldFlags) ||
       !::JS_SetReservedSlot(cx, wrapperObj, sResolvingSlot, JSVAL_TRUE))) {
    return JS_FALSE;
  }

  if (!::JS_DefineUCProperty(cx, wrapperObj, ::JS_GetStringChars(str),
                            ::JS_GetStringLength(str), v, nsnull, nsnull,
                            attrs)) {
    return JS_FALSE;
  }

  if (!isNativeWrapper &&
      !::JS_SetReservedSlot(cx, wrapperObj, sResolvingSlot, oldFlags)) {
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

    return RewrapIfDeepWrapper(cx, obj, v, vp, isNativeWrapper);
  }
}


JSBool
XPCWrapper::NativeToString(JSContext *cx, XPCWrappedNative *wrappedNative,
                           uintN argc, jsval *argv, jsval *rval,
                           JSBool isNativeWrapper)
{
  
  
  XPCJSRuntime *rt = nsXPConnect::GetRuntime();
  if (!rt)
    return JS_FALSE;

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
    
    
    
    
    

    char *wrapperStr = nsnull;
    nsAutoString resultString;
    if (isNativeWrapper) {
      resultString.AppendLiteral("[object XPCNativeWrapper ");

      wrapperStr = wrappedNative->ToString(ccx);
      if (!wrapperStr) {
        return JS_FALSE;
      }
    } else {
      resultString.AppendLiteral("[object XPCCrossOriginWrapper ");

      wrapperStr = wrappedNative->ToString(ccx);
      if (!wrapperStr) {
        return JS_FALSE;
      }
    }

    resultString.AppendASCII(wrapperStr);
    JS_smprintf_free(wrapperStr);

    resultString.Append(']');

    str = ::JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar *>
                                                    (resultString.get()),
                                resultString.Length());
  }

  NS_ENSURE_TRUE(str, JS_FALSE);

  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}
