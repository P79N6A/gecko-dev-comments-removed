





#ifndef vm_SelfHosting_h_
#define vm_SelfHosting_h_

#include "jsapi.h"

class JSAtom;

namespace js {








extern const JSWrapObjectCallbacks SelfHostingWrapObjectCallbacks;





bool IsSelfHostedFunctionWithName(JSFunction *fun, JSAtom *name);

} 

#endif 
