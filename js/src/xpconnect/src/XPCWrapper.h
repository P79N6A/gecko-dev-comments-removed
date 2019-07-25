









































#ifndef XPC_WRAPPER_H
#define XPC_WRAPPER_H 1

#include "xpcprivate.h"

namespace XPCNativeWrapper {





#define NATIVE_HAS_FLAG(_wn, _flag)                \
  ((_wn)->GetScriptableInfo() &&                   \
   (_wn)->GetScriptableInfo()->GetFlags()._flag())


JSBool
WrapFunction(JSContext* cx, JSObject* funobj, jsval *rval);



JSBool
RewrapValue(JSContext *cx, JSObject *obj, jsval v, jsval *rval);

} 

namespace XPCCrossOriginWrapper {


JSBool
WrapObject(JSContext *cx, JSObject *parent, jsval *vp,
           XPCWrappedNative *wn = nsnull);


JSBool
WrapFunction(JSContext *cx, JSObject *wrapperObj, JSObject *funobj,
             jsval *rval);



JSBool
RewrapIfNeeded(JSContext *cx, JSObject *wrapperObj, jsval *vp);


JSBool
WrapperMoved(JSContext *cx, XPCWrappedNative *innerObj,
             XPCWrappedNativeScope *newScope);

void
WindowNavigated(JSContext *cx, XPCWrappedNative *innerObj);




nsresult
CanAccessWrapper(JSContext *cx, JSObject *outerObj, JSObject *wrappedObj,
                 JSBool *privilegeEnabled);




inline JSBool
ClassNeedsXOW(const char *name)
{
  switch (*name) {
    case 'W':
      return strcmp(++name, "indow") == 0;
    case 'L':
      return strcmp(++name, "ocation") == 0;
    default:
      break;
  }

  return JS_FALSE;
}

} 

namespace ChromeObjectWrapper {

JSBool
WrapObject(JSContext *cx, JSObject *parent, jsval v, jsval *vp);

}

namespace XPCSafeJSObjectWrapper {

JSObject *
GetUnsafeObject(JSContext *cx, JSObject *obj);

JSBool
WrapObject(JSContext *cx, JSObject *scope, jsval v, jsval *vp);

PRBool
AttachNewConstructorObject(XPCCallContext &ccx, JSObject *aGlobalObject);

}

namespace SystemOnlyWrapper {

JSBool
WrapObject(JSContext *cx, JSObject *parent, jsval v, jsval *vp);

JSBool
MakeSOW(JSContext *cx, JSObject *obj);


JSBool
AllowedToAct(JSContext *cx, jsid id);

JSBool
CheckFilename(JSContext *cx, jsid id, JSStackFrame *fp);

}

namespace ChromeObjectWrapper    { extern js::Class COWClass; }
namespace XPCSafeJSObjectWrapper { extern js::Class SJOWClass; }
namespace SystemOnlyWrapper      { extern js::Class SOWClass; }
namespace XPCCrossOriginWrapper  { extern js::Class XOWClass; }

extern nsIScriptSecurityManager *gScriptSecurityManager;






