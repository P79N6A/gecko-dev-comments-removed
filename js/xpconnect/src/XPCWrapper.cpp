





#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "WrapperFactory.h"
#include "AccessCheck.h"

using namespace xpc;
using namespace mozilla;

namespace XPCNativeWrapper {

static inline
bool
ThrowException(nsresult ex, JSContext *cx)
{
  XPCThrower::Throw(ex, cx);

  return false;
}

static bool
UnwrapNW(JSContext *cx, unsigned argc, jsval *vp)
{
  if (argc != 1) {
    return ThrowException(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx);
  }

  JS::RootedValue v(cx, JS_ARGV(cx, vp)[0]);
  if (!v.isObject() || !js::IsWrapper(&v.toObject())) {
    JS_SET_RVAL(cx, vp, v);
    return true;
  }

  if (AccessCheck::wrapperSubsumes(&v.toObject())) {
    bool ok = xpc::WrapperFactory::WaiveXrayAndWrap(cx, &v);
    NS_ENSURE_TRUE(ok, false);
  }

  JS_SET_RVAL(cx, vp, v);
  return true;
}

static bool
XrayWrapperConstructor(JSContext *cx, unsigned argc, jsval *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() == 0) {
    return ThrowException(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx);
  }

  if (!args[0].isObject()) {
    args.rval().set(args[0]);
    return true;
  }

  args.rval().setObject(*js::UncheckedUnwrap(&args[0].toObject()));
  return JS_WrapValue(cx, args.rval());
}

bool
AttachNewConstructorObject(JSContext *aCx, JS::HandleObject aGlobalObject)
{
  
  
  JSAutoCompartment ac(aCx, aGlobalObject);
  JSFunction *xpcnativewrapper =
    JS_DefineFunction(aCx, aGlobalObject, "XPCNativeWrapper",
                      XrayWrapperConstructor, 1,
                      JSPROP_READONLY | JSPROP_PERMANENT | JSFUN_STUB_GSOPS | JSFUN_CONSTRUCTOR);
  if (!xpcnativewrapper) {
    return false;
  }
  return JS_DefineFunction(aCx, JS_GetFunctionObject(xpcnativewrapper), "unwrap", UnwrapNW, 1,
                           JSPROP_READONLY | JSPROP_PERMANENT) != nullptr;
}

} 

namespace XPCWrapper {

JSObject *
UnsafeUnwrapSecurityWrapper(JSObject *obj)
{
  if (js::IsProxy(obj)) {
    return js::UncheckedUnwrap(obj);
  }

  return obj;
}

} 
