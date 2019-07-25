









































#ifndef XPC_WRAPPER_H
#define XPC_WRAPPER_H 1

#include "xpcprivate.h"

namespace XPCNativeWrapper {





#define NATIVE_HAS_FLAG(_wn, _flag)                \
  ((_wn)->GetScriptableInfo() &&                   \
   (_wn)->GetScriptableInfo()->GetFlags()._flag())

PRBool
AttachNewConstructorObject(XPCCallContext &ccx, JSObject *aGlobalObject);

} 






namespace XPCWrapper {




inline nsIScriptSecurityManager *
GetSecurityManager()
{
  return nsXPConnect::gScriptSecurityManager;
}

inline JSBool
IsSecurityWrapper(JSObject *wrapper)
{
  return wrapper->isWrapper();
}











JSObject *
Unwrap(JSContext *cx, JSObject *wrapper);

JSObject *
UnsafeUnwrapSecurityWrapper(JSObject *obj);

} 


#endif
