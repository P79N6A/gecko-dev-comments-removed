






































#include "xpcprivate.h"
#include "jsdbgapi.h"
#include "jsscript.h" 
#include "XPCWrapper.h"
#include "jsregexp.h"
#include "nsJSPrincipals.h"

static JSBool
XPC_SJOW_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_SJOW_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_SJOW_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_SJOW_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
XPC_SJOW_Enumerate(JSContext *cx, JSObject *obj);

static JSBool
XPC_SJOW_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                    JSObject **objp);

static JSBool
XPC_SJOW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

static void
XPC_SJOW_Finalize(JSContext *cx, JSObject *obj);

static JSBool
XPC_SJOW_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                     jsval *vp);

static JSBool
XPC_SJOW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval);

static JSBool
XPC_SJOW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                   jsval *rval);

static JSBool
XPC_SJOW_Create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval);

static JSBool
XPC_SJOW_Equality(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);

static JSObject *
XPC_SJOW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly);

static JSObject *
XPC_SJOW_WrappedObject(JSContext *cx, JSObject *obj);

using namespace XPCSafeJSObjectWrapper;
using namespace XPCWrapper;

static inline
JSBool
ThrowException(nsresult ex, JSContext *cx)
{
  DoThrowException(ex, cx);

  return JS_FALSE;
}





static nsresult
FindPrincipals(JSContext *cx, JSObject *obj, nsIPrincipal **objectPrincipal,
               nsIPrincipal **subjectPrincipal,
               nsIScriptSecurityManager **secMgr,
               JSStackFrame **fp = nsnull)
{
  XPCCallContext ccx(JS_CALLER, cx);

  if (!ccx.IsValid()) {
    return NS_ERROR_UNEXPECTED;
  }

  nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();

  if (subjectPrincipal) {
    JSStackFrame *fp2;
    NS_IF_ADDREF(*subjectPrincipal = ssm->GetCxSubjectPrincipalAndFrame(cx, &fp2));
    if (fp) {
      *fp = fp2;
    }
  }

  ssm->GetObjectPrincipal(cx, obj, objectPrincipal);

  if (secMgr) {
    NS_ADDREF(*secMgr = ssm);
  }

  return *objectPrincipal ? NS_OK : NS_ERROR_XPC_SECURITY_MANAGER_VETO;
}

static PRBool
CanCallerAccess(JSContext *cx, JSObject *wrapperObj, JSObject *unsafeObj)
{
  
  nsCOMPtr<nsIPrincipal> subjPrincipal, objPrincipal;
  nsCOMPtr<nsIScriptSecurityManager> ssm;
  JSStackFrame *fp;
  nsresult rv = FindPrincipals(cx, unsafeObj, getter_AddRefs(objPrincipal),
                               getter_AddRefs(subjPrincipal),
                               getter_AddRefs(ssm), &fp);
  if (NS_FAILED(rv)) {
    return ThrowException(rv, cx);
  }

  
  if (!subjPrincipal || !fp) {
    return PR_TRUE;
  }

  PRBool subsumes;
  rv = subjPrincipal->Subsumes(objPrincipal, &subsumes);

  if (NS_FAILED(rv) || !subsumes) {
    PRBool enabled = PR_FALSE;
    rv = ssm->IsCapabilityEnabled("UniversalXPConnect", &enabled);
    if (NS_FAILED(rv)) {
      return ThrowException(rv, cx);
    }

    if (!enabled) {
      return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
    }
  }

  if (wrapperObj) {
    jsval flags;
    JS_GetReservedSlot(cx, wrapperObj, sFlagsSlot, &flags);
    if (HAS_FLAGS(flags, FLAG_SOW) &&
        !SystemOnlyWrapper::CheckFilename(cx, JSID_VOID, fp)) {
      return JS_FALSE;
    }
  }

  return PR_TRUE;
}






static const PRUint32 sPrincipalSlot = sNumSlots;


