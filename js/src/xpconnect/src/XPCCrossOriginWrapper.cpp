






































#include "xpcprivate.h"
#include "nsDOMError.h"
#include "jsdbgapi.h"
#include "jsobj.h"    
#include "jscntxt.h"  
#include "XPCWrapper.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowCollection.h"




JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_Enumerate(JSContext *cx, JSObject *obj);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                   JSObject **objp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

JS_STATIC_DLL_CALLBACK(void)
XPC_XOW_Finalize(JSContext *cx, JSObject *obj);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_CheckAccess(JSContext *cx, JSObject *obj, jsval id, JSAccessMode mode,
                    jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                  jsval *rval);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_Equality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

JS_STATIC_DLL_CALLBACK(JSObject *)
XPC_XOW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly);

JS_STATIC_DLL_CALLBACK(JSObject *)
XPC_XOW_WrappedObject(JSContext *cx, JSObject *obj);

JSExtendedClass sXPC_XOW_JSClass = {
  
  { "XPCCrossOriginWrapper",
    JSCLASS_NEW_RESOLVE | JSCLASS_IS_EXTENDED |
    JSCLASS_HAS_RESERVED_SLOTS(XPCWrapper::sNumSlots + 1),
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










static const int XPC_XOW_ScopeSlot = XPCWrapper::sNumSlots;

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval);


static inline
JSBool
ThrowException(nsresult ex, JSContext *cx)
{
  XPCThrower::Throw(ex, cx);

  return JS_FALSE;
}


static inline
JSObject *
GetWrapper(JSContext *cx, JSObject *obj)
{
  while (JS_GET_CLASS(cx, obj) != &sXPC_XOW_JSClass.base) {
    obj = JS_GetPrototype(cx, obj);
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
  if (JS_GET_CLASS(cx, wrapper) != &sXPC_XOW_JSClass.base) {
    return nsnull;
  }

  jsval v;
  if (!JS_GetReservedSlot(cx, wrapper, XPCWrapper::sWrappedObjSlot, &v)) {
    JS_ClearPendingException(cx);
    return nsnull;
  }

  if (!JSVAL_IS_OBJECT(v)) {
    return nsnull;
  }

  return JSVAL_TO_OBJECT(v);
}

static inline
nsIScriptSecurityManager *
GetSecurityManager()
{
  extern nsIScriptSecurityManager *gScriptSecurityManager;

  return gScriptSecurityManager;
}

JSBool
XPC_XOW_WrapperMoved(JSContext *cx, XPCWrappedNative *innerObj,
                     XPCWrappedNativeScope *newScope)
{
  typedef WrappedNative2WrapperMap::Link Link;
  XPCJSRuntime *rt = nsXPConnect::GetRuntime();
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

  return JS_SetReservedSlot(cx, xow, XPCWrapper::sNumSlots,
                            PRIVATE_TO_JSVAL(newScope)) &&
         JS_SetParent(cx, xow, newScope->GetGlobalJSObject());
}

static JSBool
IsValFrame(JSContext *cx, JSObject *obj, jsval v, XPCWrappedNative *wn)
{
  
  if (JS_GET_CLASS(cx, obj)->name[0] != 'W') {
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

  if (JSVAL_IS_INT(v)) {
    col->Item(JSVAL_TO_INT(v), getter_AddRefs(domwin));
  } else {
    nsAutoString str(reinterpret_cast<PRUnichar *>
                                     (JS_GetStringChars(JSVAL_TO_STRING(v))));
    col->NamedItem(str, getter_AddRefs(domwin));
  }

  return domwin != nsnull;
}







nsresult
IsWrapperSameOrigin(JSContext *cx, JSObject *wrappedObj)
{
  
  nsIScriptSecurityManager *ssm = GetSecurityManager();
  if (!ssm) {
    ThrowException(NS_ERROR_NOT_INITIALIZED, cx);
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsIPrincipal *subjectPrin = ssm->GetCxSubjectPrincipal(cx);

  if (!subjectPrin) {
    ThrowException(NS_ERROR_FAILURE, cx);
    return NS_ERROR_FAILURE;
  }

  PRBool isSystem = PR_FALSE;
  nsresult rv = ssm->IsSystemPrincipal(subjectPrin, &isSystem);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (isSystem) {
    return NS_OK;
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

  
  return ssm->CheckSameOriginPrincipal(subjectPrin, objectPrin);
}

static JSBool
WrapSameOriginProp(JSContext *cx, JSObject *outerObj, jsval *vp);

static JSBool
XPC_XOW_FunctionWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                        jsval *rval)
{
  JSObject *wrappedObj, *outerObj = obj;

  
  
  

  wrappedObj = GetWrapper(cx, obj);
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
  if (!JS_GetReservedSlot(cx, funObj, 0, &funToCall)) {
    return JS_FALSE;
  }

  JSFunction *fun = JS_ValueToFunction(cx, funToCall);
  if (!fun) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  nsresult rv = IsWrapperSameOrigin(cx, JSVAL_TO_OBJECT(funToCall));
  if (NS_FAILED(rv) && rv != NS_ERROR_DOM_PROP_ACCESS_DENIED) {
    return ThrowException(rv, cx);
  }

  JSNative native = JS_GetFunctionNative(cx, fun);
  NS_ASSERTION(native, "How'd we get here with a scripted function?");

  
  
  

  argv[-2] = funToCall;
  argv[-1] = OBJECT_TO_JSVAL(wrappedObj);
  if (!native(cx, wrappedObj, argc, argv, rval)) {
    return JS_FALSE;
  }

  if (NS_SUCCEEDED(rv)) {
    return WrapSameOriginProp(cx, outerObj, rval);
  }

  return XPC_XOW_RewrapIfNeeded(cx, obj, rval);
}

static JSBool
WrapSameOriginProp(JSContext *cx, JSObject *outerObj, jsval *vp)
{
  
  
  if (JSVAL_IS_PRIMITIVE(*vp)) {
    return JS_TRUE;
  }

  JSObject *wrappedObj = JSVAL_TO_OBJECT(*vp);
  JSClass *clasp = JS_GET_CLASS(cx, wrappedObj);
  if (XPC_XOW_ClassNeedsXOW(clasp->name)) {
    return XPC_XOW_WrapObject(cx, JS_GetGlobalForObject(cx, outerObj), vp);
  }

  
  
  if (clasp == &sXPC_XOW_JSClass.base &&
      JS_GetParent(cx, wrappedObj) != JS_GetParent(cx, outerObj)) {
    *vp = OBJECT_TO_JSVAL(GetWrappedObject(cx, wrappedObj));
    return XPC_XOW_WrapObject(cx, JS_GetParent(cx, outerObj), vp);
  }

  if (JS_ObjectIsFunction(cx, wrappedObj) &&
      JS_GetFunctionNative(cx, reinterpret_cast<JSFunction *>
                                               (JS_GetPrivate(cx, wrappedObj))) ==
      XPCWrapper::sEvalNative) {
    return XPC_XOW_WrapFunction(cx, outerObj, wrappedObj, vp);
  }

  return JS_TRUE;
}

JSBool
XPC_XOW_WrapFunction(JSContext *cx, JSObject *outerObj, JSObject *funobj,
                     jsval *rval)
{
  jsval funobjVal = OBJECT_TO_JSVAL(funobj);
  JSFunction *wrappedFun = reinterpret_cast<JSFunction *>(JS_GetPrivate(cx, funobj));
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
  if (!JS_SetReservedSlot(cx, funWrapperObj, 0, funobjVal)) {
    return JS_FALSE;
  }

  *rval = OBJECT_TO_JSVAL(funWrapperObj);
  return JS_TRUE;
}

JSBool
XPC_XOW_RewrapIfNeeded(JSContext *cx, JSObject *outerObj, jsval *vp)
{
  
  if (JSVAL_IS_PRIMITIVE(*vp)) {
    return JS_TRUE;
  }

  JSObject *obj = JSVAL_TO_OBJECT(*vp);

  if (JS_ObjectIsFunction(cx, obj)) {
    return XPC_XOW_WrapFunction(cx, outerObj, obj, vp);
  }

  if (JS_GET_CLASS(cx, obj) == &sXPC_XOW_JSClass.base &&
      JS_GetParent(cx, outerObj) != JS_GetParent(cx, obj)) {
    *vp = OBJECT_TO_JSVAL(GetWrappedObject(cx, obj));
  } else if (!XPCWrappedNative::GetWrappedNativeOfJSObject(cx, obj)) {
    return JS_TRUE;
  }

  return XPC_XOW_WrapObject(cx, JS_GetGlobalForObject(cx, outerObj), vp);
}

JSBool
XPC_XOW_WrapObject(JSContext *cx, JSObject *parent, jsval *vp)
{
  
  JSObject *wrappedObj;
  XPCWrappedNative *wn;
  if (!JSVAL_IS_OBJECT(*vp) ||
      !(wrappedObj = JSVAL_TO_OBJECT(*vp)) ||
      JS_GET_CLASS(cx, wrappedObj) == &sXPC_XOW_JSClass.base ||
      !(wn = XPCWrappedNative::GetWrappedNativeOfJSObject(cx, wrappedObj))) {
    return JS_TRUE;
  }

  XPCJSRuntime *rt = nsXPConnect::GetRuntime();
  XPCCallContext ccx(NATIVE_CALLER, cx);
  NS_ENSURE_TRUE(ccx.IsValid(), JS_FALSE);

  
  parent = JS_GetGlobalForObject(cx, parent);
  JSClass *clasp = JS_GET_CLASS(cx, parent);
  if (clasp->flags & JSCLASS_IS_EXTENDED) {
    JSExtendedClass *xclasp = reinterpret_cast<JSExtendedClass *>(clasp);
    if (xclasp->innerObject) {
      parent = xclasp->innerObject(cx, parent);
      if (!parent) {
        return JS_FALSE;
      }
    }
  }

  XPCWrappedNativeScope *parentScope =
    XPCWrappedNativeScope::FindInJSObjectScope(ccx, parent);

#ifdef DEBUG_mrbkap
  printf("Wrapping object at %p (%s) [%p]\n",
         (void *)wrappedObj, JS_GET_CLASS(cx, wrappedObj)->name,
         (void *)parentScope);
#endif

  JSObject *outerObj = nsnull;
  WrappedNative2WrapperMap *map = parentScope->GetWrapperMap();

  { 
    XPCAutoLock al(rt->GetMapLock());
    outerObj = map->Find(wrappedObj);
  }

  if (outerObj) {
    NS_ASSERTION(JS_GET_CLASS(cx, outerObj) == &sXPC_XOW_JSClass.base,
                              "What crazy object are we getting here?");
#ifdef DEBUG_mrbkap
    printf("But found a wrapper in the map %p!\n", (void *)outerObj);
#endif
    *vp = OBJECT_TO_JSVAL(outerObj);
    return JS_TRUE;
  }

  
  
  
  outerObj = JS_NewObject(cx, &sXPC_XOW_JSClass.base, nsnull, nsnull);
  if (!outerObj) {
    return JS_FALSE;
  }

  
  
  if (!JS_SetParent(cx, outerObj, parent) ||
      !JS_SetPrototype(cx, outerObj, nsnull)) {
    return JS_FALSE;
  }

  if (!JS_SetReservedSlot(cx, outerObj, XPCWrapper::sWrappedObjSlot, *vp) ||
      !JS_SetReservedSlot(cx, outerObj, XPCWrapper::sResolvingSlot,
                          JSVAL_FALSE) ||
      !JS_SetReservedSlot(cx, outerObj, XPC_XOW_ScopeSlot,
                          PRIVATE_TO_JSVAL(parentScope))) {
    return JS_FALSE;
  }

  *vp = OBJECT_TO_JSVAL(outerObj);

  { 
    XPCAutoLock al(rt->GetMapLock());
    map->Add(wn->GetScope()->GetWrapperMap(), wrappedObj, outerObj);
  }

  return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  
  

  obj = GetWrapper(cx, obj);
  jsval resolving;
  if (!JS_GetReservedSlot(cx, obj, XPCWrapper::sResolvingSlot, &resolving)) {
    return JS_FALSE;
  }

  if (JSVAL_TO_BOOLEAN(resolving)) {
    
    return JS_TRUE;
  }

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }
  nsresult rv = IsWrapperSameOrigin(cx, wrappedObj);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      
      return ThrowException(rv, cx);
    }
    return JS_FALSE;
  }

  
  return XPCWrapper::AddProperty(cx, wrappedObj, id, vp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }
  nsresult rv = IsWrapperSameOrigin(cx, wrappedObj);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      
      return ThrowException(rv, cx);
    }
    return JS_FALSE;
  }

  
  return XPCWrapper::DelProperty(cx, wrappedObj, id, vp);
}

