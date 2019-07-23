






































#include "xpcprivate.h"
#include "nsDOMError.h"
#include "jsdbgapi.h"
#include "jscntxt.h"  
#include "XPCNativeWrapper.h"
#include "XPCWrapper.h"




static JSBool
XPC_SOW_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
XPC_SOW_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
XPC_SOW_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
XPC_SOW_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
XPC_SOW_Enumerate(JSContext *cx, JSObject *obj);

static JSBool
XPC_SOW_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                   JSObject **objp);

static JSBool
XPC_SOW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

static JSBool
XPC_SOW_CheckAccess(JSContext *cx, JSObject *obj, jsval id, JSAccessMode mode,
                    jsval *vp);

static JSBool
XPC_SOW_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

static JSBool
XPC_SOW_Equality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

static JSObject *
XPC_SOW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly);

static JSObject *
XPC_SOW_WrappedObject(JSContext *cx, JSObject *obj);

JSExtendedClass sXPC_SOW_JSClass = {
  
  { "SystemOnlyWrapper",
    JSCLASS_NEW_RESOLVE | JSCLASS_IS_EXTENDED |
    JSCLASS_HAS_RESERVED_SLOTS(XPCWrapper::sNumSlots),
    XPC_SOW_AddProperty, XPC_SOW_DelProperty,
    XPC_SOW_GetProperty, XPC_SOW_SetProperty,
    XPC_SOW_Enumerate,   (JSResolveOp)XPC_SOW_NewResolve,
    XPC_SOW_Convert,     nsnull,
    nsnull,              XPC_SOW_CheckAccess,
    nsnull,              nsnull,
    nsnull,              XPC_SOW_HasInstance,
    nsnull,              nsnull
  },

  
  XPC_SOW_Equality,
  nsnull,             
  nsnull,             
  XPC_SOW_Iterator,
  XPC_SOW_WrappedObject,
  JSCLASS_NO_RESERVED_MEMBERS
};

static JSBool
XPC_SOW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval);


static inline JSBool
ThrowException(nsresult rv, JSContext *cx)
{
  return XPCWrapper::ThrowException(rv, cx);
}



static inline JSObject *
GetWrappedJSObject(JSContext *cx, JSObject *obj)
{
  JSClass *clasp = STOBJ_GET_CLASS(obj);
  if (!(clasp->flags & JSCLASS_IS_EXTENDED)) {
    return obj;
  }

  JSExtendedClass *xclasp = (JSExtendedClass *)clasp;
  if (!xclasp->wrappedObject) {
    return obj;
  }

  return xclasp->wrappedObject(cx, obj);
}


static inline
JSObject *
GetWrapper(JSObject *obj)
{
  while (STOBJ_GET_CLASS(obj) != &sXPC_SOW_JSClass.base) {
    obj = STOBJ_GET_PROTO(obj);
    if (!obj) {
      break;
    }
  }

  return obj;
}

static inline
JSObject *
GetWrappedObject(JSContext *cx, JSObject *wrapper)
{
  return XPCWrapper::UnwrapGeneric(cx, &sXPC_SOW_JSClass, wrapper);
}


JSBool
AllowedToAct(JSContext *cx, jsval idval)
{
  nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
  if (!ssm) {
    return JS_TRUE;
  }

  JSStackFrame *fp;
  nsIPrincipal *principal = ssm->GetCxSubjectPrincipalAndFrame(cx, &fp);
  if (!principal) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  if (!fp) {
    if (!JS_FrameIterator(cx, &fp)) {
      
      
      return JS_TRUE;
    }

    
    
    fp = nsnull;
  }

  void *annotation = fp ? JS_GetFrameAnnotation(cx, fp) : nsnull;
  PRBool privileged;
  if (NS_SUCCEEDED(principal->IsCapabilityEnabled("UniversalXPConnect",
                                                  annotation,
                                                  &privileged)) &&
      privileged) {
    
    return JS_TRUE;
  }

  
  
  static const char prefix[] = "chrome://global/";
  const char *filename;
  if (fp &&
      (filename = fp->script->filename) &&
      !strncmp(filename, prefix, NS_ARRAY_LENGTH(prefix) - 1)) {
    return JS_TRUE;
  }

  if (JSVAL_IS_VOID(idval)) {
    ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
  } else {
    
    JSString *str = JS_ValueToString(cx, idval);
    if (str) {
      JS_ReportError(cx, "Permission denied to access property '%hs' from a non-chrome context",
                     JS_GetStringChars(str));
    }
  }

  return JS_FALSE;
}

