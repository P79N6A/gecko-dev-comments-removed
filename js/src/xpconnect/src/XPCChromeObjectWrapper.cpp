






































#include "xpcprivate.h"
#include "nsDOMError.h"
#include "jsdbgapi.h"
#include "jscntxt.h"  
#include "jsobj.h"
#include "XPCNativeWrapper.h"
#include "XPCWrapper.h"




namespace {

const PRUint32 sPropIsReadable = 0x1;
const PRUint32 sPropIsWritable = 0x2;

const PRUint32 sExposedPropsSlot = XPCWrapper::sNumSlots;

class AutoIdArray {
public:
  AutoIdArray(JSContext *cx, JSIdArray *ida) : cx(cx), ida(ida) {
  }

  ~AutoIdArray() {
    if (ida) {
      JS_DestroyIdArray(cx, ida);
    }
  }

  JSIdArray *array() {
    return ida;
  }

private:
  JSContext *cx;
  JSIdArray *ida;
};

JSBool
GetExposedProperties(JSContext *cx, JSObject *obj, jsval *rval)
{
  jsid exposedPropsId = GetRTIdByIndex(cx, XPCJSRuntime::IDX_EXPOSEDPROPS);

  JSBool found = JS_FALSE;
  if (!JS_HasPropertyById(cx, obj, exposedPropsId, &found))
    return JS_FALSE;
  if (!found) {
    *rval = JSVAL_VOID;
    return JS_TRUE;
  }

  *rval = JSVAL_NULL;
  jsval exposedProps;
  if (!JS_LookupPropertyById(cx, obj, exposedPropsId, &exposedProps))
    return JS_FALSE;

  if (JSVAL_IS_VOID(exposedProps) || JSVAL_IS_NULL(exposedProps))
    return JS_TRUE;

  if (!JSVAL_IS_OBJECT(exposedProps)) {
    JS_ReportError(cx,
                   "__exposedProps__ must be undefined, null, or an Object");
    return JS_FALSE;
  }

  obj = JSVAL_TO_OBJECT(exposedProps);

  AutoIdArray guard(cx, JS_Enumerate(cx, obj));
  JSIdArray *props = guard.array();
  if (!props)
    return JS_FALSE;

  if (props->length == 0)
    return JS_TRUE;

  JSObject *info = JS_NewObjectWithGivenProto(cx, NULL, NULL, obj);
  if (!info)
    return JS_FALSE;
  *rval = OBJECT_TO_JSVAL(info);

  for (int i = 0; i < props->length; i++) {
    jsid propId = props->vector[i];

    jsval propVal;
    if (!JS_LookupPropertyById(cx, obj, propId, &propVal))
      return JS_FALSE;

    if (!JSVAL_IS_STRING(propVal)) {
      JS_ReportError(cx, "property must be a string");
      return JS_FALSE;
    }

    JSString *str = JSVAL_TO_STRING(propVal);
    const jschar *chars = JS_GetStringChars(str);
    size_t length = JS_GetStringLength(str);
    int32 propPerms = 0;
    for (size_t i = 0; i < length; ++i) {
      switch (chars[i]) {
        case 'r':
          if (propPerms & sPropIsReadable) {
            JS_ReportError(cx, "duplicate 'readable' property flag");
            return JS_FALSE;
          }
          propPerms |= sPropIsReadable;
          break;

        case 'w':
          if (propPerms & sPropIsWritable) {
            JS_ReportError(cx, "duplicate 'writable' property flag");
            return JS_FALSE;
          }
          propPerms |= sPropIsWritable;
          break;

        default:
          JS_ReportError(cx, "properties can only be readable or read and writable");
          return JS_FALSE;
      }
    }

    if (propPerms == 0) {
      JS_ReportError(cx, "specified properties must have a permission bit set");
      return JS_FALSE;
    }

    if (!JS_DefinePropertyById(cx, info, propId, INT_TO_JSVAL(propPerms),
                               NULL, NULL,
                               JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)) {
      return JS_FALSE;
    }
  }

  return JS_TRUE;
}

JSBool
CanTouchProperty(JSContext *cx, JSObject *wrapperObj, jsid id, JSBool isSet,
                 JSBool *allowedp)
{
  jsval exposedProps;
  if (!JS_GetReservedSlot(cx, wrapperObj, sExposedPropsSlot, &exposedProps)) {
    return JS_FALSE;
  }

  if (JSVAL_IS_PRIMITIVE(exposedProps)) {
    
    
    
    
    
    *allowedp = JSVAL_IS_VOID(exposedProps);
    return JS_TRUE;
  }

  JSObject *hash = JSVAL_TO_OBJECT(exposedProps);

  jsval allowedval;
  if (!JS_LookupPropertyById(cx, hash, id, &allowedval)) {
    return JS_FALSE;
  }

  const PRUint32 wanted = isSet ? sPropIsWritable : sPropIsReadable;

  
  *allowedp = JSVAL_IS_INT(allowedval) &&
              (JSVAL_TO_INT(allowedval) & wanted) != 0;

  return JS_TRUE;
}

}