static JSBool
XPC_XOW_GetOrSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp,
                         JSBool isSet)
{
  if (id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
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
  obj = GetWrapper(cx, obj);
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
  nsresult rv = IsWrapperSameOrigin(cx, wrappedObj);
  if (NS_FAILED(rv)) {
    if (rv != NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      return JS_FALSE;
    }

    
    
    
    

    XPCWrappedNative *wn =
      XPCWrappedNative::GetWrappedNativeOfJSObject(cx, wrappedObj);
    NS_ASSERTION(wn, "How did we wrap a non-WrappedNative?");
    if (!IsValFrame(cx, wrappedObj, id, wn)) {
      nsIScriptSecurityManager *ssm = GetSecurityManager();
      if (!ssm) {
        return ThrowException(NS_ERROR_NOT_INITIALIZED, cx);
      }

      PRUint32 check = isSet
                       ? (PRUint32)nsIXPCSecurityManager::ACCESS_SET_PROPERTY
                       : (PRUint32)nsIXPCSecurityManager::ACCESS_GET_PROPERTY;
      rv = ssm->CheckPropertyAccess(cx, wrappedObj,
                                    JS_GET_CLASS(cx, wrappedObj)->name,
                                    id, check);
      if (NS_FAILED(rv)) {
        
        return JS_FALSE;
      }
    }

    if (!XPCWrapper::GetOrSetNativeProperty(cx, obj, wn, id, vp, isSet,
                                            JS_FALSE)) {
      return JS_FALSE;
    }

    return XPC_XOW_RewrapIfNeeded(cx, obj, vp);
  }

  JSObject *proto = nsnull; 
  JSBool checkProto =
    (isSet && id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_PROTO));
  if (checkProto) {
    proto = JS_GetPrototype(cx, wrappedObj);
  }

  
  
  jsid asId;

  if (!JS_ValueToId(cx, id, &asId)) {
    return JS_FALSE;
  }

  JSBool ok = isSet
              ? OBJ_SET_PROPERTY(cx, wrappedObj, asId, vp)
              : OBJ_GET_PROPERTY(cx, wrappedObj, asId, vp);
  if (!ok) {
    return JS_FALSE;
  }

  if (checkProto) {
    JSObject *newProto = JS_GetPrototype(cx, wrappedObj);

    
    
    
    
    
    

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

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  return XPC_XOW_GetOrSetProperty(cx, obj, id, vp, JS_FALSE);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  return XPC_XOW_GetOrSetProperty(cx, obj, id, vp, JS_TRUE);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_Enumerate(JSContext *cx, JSObject *obj)
{
  obj = GetWrapper(cx, obj);
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    return JS_TRUE;
  }
  nsresult rv = IsWrapperSameOrigin(cx, wrappedObj);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      
      return ThrowException(rv, cx);
    }

    return JS_FALSE;
  }

  return XPCWrapper::Enumerate(cx, obj, wrappedObj);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                   JSObject **objp)
{
  obj = GetWrapper(cx, obj);

  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    *objp = nsnull;
    return JS_TRUE;
  }

  nsresult rv = IsWrapperSameOrigin(cx, wrappedObj);
  if (NS_FAILED(rv)) {
    if (rv != NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      return JS_FALSE;
    }

    
    
    
    

    XPCWrappedNative *wn =
      XPCWrappedNative::GetWrappedNativeOfJSObject(cx, wrappedObj);
    NS_ASSERTION(wn, "How did we wrap a non-WrappedNative?");
    if (!IsValFrame(cx, wrappedObj, id, wn)) {
      nsIScriptSecurityManager *ssm = GetSecurityManager();
      if (!ssm) {
        return ThrowException(NS_ERROR_NOT_INITIALIZED, cx);
      }
      PRUint32 action = (flags & JSRESOLVE_ASSIGNING)
                        ? (PRUint32)nsIXPCSecurityManager::ACCESS_SET_PROPERTY
                        : (PRUint32)nsIXPCSecurityManager::ACCESS_GET_PROPERTY;
      rv = ssm->CheckPropertyAccess(cx, wrappedObj,
                                    JS_GET_CLASS(cx, wrappedObj)->name,
                                    id, action);
      if (NS_FAILED(rv)) {
        
        return JS_FALSE;
      }
    }

    
    return XPCWrapper::ResolveNativeProperty(cx, obj, wrappedObj, wn, id,
                                             flags, objp, JS_FALSE);

  }

  if (id == GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    jsval oldSlotVal;
    if (!::JS_GetReservedSlot(cx, obj, XPCWrapper::sResolvingSlot, &oldSlotVal) ||
        !::JS_SetReservedSlot(cx, obj, XPCWrapper::sResolvingSlot, JSVAL_TRUE)) {
      return JS_FALSE;
    }

    JSBool ok = JS_DefineFunction(cx, obj, "toString",
                                  XPC_XOW_toString, 0, 0) != nsnull;

    if (ok && (ok = ::JS_SetReservedSlot(cx, obj, XPCWrapper::sResolvingSlot,
                                         oldSlotVal))) {
      *objp = obj;
    }

    return ok;
  }

  return XPCWrapper::NewResolve(cx, obj, wrappedObj, id, flags, objp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
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

  
  nsresult rv = IsWrapperSameOrigin(cx, wrappedObj);
  if (NS_FAILED(rv) &&
      (rv != NS_ERROR_DOM_PROP_ACCESS_DENIED ||
       (type != JSTYPE_STRING && type != JSTYPE_VOID))) {
    
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      ThrowException(rv, cx);
    }
    return JS_FALSE;
  }

  if (!JS_GET_CLASS(cx, wrappedObj)->convert(cx, wrappedObj, type, vp)) {
    return JS_FALSE;
  }

  return NS_SUCCEEDED(rv)
         ? WrapSameOriginProp(cx, obj, vp)
         : XPC_XOW_RewrapIfNeeded(cx, obj, vp);
}