static JSBool
XPC_SOW_FunctionWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                        jsval *rval)
{
  if (!AllowedToAct(cx, JSVAL_VOID)) {
    return JS_FALSE;
  }

  JSObject *wrappedObj;

  
  
  
  

  wrappedObj = GetWrapper(obj);
  if (wrappedObj) {
    wrappedObj = GetWrappedObject(cx, wrappedObj);
    if (!wrappedObj) {
      return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
    }
  } else {
    wrappedObj = obj;
  }

  JSObject *funObj = JSVAL_TO_OBJECT(argv[-2]);
  jsval funToCall;
  if (!JS_GetReservedSlot(cx, funObj, XPCWrapper::eWrappedFunctionSlot,
                          &funToCall)) {
    return JS_FALSE;
  }

  return JS_CallFunctionValue(cx, wrappedObj, funToCall, argc, argv, rval);
}

JSBool
XPC_SOW_WrapFunction(JSContext *cx, JSObject *outerObj, JSObject *funobj,
                     jsval *rval)
{
  jsval funobjVal = OBJECT_TO_JSVAL(funobj);
  JSFunction *wrappedFun =
    reinterpret_cast<JSFunction *>(xpc_GetJSPrivate(funobj));
  JSNative native = JS_GetFunctionNative(cx, wrappedFun);
  if (!native || native == XPC_SOW_FunctionWrapper) {
    *rval = funobjVal;
    return JS_TRUE;
  }

  JSFunction *funWrapper =
    JS_NewFunction(cx, XPC_SOW_FunctionWrapper,
                   JS_GetFunctionArity(wrappedFun), 0,
                   JS_GetGlobalForObject(cx, outerObj),
                   JS_GetFunctionName(wrappedFun));
  if (!funWrapper) {
    return JS_FALSE;
  }

  JSObject *funWrapperObj = JS_GetFunctionObject(funWrapper);
  *rval = OBJECT_TO_JSVAL(funWrapperObj);

  return JS_SetReservedSlot(cx, funWrapperObj,
                            XPCWrapper::eWrappedFunctionSlot,
                            funobjVal);
}

static JSBool
XPC_SOW_RewrapValue(JSContext *cx, JSObject *wrapperObj, jsval *vp)
{
  jsval v = *vp;
  if (JSVAL_IS_PRIMITIVE(v)) {
    return JS_TRUE;
  }

  JSObject *obj = JSVAL_TO_OBJECT(v);

  if (JS_ObjectIsFunction(cx, obj)) {
    
    JSNative native = JS_GetFunctionNative(cx, JS_ValueToFunction(cx, v));

    
    
    
    if (!native) {
     return JS_TRUE;
    }

    if (native == XPC_SOW_FunctionWrapper) {
      
      
      if (STOBJ_GET_PROTO(wrapperObj) == STOBJ_GET_PARENT(obj)) {
        return JS_TRUE;
      }

      
      if (!JS_GetReservedSlot(cx, obj, XPCWrapper::eWrappedFunctionSlot, &v)) {
        return JS_FALSE;
      }
      obj = JSVAL_TO_OBJECT(v);
    }

    return XPC_SOW_WrapFunction(cx, wrapperObj, obj, vp);
  }

  if (STOBJ_GET_CLASS(obj) == &sXPC_SOW_JSClass.base) {
    
    
    
    
    if (STOBJ_GET_PARENT(wrapperObj) == STOBJ_GET_PARENT(obj)) {
      
      return JS_TRUE;
    }

    obj = GetWrappedObject(cx, obj);
    if (!obj) {
      
      *vp = JSVAL_NULL;
      return JS_TRUE;
    }
    v = *vp = OBJECT_TO_JSVAL(obj);
  }

  return XPC_SOW_WrapObject(cx, STOBJ_GET_PARENT(wrapperObj), v, vp);
}

static JSBool
XPC_SOW_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  NS_ASSERTION(STOBJ_GET_CLASS(obj) == &sXPC_SOW_JSClass.base, "Wrong object");

  jsval resolving;
  if (!JS_GetReservedSlot(cx, obj, XPCWrapper::sFlagsSlot, &resolving)) {
    return JS_FALSE;
  }

  if (HAS_FLAGS(resolving, FLAG_RESOLVING)) {
    
    return JS_TRUE;
  }

  if (!AllowedToAct(cx, id)) {
    return JS_FALSE;
  }

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return JS_TRUE;
  }

  return XPCWrapper::AddProperty(cx, obj, JS_TRUE, wrappedObj, id, vp);
}

