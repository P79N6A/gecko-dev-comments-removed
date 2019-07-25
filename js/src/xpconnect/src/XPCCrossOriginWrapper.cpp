






































#include "xpcprivate.h"
#include "nsDOMError.h"
#include "jsdbgapi.h"
#include "jscntxt.h"  
#include "XPCWrapper.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowCollection.h"




static JSBool
XPC_XOW_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_XOW_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_XOW_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_XOW_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_XOW_Enumerate(JSContext *cx, JSObject *obj);

static JSBool
XPC_XOW_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                   JSObject **objp);

static JSBool
XPC_XOW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

static void
XPC_XOW_Finalize(JSContext *cx, JSObject *obj);

static JSBool
XPC_XOW_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                    jsval *vp);

static JSBool
XPC_XOW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

static JSBool
XPC_XOW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                  jsval *rval);

static JSBool
XPC_XOW_HasInstance(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);

static JSBool
XPC_XOW_Equality(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);

static JSObject *
XPC_XOW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly);

static JSObject *
XPC_XOW_WrappedObject(JSContext *cx, JSObject *obj);










static const int XPC_XOW_ScopeSlot = XPCWrapper::sNumSlots;
static const int sUXPCObjectSlot = XPCWrapper::sNumSlots + 1;

using namespace XPCWrapper;


static inline
JSBool
ThrowException(nsresult ex, JSContext *cx)
{
  XPCWrapper::DoThrowException(ex, cx);

  return JS_FALSE;
}


static inline
JSObject *
GetWrapper(JSObject *obj)
{
  while (obj->getJSClass() != &XPCCrossOriginWrapper::XOWClass.base) {
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
  return XPCWrapper::UnwrapGeneric(cx, &XPCCrossOriginWrapper::XOWClass, wrapper);
}

static JSBool
XPC_XOW_FunctionWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                        jsval *rval);



static const PRUint32 FLAG_IS_UXPC_OBJECT = XPCWrapper::LAST_FLAG << 1;



static const PRUint32 FLAG_IS_CACHED = XPCWrapper::LAST_FLAG << 2;