static nsIPrincipal *
FindObjectPrincipals(JSContext *cx, JSObject *safeObj, JSObject *innerObj)
{
  
  jsval v;
  if (!JS_GetReservedSlot(cx, safeObj, sPrincipalSlot, &v)) {
    return nsnull;
  }

  if (!JSVAL_IS_VOID(v)) {
    
    return static_cast<nsIPrincipal *>(JSVAL_TO_PRIVATE(v));
  }

  nsCOMPtr<nsIPrincipal> objPrincipal;
  nsresult rv = FindPrincipals(cx, innerObj, getter_AddRefs(objPrincipal), nsnull,
                               nsnull);
  if (NS_FAILED(rv)) {
    return nsnull;
  }

  if (!JS_SetReservedSlot(cx, safeObj, sPrincipalSlot,
                          PRIVATE_TO_JSVAL(objPrincipal.get()))) {
    return nsnull;
  }

  
  return objPrincipal.forget().get();
}

static inline JSObject *
FindSafeObject(JSObject *obj)
{
  while (obj->getJSClass() != &SJOWClass.base) {
    obj = obj->getProto();

    if (!obj) {
      break;
    }
  }

  return obj;
}

static JSBool
XPC_SJOW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                  jsval *rval);

namespace XPCSafeJSObjectWrapper {




JSExtendedClass SJOWClass = {
  
  { "XPCSafeJSObjectWrapper",
    JSCLASS_NEW_RESOLVE | JSCLASS_IS_EXTENDED |
    JSCLASS_HAS_RESERVED_SLOTS(XPCWrapper::sNumSlots + 1),
    XPC_SJOW_AddProperty, XPC_SJOW_DelProperty,
    XPC_SJOW_GetProperty, XPC_SJOW_SetProperty,
    XPC_SJOW_Enumerate,   (JSResolveOp)XPC_SJOW_NewResolve,
    XPC_SJOW_Convert,     XPC_SJOW_Finalize,
    nsnull,               XPC_SJOW_CheckAccess,
    XPC_SJOW_Call,        XPC_SJOW_Create,
    nsnull,               nsnull,
    nsnull,               nsnull
  },
  
  XPC_SJOW_Equality,
  nsnull, 
  nsnull, 
  XPC_SJOW_Iterator,
  XPC_SJOW_WrappedObject,
  JSCLASS_NO_RESERVED_MEMBERS
};

JSBool
WrapObject(JSContext *cx, JSObject *scope, jsval v, jsval *vp)
{
  
  
  JSObject *objToWrap = UnsafeUnwrapSecurityWrapper(cx, JSVAL_TO_OBJECT(v));
  if (!objToWrap) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  
  
  

  if (objToWrap->getClass() == &js_ScriptClass ||
      (JS_ObjectIsFunction(cx, objToWrap) &&
       JS_GetFunctionFastNative(cx, JS_ValueToFunction(cx, v)) ==
       XPCWrapper::sEvalNative)) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  XPCWrappedNativeScope *xpcscope =
    XPCWrappedNativeScope::FindInJSObjectScope(cx, scope);
  NS_ASSERTION(xpcscope, "what crazy scope are we in?");

  XPCWrappedNative *wrappedNative;
  WrapperType type = xpcscope->GetWrapperFor(cx, objToWrap, SJOW,
                                             &wrappedNative);

  
  
  if (type != NONE && type != XOW && !(type & SJOW)) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  SLIM_LOG_WILL_MORPH(cx, objToWrap);
  if (IS_SLIM_WRAPPER(objToWrap) && !MorphSlimWrapper(cx, objToWrap)) {
    return ThrowException(NS_ERROR_FAILURE, cx);
  }

  JSObject *wrapperObj =
    JS_NewObjectWithGivenProto(cx, &SJOWClass.base, nsnull, scope);

  if (!wrapperObj) {
    
    return JS_FALSE;
  }

  *vp = OBJECT_TO_JSVAL(wrapperObj);
  if (!JS_SetReservedSlot(cx, wrapperObj, XPCWrapper::sWrappedObjSlot,
                          OBJECT_TO_JSVAL(objToWrap)) ||
      !JS_SetReservedSlot(cx, wrapperObj, XPCWrapper::sFlagsSlot, JSVAL_ZERO)) {
    return JS_FALSE;
  }

  return JS_TRUE;
}

PRBool
AttachNewConstructorObject(XPCCallContext &ccx, JSObject *aGlobalObject)
{
  
  
  
  if (!XPCWrapper::FindEval(ccx, aGlobalObject)) {
    return PR_FALSE;
  }

  JSObject *class_obj =
    ::JS_InitClass(ccx, aGlobalObject, nsnull, &SJOWClass.base,
                   XPC_SJOW_Construct, 0, nsnull, nsnull, nsnull, nsnull);
  if (!class_obj) {
    NS_WARNING("can't initialize the XPCSafeJSObjectWrapper class");
    return PR_FALSE;
  }

  if (!::JS_DefineFunction(ccx, class_obj, "toString", XPC_SJOW_toString,
                           0, 0)) {
    return PR_FALSE;
  }

  
  
  ::JS_SetPrototype(ccx, class_obj, nsnull);
  if (!::JS_SealObject(ccx, class_obj, JS_FALSE)) {
    NS_WARNING("Failed to seal XPCSafeJSObjectWrapper.prototype");
    return PR_FALSE;
  }

  JSBool found;
  return ::JS_SetPropertyAttributes(ccx, aGlobalObject,
                                    SJOWClass.base.name,
                                    JSPROP_READONLY | JSPROP_PERMANENT,
                                    &found);
}

JSObject *
GetUnsafeObject(JSContext *cx, JSObject *obj)
{
  obj = FindSafeObject(obj);

  if (!obj) {
    return nsnull;
  }

  jsval v;
  if (!JS_GetReservedSlot(cx, obj, XPCWrapper::sWrappedObjSlot, &v)) {
    JS_ClearPendingException(cx);
    return nsnull;
  }

  return JSVAL_IS_OBJECT(v) ? JSVAL_TO_OBJECT(v) : nsnull;
}

} 




