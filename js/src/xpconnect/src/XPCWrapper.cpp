









































#include "XPCWrapper.h"
#include "AccessCheck.h"
#include "WrapperFactory.h"

namespace XPCNativeWrapper {

static inline
JSBool
ThrowException(nsresult ex, JSContext *cx)
{
  XPCThrower::Throw(ex, cx);

  return JS_FALSE;
}

static JSBool
UnwrapNW(JSContext *cx, uintN argc, jsval *vp)
{
  if (argc != 1) {
    return ThrowException(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx);
  }

  jsval v = JS_ARGV(cx, vp)[0];
  if (JSVAL_IS_PRIMITIVE(v)) {
    return ThrowException(NS_ERROR_INVALID_ARG, cx);
  }

  JSObject *obj = JSVAL_TO_OBJECT(v);
  if (!obj->isWrapper()) {
    JS_SET_RVAL(cx, vp, v);
    return JS_TRUE;
  }

  if (xpc::WrapperFactory::IsXrayWrapper(obj) &&
      !xpc::WrapperFactory::IsPartiallyTransparent(obj)) {
    return JS_GetProperty(cx, obj, "wrappedJSObject", vp);
  }

  JS_SET_RVAL(cx, vp, v);
  return JS_TRUE;
}

static JSBool
XrayWrapperConstructor(JSContext *cx, uintN argc, jsval *vp)
{
  if (argc == 0) {
    return ThrowException(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx);
  }

  if (JSVAL_IS_PRIMITIVE(vp[2])) {
    return ThrowException(NS_ERROR_ILLEGAL_VALUE, cx);
  }

  JSObject *obj = JSVAL_TO_OBJECT(vp[2]);
  if (!obj->isWrapper()) {
    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
  }

  obj = obj->unwrap();

  *vp = OBJECT_TO_JSVAL(obj);
  return JS_WrapValue(cx, vp);
}

PRBool
AttachNewConstructorObject(XPCCallContext &ccx, JSObject *aGlobalObject)
{
  JSObject *xpcnativewrapper =
    JS_DefineFunction(ccx, aGlobalObject, "XPCNativeWrapper",
                      XrayWrapperConstructor, 1,
                      JSPROP_READONLY | JSPROP_PERMANENT | JSFUN_STUB_GSOPS | JSFUN_CONSTRUCTOR);
  if (!xpcnativewrapper) {
    return PR_FALSE;
  }
  return JS_DefineFunction(ccx, xpcnativewrapper, "unwrap", UnwrapNW, 1,
                           JSPROP_READONLY | JSPROP_PERMANENT) != nsnull;
}
}

namespace XPCWrapper {

JSObject *
Unwrap(JSContext *cx, JSObject *wrapper)
{
  if (wrapper->isWrapper()) {
    if (xpc::AccessCheck::isScriptAccessOnly(cx, wrapper))
      return nsnull;
    return wrapper->unwrap();
  }

  return nsnull;
}

JSObject *
UnsafeUnwrapSecurityWrapper(JSObject *obj)
{
  if (obj->isProxy()) {
    return obj->unwrap();
  }

  return obj;
}

}
