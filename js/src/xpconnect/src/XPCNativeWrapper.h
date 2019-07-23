






































#include "nscore.h"
#include "jsapi.h"

class nsIPrincipal;

namespace XPCNativeWrapper {

namespace internal { extern JSExtendedClass NWClass; }

PRBool
AttachNewConstructorObject(XPCCallContext &ccx, JSObject *aGlobalObject);

JSObject *
GetNewOrUsed(JSContext *cx, XPCWrappedNative *wrapper,
             JSObject *scope, nsIPrincipal *aObjectPrincipal);
JSBool
CreateExplicitWrapper(JSContext *cx, XPCWrappedNative *wrapper, JSBool deep,
                      jsval *rval);

inline PRBool
IsNativeWrapperClass(JSClass *clazz)
{
  return clazz == &internal::NWClass.base;
}

inline PRBool
IsNativeWrapper(JSObject *obj)
{
  return STOBJ_GET_CLASS(obj) == &internal::NWClass.base;
}

JSBool
GetWrappedNative(JSContext *cx, JSObject *obj,
                 XPCWrappedNative **aWrappedNative);


inline XPCWrappedNative *
SafeGetWrappedNative(JSObject *obj)
{
  return static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));
}

inline JSClass *
GetJSClass()
{
  return &internal::NWClass.base;
}

void
ClearWrappedNativeScopes(JSContext* cx, XPCWrappedNative* wrapper);

}
