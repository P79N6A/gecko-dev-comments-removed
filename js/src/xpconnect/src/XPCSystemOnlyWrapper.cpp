






































#include "xpcprivate.h"
#include "nsDOMError.h"
#include "jsdbgapi.h"
#include "jscntxt.h"  
#include "XPCNativeWrapper.h"
#include "XPCWrapper.h"




static JSBool
XPC_SOW_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_SOW_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_SOW_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_SOW_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_SOW_Enumerate(JSContext *cx, JSObject *obj);

static JSBool
XPC_SOW_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                   JSObject **objp);

static JSBool
XPC_SOW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

static JSBool
XPC_SOW_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                    jsval *vp);

static JSBool
XPC_SOW_HasInstance(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp);

static JSBool
XPC_SOW_Equality(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp);

static JSObject *
XPC_SOW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly);

static JSObject *
XPC_SOW_WrappedObject(JSContext *cx, JSObject *obj);

using namespace XPCWrapper;


static inline JSBool
ThrowException(nsresult rv, JSContext *cx)
{
  return DoThrowException(rv, cx);
}

static const char prefix[] = "chrome://global/";

namespace SystemOnlyWrapper {

JSExtendedClass SOWClass = {
  
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

JSBool
WrapObject(JSContext *cx, JSObject *parent, jsval v, jsval *vp)
{
  
  
  JSObject *innerObj = JSVAL_TO_OBJECT(v);
  if (IS_SLIM_WRAPPER(innerObj) && !MorphSlimWrapper(cx, innerObj)) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  JSObject *wrapperObj =
    JS_NewObjectWithGivenProto(cx, &SOWClass.base, NULL, parent);
  if (!wrapperObj) {
    return JS_FALSE;
  }

  *vp = OBJECT_TO_JSVAL(wrapperObj);
  js::AutoObjectRooter tvr(cx, wrapperObj);

  if (!JS_SetReservedSlot(cx, wrapperObj, sWrappedObjSlot, v) ||
      !JS_SetReservedSlot(cx, wrapperObj, sFlagsSlot, JSVAL_ZERO)) {
    return JS_FALSE;
  }

  return JS_TRUE;
}

JSBool
MakeSOW(JSContext *cx, JSObject *obj)
{
#ifdef DEBUG
  {
    JSClass *clasp = obj->getJSClass();
    NS_ASSERTION(clasp != &SystemOnlyWrapper::SOWClass.base &&
                 clasp != &XPCCrossOriginWrapper::XOWClass.base,
                 "bad call");
  }
#endif

  jsval flags;
  return JS_GetReservedSlot(cx, obj, sFlagsSlot, &flags) &&
         JS_SetReservedSlot(cx, obj, sFlagsSlot,
                            INT_TO_JSVAL(JSVAL_TO_INT(flags) | FLAG_SOW));
}



JSBool
AllowedToAct(JSContext *cx, jsid id)
{
  
  nsIScriptSecurityManager *ssm = GetSecurityManager();
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
  } else if (!fp->script) {
    fp = nsnull;
  }

  PRBool privileged;
  if (NS_SUCCEEDED(ssm->IsSystemPrincipal(principal, &privileged)) &&
      privileged) {
    
    return JS_TRUE;
  }

  
  
  const char *filename;
  if (fp &&
      (filename = fp->script->filename) &&
      !strncmp(filename, prefix, NS_ARRAY_LENGTH(prefix) - 1)) {
    return JS_TRUE;
  }

  
  nsresult rv = ssm->IsCapabilityEnabled("UniversalXPConnect", &privileged);
  if (NS_SUCCEEDED(rv) && privileged) {
    return JS_TRUE;
  }

  if (JSID_IS_VOID(id)) {
    ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
  } else {
    
    jsval idval;
    JSString *str;
    if (JS_IdToValue(cx, id, &idval) && (str = JS_ValueToString(cx, idval))) {
      JS_ReportError(cx, "Permission denied to access property '%hs' from a non-chrome context",
                     JS_GetStringChars(str));
    }
  }

  return JS_FALSE;
}

JSBool
CheckFilename(JSContext *cx, jsid id, JSStackFrame *fp)
{
  const char *filename;
  if (fp &&
      (filename = fp->script->filename) &&
      !strncmp(filename, prefix, NS_ARRAY_LENGTH(prefix) - 1)) {
    return JS_TRUE;
  }

  if (JSID_IS_VOID(id)) {
    ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
  } else {
    jsval idval;
    JSString *str;
    if (JS_IdToValue(cx, id, &idval) && (str = JS_ValueToString(cx, idval))) {
      JS_ReportError(cx, "Permission denied to access property '%hs' from a non-chrome context",
                     JS_GetStringChars(str));
    }
  }

  return JS_FALSE;
}

} 

using namespace SystemOnlyWrapper;



static inline JSObject *
GetWrappedJSObject(JSContext *cx, JSObject *obj)
{
  JSClass *clasp = obj->getJSClass();
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
  while (obj->getJSClass() != &SOWClass.base) {
    obj = obj->getProto();
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
  return UnwrapGeneric(cx, &SOWClass, wrapper);
}

static JSBool
XPC_SOW_FunctionWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                        jsval *rval)
{
  if (!AllowedToAct(cx, JSID_VOID)) {
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
  if (!JS_GetReservedSlot(cx, funObj, eWrappedFunctionSlot, &funToCall)) {
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
                            eWrappedFunctionSlot,
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
      
      
      if (wrapperObj->getProto() == obj->getParent()) {
        return JS_TRUE;
      }

      
      if (!JS_GetReservedSlot(cx, obj, eWrappedFunctionSlot, &v)) {
        return JS_FALSE;
      }
      obj = JSVAL_TO_OBJECT(v);
    }

    return XPC_SOW_WrapFunction(cx, wrapperObj, obj, vp);
  }

  if (obj->getJSClass() == &SOWClass.base) {
    
    
    
    
    if (wrapperObj->getParent() == obj->getParent()) {
      
      return JS_TRUE;
    }

    obj = GetWrappedObject(cx, obj);
    if (!obj) {
      
      *vp = JSVAL_NULL;
      return JS_TRUE;
    }
    v = *vp = OBJECT_TO_JSVAL(obj);
  }

  return WrapObject(cx, wrapperObj->getParent(), v, vp);
}

