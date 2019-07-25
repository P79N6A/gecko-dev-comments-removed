









































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

extern nsIScriptSecurityManager *gScriptSecurityManager;






namespace XPCWrapper {




inline nsIScriptSecurityManager *
GetSecurityManager()
{
  return ::gScriptSecurityManager;
}

inline JSBool
IsSecurityWrapper(JSObject *wrapper)
{
  return wrapper->isWrapper();
}











JSObject *
Unwrap(JSContext *cx, JSObject *wrapper);

JSObject *
UnsafeUnwrapSecurityWrapper(JSContext *cx, JSObject *obj);

} 


#endif
