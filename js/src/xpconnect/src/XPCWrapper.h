









































#ifndef XPC_WRAPPER_H
#define XPC_WRAPPER_H 1

#include "xpcprivate.h"


#define FLAG_DEEP     0x1
#define FLAG_EXPLICIT 0x2


#define FLAG_RESOLVING 0x4

#define HAS_FLAGS(_val, _flags) \
  ((PRUint32(JSVAL_TO_INT(_val)) & (_flags)) != 0)

#define NATIVE_HAS_FLAG(_wn, _flag)                \
  ((_wn)->GetScriptableInfo() &&                   \
   (_wn)->GetScriptableInfo()->GetFlags()._flag())

JSBool
XPC_NW_WrapFunction(JSContext* cx, JSObject* funobj, jsval *rval);

JSBool
XPC_NW_RewrapIfDeepWrapper(JSContext *cx, JSObject *obj, jsval v,
                           jsval *rval);


JSBool
XPC_XOW_WrapFunction(JSContext *cx, JSObject *wrapperObj, JSObject *funobj,
                     jsval *rval);

JSBool
XPC_XOW_RewrapIfNeeded(JSContext *cx, JSObject *wrapperObj, jsval *vp);

JSBool
XPC_XOW_WrapperMoved(JSContext *cx, XPCWrappedNative *innerObj,
                     XPCWrappedNativeScope *newScope);

nsresult
IsWrapperSameOrigin(JSContext *cx, JSObject *wrappedObj);

inline JSBool
XPC_XOW_ClassNeedsXOW(const char *name)
{
  
  return !strcmp(name, "Window")            ||
         !strcmp(name, "Location")          ||
         !strcmp(name, "HTMLIFrameElement") ||
         !strcmp(name, "HTMLFrameElement");
}

extern JSExtendedClass sXPC_XOW_JSClass;






class XPCWrapper
{
public:
  



  static const PRUint32 sWrappedObjSlot;

  




  static const PRUint32 sResolvingSlot;

  


  static const PRUint32 sNumSlots;

  



  static JSNative sEvalNative;

  


  static JSBool FindEval(XPCCallContext &ccx, JSObject *obj) {
    if (sEvalNative) {
      return JS_TRUE;
    }

    jsval eval_val;
    if (!::JS_GetProperty(ccx, obj, "eval", &eval_val)) {
      return ThrowException(NS_ERROR_UNEXPECTED, ccx);
    }

    if (JSVAL_IS_PRIMITIVE(eval_val) ||
        !::JS_ObjectIsFunction(ccx, JSVAL_TO_OBJECT(eval_val))) {
      return ThrowException(NS_ERROR_UNEXPECTED, ccx);
    }

    sEvalNative =
      ::JS_GetFunctionNative(ccx, ::JS_ValueToFunction(ccx, eval_val));

    if (!sEvalNative) {
      return ThrowException(NS_ERROR_UNEXPECTED, ccx);
    }

    return JS_TRUE;
  }

  


  static JSBool ThrowException(nsresult ex, JSContext *cx) {
    XPCThrower::Throw(ex, cx);
    return JS_FALSE;
  }

  


  static nsIScriptSecurityManager *GetSecurityManager() {
    extern nsIScriptSecurityManager *gScriptSecurityManager;

    return gScriptSecurityManager;
  }

  



  static JSBool MaybePreserveWrapper(JSContext *cx, XPCWrappedNative *wn,
                                     uintN flags) {
    if ((flags & JSRESOLVE_ASSIGNING) &&
        (::JS_GetOptions(cx) & JSOPTION_PRIVATE_IS_NSISUPPORTS)) {
      nsCOMPtr<nsIXPCScriptNotify> scriptNotify = 
        do_QueryInterface(static_cast<nsISupports*>
                                     (JS_GetContextPrivate(cx)));
      if (scriptNotify) {
        return NS_SUCCEEDED(scriptNotify->PreserveWrapper(wn));
      }
    }
    return JS_TRUE;
  }

  


  static JSObject *Unwrap(JSContext *cx, JSObject *wrapper) {
    if (JS_GET_CLASS(cx, wrapper) != &sXPC_XOW_JSClass.base) {
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

    JSObject *wrappedObj = JSVAL_TO_OBJECT(v);
    nsresult rv = IsWrapperSameOrigin(cx, wrappedObj);
    if (NS_FAILED(rv)) {
      JS_ClearPendingException(cx);
      return nsnull;
    }

    return wrappedObj;
  }

  



  static JSBool RewrapIfDeepWrapper(JSContext *cx, JSObject *obj, jsval v,
                                    jsval *rval, JSBool isNativeWrapper) {
    *rval = v;
    return isNativeWrapper
           ? XPC_NW_RewrapIfDeepWrapper(cx, obj, v, rval)
           : XPC_XOW_RewrapIfNeeded(cx, obj, rval);
  }

  





  static inline JSBool WrapFunction(JSContext *cx, JSObject *wrapperObj,
                                    JSObject *funobj, jsval *v,
                                    JSBool isNativeWrapper) {
    return isNativeWrapper
           ? XPC_NW_WrapFunction(cx, funobj, v)
           : XPC_XOW_WrapFunction(cx, wrapperObj, funobj, v);
  }

  





  static JSObject *CreateIteratorObj(JSContext *cx,
                                     JSObject *tempWrapper,
                                     JSObject *wrapperObj,
                                     JSObject *innerObj,
                                     JSBool keysonly);

  


  static JSBool AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

  


  static JSBool DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

  


  static JSBool Enumerate(JSContext *cx, JSObject *wrapperObj,
                          JSObject *innerObj);

  




  static JSBool NewResolve(JSContext *cx, JSObject *wrapperObj,
                           JSObject *innerObj, jsval id, uintN flags,
                           JSObject **objp, JSBool preserveVal = JS_FALSE);

  




  static JSBool ResolveNativeProperty(JSContext *cx, JSObject *wrapperObj,
                                      JSObject *innerObj, XPCWrappedNative *wn,
                                      jsval id, uintN flags, JSObject **objp,
                                      JSBool isNativeWrapper);

  




  static JSBool GetOrSetNativeProperty(JSContext *cx, JSObject *obj,
                                       XPCWrappedNative *wrappedNative,
                                       jsval id, jsval *vp, JSBool aIsSet,
                                       JSBool isNativeWrapper);

  


  static JSBool NativeToString(JSContext *cx, XPCWrappedNative *wrappedNative,
                               uintN argc, jsval *argv, jsval *rval,
                               JSBool isNativeWrapper);
};


#endif
