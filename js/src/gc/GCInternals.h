






#ifndef jsgc_internal_h___
#define jsgc_internal_h___

#include "jsapi.h"

namespace js {
namespace gc {

void
MarkRuntime(JSTracer *trc, bool useSavedRoots = false);

} 
} 

#endif 
