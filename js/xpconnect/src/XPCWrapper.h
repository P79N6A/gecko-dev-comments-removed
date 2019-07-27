





#ifndef XPC_WRAPPER_H
#define XPC_WRAPPER_H 1

#include "xpcprivate.h"
#include "jswrapper.h"

namespace XPCNativeWrapper {





#define NATIVE_HAS_FLAG(_wn, _flag)                                           \
  ((_wn)->GetScriptableInfo() &&                                              \
   (_wn)->GetScriptableInfo()->GetFlags()._flag())

bool
AttachNewConstructorObject(JSContext* aCx, JS::HandleObject aGlobalObject);

} 






namespace XPCWrapper {

JSObject*
UnsafeUnwrapSecurityWrapper(JSObject* obj);

} 


#endif