static JSBool
XPC_SOW_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  if (!AllowedToAct(cx, id)) {
    return JS_FALSE;
  }

  return XPCWrapper::DelProperty(cx, wrappedObj, id, vp);
}

static JSBool
XPC_SOW_GetOrSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp,
                         JSBool isSet)
{
  if (id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    return JS_TRUE;
  }

  obj = GetWrapper(obj);
  if (!obj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  if (!AllowedToAct(cx, id)) {
    return JS_FALSE;
  }

  JSAutoTempValueRooter tvr(cx, 1, vp);

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  if (isSet && id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_PROTO)) {
    
    return ThrowException(NS_ERROR_INVALID_ARG, cx); 
  }

  jsid interned_id;
  if (!JS_ValueToId(cx, id, &interned_id)) {
    return JS_FALSE;
  }

  JSBool ok = isSet
              ? JS_SetPropertyById(cx, wrappedObj, interned_id, vp)
              : JS_GetPropertyById(cx, wrappedObj, interned_id, vp);
  if (!ok) {
    return JS_FALSE;
  }

  return XPC_SOW_RewrapValue(cx, obj, vp);
}

static JSBool
XPC_SOW_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  return XPC_SOW_GetOrSetProperty(cx, obj, id, vp, JS_FALSE);
}

static JSBool
XPC_SOW_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  return XPC_SOW_GetOrSetProperty(cx, obj, id, vp, JS_TRUE);
}

static JSBool
XPC_SOW_Enumerate(JSContext *cx, JSObject *obj)
{
  obj = GetWrapper(obj);
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    return JS_TRUE;
  }

  if (!AllowedToAct(cx, JSVAL_VOID)) {
    return JS_FALSE;
  }

  return XPCWrapper::Enumerate(cx, obj, wrappedObj);
}

static JSBool
XPC_SOW_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                   JSObject **objp)
{
  obj = GetWrapper(obj);

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    *objp = nsnull;
    return JS_TRUE;
  }

  if (!AllowedToAct(cx, id)) {
    return JS_FALSE;
  }

  if (id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    jsval oldSlotVal;
    if (!JS_GetReservedSlot(cx, obj, XPCWrapper::sFlagsSlot, &oldSlotVal) ||
        !JS_SetReservedSlot(cx, obj, XPCWrapper::sFlagsSlot,
                            INT_TO_JSVAL(JSVAL_TO_INT(oldSlotVal) |
                                         FLAG_RESOLVING))) {
      return JS_FALSE;
    }

    JSBool ok = JS_DefineFunction(cx, obj, "toString",
                                  XPC_SOW_toString, 0, 0) != nsnull;

    JS_SetReservedSlot(cx, obj, XPCWrapper::sFlagsSlot, oldSlotVal);

    if (ok) {
      *objp = obj;
    }

    return ok;
  }

  return XPCWrapper::NewResolve(cx, obj, JS_TRUE, wrappedObj, id, flags, objp);
}

static JSBool
XPC_SOW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
  if (!AllowedToAct(cx, JSVAL_VOID)) {
    return JS_FALSE;
  }

  
  if (type == JSTYPE_OBJECT) {
    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
  }

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    

    if (type == JSTYPE_STRING || type == JSTYPE_VOID) {
      return XPC_SOW_toString(cx, obj, 0, nsnull, vp);
    }

    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
  }

  return STOBJ_GET_CLASS(wrappedObj)->convert(cx, wrappedObj, type, vp);
}

static JSBool
XPC_SOW_CheckAccess(JSContext *cx, JSObject *obj, jsval prop, JSAccessMode mode,
                    jsval *vp)
{
  
  

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    *vp = JSVAL_VOID;
    return JS_TRUE;
  }

  uintN junk;
  jsid id;
  return JS_ValueToId(cx, prop, &id) &&
         JS_CheckAccess(cx, wrappedObj, id, mode, vp, &junk);
}

static JSBool
XPC_SOW_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
  if (!AllowedToAct(cx, JSVAL_VOID)) {
    return JS_FALSE;
  }

  JSObject *iface = GetWrappedObject(cx, obj);
  if (!iface) {
    *bp = JS_FALSE;
    return JS_TRUE;
  }

  JSClass *clasp = STOBJ_GET_CLASS(iface);

  *bp = JS_FALSE;
  if (!clasp->hasInstance) {
    return JS_TRUE;
  }

  
  
  
  if (!JSVAL_IS_PRIMITIVE(v)) {
    JSObject *test = JSVAL_TO_OBJECT(v);

    
    test = GetWrappedObject(cx, test);
    if (test) {
      v = OBJECT_TO_JSVAL(test);
    }
  }

  return clasp->hasInstance(cx, iface, v, bp);
}