static JSBool
WrapJSValue(JSContext *cx, JSObject *obj, jsval val, jsval *rval)
{
  JSBool ok = JS_TRUE;

  if (JSVAL_IS_PRIMITIVE(val)) {
    *rval = val;
  } else {
    if (!RewrapObject(cx, obj->getParent(), JSVAL_TO_OBJECT(val), SJOW,
                      rval)) {
      return JS_FALSE;
    }
    
    
    
    JSObject *safeObj = JSVAL_TO_OBJECT(*rval);
    if (safeObj->getJSClass() == &SJOWClass.base &&
        JS_GetGlobalForObject(cx, obj) != JS_GetGlobalForObject(cx, safeObj)) {
      
      
      
      
      nsCOMPtr<nsIPrincipal> srcObjPrincipal;
      nsCOMPtr<nsIPrincipal> subjPrincipal;
      nsCOMPtr<nsIPrincipal> valObjPrincipal;

      nsresult rv = FindPrincipals(cx, obj, getter_AddRefs(srcObjPrincipal),
                                   getter_AddRefs(subjPrincipal), nsnull);
      if (NS_FAILED(rv)) {
        return ThrowException(rv, cx);
      }

      rv = FindPrincipals(cx, JSVAL_TO_OBJECT(val),
                          getter_AddRefs(valObjPrincipal), nsnull, nsnull);
      if (NS_FAILED(rv)) {
        return ThrowException(rv, cx);
      }

      PRBool subsumes = PR_FALSE;
      rv = srcObjPrincipal->Subsumes(valObjPrincipal, &subsumes);
      if (NS_FAILED(rv)) {
        return ThrowException(rv, cx);
      }

      
      
      if (!subsumes && subjPrincipal) {
        PRBool subjSubsumes = PR_FALSE;
        rv = subjPrincipal->Subsumes(srcObjPrincipal, &subjSubsumes);
        if (NS_SUCCEEDED(rv) && subjSubsumes) {
          rv = subjPrincipal->Subsumes(valObjPrincipal, &subjSubsumes);
          if (NS_SUCCEEDED(rv) && subjSubsumes) {
            subsumes = PR_TRUE;
          }
        }
      }

      if (!subsumes) {
        
        
        
        
        
        if (!::JS_SetReservedSlot(cx, safeObj, sPrincipalSlot,
                                  PRIVATE_TO_JSVAL(srcObjPrincipal.get()))) {
          return JS_FALSE;
        }

        
        
        nsIPrincipal *tmp = nsnull;
        srcObjPrincipal.swap(tmp);
      }
    }
  }

  return ok;
}