namespace XPCCrossOriginWrapper {

JSExtendedClass XOWClass = {
  
  { "XPCCrossOriginWrapper",
    JSCLASS_NEW_RESOLVE | JSCLASS_IS_EXTENDED |
    JSCLASS_HAS_RESERVED_SLOTS(XPCWrapper::sNumSlots + 2),
    XPC_XOW_AddProperty, XPC_XOW_DelProperty,
    XPC_XOW_GetProperty, XPC_XOW_SetProperty,
    XPC_XOW_Enumerate,   (JSResolveOp)XPC_XOW_NewResolve,
    XPC_XOW_Convert,     XPC_XOW_Finalize,
    nsnull,              XPC_XOW_CheckAccess,
    XPC_XOW_Call,        XPC_XOW_Construct,
    nsnull,              XPC_XOW_HasInstance,
    nsnull,              nsnull
  },

  
  XPC_XOW_Equality,
  nsnull,             
  nsnull,             
  XPC_XOW_Iterator,
  XPC_XOW_WrappedObject,
  JSCLASS_NO_RESERVED_MEMBERS
};

JSBool
WrapperMoved(JSContext *cx, XPCWrappedNative *innerObj,
                     XPCWrappedNativeScope *newScope)
{
  typedef WrappedNative2WrapperMap::Link Link;
  XPCJSRuntime *rt = nsXPConnect::GetRuntimeInstance();
  WrappedNative2WrapperMap *map = innerObj->GetScope()->GetWrapperMap();
  Link *link;

  { 
    XPCAutoLock al(rt->GetMapLock());
    link = map->FindLink(innerObj->GetFlatJSObject());
  }

  if (!link) {
    
    return JS_TRUE;
  }

  JSObject *xow = link->obj;

  { 
    XPCAutoLock al(rt->GetMapLock());
    if (!newScope->GetWrapperMap()->AddLink(innerObj->GetFlatJSObject(), link))
      return JS_FALSE;
    map->Remove(innerObj->GetFlatJSObject());
  }

  if (!xow) {
    
    return JS_TRUE;
  }

  return JS_SetReservedSlot(cx, xow, XPC_XOW_ScopeSlot,
                            PRIVATE_TO_JSVAL(newScope)) &&
         JS_SetParent(cx, xow, newScope->GetGlobalJSObject());
}

void
WindowNavigated(JSContext *cx, XPCWrappedNative *innerObj)
{
  NS_ABORT_IF_FALSE(innerObj->NeedsXOW(), "About to write to unowned memory");

  
  XPCWrappedNativeWithXOW *wnxow =
    static_cast<XPCWrappedNativeWithXOW *>(innerObj);
  JSObject *oldXOW = wnxow->GetXOW();
  if (oldXOW) {
    jsval flags = GetFlags(cx, oldXOW);
    NS_ASSERTION(HAS_FLAGS(flags, FLAG_IS_CACHED), "Wrapper should be cached");

    SetFlags(cx, oldXOW, RemoveFlags(flags, FLAG_IS_CACHED));

    NS_ASSERTION(wnxow->GetXOW() == oldXOW, "bad XOW in cache");
    wnxow->SetXOW(nsnull);
  }
}







nsresult
CanAccessWrapper(JSContext *cx, JSObject *outerObj, JSObject *wrappedObj,
                 JSBool *privilegeEnabled)
{
  
  
  

  if (privilegeEnabled) {
    *privilegeEnabled = JS_FALSE;
  }

  if (outerObj) {
    JSObject *outerParent = outerObj->getParent();
    JSObject *innerParent = wrappedObj->getParent();
    if (!innerParent) {
      innerParent = wrappedObj;
      OBJ_TO_INNER_OBJECT(cx, innerParent);
      if (!innerParent) {
        return NS_ERROR_FAILURE;
      }
    } else {
      innerParent = JS_GetGlobalForObject(cx, innerParent);
    }

    if (outerParent == innerParent) {
      return NS_OK;
    }
  }

  
  
  nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
  if (!ssm) {
    ThrowException(NS_ERROR_NOT_INITIALIZED, cx);
    return NS_ERROR_NOT_INITIALIZED;
  }

  JSStackFrame *fp = nsnull;
  nsIPrincipal *subjectPrin = ssm->GetCxSubjectPrincipalAndFrame(cx, &fp);

  if (!subjectPrin) {
    ThrowException(NS_ERROR_FAILURE, cx);
    return NS_ERROR_FAILURE;
  }

  PRBool isSystem = PR_FALSE;
  nsresult rv = ssm->IsSystemPrincipal(subjectPrin, &isSystem);
  NS_ENSURE_SUCCESS(rv, rv);

  if (privilegeEnabled) {
    *privilegeEnabled = JS_FALSE;
  }

  nsCOMPtr<nsIPrincipal> objectPrin;
  rv = ssm->GetObjectPrincipal(cx, wrappedObj, getter_AddRefs(objectPrin));
  if (NS_FAILED(rv)) {
    return rv;
  }
  NS_ASSERTION(objectPrin, "Object didn't have principals?");

  
  if (subjectPrin == objectPrin) {
    return NS_OK;
  }

  
  PRBool subsumes;
  rv = subjectPrin->Subsumes(objectPrin, &subsumes);
  if (NS_SUCCEEDED(rv) && !subsumes) {
    
    
    rv = ssm->IsCapabilityEnabled("UniversalXPConnect", &isSystem);
    if (NS_SUCCEEDED(rv) && isSystem) {
      rv = NS_OK;
      if (privilegeEnabled) {
        *privilegeEnabled = JS_TRUE;
      }
    } else {
      rv = NS_ERROR_DOM_PROP_ACCESS_DENIED;
    }
  }
  return rv;
}

JSBool
WrapFunction(JSContext *cx, JSObject *outerObj, JSObject *funobj, jsval *rval)
{
  jsval funobjVal = OBJECT_TO_JSVAL(funobj);
  JSFunction *wrappedFun =
    reinterpret_cast<JSFunction *>(xpc_GetJSPrivate(funobj));
  JSNative native = JS_GetFunctionNative(cx, wrappedFun);
  if (!native || native == XPC_XOW_FunctionWrapper) {
    *rval = funobjVal;
    return JS_TRUE;
  }

  JSFunction *funWrapper =
    JS_NewFunction(cx, XPC_XOW_FunctionWrapper,
                   JS_GetFunctionArity(wrappedFun), 0,
                   JS_GetGlobalForObject(cx, outerObj),
                   JS_GetFunctionName(wrappedFun));
  if (!funWrapper) {
    return JS_FALSE;
  }

  JSObject *funWrapperObj = JS_GetFunctionObject(funWrapper);
  *rval = OBJECT_TO_JSVAL(funWrapperObj);

  if (!JS_SetReservedSlot(cx, funWrapperObj, eWrappedFunctionSlot, funobjVal) ||
      !JS_SetReservedSlot(cx, funWrapperObj, eAllAccessSlot, JSVAL_FALSE)) {
    return JS_FALSE;
  }

  return JS_TRUE;
}

JSBool
RewrapIfNeeded(JSContext *cx, JSObject *outerObj, jsval *vp)
{
  
  if (JSVAL_IS_PRIMITIVE(*vp)) {
    return JS_TRUE;
  }

  JSObject *obj = JSVAL_TO_OBJECT(*vp);

  if (JS_ObjectIsFunction(cx, obj)) {
    return WrapFunction(cx, outerObj, obj, vp);
  }

  XPCWrappedNative *wn = nsnull;
  if (obj->getJSClass() == &XOWClass.base &&
      outerObj->getParent() != obj->getParent()) {
    *vp = OBJECT_TO_JSVAL(GetWrappedObject(cx, obj));
  } else if (!(wn = XPCWrappedNative::GetAndMorphWrappedNativeOfJSObject(cx, obj))) {
    return JS_TRUE;
  }

  return WrapObject(cx, JS_GetGlobalForObject(cx, outerObj), vp, wn);
}

JSBool
WrapObject(JSContext *cx, JSObject *parent, jsval *vp, XPCWrappedNative* wn)
{
  NS_ASSERTION(XPCPerThreadData::IsMainThread(cx),
               "Can't do this off the main thread!");

  
  
  JSObject *wrappedObj;
  if (JSVAL_IS_PRIMITIVE(*vp) ||
      !(wrappedObj = JSVAL_TO_OBJECT(*vp)) ||
      wrappedObj->getJSClass() == &XOWClass.base) {
    return JS_TRUE;
  }

  if (!wn &&
      !(wn = XPCWrappedNative::GetAndMorphWrappedNativeOfJSObject(cx, wrappedObj))) {
    return JS_TRUE;
  }

  
  parent = JS_GetGlobalForObject(cx, parent);
  OBJ_TO_INNER_OBJECT(cx, parent);
  if (!parent) {
    return JS_FALSE;
  }

  XPCWrappedNativeWithXOW *wnxow = nsnull;
  if (wn->NeedsXOW()) {
    JSObject *innerWrappedObj = wrappedObj;
    OBJ_TO_INNER_OBJECT(cx, innerWrappedObj);
    if (!innerWrappedObj) {
      return JS_FALSE;
    }

    if (innerWrappedObj == parent) {
      wnxow = static_cast<XPCWrappedNativeWithXOW *>(wn);
      JSObject *xow = wnxow->GetXOW();
      if (xow) {
        *vp = OBJECT_TO_JSVAL(xow);
        return JS_TRUE;
      }
    }
  }

  XPCWrappedNative *parentwn =
    XPCWrappedNative::GetWrappedNativeOfJSObject(cx, parent);
  XPCWrappedNativeScope *parentScope;
  if (NS_LIKELY(parentwn)) {
    parentScope = parentwn->GetScope();
  } else {
    parentScope = XPCWrappedNativeScope::FindInJSObjectScope(cx, parent);
  }

  JSObject *outerObj = nsnull;
  WrappedNative2WrapperMap *map = parentScope->GetWrapperMap();

  outerObj = map->Find(wrappedObj);
  if (outerObj) {
    NS_ASSERTION(outerObj->getJSClass() == &XOWClass.base,
                 "What crazy object are we getting here?");
    *vp = OBJECT_TO_JSVAL(outerObj);

    if (wnxow) {
      
      SetFlags(cx, outerObj, AddFlags(GetFlags(cx, outerObj), FLAG_IS_CACHED));
      wnxow->SetXOW(outerObj);
    }

    return JS_TRUE;
  }

  outerObj = JS_NewObjectWithGivenProto(cx, &XOWClass.base, nsnull,
                                        parent);
  if (!outerObj) {
    return JS_FALSE;
  }

  jsval flags = INT_TO_JSVAL(wnxow ? FLAG_IS_CACHED : 0);
  if (!JS_SetReservedSlot(cx, outerObj, sWrappedObjSlot, *vp) ||
      !JS_SetReservedSlot(cx, outerObj, sFlagsSlot, flags) ||
      !JS_SetReservedSlot(cx, outerObj, XPC_XOW_ScopeSlot,
                          PRIVATE_TO_JSVAL(parentScope))) {
    return JS_FALSE;
  }

  *vp = OBJECT_TO_JSVAL(outerObj);

  map->Add(wn->GetScope()->GetWrapperMap(), wrappedObj, outerObj);
  if(wnxow) {
    wnxow->SetXOW(outerObj);
  }

  return JS_TRUE;
}

} 

