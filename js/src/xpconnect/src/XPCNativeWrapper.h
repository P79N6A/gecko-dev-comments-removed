






































#include "nscore.h"
#include "jsapi.h"

class nsIPrincipal;

namespace XPCNativeWrapper {

namespace internal {
extern js::Class NW_NoCall_Class;
extern js::Class NW_Call_Class;
}

PRBool
AttachNewConstructorObject(XPCCallContext &ccx, JSObject *aGlobalObject);

JSObject *
GetNewOrUsed(JSContext *cx, XPCWrappedNative *wrapper,
             JSObject *scope, nsIPrincipal *aObjectPrincipal);
JSBool
CreateExplicitWrapper(JSContext *cx, XPCWrappedNative *wrapper, jsval *rval);

inline PRBool
IsNativeWrapperClass(js::Class *clazz)
{
  return clazz == &internal::NW_NoCall_Class ||
         clazz == &internal::NW_Call_Class;
}

inline PRBool
IsNativeWrapper(JSObject *obj)
{
  return IsNativeWrapperClass(obj->getClass());
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
    ? js::Jsvalify(&internal::NW_Call_Class)
    : js::Jsvalify(&internal::NW_NoCall_Class);
}

void
ClearWrappedNativeScopes(JSContext* cx, XPCWrappedNative* wrapper);

}
