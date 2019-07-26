





#ifndef XPC_WRAPPER_H
#define XPC_WRAPPER_H 1

#include "xpcprivate.h"
#include "xpcpublic.h"

namespace XPCNativeWrapper {





#define NATIVE_HAS_FLAG(_wn, _flag)                                           \
  ((_wn)->GetScriptableInfo() &&                                              \
   (_wn)->GetScriptableInfo()->GetFlags()._flag())

bool
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
  return js::IsWrapper(wrapper);
}

JSObject *
UnsafeUnwrapSecurityWrapper(JSObject *obj);

} 


#endif