using namespace XPCCrossOriginWrapper;

static JSBool
XPC_XOW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval);

static JSBool
IsValFrame(JSObject *obj, jsid id, XPCWrappedNative *wn)
{
  
  if (obj->getClass()->name[0] != 'W') {
    return JS_FALSE;
  }

  nsCOMPtr<nsIDOMWindow> domwin(do_QueryWrappedNative(wn));
  if (!domwin) {
    return JS_FALSE;
  }

  nsCOMPtr<nsIDOMWindowCollection> col;
  domwin->GetFrames(getter_AddRefs(col));
  if (!col) {
    return JS_FALSE;
  }

  if (JSID_IS_INT(id)) {
    col->Item(JSID_TO_INT(id), getter_AddRefs(domwin));
  } else {
    nsAutoString str(reinterpret_cast<PRUnichar *>
                                     (JS_GetStringChars(JSID_TO_STRING(id))));
    col->NamedItem(str, getter_AddRefs(domwin));
  }

  return domwin != nsnull;
}

static JSBool
WrapSameOriginProp(JSContext *cx, JSObject *outerObj, jsval *vp);

static JSBool
XPC_XOW_FunctionWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                        jsval *rval)
{
  JSObject *wrappedObj, *outerObj = obj;

  
  
  

  outerObj = wrappedObj = GetWrapper(obj);
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

  JSFunction *fun = JS_ValueToFunction(cx, funToCall);
  if (!fun) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  nsresult rv = CanAccessWrapper(cx, outerObj, JSVAL_TO_OBJECT(funToCall), nsnull);
  if (NS_FAILED(rv) && rv != NS_ERROR_DOM_PROP_ACCESS_DENIED) {
    return ThrowException(rv, cx);
  }

#ifdef DEBUG
  JSNative native = JS_GetFunctionNative(cx, fun);
  NS_ASSERTION(native, "How'd we get here with a scripted function?");
#endif

  if (!JS_CallFunctionValue(cx, wrappedObj, funToCall, argc, argv, rval)) {
    return JS_FALSE;
  }

  if (NS_SUCCEEDED(rv)) {
    return WrapSameOriginProp(cx, outerObj, rval);
  }

  return RewrapIfNeeded(cx, obj, rval);
}