static JSBool
XPC_COW_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
XPC_COW_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
XPC_COW_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
XPC_COW_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
XPC_COW_Enumerate(JSContext *cx, JSObject *obj);

static JSBool
XPC_COW_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                   JSObject **objp);

static JSBool
XPC_COW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

static JSBool
XPC_COW_CheckAccess(JSContext *cx, JSObject *obj, jsval id, JSAccessMode mode,
                    jsval *vp);

static JSBool
XPC_COW_Equality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

static JSObject *
XPC_COW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly);

static JSObject *
XPC_COW_WrappedObject(JSContext *cx, JSObject *obj);

JSExtendedClass sXPC_COW_JSClass = {
  
  { "ChromeObjectWrapper",
    JSCLASS_NEW_RESOLVE | JSCLASS_IS_EXTENDED |
    JSCLASS_HAS_RESERVED_SLOTS(XPCWrapper::sNumSlots + 1),
    XPC_COW_AddProperty, XPC_COW_DelProperty,
    XPC_COW_GetProperty, XPC_COW_SetProperty,
    XPC_COW_Enumerate,   (JSResolveOp)XPC_COW_NewResolve,
    XPC_COW_Convert,     JS_FinalizeStub,
    nsnull,              XPC_COW_CheckAccess,
    nsnull,              nsnull,
    nsnull,              nsnull,
    nsnull,              nsnull
  },

  
  XPC_COW_Equality,
  nsnull,             
  nsnull,             
  XPC_COW_Iterator,
  XPC_COW_WrappedObject,
  JSCLASS_NO_RESERVED_MEMBERS
};


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
    if (XPCNativeWrapper::IsNativeWrapper(obj)) {
      XPCWrappedNative *wn = XPCNativeWrapper::SafeGetWrappedNative(obj);
      return wn ? wn->GetFlatJSObject() : nsnull;
    }

    return obj;
  }

  return xclasp->wrappedObject(cx, obj);
}



static inline
JSObject *
GetWrapper(JSObject *obj)
{
  while (STOBJ_GET_CLASS(obj) != &sXPC_COW_JSClass.base) {
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
  return XPCWrapper::UnwrapGeneric(cx, &sXPC_COW_JSClass, wrapper);
}


JSBool
XPC_COW_RewrapForChrome(JSContext *cx, JSObject *wrapperObj, jsval *vp);
JSBool
XPC_COW_RewrapForContent(JSContext *cx, JSObject *wrapperObj, jsval *vp);



static JSBool
XPC_COW_FunctionWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                        jsval *rval)
{
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

  jsval funToCall;
  if (!JS_GetReservedSlot(cx, JSVAL_TO_OBJECT(argv[-2]),
                          XPCWrapper::eWrappedFunctionSlot, &funToCall)) {
    return JS_FALSE;
  }

  for (uintN i = 0; i < argc; ++i) {
    if (!XPC_COW_RewrapForChrome(cx, obj, &argv[i])) {
      return JS_FALSE;
    }
  }

  if (!JS_CallFunctionValue(cx, wrappedObj, funToCall, argc, argv, rval)) {
    return JS_FALSE;
  }

  return XPC_COW_RewrapForContent(cx, obj, rval);
}