static JSBool
XPC_SOW_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  NS_ASSERTION(obj->getJSClass() == &SOWClass.base, "Wrong object");

  jsval resolving;
  if (!JS_GetReservedSlot(cx, obj, sFlagsSlot, &resolving)) {
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

  return AddProperty(cx, obj, JS_TRUE, wrappedObj, id, vp);
}

static JSBool
XPC_SOW_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  if (!AllowedToAct(cx, id)) {
    return JS_FALSE;
  }

  return DelProperty(cx, wrappedObj, id, vp);
}

static JSBool
XPC_SOW_GetOrSetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp,
                         JSBool isSet)
{
  obj = GetWrapper(obj);
  if (!obj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  if (!AllowedToAct(cx, id)) {
    return JS_FALSE;
  }

  js::AutoArrayRooter tvr(cx, 1, vp);

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  if (isSet && id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_PROTO)) {
    
    return ThrowException(NS_ERROR_INVALID_ARG, cx); 
  }

  JSBool ok = isSet
              ? JS_SetPropertyById(cx, wrappedObj, id, vp)
              : JS_GetPropertyById(cx, wrappedObj, id, vp);
  if (!ok) {
    return JS_FALSE;
  }

  return XPC_SOW_RewrapValue(cx, obj, vp);
}

static JSBool
XPC_SOW_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  return XPC_SOW_GetOrSetProperty(cx, obj, id, vp, JS_FALSE);
}

static JSBool
XPC_SOW_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
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

  if (!AllowedToAct(cx, JSID_VOID)) {
    return JS_FALSE;
  }

  return Enumerate(cx, obj, wrappedObj);
}

static JSBool
XPC_SOW_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
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

  return NewResolve(cx, obj, JS_FALSE, wrappedObj, id, flags, objp);
}

static JSBool
XPC_SOW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
  if (!AllowedToAct(cx, JSID_VOID)) {
    return JS_FALSE;
  }

  
  if (type == JSTYPE_OBJECT) {
    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
  }

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    
    return JS_TRUE;
  }

  return wrappedObj->getJSClass()->convert(cx, wrappedObj, type, vp);
}

static JSBool
XPC_SOW_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                    jsval *vp)
{
  
  

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    *vp = JSVAL_VOID;
    return JS_TRUE;
  }

  uintN junk;
  return JS_CheckAccess(cx, wrappedObj, id, mode, vp, &junk);
}

static JSBool
XPC_SOW_HasInstance(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp)
{
  if (!AllowedToAct(cx, JSID_VOID)) {
    return JS_FALSE;
  }

  JSObject *iface = GetWrappedObject(cx, obj);
  if (!iface) {
    *bp = JS_FALSE;
    return JS_TRUE;
  }

  JSClass *clasp = iface->getJSClass();

  *bp = JS_FALSE;
  if (!clasp->hasInstance) {
    return JS_TRUE;
  }

  
  
  
  jsval v = *valp;
  if (!JSVAL_IS_PRIMITIVE(v)) {
    JSObject *test = JSVAL_TO_OBJECT(v);

    
    test = GetWrappedObject(cx, test);
    if (test) {
      v = OBJECT_TO_JSVAL(test);
    }
  }

  return clasp->hasInstance(cx, iface, &v, bp);
}

static JSBool
XPC_SOW_Equality(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp)
{
  
  jsval v = *valp;
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
    
    JSClass *clasp = lhs->getJSClass();
    if (clasp->flags & JSCLASS_IS_EXTENDED) {
      JSExtendedClass *xclasp = (JSExtendedClass *) clasp;
      
      jsval rhsVal = OBJECT_TO_JSVAL(rhs);
      return xclasp->equality(cx, lhs, &rhsVal, bp);
    }
  }

  
  JSClass *clasp = rhs->getJSClass();
  if (clasp->flags & JSCLASS_IS_EXTENDED) {
    JSExtendedClass *xclasp = (JSExtendedClass *) clasp;
    
    jsval lhsVal = OBJECT_TO_JSVAL(lhs);
    return xclasp->equality(cx, rhs, &lhsVal, bp);
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

  JSObject *wrapperIter = JS_NewObject(cx, &SOWClass.base, nsnull,
                                       JS_GetGlobalForObject(cx, obj));
  if (!wrapperIter) {
    return nsnull;
  }

  js::AutoObjectRooter tvr(cx, wrapperIter);

  
  jsval v = OBJECT_TO_JSVAL(wrappedObj);
  if (!JS_SetReservedSlot(cx, wrapperIter, sWrappedObjSlot, v) ||
      !JS_SetReservedSlot(cx, wrapperIter, sFlagsSlot, JSVAL_ZERO)) {
    return nsnull;
  }

  return CreateIteratorObj(cx, wrapperIter, obj, wrappedObj, keysonly);
}

static JSObject *
XPC_SOW_WrappedObject(JSContext *cx, JSObject *obj)
{
  return GetWrappedObject(cx, obj);
}