static JSBool
WrapSameOriginProp(JSContext *cx, JSObject *outerObj, jsval *vp)
{
  
  
  if (JSVAL_IS_PRIMITIVE(*vp)) {
    return JS_TRUE;
  }

  JSObject *wrappedObj = JSVAL_TO_OBJECT(*vp);
  JSClass *clasp = wrappedObj->getJSClass();
  if (ClassNeedsXOW(clasp->name)) {
    return WrapObject(cx, JS_GetGlobalForObject(cx, outerObj), vp);
  }

  
  
  if (clasp == &XOWClass.base &&
      wrappedObj->getParent() != outerObj->getParent()) {
    *vp = OBJECT_TO_JSVAL(GetWrappedObject(cx, wrappedObj));
    return WrapObject(cx, outerObj->getParent(), vp);
  }

  return JS_TRUE;
}

static JSBool
XPC_XOW_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  
  

  obj = GetWrapper(obj);
  jsval resolving;
  if (!JS_GetReservedSlot(cx, obj, sFlagsSlot, &resolving)) {
    return JS_FALSE;
  }

  if (!JSVAL_IS_PRIMITIVE(*vp)) {
    JSObject *addedObj = JSVAL_TO_OBJECT(*vp);
    if (addedObj->getJSClass() == &XOWClass.base &&
        addedObj->getParent() != obj->getParent()) {
      *vp = OBJECT_TO_JSVAL(GetWrappedObject(cx, addedObj));
      if (!WrapObject(cx, obj->getParent(), vp, nsnull)) {
        return JS_FALSE;
      }
    }
  }

  if (HAS_FLAGS(resolving, FLAG_RESOLVING)) {
    
    return JS_TRUE;
  }

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  JSBool privilegeEnabled = JS_FALSE;
  nsresult rv = CanAccessWrapper(cx, obj, wrappedObj, &privilegeEnabled);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      
      return ThrowException(rv, cx);
    }
    return JS_FALSE;
  }

  
  return AddProperty(cx, obj, JS_TRUE, wrappedObj, id, vp);
}