JSBool
XPC_COW_WrapFunction(JSContext *cx, JSObject *outerObj, JSObject *funobj,
                     jsval *rval)
{
  jsval funobjVal = OBJECT_TO_JSVAL(funobj);
  JSFunction *wrappedFun =
    reinterpret_cast<JSFunction *>(xpc_GetJSPrivate(funobj));
  JSNative native = JS_GetFunctionNative(cx, wrappedFun);
  if (!native || native == XPC_COW_FunctionWrapper) {
    *rval = funobjVal;
    return JS_TRUE;
  }

  JSFunction *funWrapper =
    JS_NewFunction(cx, XPC_COW_FunctionWrapper,
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

JSBool
XPC_COW_RewrapForChrome(JSContext *cx, JSObject *wrapperObj, jsval *vp)
{
  jsval v = *vp;
  if (JSVAL_IS_PRIMITIVE(v)) {
    return JS_TRUE;
  }

  
  JSObject *obj = GetWrappedJSObject(cx, JSVAL_TO_OBJECT(v));
  if (!obj) {
    *vp = JSVAL_NULL;
    return JS_TRUE;
  }

  XPCWrappedNative *wn;
  if (IS_WRAPPER_CLASS(STOBJ_GET_CLASS(obj)) &&
      (wn = XPCWrappedNative::GetWrappedNativeOfJSObject(cx, obj)) &&
      !nsXPCWrappedJSClass::IsWrappedJS(wn->Native())) {
    
    
    return XPCNativeWrapper::CreateExplicitWrapper(cx, wn, JS_TRUE, vp);
  }

  return XPC_SJOW_Construct(cx, obj, 1, vp, vp);
}

JSBool
XPC_COW_RewrapForContent(JSContext *cx, JSObject *wrapperObj, jsval *vp)
{
  jsval v = *vp;
  if (JSVAL_IS_PRIMITIVE(v)) {
    return JS_TRUE;
  }

  JSObject *obj = GetWrappedJSObject(cx, JSVAL_TO_OBJECT(v));
  if (!obj) {
    *vp = JSVAL_NULL;
    return JS_TRUE;
  }

  if (JS_ObjectIsFunction(cx, obj)) {
    return XPC_COW_WrapFunction(cx, wrapperObj, obj, vp);
  }

  return XPC_COW_WrapObject(cx, JS_GetScopeChain(cx), OBJECT_TO_JSVAL(obj),
                            vp);
}

static JSBool
XPC_COW_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  obj = GetWrapper(obj);
  jsval resolving;
  if (!JS_GetReservedSlot(cx, obj, XPCWrapper::sFlagsSlot, &resolving)) {
    return JS_FALSE;
  }

  if (HAS_FLAGS(resolving, FLAG_RESOLVING)) {
    
    return JS_TRUE;
  }

  
  
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  jsid interned_id;
  JSPropertyDescriptor desc;

  if (!JS_ValueToId(cx, id, &interned_id) ||
      !XPCWrapper::GetPropertyAttrs(cx, obj, interned_id, JSRESOLVE_QUALIFIED,
                                    JS_TRUE, &desc)) {
    return JS_FALSE;
  }

  NS_ASSERTION(desc.obj == obj, "The JS engine lies!");

  if (desc.attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
    
    if (!AllowedToAct(cx, id)) {
      return JS_FALSE;
    }
  }

  return XPC_COW_RewrapForChrome(cx, obj, vp) &&
         JS_DefinePropertyById(cx, wrappedObj, interned_id, *vp,
                               desc.getter, desc.setter, desc.attrs);
}

static JSBool
XPC_COW_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  
  return XPCWrapper::DelProperty(cx, wrappedObj, id, vp);
}

static JSBool
XPC_COW_GetOrSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp,
                         JSBool isSet)
{
  obj = GetWrapper(obj);
  if (!obj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  AUTO_MARK_JSVAL(ccx, vp);

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  jsid interned_id;
  if (!JS_ValueToId(cx, id, &interned_id)) {
    return JS_FALSE;
  }

  if (interned_id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_PROTO) ||
      interned_id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_PARENT) ||
      interned_id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_EXPOSEDPROPS)) {
    
    return ThrowException(NS_ERROR_INVALID_ARG, cx); 
  }

  JSBool canTouch;
  if (!CanTouchProperty(cx, obj, interned_id, isSet, &canTouch)) {
    return JS_FALSE;
  }

  if (!canTouch) {
    return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
  }

  if (!XPC_COW_RewrapForChrome(cx, obj, vp)) {
    return JS_FALSE;
  }

  JSBool ok = isSet
              ? JS_SetPropertyById(cx, wrappedObj, interned_id, vp)
              : JS_GetPropertyById(cx, wrappedObj, interned_id, vp);
  if (!ok) {
    return JS_FALSE;
  }

  return XPC_COW_RewrapForContent(cx, obj, vp);
}

static JSBool
XPC_COW_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  return XPC_COW_GetOrSetProperty(cx, obj, id, vp, JS_FALSE);
}

