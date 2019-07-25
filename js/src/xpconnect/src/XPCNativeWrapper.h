






































#include "nscore.h"
#include "jsapi.h"

class nsIPrincipal;

namespace XPCNativeWrapper {

namespace internal {
  extern JSExtendedClass NW_NoCall_Class;
  extern JSExtendedClass NW_Call_Class;
}

PRBool
AttachNewConstructorObject(XPCCallContext &ccx, JSObject *aGlobalObject);

JSObject *
GetNewOrUsed(JSContext *cx, XPCWrappedNative *wrapper,
             JSObject *scope, nsIPrincipal *aObjectPrincipal);
JSBool
CreateExplicitWrapper(JSContext *cx, XPCWrappedNative *wrapper, jsval *rval);

inline PRBool
IsNativeWrapperClass(JSClass *clazz)
{
  return clazz == &internal::NW_NoCall_Class.base ||
         clazz == &internal::NW_Call_Class.base;
}

inline PRBool
IsNativeWrapper(JSObject *obj)
{
  return IsNativeWrapperClass(obj->getJSClass());
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
GetJSClass(bool call)
{
  return call
    ? &internal::NW_Call_Class.base
    : &internal::NW_NoCall_Class.base;
}

void
ClearWrappedNativeScopes(JSContext* cx, XPCWrappedNative* wrapper);

}