static JSBool
XPC_XOW_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  nsresult rv = CanAccessWrapper(cx, obj, wrappedObj, nsnull);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      
      return ThrowException(rv, cx);
    }
    return JS_FALSE;
  }

  
  return DelProperty(cx, wrappedObj, id, vp);
}

static JSBool
XPC_XOW_GetOrSetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp,
                         JSBool isSet)
{
  if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    return JS_TRUE;
  }

  
  
  
  jsval v = *vp;
  if (!JSVAL_IS_PRIMITIVE(v) &&
      JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(v)) &&
      JS_GetFunctionNative(cx, JS_ValueToFunction(cx, v)) ==
      XPC_XOW_FunctionWrapper) {
    return JS_TRUE;
  }

  JSObject *origObj = obj;
  obj = GetWrapper(obj);
  if (!obj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  js::AutoArrayRooter rooter(cx, 1, vp);

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  JSBool privilegeEnabled;
  nsresult rv = CanAccessWrapper(cx, obj, wrappedObj, &privilegeEnabled);
  if (NS_FAILED(rv)) {
    if (rv != NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      return JS_FALSE;
    }

    XPCCallContext ccx(JS_CALLER, cx);
    if (!ccx.IsValid()) {
      return ThrowException(NS_ERROR_FAILURE, cx);
    }

    
    
    
    

    XPCWrappedNative *wn =
      XPCWrappedNative::GetWrappedNativeOfJSObject(cx, wrappedObj);
    NS_ASSERTION(wn, "How did we wrap a non-WrappedNative?");
    if (!IsValFrame(wrappedObj, id, wn)) {
      nsIScriptSecurityManager *ssm = GetSecurityManager();
      if (!ssm) {
        return ThrowException(NS_ERROR_NOT_INITIALIZED, cx);
      }
      rv = ssm->CheckPropertyAccess(cx, wrappedObj,
                                    wrappedObj->getClass()->name,
                                    id, isSet ? sSecMgrSetProp
                                              : sSecMgrGetProp);
      if (NS_FAILED(rv)) {
        
        return JS_FALSE;
      }
    }

    return GetOrSetNativeProperty(cx, obj, wn, id, vp, isSet, JS_FALSE);
  }

  JSObject *proto = nsnull; 
  JSBool checkProto =
    (isSet && id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_PROTO));
  if (checkProto) {
    proto = wrappedObj->getProto();
  }

  JSBool ok = isSet
              ? JS_SetPropertyById(cx, wrappedObj, id, vp)
              : JS_GetPropertyById(cx, wrappedObj, id, vp);
  if (!ok) {
    return JS_FALSE;
  }

  if (checkProto) {
    JSObject *newProto = wrappedObj->getProto();

    
    
    
    
    
    

    if (origObj != obj) {
      
      if (!JS_SetPrototype(cx, wrappedObj, proto) ||
          !JS_SetPrototype(cx, origObj, newProto)) {
        return JS_FALSE;
      }
    } else if (newProto) {
      
      
      
      

      JS_SetPrototype(cx, wrappedObj, proto);
      JS_ReportError(cx, "invalid __proto__ value (can only be set to null)");
      return JS_FALSE;
    }
  }

  return WrapSameOriginProp(cx, obj, vp);
}