static JSBool
XPC_SJOW_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  
  
  if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_CONSTRUCTOR) ||
      id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    return JS_TRUE;
  }

  obj = FindSafeObject(obj);
  NS_ASSERTION(obj != nsnull, "FindSafeObject() returned null in class hook!");

  
  
  jsval isResolving;
  JSBool ok = ::JS_GetReservedSlot(cx, obj, sFlagsSlot, &isResolving);
  if (!ok || HAS_FLAGS(isResolving, FLAG_RESOLVING)) {
    return ok;
  }

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  
  if (!CanCallerAccess(cx, obj, unsafeObj)) {
    
    return JS_FALSE;
  }

  if (!JSVAL_IS_PRIMITIVE(*vp)) {
    
    
    JSObject *added = JSVAL_TO_OBJECT(*vp);
    if (!RewrapObject(cx, JS_GetGlobalForObject(cx, unsafeObj), added,
                      UNKNOWN, vp)) {
      return JS_FALSE;
    }
  }

  return XPCWrapper::AddProperty(cx, obj, JS_FALSE, unsafeObj, id, vp);
}

static JSBool
XPC_SJOW_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  
  if (!CanCallerAccess(cx, obj, unsafeObj)) {
    
    return JS_FALSE;
  }

  return XPCWrapper::DelProperty(cx, unsafeObj, id, vp);
}

NS_STACK_CLASS class SafeCallGuard {
public:
  SafeCallGuard(JSContext *cx, nsIPrincipal *principal)
    : cx(cx), statics(cx), tvr(cx) {
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (ssm) {
      
      
      nsresult rv = ssm->PushContextPrincipal(cx, nsnull, principal);
      if (NS_FAILED(rv)) {
        NS_WARNING("Not allowing call because we're out of memory");
        JS_ReportOutOfMemory(cx);
        this->cx = nsnull;
        return;
      }
    }

    js_SaveAndClearRegExpStatics(cx, &statics, &tvr);
    fp = JS_SaveFrameChain(cx);
    options =
      JS_SetOptions(cx, JS_GetOptions(cx) | JSOPTION_DONT_REPORT_UNCAUGHT);
  }

  JSBool ready() {
    return cx != nsnull;
  }

  ~SafeCallGuard() {
    if (cx) {
      JS_SetOptions(cx, options);
      JS_RestoreFrameChain(cx, fp);
      js_RestoreRegExpStatics(cx, &statics, &tvr);
      nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
      if (ssm) {
        ssm->PopContextPrincipal(cx);
      }
    }
  }

private:
  JSContext *cx;
  JSRegExpStatics statics;
  js::AutoStringRooter tvr;
  uint32 options;
  JSStackFrame *fp;
};

