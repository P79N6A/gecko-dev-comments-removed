






































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
XPC_COW_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_COW_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_COW_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_COW_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_COW_Enumerate(JSContext *cx, JSObject *obj);

static JSBool
XPC_COW_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                   JSObject **objp);

static JSBool
XPC_COW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

static JSBool
XPC_COW_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                    jsval *vp);

static JSBool
XPC_COW_Equality(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp);

static JSObject *
XPC_COW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly);

static JSObject *
XPC_COW_WrappedObject(JSContext *cx, JSObject *obj);

static JSBool
WrapFunction(JSContext *cx, JSObject *scope, JSObject *funobj, jsval *vp);

using namespace XPCWrapper;

namespace ChromeObjectWrapper {

JSExtendedClass COWClass = {
  
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

JSBool
WrapObject(JSContext *cx, JSObject *parent, jsval v, jsval *vp)
{
  if (JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(v))) {
    return WrapFunction(cx, parent, JSVAL_TO_OBJECT(v), vp);
  }

  JSObject *wrapperObj =
    JS_NewObjectWithGivenProto(cx, &COWClass.base, NULL, parent);
  if (!wrapperObj) {
    return JS_FALSE;
  }

  *vp = OBJECT_TO_JSVAL(wrapperObj);

  js::AutoValueRooter exposedProps(cx, js::undefinedValue());

  if (!GetExposedProperties(cx, JSVAL_TO_OBJECT(v), exposedProps.jsval_addr())) {
    return JS_FALSE;
  }

  if (!JS_SetReservedSlot(cx, wrapperObj, XPCWrapper::sWrappedObjSlot, v) ||
      !JS_SetReservedSlot(cx, wrapperObj, XPCWrapper::sFlagsSlot, JSVAL_ZERO) ||
      !JS_SetReservedSlot(cx, wrapperObj, sExposedPropsSlot,
                          exposedProps.jsval_value())) {
    return JS_FALSE;
  }

  return JS_TRUE;
}

} 

using namespace ChromeObjectWrapper;


static inline JSBool
ThrowException(nsresult rv, JSContext *cx)
{
  return XPCWrapper::DoThrowException(rv, cx);
}




static inline JSObject *
GetWrappedJSObject(JSContext *cx, JSObject *obj)
{
  JSClass *clasp = obj->getJSClass();
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
  while (obj->getJSClass() != &COWClass.base) {
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
  return XPCWrapper::UnwrapGeneric(cx, &COWClass, wrapper);
}


JSBool
RewrapForChrome(JSContext *cx, JSObject *wrapperObj, jsval *vp);
JSBool
RewrapForContent(JSContext *cx, JSObject *wrapperObj, jsval *vp);



static JSBool
XPC_COW_FunctionWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                        jsval *rval)
{
  jsval funToCall;
  if (!JS_GetReservedSlot(cx, JSVAL_TO_OBJECT(argv[-2]),
                          XPCWrapper::eWrappedFunctionSlot, &funToCall)) {
    return JS_FALSE;
  }

  JSObject *scope = JS_GetGlobalForObject(cx, JSVAL_TO_OBJECT(funToCall));
  for (uintN i = 0; i < argc; ++i) {
    if (!JSVAL_IS_PRIMITIVE(argv[i]) &&
        !RewrapObject(cx, scope, JSVAL_TO_OBJECT(argv[i]), XPCNW_EXPLICIT,
                      &argv[i])) {
      return JS_FALSE;
    }
  }

  if (!RewrapObject(cx, scope, obj, XPCNW_EXPLICIT, rval) ||
      !JS_CallFunctionValue(cx, JSVAL_TO_OBJECT(*rval), funToCall, argc, argv,
                            rval)) {
    return JS_FALSE;
  }

  scope = JS_GetScopeChain(cx);
  if (!scope) {
    return JS_FALSE;
  }

  return JSVAL_IS_PRIMITIVE(*rval) ||
         RewrapObject(cx, JS_GetGlobalForObject(cx, scope),
                      JSVAL_TO_OBJECT(*rval), COW, rval);
}