static JSBool
XPC_XOW_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  return XPC_XOW_GetOrSetProperty(cx, obj, id, vp, JS_FALSE);
}

static JSBool
XPC_XOW_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  return XPC_XOW_GetOrSetProperty(cx, obj, id, vp, JS_TRUE);
}

static JSBool
XPC_XOW_Enumerate(JSContext *cx, JSObject *obj)
{
  obj = GetWrapper(obj);
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    return JS_TRUE;
  }

  nsresult rv = CanAccessWrapper(cx, obj, wrappedObj, nsnull);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      
      return ThrowException(rv, cx);
    }

    return JS_FALSE;
  }

  return Enumerate(cx, obj, wrappedObj);
}
















static JSObject *
GetUXPCObject(JSContext *cx, JSObject *obj)
{
  NS_ASSERTION(obj->getJSClass() == &XOWClass.base, "wrong object");

  jsval v;
  if (!JS_GetReservedSlot(cx, obj, sFlagsSlot, &v)) {
    return nsnull;
  }

  if (HAS_FLAGS(v, FLAG_IS_UXPC_OBJECT)) {
    return obj;
  }

  if (!JS_GetReservedSlot(cx, obj, sUXPCObjectSlot, &v)) {
    return nsnull;
  }

  if (JSVAL_IS_OBJECT(v)) {
    return JSVAL_TO_OBJECT(v);
  }

  JSObject *uxpco =
    JS_NewObjectWithGivenProto(cx, &XOWClass.base, nsnull, obj->getParent());
  if (!uxpco) {
    return nsnull;
  }

  js::AutoObjectRooter tvr(cx, uxpco);

  jsval wrappedObj, parentScope;
  if (!JS_GetReservedSlot(cx, obj, sWrappedObjSlot, &wrappedObj) ||
      !JS_GetReservedSlot(cx, obj, XPC_XOW_ScopeSlot, &parentScope)) {
    return nsnull;
  }

  if (!JS_SetReservedSlot(cx, uxpco, sWrappedObjSlot, wrappedObj) ||
      !JS_SetReservedSlot(cx, uxpco, sFlagsSlot,
                          INT_TO_JSVAL(FLAG_IS_UXPC_OBJECT)) ||
      !JS_SetReservedSlot(cx, uxpco, XPC_XOW_ScopeSlot, parentScope)) {
    return nsnull;
  }

  if (!JS_SetReservedSlot(cx, obj, sUXPCObjectSlot, OBJECT_TO_JSVAL(uxpco))) {
    return nsnull;
  }

  return uxpco;
}

static JSBool
XPC_XOW_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                   JSObject **objp)
{
  obj = GetWrapper(obj);

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    *objp = nsnull;
    return JS_TRUE;
  }

  JSBool privilegeEnabled;
  nsresult rv = CanAccessWrapper(cx, obj, wrappedObj, &privilegeEnabled);
  if (NS_FAILED(rv)) {
    if (rv != NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      return JS_FALSE;
    }

    XPCCallContext ccx(JS_CALLER, cx);
    if (!ccx.IsValid()) {
      return ThrowException(NS_ERROR_FAILURE, cx);
    }

    
    
    
    

    XPCWrappedNative *wn =
      XPCWrappedNative::GetWrappedNativeOfJSObject(cx, wrappedObj);
    NS_ASSERTION(wn, "How did we wrap a non-WrappedNative?");
    if (!IsValFrame(wrappedObj, id, wn)) {
      nsIScriptSecurityManager *ssm = GetSecurityManager();
      if (!ssm) {
        return ThrowException(NS_ERROR_NOT_INITIALIZED, cx);
      }

      PRUint32 action = (flags & JSRESOLVE_ASSIGNING)
                        ? sSecMgrSetProp
                        : sSecMgrGetProp;
      rv = ssm->CheckPropertyAccess(cx, wrappedObj,
                                    wrappedObj->getClass()->name,
                                    id, action);
      if (NS_FAILED(rv)) {
        
        return JS_FALSE;
      }
    }

    
    return ResolveNativeProperty(cx, obj, wrappedObj, wn, id,
                                 flags, objp, JS_FALSE);

  }

  if (privilegeEnabled && !(obj = GetUXPCObject(cx, obj))) {
    return JS_FALSE;
  }

  if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    jsval oldSlotVal;
    if (!JS_GetReservedSlot(cx, obj, sFlagsSlot, &oldSlotVal) ||
        !JS_SetReservedSlot(cx, obj, sFlagsSlot,
                            INT_TO_JSVAL(JSVAL_TO_INT(oldSlotVal) |
                                         FLAG_RESOLVING))) {
      return JS_FALSE;
    }

    JSBool ok = JS_DefineFunction(cx, obj, "toString",
                                  XPC_XOW_toString, 0, 0) != nsnull;

    JS_SetReservedSlot(cx, obj, sFlagsSlot, oldSlotVal);

    if (ok) {
      *objp = obj;
    }

    return ok;
  }

  return NewResolve(cx, obj, JS_TRUE, wrappedObj, id, flags, objp);
}

