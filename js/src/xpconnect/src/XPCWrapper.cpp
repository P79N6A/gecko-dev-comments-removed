









































#include "XPCWrapper.h"
#include "jsscope.h"

const PRUint32
XPCWrapper::sWrappedObjSlot = 1;

const PRUint32
XPCWrapper::sResolvingSlot = 0;

const PRUint32
XPCWrapper::sNumSlots = 2;

JSNative
XPCWrapper::sEvalNative = nsnull;

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
    
    if (!OBJ_GET_PROPERTY(cx, STOBJ_GET_PARENT(obj), id, &v)) {
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
                        JSObject *innerObj, jsval id, jsval *vp)
{
  jsid interned_id;
  if (!::JS_ValueToId(cx, id, &interned_id)) {
    return JS_FALSE;
  }

  JSProperty *prop;
  JSObject *wrapperObjp;
  if (!OBJ_LOOKUP_PROPERTY(cx, wrapperObj, interned_id, &wrapperObjp, &prop)) {
    return JS_FALSE;
  }

  NS_ASSERTION(prop && OBJ_IS_NATIVE(wrapperObjp),
               "What weird wrapper are we using?");

  JSBool isXOW = (STOBJ_GET_CLASS(wrapperObj) == &sXPC_XOW_JSClass.base);
  uintN attrs = JSPROP_ENUMERATE;
  JSPropertyOp getter = nsnull;
  JSPropertyOp setter = nsnull;
  jsval v = *vp;
  if (isXOW) {
    JSScopeProperty *sprop = reinterpret_cast<JSScopeProperty *>(prop);

    attrs = sprop->attrs;
    if (attrs & JSPROP_GETTER) {
      getter = sprop->getter;
    }
    if (attrs & JSPROP_SETTER) {
      setter = sprop->setter;
    }

    if (SPROP_HAS_VALID_SLOT(sprop, OBJ_SCOPE(wrapperObjp))) {
      v = OBJ_GET_SLOT(cx, wrapperObjp, sprop->slot);
    }
  }

  OBJ_DROP_PROPERTY(cx, wrapperObjp, prop);

  const uintN interesting_attrs = isXOW
                                  ? (JSPROP_ENUMERATE |
                                     JSPROP_READONLY  |
                                     JSPROP_PERMANENT |
                                     JSPROP_SHARED    |
                                     JSPROP_GETTER    |
                                     JSPROP_SETTER)
                                  : JSPROP_ENUMERATE;
  return OBJ_DEFINE_PROPERTY(cx, innerObj, interned_id, v, getter,
                             setter, (attrs & interesting_attrs), nsnull);
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
    JSProperty *prop;

    
    
    ok = OBJ_LOOKUP_PROPERTY(cx, wrapperObj, ida->vector[i], &pobj, &prop);
    if (!ok) {
      break;
    }

    if (prop) {
      OBJ_DROP_PROPERTY(cx, pobj, prop);
    }

    if (pobj != wrapperObj) {
      ok = OBJ_DEFINE_PROPERTY(cx, wrapperObj, ida->vector[i], JSVAL_VOID,
                               nsnull, nsnull, JSPROP_ENUMERATE | JSPROP_SHARED,
                               nsnull);
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
                       JSObject *innerObj, jsval id, uintN flags,
                       JSObject **objp, JSBool preserveVal)
{
  jsval v = JSVAL_VOID;

  jsid interned_id;
  if (!::JS_ValueToId(cx, id, &interned_id)) {
    return JS_FALSE;
  }

  JSProperty *prop;
  JSObject *innerObjp;
  if (!OBJ_LOOKUP_PROPERTY(cx, innerObj, interned_id, &innerObjp, &prop)) {
    return JS_FALSE;
  }

  if (!prop) {
    
    return JS_TRUE;
  }

  JSBool isXOW = (STOBJ_GET_CLASS(wrapperObj) == &sXPC_XOW_JSClass.base);
  uintN attrs = JSPROP_ENUMERATE;
  JSPropertyOp getter = nsnull;
  JSPropertyOp setter = nsnull;
  if (isXOW && OBJ_IS_NATIVE(innerObjp)) {
    JSScopeProperty *sprop = reinterpret_cast<JSScopeProperty *>(prop);

    attrs = sprop->attrs;
    if (attrs & JSPROP_GETTER) {
      getter = sprop->getter;
    }
    if (attrs & JSPROP_SETTER) {
      setter = sprop->setter;
    }

    if (preserveVal && SPROP_HAS_VALID_SLOT(sprop, OBJ_SCOPE(innerObjp))) {
      v = OBJ_GET_SLOT(cx, innerObjp, sprop->slot);
    }
  }

  OBJ_DROP_PROPERTY(cx, innerObjp, prop);

  
  
  
  if (!preserveVal && isXOW && !JSVAL_IS_PRIMITIVE(v)) {
    JSObject *obj = JSVAL_TO_OBJECT(v);
    if (JS_ObjectIsFunction(cx, obj)) {
      JSFunction *fun = reinterpret_cast<JSFunction *>(xpc_GetJSPrivate(obj));
      if (JS_GetFunctionNative(cx, fun) == sEvalNative &&
          !WrapFunction(cx, wrapperObj, obj, &v, JS_FALSE)) {
        return JS_FALSE;
      }
    }
  }

  jsval oldSlotVal;
  if (!::JS_GetReservedSlot(cx, wrapperObj, sResolvingSlot, &oldSlotVal) ||
      !::JS_SetReservedSlot(cx, wrapperObj, sResolvingSlot, JSVAL_TRUE)) {
    return JS_FALSE;
  }

  const uintN interesting_attrs = isXOW
                                  ? (JSPROP_ENUMERATE |
                                     JSPROP_READONLY  |
                                     JSPROP_PERMANENT |
                                     JSPROP_SHARED    |
                                     JSPROP_GETTER    |
                                     JSPROP_SETTER)
                                  : JSPROP_ENUMERATE;
  JSBool ok = OBJ_DEFINE_PROPERTY(cx, wrapperObj, interned_id, v, getter,
                                  setter, (attrs & interesting_attrs), nsnull);

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

  JSString *str = JSVAL_TO_STRING(id);
  if (!str) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  
  
  jsval v;
  uintN attrs = JSPROP_ENUMERATE;

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
  }

  
  AUTO_MARK_JSVAL(ccx, v);

  
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