static JSBool
WrapFunction(JSContext *cx, JSObject *scope, JSObject *funobj, jsval *rval)
{
  scope = JS_GetGlobalForObject(cx, scope);
  jsval funobjVal = OBJECT_TO_JSVAL(funobj);
  JSFunction *wrappedFun =
    reinterpret_cast<JSFunction *>(xpc_GetJSPrivate(funobj));
  JSNative native = JS_GetFunctionNative(cx, wrappedFun);
  if (native == XPC_COW_FunctionWrapper) {
    if (funobj->getParent() == scope) {
      *rval = funobjVal;
      return JS_TRUE;
    }

    JS_GetReservedSlot(cx, funobj, XPCWrapper::eWrappedFunctionSlot, &funobjVal);
  }

  JSFunction *funWrapper =
    JS_NewFunction(cx, XPC_COW_FunctionWrapper,
                   JS_GetFunctionArity(wrappedFun), 0,
                   scope,
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
RewrapForChrome(JSContext *cx, JSObject *wrapperObj, jsval *vp)
{
  jsval v = *vp;
  if (JSVAL_IS_PRIMITIVE(v)) {
    return JS_TRUE;
  }

  JSObject *scope =
    JS_GetGlobalForObject(cx, GetWrappedObject(cx, wrapperObj));
  return RewrapObject(cx, scope, JSVAL_TO_OBJECT(v), XPCNW_EXPLICIT, vp);
}

JSBool
RewrapForContent(JSContext *cx, JSObject *wrapperObj, jsval *vp)
{
  jsval v = *vp;
  if (JSVAL_IS_PRIMITIVE(v)) {
    return JS_TRUE;
  }

  JSObject *scope = JS_GetScopeChain(cx);
  if (!scope) {
    return JS_FALSE;
  }

  return RewrapObject(cx, JS_GetGlobalForObject(cx, scope),
                      JSVAL_TO_OBJECT(v), COW, vp);
}

static JSBool
CheckSOW(JSContext *cx, JSObject *wrapperObj, jsid id)
{
  jsval flags;
  JS_GetReservedSlot(cx, wrapperObj, sFlagsSlot, &flags);

  return HAS_FLAGS(flags, FLAG_SOW)
         ? SystemOnlyWrapper::AllowedToAct(cx, id) : JS_TRUE;
}

static JSBool
XPC_COW_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  obj = GetWrapper(obj);
  jsval flags;
  if (!JS_GetReservedSlot(cx, obj, XPCWrapper::sFlagsSlot, &flags)) {
    return JS_FALSE;
  }

  if (HAS_FLAGS(flags, FLAG_RESOLVING)) {
    
    return JS_TRUE;
  }

  if (!CheckSOW(cx, obj, id)) {
    return JS_FALSE;
  }

  
  
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  JSPropertyDescriptor desc;

  if (!XPCWrapper::GetPropertyAttrs(cx, obj, id, JSRESOLVE_QUALIFIED,
                                    JS_TRUE, &desc)) {
    return JS_FALSE;
  }

  NS_ASSERTION(desc.obj == obj, "The JS engine lies!");

  if (desc.attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
    
    
    if (!HAS_FLAGS(flags, FLAG_SOW) && !SystemOnlyWrapper::AllowedToAct(cx, id)) {
      return JS_FALSE;
    }
  }

  return RewrapForChrome(cx, obj, vp) &&
         JS_DefinePropertyById(cx, wrappedObj, id, *vp,
                               desc.getter, desc.setter, desc.attrs);
}

static JSBool
XPC_COW_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  if (!CheckSOW(cx, obj, id)) {
    return JS_FALSE;
  }

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  JSBool canTouch;
  if (!CanTouchProperty(cx, obj, id, JS_TRUE, &canTouch)) {
    return JS_FALSE;
  }

  if (!canTouch) {
    return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
  }

  
  return XPCWrapper::DelProperty(cx, wrappedObj, id, vp);
}

