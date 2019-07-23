






































#include "nscore.h"
#include "jsapi.h"

class nsIPrincipal;

class XPCNativeWrapper
{
public:
  static PRBool AttachNewConstructorObject(XPCCallContext &ccx,
                                           JSObject *aGlobalObject);

  static JSObject *GetNewOrUsed(JSContext *cx, XPCWrappedNative *wrapper,
                                nsIPrincipal *aObjectPrincipal);

  static PRBool IsNativeWrapperClass(JSClass *clazz)
  {
    return clazz == &sXPC_NW_JSClass.base;
  }

  static PRBool IsNativeWrapper(JSObject *obj)
  {
    return STOBJ_GET_CLASS(obj) == &sXPC_NW_JSClass.base;
  }

  static XPCWrappedNative *GetWrappedNative(JSObject *obj)
  {
    return (XPCWrappedNative *)xpc_GetJSPrivate(obj);
  }

  static JSClass *GetJSClass()
  {
    return &sXPC_NW_JSClass.base;
  }

  static void ClearWrappedNativeScopes(JSContext* cx,
                                       XPCWrappedNative* wrapper);

protected:
  static JSExtendedClass sXPC_NW_JSClass;
};

JSBool
XPC_XOW_WrapObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                   jsval *rval);