static JSBool
XPC_SJOW_GetOrSetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp,
                          JSBool aIsSet)
{
  
  if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    return JS_TRUE;
  }

  obj = FindSafeObject(obj);
  NS_ASSERTION(obj != nsnull, "FindSafeObject() returned null in class hook!");

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  
  if (!CanCallerAccess(cx, obj, unsafeObj)) {
    
    return JS_FALSE;
  }

  {
    SafeCallGuard guard(cx, FindObjectPrincipals(cx, obj, unsafeObj));
    if (!guard.ready()) {
      return JS_FALSE;
    }

    if (aIsSet &&
        !JSVAL_IS_PRIMITIVE(*vp) &&
        !RewrapObject(cx, JS_GetGlobalForObject(cx, unsafeObj),
                      JSVAL_TO_OBJECT(*vp), UNKNOWN, vp)) {
      return JS_FALSE;
    }

    JSBool ok = aIsSet
                ? JS_SetPropertyById(cx, unsafeObj, id, vp)
                : JS_GetPropertyById(cx, unsafeObj, id, vp);
    if (!ok) {
      return JS_FALSE;
    }
  }

  return WrapJSValue(cx, obj, *vp, vp);
}

static JSBool
XPC_SJOW_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  return XPC_SJOW_GetOrSetProperty(cx, obj, id, vp, PR_FALSE);
}

static JSBool
XPC_SJOW_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  return XPC_SJOW_GetOrSetProperty(cx, obj, id, vp, PR_TRUE);
}

static JSBool
XPC_SJOW_Enumerate(JSContext *cx, JSObject *obj)
{
  obj = FindSafeObject(obj);
  NS_ASSERTION(obj != nsnull, "FindSafeObject() returned null in class hook!");

  
  
  
  
  
  

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    return JS_TRUE;
  }

  
  if (!CanCallerAccess(cx, obj, unsafeObj)) {
    
    return JS_FALSE;
  }

  
  
  
  

  return XPCWrapper::Enumerate(cx, obj, unsafeObj);
}

static JSBool
XPC_SJOW_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                    JSObject **objp)
{
  obj = FindSafeObject(obj);
  NS_ASSERTION(obj != nsnull, "FindSafeObject() returned null in class hook!");

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    

    return JS_TRUE;
  }

  
  if (!CanCallerAccess(cx, obj, unsafeObj)) {
    
    return JS_FALSE;
  }

  
  if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_TO_STRING)) {
    *objp = obj;
    return JS_DefineFunction(cx, obj, "toString",
                             XPC_SJOW_toString, 0, 0) != nsnull;
  }

  return XPCWrapper::NewResolve(cx, obj, JS_FALSE, unsafeObj, id, flags,
                                objp);
}

static JSBool
XPC_SJOW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
  NS_ASSERTION(type != JSTYPE_STRING, "toString failed us");
  return JS_TRUE;
}

static void
XPC_SJOW_Finalize(JSContext *cx, JSObject *obj)
{
  
  jsval v;
  if (::JS_GetReservedSlot(cx, obj, sPrincipalSlot, &v) && !JSVAL_IS_VOID(v)) {
    nsIPrincipal *principal = (nsIPrincipal *)JSVAL_TO_PRIVATE(v);

    NS_RELEASE(principal);
  }
}

static JSBool
XPC_SJOW_CheckAccess(JSContext *cx, JSObject *obj, jsid id,
                     JSAccessMode mode, jsval *vp)
{
  
  if ((mode & JSACC_WATCH) == JSACC_PROTO && (mode & JSACC_WRITE)) {
    return ThrowException(NS_ERROR_XPC_SECURITY_MANAGER_VETO, cx);
  }

  
  JSSecurityCallbacks *callbacks = JS_GetSecurityCallbacks(cx);
  if (callbacks && callbacks->checkObjectAccess &&
      !callbacks->checkObjectAccess(cx, obj, id, mode, vp)) {
    return JS_FALSE;
  }

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    return JS_TRUE;
  }

  
  
  if (callbacks && callbacks->checkObjectAccess &&
      !callbacks->checkObjectAccess(cx, unsafeObj, id, mode, vp)) {
    return JS_FALSE;
  }

  JSClass *clazz = unsafeObj->getJSClass();
  return !clazz->checkAccess ||
    clazz->checkAccess(cx, unsafeObj, id, mode, vp);
}