JS_STATIC_DLL_CALLBACK(void)
XPC_XOW_Finalize(JSContext *cx, JSObject *obj)
{
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    return;
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

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_CheckAccess(JSContext *cx, JSObject *obj, jsval prop, JSAccessMode mode,
                    jsval *vp)
{
  
  

  uintN junk;
  jsid id;
  return JS_ValueToId(cx, prop, &id) &&
         JS_CheckAccess(cx, GetWrappedObject(cx, obj), id, mode, vp, &junk);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  JSObject *wrappedObj = GetWrappedObject(cx, obj);
  if (!wrappedObj) {
    
    return JS_TRUE;
  }
  nsresult rv = IsWrapperSameOrigin(cx, wrappedObj);
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

  return XPC_XOW_RewrapIfNeeded(cx, callee, rval);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                  jsval *rval)
{
  JSObject *realObj = GetWrapper(cx, JSVAL_TO_OBJECT(argv[-2]));
  JSObject *wrappedObj = GetWrappedObject(cx, realObj);
  if (!wrappedObj) {
    
    return JS_TRUE;
  }
  nsresult rv = IsWrapperSameOrigin(cx, wrappedObj);
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

  return XPC_XOW_RewrapIfNeeded(cx, callee, rval);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
  JSObject *iface = GetWrappedObject(cx, obj);
  nsresult rv = IsWrapperSameOrigin(cx, iface);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
      
      return ThrowException(rv, cx);
    }
    return JS_FALSE;
  }

  JSClass *clasp = JS_GET_CLASS(cx, iface);

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

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_Equality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
  
  if (JSVAL_IS_PRIMITIVE(v)) {
    *bp = JS_FALSE;
    return JS_TRUE;
  }

  JSObject *test = JSVAL_TO_OBJECT(v);
  if (JS_GET_CLASS(cx, test) == &sXPC_XOW_JSClass.base) {
    if (!JS_GetReservedSlot(cx, test, XPCWrapper::sWrappedObjSlot, &v)) {
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
  return ((JSExtendedClass *)JS_GET_CLASS(cx, obj))->
    equality(cx, obj, OBJECT_TO_JSVAL(test), bp);
}

JS_STATIC_DLL_CALLBACK(JSObject *)
XPC_XOW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly)
{
  JSObject *wrapperIter = JS_NewObject(cx, &sXPC_XOW_JSClass.base, nsnull,
                                       JS_GetGlobalForObject(cx, obj));
  if (!wrapperIter) {
    return nsnull;
  }

  JSAutoTempValueRooter tvr(cx, OBJECT_TO_JSVAL(wrapperIter));

  
  JSObject *innerObj = GetWrappedObject(cx, obj);
  if (!innerObj) {
    ThrowException(NS_ERROR_INVALID_ARG, cx);
    return nsnull;
  }

  jsval v = OBJECT_TO_JSVAL(innerObj);
  if (!JS_SetReservedSlot(cx, wrapperIter, XPCWrapper::sWrappedObjSlot, v) ||
      !JS_SetReservedSlot(cx, wrapperIter, XPCWrapper::sResolvingSlot,
                          JSVAL_FALSE) ||
      !JS_SetReservedSlot(cx, wrapperIter, XPCWrapper::sNumSlots,
                          PRIVATE_TO_JSVAL(nsnull))) {
    return nsnull;
  }

  return XPCWrapper::CreateIteratorObj(cx, wrapperIter, obj, innerObj,
                                       keysonly);
}

JS_STATIC_DLL_CALLBACK(JSObject *)
XPC_XOW_WrappedObject(JSContext *cx, JSObject *obj)
{
  return GetWrappedObject(cx, obj);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_XOW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval)
{
  obj = GetWrapper(cx, obj);
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

  nsresult rv = IsWrapperSameOrigin(cx, wrappedObj);
  if (rv == NS_ERROR_DOM_PROP_ACCESS_DENIED) {
    nsIScriptSecurityManager *ssm = GetSecurityManager();
    if (!ssm) {
      return ThrowException(NS_ERROR_NOT_INITIALIZED, cx);
    }
    rv = ssm->CheckPropertyAccess(cx, wrappedObj,
                                  JS_GET_CLASS(cx, wrappedObj)->name,
                                  GetRTStringByIndex(cx, XPCJSRuntime::IDX_TO_STRING),
                                  nsIXPCSecurityManager::ACCESS_GET_PROPERTY);
  }
  if (NS_FAILED(rv)) {
    return JS_FALSE;
  }

  XPCWrappedNative *wn =
    XPCWrappedNative::GetWrappedNativeOfJSObject(cx, wrappedObj);
  return XPCWrapper::NativeToString(cx, wn, argc, argv, rval, JS_FALSE);
}
