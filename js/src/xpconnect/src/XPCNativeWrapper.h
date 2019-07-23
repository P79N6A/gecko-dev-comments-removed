






































#include "nscore.h"
#include "jsapi.h"

class XPCNativeWrapper
{
public:
  static PRBool AttachNewConstructorObject(XPCCallContext &ccx,
                                           JSObject *aGlobalObject);

  static JSObject *GetNewOrUsed(JSContext *cx, XPCWrappedNative *wrapper);

  static PRBool IsNativeWrapperClass(JSClass *clazz)
  {
    return clazz == &sXPC_NW_JSClass.base;
  }

  static PRBool IsNativeWrapper(JSContext *cx, JSObject *obj)
  {
    return JS_GET_CLASS(cx, obj) == &sXPC_NW_JSClass.base;
  }

  static XPCWrappedNative *GetWrappedNative(JSContext *cx, JSObject *obj)
  {
    return (XPCWrappedNative *)::JS_GetPrivate(cx, obj);
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