static JSBool
XPC_SJOW_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
  JSObject *tmp = FindSafeObject(obj);
  JSObject *unsafeObj, *callThisObj = nsnull;

  if (tmp) {
    
    
    
    
    
    obj = tmp;
  } else {
    
    
    
    
    
    callThisObj = obj;

    
    
    if (!CanCallerAccess(cx, nsnull, callThisObj)) {
      
      return JS_FALSE;
    }

    obj = FindSafeObject(JSVAL_TO_OBJECT(argv[-2]));

    if (!obj) {
      return ThrowException(NS_ERROR_INVALID_ARG, cx);
    }
  }

  unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    return ThrowException(NS_ERROR_UNEXPECTED, cx);
  }

  if (!callThisObj) {
    callThisObj = unsafeObj;
  }

  JSObject *safeObj = JSVAL_TO_OBJECT(argv[-2]);
  JSObject *funToCall = GetUnsafeObject(cx, safeObj);

  if (!funToCall) {
    
    
    
    return JS_TRUE;
  }

  
  
  if (!CanCallerAccess(cx, safeObj, unsafeObj) ||
      !CanCallerAccess(cx, nsnull, funToCall)) {
    
    return JS_FALSE;
  }

  {
    SafeCallGuard guard(cx, FindObjectPrincipals(cx, safeObj, funToCall));

    JSObject *scope = JS_GetGlobalForObject(cx, funToCall);
    for (uintN i = 0; i < argc; ++i) {
      
      if (!JSVAL_IS_PRIMITIVE(argv[i]) &&
          !RewrapObject(cx, scope, JSVAL_TO_OBJECT(argv[i]), NONE, &argv[i])) {
        return JS_FALSE;
      }
    }

    jsval v;
    if (!RewrapObject(cx, scope, callThisObj, NONE, &v)) {
      return JS_FALSE;
    }

    if (!JS_CallFunctionValue(cx, JSVAL_TO_OBJECT(v), OBJECT_TO_JSVAL(funToCall),
                              argc, argv, rval)) {
      return JS_FALSE;
    }
  }

  return WrapJSValue(cx, safeObj, *rval, rval);
}

static JSBool
XPC_SJOW_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                   jsval *rval)
{
  if (argc < 1) {
    return ThrowException(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx);
  }

  
  
  
  JSObject *scope = JS_GetGlobalForObject(cx, obj);

  if (JSVAL_IS_PRIMITIVE(argv[0])) {
    JSStackFrame *fp = nsnull;
    if (JS_FrameIterator(cx, &fp) && JS_IsConstructorFrame(cx, fp)) {
      return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
    }

    *rval = argv[0];
    return JS_TRUE;
  }

  JSObject *objToWrap = UnsafeUnwrapSecurityWrapper(cx, JSVAL_TO_OBJECT(argv[0]));
  if (!objToWrap) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  
  if (!CanCallerAccess(cx, nsnull, objToWrap)) {
    
    return JS_FALSE;
  }

  return WrapObject(cx, scope, OBJECT_TO_JSVAL(objToWrap), rval);
}

static JSBool
XPC_SJOW_Create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
  JSObject *callee = JSVAL_TO_OBJECT(argv[-2]);
  NS_ASSERTION(GetUnsafeObject(cx, callee), "How'd we get here?");
  JSObject *unsafeObj = GetUnsafeObject(cx, callee);

  
  if (!CanCallerAccess(cx, callee, unsafeObj)) {
    
    return JS_FALSE;
  }

  {
    SafeCallGuard guard(cx, FindObjectPrincipals(cx, callee, unsafeObj));
    if (!guard.ready()) {
      return JS_FALSE;
    }

    JSObject *scope = JS_GetGlobalForObject(cx, unsafeObj);
    for (uintN i = 0; i < argc; ++i) {
      
      if (!JSVAL_IS_PRIMITIVE(argv[i]) &&
          !RewrapObject(cx, scope, JSVAL_TO_OBJECT(argv[i]), NONE, &argv[i])) {
        return JS_FALSE;
      }
    }

    jsval v;
    if (!RewrapObject(cx, scope, obj, NONE, &v)) {
      return JS_FALSE;
    }

    if (!JS_CallFunctionValue(cx, JSVAL_TO_OBJECT(v), OBJECT_TO_JSVAL(unsafeObj),
                              argc, argv, rval)) {
      return JS_FALSE;
    }
  }

  return WrapJSValue(cx, callee, *rval, rval);
}