static JSBool
XPC_XOW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
  
  if (type == JSTYPE_OBJECT) {
    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
  }

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    

    if (type == JSTYPE_STRING || type == JSTYPE_VOID) {
      return XPC_XOW_toString(cx, obj, 0, nsnull, vp);
    }

    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
  }

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  
  nsresult rv = CanAccessWrapper(cx, obj, wrappedObj, nsnull);
  if (NS_FAILED(rv) &&
      (rv != NS_ERROR_DOM_PROP_ACCESS_DENIED ||
       (type != JSTYPE_STRING && type != JSTYPE_VOID))) {
    
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      ThrowException(rv, cx);
    }
    return JS_FALSE;
  }

  if (!wrappedObj->getJSClass()->convert(cx, wrappedObj, type, vp)) {
    return JS_FALSE;
  }

  return NS_SUCCEEDED(rv)
         ? WrapSameOriginProp(cx, obj, vp)
         : RewrapIfNeeded(cx, obj, vp);
}

static void
XPC_XOW_Finalize(JSContext *cx, JSObject *obj)
{
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return;
  }

  jsval flags = GetFlags(cx, obj);
  if (HAS_FLAGS(flags, FLAG_IS_CACHED)) {
    
    XPCWrappedNativeWithXOW *wnxow =
      static_cast<XPCWrappedNativeWithXOW *>(xpc_GetJSPrivate(wrappedObj));
    wnxow->SetXOW(nsnull);

    JS_SetReservedSlot(cx, obj, sWrappedObjSlot, JSVAL_VOID);
    SetFlags(cx, obj, RemoveFlags(flags, FLAG_IS_CACHED));
  }

  
  jsval scopeVal;
  if (!JS_GetReservedSlot(cx, obj, XPC_XOW_ScopeSlot, &scopeVal)) {
    return;
  }

  
  
  
  XPCWrappedNativeScope *scope = reinterpret_cast<XPCWrappedNativeScope *>
                                                 (JSVAL_TO_PRIVATE(scopeVal));
  if (!scope) {
    return;
  }

  
  scope->GetWrapperMap()->Remove(wrappedObj);
}

static JSBool
XPC_XOW_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                    jsval *vp)
{
  
  

  uintN junk;
  return JS_CheckAccess(cx, GetWrappedObject(cx, obj), id, mode, vp, &junk);
}

static JSBool
XPC_XOW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    return JS_TRUE;
  }

  nsresult rv = CanAccessWrapper(cx, obj, wrappedObj, nsnull);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      
      return ThrowException(rv, cx);
    }

    return JS_FALSE;
  }

  JSObject *callee = JSVAL_TO_OBJECT(argv[-2]);
  NS_ASSERTION(GetWrappedObject(cx, callee), "How'd we get here?");
  callee = GetWrappedObject(cx, callee);
  if (!JS_CallFunctionValue(cx, obj, OBJECT_TO_JSVAL(callee), argc, argv,
                            rval)) {
    return JS_FALSE;
  }

  return RewrapIfNeeded(cx, callee, rval);
}

static JSBool
XPC_XOW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                  jsval *rval)
{
  JSObject *realObj = GetWrapper(JSVAL_TO_OBJECT(argv[-2]));
  JSObject *wrappedObj = GetWrappedObject(cx, realObj);
  if (!wrappedObj) {
    
    return JS_TRUE;
  }

  nsresult rv = CanAccessWrapper(cx, realObj, wrappedObj, nsnull);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      
      return ThrowException(rv, cx);
    }
    return JS_FALSE;
  }

  if (!JS_CallFunctionValue(cx, obj, OBJECT_TO_JSVAL(wrappedObj), argc, argv,
                            rval)) {
    return JS_FALSE;
  }

  return RewrapIfNeeded(cx, wrappedObj, rval);
}