static JSBool
XPC_COW_GetOrSetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp,
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

  if (!CheckSOW(cx, obj, id)) {
    return JS_FALSE;
  }

  AUTO_MARK_JSVAL(ccx, vp);

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_PROTO) ||
      id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_EXPOSEDPROPS)) {
    
    return ThrowException(NS_ERROR_INVALID_ARG, cx); 
  }

  JSBool canTouch;
  if (!CanTouchProperty(cx, obj, id, isSet, &canTouch)) {
    return JS_FALSE;
  }

  if (!canTouch) {
    return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
  }

  if (isSet && !RewrapForChrome(cx, obj, vp)) {
    return JS_FALSE;
  }

  JSBool ok = isSet ? JS_SetPropertyById(cx, wrappedObj, id, vp)
                    : JS_GetPropertyById(cx, wrappedObj, id, vp);
  if (!ok) {
    return JS_FALSE;
  }

  return RewrapForContent(cx, obj, vp);
}

static JSBool
XPC_COW_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  return XPC_COW_GetOrSetProperty(cx, obj, id, vp, JS_FALSE);
}

static JSBool
XPC_COW_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
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

  if (!CheckSOW(cx, obj, JSID_VOID)) {
    return JS_FALSE;
  }

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  return XPCWrapper::Enumerate(cx, obj, wrappedObj);
}

static JSBool
XPC_COW_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                   JSObject **objp)
{
  obj = GetWrapper(obj);

  if (!CheckSOW(cx, obj, id)) {
    return JS_FALSE;
  }

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    *objp = nsnull;
    return JS_TRUE;
  }

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  JSBool canTouch;
  if (!CanTouchProperty(cx, obj, id, (flags & JSRESOLVE_ASSIGNING) != 0,
                        &canTouch)) {
    return JS_FALSE;
  }

  if (!canTouch) {
    return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
  }

  return XPCWrapper::NewResolve(cx, obj, JS_FALSE, wrappedObj, id, flags,
                                objp);
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

  if (!wrappedObj->getJSClass()->convert(cx, wrappedObj, type, vp)) {
    return JS_FALSE;
  }

  return RewrapForContent(cx, obj, vp);
}

static JSBool
XPC_COW_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                    jsval *vp)
{
  
  

  uintN junk;
  return JS_CheckAccess(cx, GetWrappedObject(cx, obj), id, mode, vp, &junk);
}

static JSBool
XPC_COW_Equality(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp)
{
  
  if (JSVAL_IS_PRIMITIVE(*valp)) {
    *bp = JS_FALSE;
    return JS_TRUE;
  }

  JSObject *test = GetWrappedJSObject(cx, JSVAL_TO_OBJECT(*valp));

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
  jsval testVal = OBJECT_TO_JSVAL(test);
  return ((JSExtendedClass *)obj->getJSClass())->
    equality(cx, obj, &testVal, bp);
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

  jsval exposedProps;
  if (!JS_GetReservedSlot(cx, obj, sExposedPropsSlot, &exposedProps)) {
    return JS_FALSE;
  }

  JSObject *propertyContainer;
  if (JSVAL_IS_VOID(exposedProps)) {
    
    propertyContainer = wrappedObj;
  } else if (JSVAL_IS_PRIMITIVE(exposedProps)) {
    
    propertyContainer = nsnull;
  } else {
    
    propertyContainer = JSVAL_TO_OBJECT(exposedProps);
  }

  return CreateSimpleIterator(cx, obj, keysonly, propertyContainer);
}

static JSObject *
XPC_COW_WrappedObject(JSContext *cx, JSObject *obj)
{
  return GetWrappedObject(cx, obj);
}