static JSBool
XPC_SJOW_Equality(JSContext *cx, JSObject *obj, const jsval *valp, JSBool *bp)
{
  if (JSVAL_IS_PRIMITIVE(*valp)) {
    *bp = JS_FALSE;
  } else {
    JSObject *unsafeObj = GetUnsafeObject(cx, obj);

    JSObject *other = JSVAL_TO_OBJECT(*valp);
    JSObject *otherUnsafe = GetUnsafeObject(cx, other);

    
    
    
    
    
    
    if (obj == other || unsafeObj == other ||
        (unsafeObj && unsafeObj == otherUnsafe)) {
      *bp = JS_TRUE;
    } else {
      nsISupports *objIdentity = XPC_GetIdentityObject(cx, obj);
      nsISupports *otherIdentity = XPC_GetIdentityObject(cx, other);

      *bp = objIdentity && objIdentity == otherIdentity;
    }
  }

  return JS_TRUE;
}

static JSObject *
XPC_SJOW_Iterator(JSContext *cx, JSObject *obj, JSBool keysonly)
{
  obj = FindSafeObject(obj);
  NS_ASSERTION(obj != nsnull, "FindSafeObject() returned null in class hook!");

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);
  if (!unsafeObj) {
    ThrowException(NS_ERROR_INVALID_ARG, cx);

    return nsnull;
  }

  
  if (!CanCallerAccess(cx, obj, unsafeObj)) {
    
    return nsnull;
  }

  
  JSObject *wrapperIter =
    JS_NewObjectWithGivenProto(cx, &SJOWClass.base, nsnull,
                               JS_GetGlobalForObject(cx, obj));
  if (!wrapperIter) {
    return nsnull;
  }

  if (!JS_SetReservedSlot(cx, wrapperIter, XPCWrapper::sWrappedObjSlot,
                          OBJECT_TO_JSVAL(unsafeObj)) ||
      !JS_SetReservedSlot(cx, wrapperIter, XPCWrapper::sFlagsSlot,
                          JSVAL_ZERO)) {
    return nsnull;
  }

  js::AutoObjectRooter tvr(cx, wrapperIter);

  
  return XPCWrapper::CreateIteratorObj(cx, wrapperIter, obj, unsafeObj,
                                       keysonly);
}

static JSObject *
XPC_SJOW_WrappedObject(JSContext *cx, JSObject *obj)
{
  return GetUnsafeObject(cx, obj);
}

static JSBool
XPC_SJOW_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                  jsval *rval)
{
  obj = FindSafeObject(obj);
  if (!obj) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  JSObject *unsafeObj = GetUnsafeObject(cx, obj);

  if (!unsafeObj) {
    
    
    

    JSString *str = JS_NewStringCopyZ(cx, "[object XPCSafeJSObjectWrapper]");
    if (!str) {
      return JS_FALSE;
    }

    *rval = STRING_TO_JSVAL(str);

    return JS_TRUE;
  }

  
  if (!CanCallerAccess(cx, obj, unsafeObj)) {
    
    return JS_FALSE;
  }

  {
    SafeCallGuard guard(cx, FindObjectPrincipals(cx, obj, unsafeObj));
    if (!guard.ready()) {
      return JS_FALSE;
    }

    JSString *str = JS_ValueToString(cx, OBJECT_TO_JSVAL(unsafeObj));
    if (!str) {
      return JS_FALSE;
    }
    *rval = STRING_TO_JSVAL(str);
  }
  return JS_TRUE;
}
