





#ifndef vm_SelfHosting_h_
#define vm_SelfHosting_h_

#include "jsapi.h"

class JSAtom;

namespace js {





bool IsSelfHostedFunctionWithName(JSFunction* fun, JSAtom* name);


void
FillSelfHostingCompileOptions(JS::CompileOptions& options);

} 

#endif 