static JSBool
XPC_COW_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  return XPC_COW_GetOrSetProperty(cx, obj, id, vp, JS_TRUE);
}

static JSBool
XPC_COW_Enumerate(JSContext *cx, JSObject *obj)
{
  obj = GetWrapper(obj);
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    return JS_TRUE;
  }

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  return XPCWrapper::Enumerate(cx, obj, wrappedObj);
}

static JSBool
XPC_COW_NewResolve(JSContext *cx, JSObject *obj, jsval idval, uintN flags,
                   JSObject **objp)
{
  obj = GetWrapper(obj);

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    *objp = nsnull;
    return JS_TRUE;
  }

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  jsid id;
  JSBool canTouch;
  if (!JS_ValueToId(cx, idval, &id) &&
      !CanTouchProperty(cx, obj, id, (flags & JSRESOLVE_ASSIGNING) != 0,
                        &canTouch)) {
    return JS_FALSE;
  }

  if (!canTouch) {
    return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
  }

  return XPCWrapper::NewResolve(cx, obj, JS_TRUE, wrappedObj, id, flags, objp);
}

static JSBool
XPC_COW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
  
  if (type == JSTYPE_OBJECT) {
    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
  }

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    

    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
  }

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  if (!STOBJ_GET_CLASS(wrappedObj)->convert(cx, wrappedObj, type, vp)) {
    return JS_FALSE;
  }

  return XPC_COW_RewrapForContent(cx, obj, vp);
}

static JSBool
XPC_COW_CheckAccess(JSContext *cx, JSObject *obj, jsval prop, JSAccessMode mode,
                    jsval *vp)
{
  
  

  uintN junk;
  jsid id;
  return JS_ValueToId(cx, prop, &id) &&
         JS_CheckAccess(cx, GetWrappedObject(cx, obj), id, mode, vp, &junk);
}

static JSBool
XPC_COW_Equality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
  
  if (JSVAL_IS_PRIMITIVE(v)) {
    *bp = JS_FALSE;
    return JS_TRUE;
  }

  JSObject *test = GetWrappedJSObject(cx, JSVAL_TO_OBJECT(v));

  obj = GetWrappedObject(cx, obj);
  if (!obj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }
  XPCWrappedNative *other =
    XPCWrappedNative::GetWrappedNativeOfJSObject(cx, test);
  if (!other) {
    *bp = JS_FALSE;
    return JS_TRUE;
  }

  XPCWrappedNative *me = XPCWrappedNative::GetWrappedNativeOfJSObject(cx, obj);
  obj = me->GetFlatJSObject();
  test = other->GetFlatJSObject();
  return ((JSExtendedClass *)STOBJ_GET_CLASS(obj))->
    equality(cx, obj, OBJECT_TO_JSVAL(test), bp);
}

static JSObject *
XPC_COW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly)
{
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    ThrowException(NS_ERROR_INVALID_ARG, cx);
    return nsnull;
  }

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    ThrowException(NS_ERROR_FAILURE, cx);
    return nsnull;
  }

  JSObject *wrapperIter = JS_NewObject(cx, &sXPC_COW_JSClass.base, nsnull,
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
XPC_COW_WrappedObject(JSContext *cx, JSObject *obj)
{
  return GetWrappedObject(cx, obj);
}

JSBool
XPC_COW_WrapObject(JSContext *cx, JSObject *parent, jsval v, jsval *vp)
{
  JSObject *wrapperObj =
    JS_NewObjectWithGivenProto(cx, &sXPC_COW_JSClass.base, NULL, parent);
  if (!wrapperObj) {
    return JS_FALSE;
  }

  *vp = OBJECT_TO_JSVAL(wrapperObj);

  jsval exposedProps = JSVAL_VOID;
  JSAutoTempValueRooter tvr(cx, 1, &exposedProps);

  if (!GetExposedProperties(cx, JSVAL_TO_OBJECT(v), &exposedProps)) {
    return JS_FALSE;
  }

  if (!JS_SetReservedSlot(cx, wrapperObj, XPCWrapper::sWrappedObjSlot, v) ||
      !JS_SetReservedSlot(cx, wrapperObj, XPCWrapper::sFlagsSlot,
                          JSVAL_ZERO) ||
      !JS_SetReservedSlot(cx, wrapperObj, sExposedPropsSlot, exposedProps)) {
    return JS_FALSE;
  }

  return JS_TRUE;
}