static JSBool
XPC_XOW_HasInstance(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp)
{
  JSObject *iface = GetWrappedObject(cx, obj);

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  nsresult rv = CanAccessWrapper(cx, obj, iface, nsnull);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      
      return ThrowException(rv, cx);
    }
    return JS_FALSE;
  }

  JSClass *clasp = iface->getJSClass();

  *bp = JS_FALSE;
  if (!clasp->hasInstance) {
    return JS_TRUE;
  }

  
  jsval v = *valp;
  if (!JSVAL_IS_PRIMITIVE(*valp)) {
    JSObject *test = JSVAL_TO_OBJECT(v);

    
    test = GetWrappedObject(cx, test);
    if (test) {
      v = OBJECT_TO_JSVAL(test);
    }
  }

  return clasp->hasInstance(cx, iface, &v, bp);
}

static JSBool
XPC_XOW_Equality(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp)
{
  jsval v = *valp;

  
  if (JSVAL_IS_PRIMITIVE(v)) {
    *bp = JS_FALSE;
    return JS_TRUE;
  }

  JSObject *test = JSVAL_TO_OBJECT(v);
  if (test->getJSClass() == &XOWClass.base) {
    if (!JS_GetReservedSlot(cx, test, sWrappedObjSlot, &v)) {
      return JS_FALSE;
    }

    if (JSVAL_IS_PRIMITIVE(v)) {
      *bp = JS_FALSE;
      return JS_TRUE;
    }

    test = JSVAL_TO_OBJECT(v);
  }

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
XPC_XOW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly)
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

  nsresult rv = CanAccessWrapper(cx, obj, wrappedObj, nsnull);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      
      ThrowException(rv, cx);
      return nsnull;
    }

    ThrowException(NS_ERROR_FAILURE, cx);
    return nsnull;
  }

  JSObject *wrapperIter = JS_NewObject(cx, &XOWClass.base, nsnull,
                                       JS_GetGlobalForObject(cx, obj));
  if (!wrapperIter) {
    return nsnull;
  }

  js::AutoObjectRooter tvr(cx, wrapperIter);

  
  jsval v = OBJECT_TO_JSVAL(wrappedObj);
  if (!JS_SetReservedSlot(cx, wrapperIter, sWrappedObjSlot, v) ||
      !JS_SetReservedSlot(cx, wrapperIter, sFlagsSlot, JSVAL_ZERO) ||
      !JS_SetReservedSlot(cx, wrapperIter, XPC_XOW_ScopeSlot,
                          PRIVATE_TO_JSVAL(nsnull))) {
    return nsnull;
  }

  return CreateIteratorObj(cx, wrapperIter, obj, wrappedObj, keysonly);
}

static JSObject *
XPC_XOW_WrappedObject(JSContext *cx, JSObject *obj)
{
  return GetWrappedObject(cx, obj);
}

static JSBool
XPC_XOW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval)
{
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

  XPCCallContext ccx(JS_CALLER, cx);
  if (!ccx.IsValid()) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  nsresult rv = CanAccessWrapper(cx, obj, wrappedObj, nsnull);
  if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
    nsIScriptSecurityManager *ssm = GetSecurityManager();
    if (!ssm) {
      return ThrowException(NS_ERROR_NOT_INITIALIZED, cx);
    }
    rv = ssm->CheckPropertyAccess(cx, wrappedObj,
                                  wrappedObj->getClass()->name,
                                  GetRTIdByIndex(cx, XPCJSRuntime::IDX_TO_STRING),
                                  nsIXPCSecurityManager::ACCESS_GET_PROPERTY);
  }
  if (NS_FAILED(rv)) {
    return JS_FALSE;
  }

  XPCWrappedNative *wn =
    XPCWrappedNative::GetWrappedNativeOfJSObject(cx, wrappedObj);
  return NativeToString(cx, wn, argc, argv, rval, JS_FALSE);
}