namespace XPCWrapper {




extern const PRUint32 FLAG_RESOLVING;


extern const PRUint32 FLAG_SOW;




extern const PRUint32 LAST_FLAG;

inline JSBool
HAS_FLAGS(jsval v, PRUint32 flags)
{
  return (PRUint32(JSVAL_TO_INT(v)) & flags) != 0;
}





extern const PRUint32 sWrappedObjSlot;






extern const PRUint32 sFlagsSlot;




extern const PRUint32 sNumSlots;





extern JSNative sEvalNative;

enum FunctionObjectSlot {
  eWrappedFunctionSlot = 0,
  eAllAccessSlot = 1
};


extern const PRUint32 sSecMgrSetProp, sSecMgrGetProp;

inline jsval
GetFlags(JSContext *cx, JSObject *wrapper)
{
  jsval flags;
  JS_GetReservedSlot(cx, wrapper, sFlagsSlot, &flags);
  return flags;
}

inline void
SetFlags(JSContext *cx, JSObject *wrapper, jsval flags)
{
  JS_SetReservedSlot(cx, wrapper, sFlagsSlot, flags);
}

inline jsval
AddFlags(jsval origflags, PRInt32 newflags)
{
  return INT_TO_JSVAL(JSVAL_TO_INT(origflags) | newflags);
}

inline jsval
RemoveFlags(jsval origflags, PRInt32 oldflags)
{
  return INT_TO_JSVAL(JSVAL_TO_INT(origflags) & ~oldflags);
}




inline JSBool
DoThrowException(nsresult ex, JSContext *cx)
{
  XPCThrower::Throw(ex, cx);
  return JS_FALSE;
}




inline JSBool
FindEval(XPCCallContext &ccx, JSObject *obj)
{
  if (sEvalNative) {
    return JS_TRUE;
  }

  jsval eval_val;
  if (!::JS_GetProperty(ccx, obj, "eval", &eval_val)) {
    return DoThrowException(NS_ERROR_UNEXPECTED, ccx);
  }

  if (JSVAL_IS_PRIMITIVE(eval_val) ||
      !::JS_ObjectIsFunction(ccx, JSVAL_TO_OBJECT(eval_val))) {
    return DoThrowException(NS_ERROR_UNEXPECTED, ccx);
  }

  sEvalNative =
    ::JS_GetFunctionNative(ccx, ::JS_ValueToFunction(ccx, eval_val));

  if (!sEvalNative) {
    return DoThrowException(NS_ERROR_UNEXPECTED, ccx);
  }

  return JS_TRUE;
}




inline nsIScriptSecurityManager *
GetSecurityManager()
{
  return ::gScriptSecurityManager;
}





inline void
MaybePreserveWrapper(JSContext *cx, XPCWrappedNative *wn, uintN flags)
{
  if ((flags & JSRESOLVE_ASSIGNING)) {
    nsRefPtr<nsXPCClassInfo> ci;
    CallQueryInterface(wn->Native(), getter_AddRefs(ci));
    if (ci) {
      ci->PreserveWrapper(wn->Native());
    }
  }
}

inline JSBool
IsSecurityWrapper(JSObject *wrapper)
{
  return wrapper->isWrapper() || !!wrapper->getClass()->ext.wrappedObject;
}











JSObject *
Unwrap(JSContext *cx, JSObject *wrapper);




inline JSObject *
UnwrapGeneric(JSContext *cx, const js::Class *xclasp, JSObject *wrapper)
{
  if (wrapper->getClass() != xclasp) {
    return nsnull;
  }

  jsval v;
  if (!JS_GetReservedSlot(cx, wrapper, XPCWrapper::sWrappedObjSlot, &v)) {
    JS_ClearPendingException(cx);
    return nsnull;
  }

  if (JSVAL_IS_PRIMITIVE(v)) {
    return nsnull;
  }

  return JSVAL_TO_OBJECT(v);
}

inline JSObject *
UnwrapSOW(JSContext *cx, JSObject *wrapper)
{
  wrapper = UnwrapGeneric(cx, &SystemOnlyWrapper::SOWClass, wrapper);
  if (!wrapper) {
    return nsnull;
  }

  if (!SystemOnlyWrapper::AllowedToAct(cx, JSID_VOID)) {
    JS_ClearPendingException(cx);
    wrapper = nsnull;
  }

  return wrapper;
}




inline JSObject *
UnwrapXOW(JSContext *cx, JSObject *wrapper)
{
  JSObject *innerObj =
    UnwrapGeneric(cx, &XPCCrossOriginWrapper::XOWClass, wrapper);
  if (!innerObj) {
    return nsnull;
  }

  nsresult rv =
    XPCCrossOriginWrapper::CanAccessWrapper(cx, wrapper, innerObj, nsnull);
  if (NS_FAILED(rv)) {
    JS_ClearPendingException(cx);
    return nsnull;
  }

  return innerObj;
}

inline JSObject *
UnwrapCOW(JSContext *cx, JSObject *wrapper)
{
  wrapper = UnwrapGeneric(cx, &ChromeObjectWrapper::COWClass, wrapper);
  if (!wrapper) {
    return nsnull;
  }

  nsresult rv = XPCCrossOriginWrapper::CanAccessWrapper(cx, nsnull, wrapper, nsnull);
  if (NS_FAILED(rv)) {
    JS_ClearPendingException(cx);
    wrapper = nsnull;
  }

  return wrapper;
}





inline JSBool
RewrapIfDeepWrapper(JSContext *cx, JSObject *obj, jsval v, jsval *rval,
                    JSBool isNativeWrapper)
{
  *rval = v;
  return isNativeWrapper
         ? XPCNativeWrapper::RewrapValue(cx, obj, v, rval)
         : XPCCrossOriginWrapper::RewrapIfNeeded(cx, obj, rval);
}







inline JSBool
WrapFunction(JSContext *cx, JSObject *wrapperObj, JSObject *funobj, jsval *v,
             JSBool isNativeWrapper)
{
  return isNativeWrapper
         ? XPCNativeWrapper::WrapFunction(cx, funobj, v)
         : XPCCrossOriginWrapper::WrapFunction(cx, wrapperObj, funobj, v);
}





void
CheckWindow(XPCWrappedNative *wn);




JSBool
RewrapObject(JSContext *cx, JSObject *scope, JSObject *obj, WrapperType hint,
             jsval *vp);

JSObject *
UnsafeUnwrapSecurityWrapper(JSContext *cx, JSObject *obj);

JSBool
CreateWrapperFromType(JSContext *cx, JSObject *scope, XPCWrappedNative *wn,
                      WrapperType hint, jsval *vp);







JSObject *
CreateIteratorObj(JSContext *cx, JSObject *tempWrapper,
                  JSObject *wrapperObj, JSObject *innerObj,
                  JSBool keysonly);





JSObject *
CreateSimpleIterator(JSContext *cx, JSObject *scope, JSBool keysonly,
                     JSObject *propertyContainer);




JSBool
AddProperty(JSContext *cx, JSObject *wrapperObj,
            JSBool wantGetterSetter, JSObject *innerObj,
            jsid id, jsval *vp);




JSBool
DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);




JSBool
Enumerate(JSContext *cx, JSObject *wrapperObj, JSObject *innerObj);










JSBool
NewResolve(JSContext *cx, JSObject *wrapperObj, JSBool preserveVal,
           JSObject *innerObj, jsid id, uintN flags, JSObject **objp);






JSBool
ResolveNativeProperty(JSContext *cx, JSObject *wrapperObj,
                      JSObject *innerObj, XPCWrappedNative *wn,
                      jsid id, uintN flags, JSObject **objp,
                      JSBool isNativeWrapper);






JSBool
GetOrSetNativeProperty(JSContext *cx, JSObject *obj,
                       XPCWrappedNative *wrappedNative,
                       jsid id, jsval *vp, JSBool aIsSet,
                       JSBool isNativeWrapper);




JSBool
NativeToString(JSContext *cx, XPCWrappedNative *wrappedNative,
               uintN argc, jsval *argv, jsval *rval,
               JSBool isNativeWrapper);







JSBool
GetPropertyAttrs(JSContext *cx, JSObject *obj, jsid interned_id, uintN flags,
                 JSBool wantDetails, JSPropertyDescriptor *desc);

} 


#endif