static JSBool
XPC_SOW_Equality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
  
  if (JSVAL_IS_PRIMITIVE(v)) {
    *bp = JS_FALSE;
    return JS_TRUE;
  }

  if (obj == JSVAL_TO_OBJECT(v)) {
    *bp = JS_TRUE;
    return JS_TRUE;
  }

  JSObject *lhs = GetWrappedObject(cx, obj);
  JSObject *rhs = GetWrappedJSObject(cx, JSVAL_TO_OBJECT(v));
  if (lhs == rhs) {
    *bp = JS_TRUE;
    return JS_TRUE;
  }

  if (lhs) {
    
    JSClass *clasp = STOBJ_GET_CLASS(lhs);
    if (clasp->flags & JSCLASS_IS_EXTENDED) {
      JSExtendedClass *xclasp = (JSExtendedClass *) clasp;
      
      return xclasp->equality(cx, lhs, OBJECT_TO_JSVAL(rhs), bp);
    }
  }

  
  JSClass *clasp = STOBJ_GET_CLASS(rhs);
  if (clasp->flags & JSCLASS_IS_EXTENDED) {
    JSExtendedClass *xclasp = (JSExtendedClass *) clasp;
    
    return xclasp->equality(cx, rhs, OBJECT_TO_JSVAL(lhs), bp);
  }

  *bp = JS_FALSE;
  return JS_TRUE;
}

static JSObject *
XPC_SOW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly)
{
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    ThrowException(NS_ERROR_INVALID_ARG, cx);
    return nsnull;
  }

  JSObject *wrapperIter = JS_NewObject(cx, &sXPC_SOW_JSClass.base, nsnull,
                                       JS_GetGlobalForObject(cx, obj));
  if (!wrapperIter) {
    return nsnull;
  }

  JSAutoTempValueRooter tvr(cx, OBJECT_TO_JSVAL(wrapperIter));

  
  jsval v = OBJECT_TO_JSVAL(wrappedObj);
  if (!JS_SetReservedSlot(cx, wrapperIter, XPCWrapper::sWrappedObjSlot, v) ||
      !JS_SetReservedSlot(cx, wrapperIter, XPCWrapper::sFlagsSlot,
                          JSVAL_ZERO)) {
    return nsnull;
  }

  return XPCWrapper::CreateIteratorObj(cx, wrapperIter, obj, wrappedObj,
                                       keysonly);
}

static JSObject *
XPC_SOW_WrappedObject(JSContext *cx, JSObject *obj)
{
  return GetWrappedObject(cx, obj);
}

static JSBool
XPC_SOW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval)
{
  if (!AllowedToAct(cx, JSVAL_VOID)) {
    return JS_FALSE;
  }

  obj = GetWrapper(obj);
  if (!obj) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    NS_NAMED_LITERAL_CSTRING(protoString, "[object XPCCrossOriginWrapper]");
    JSString *str =
      JS_NewStringCopyN(cx, protoString.get(), protoString.Length());
    if (!str) {
      return JS_FALSE;
    }
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
  }

  XPCWrappedNative *wn =
    XPCWrappedNative::GetWrappedNativeOfJSObject(cx, wrappedObj);
  return XPCWrapper::NativeToString(cx, wn, argc, argv, rval, JS_FALSE);
}

JSBool
XPC_SOW_WrapObject(JSContext *cx, JSObject *parent, jsval v,
                   jsval *vp)
{
  
  
  JSObject *innerObj = JSVAL_TO_OBJECT(v);
  if (IS_SLIM_WRAPPER(innerObj) && !MorphSlimWrapper(cx, innerObj)) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  JSObject *wrapperObj =
    JS_NewObjectWithGivenProto(cx, &sXPC_SOW_JSClass.base, NULL, parent);
  if (!wrapperObj) {
    return JS_FALSE;
  }

  *vp = OBJECT_TO_JSVAL(wrapperObj);
  JSAutoTempValueRooter tvr(cx, *vp);

  if (!JS_SetReservedSlot(cx, wrapperObj, XPCWrapper::sWrappedObjSlot, v) ||
      !JS_SetReservedSlot(cx, wrapperObj, XPCWrapper::sFlagsSlot,
                          JSVAL_ZERO)) {
    return JS_FALSE;
  }

  return JS_TRUE;
}
