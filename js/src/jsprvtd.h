





#ifndef jsprvtd_h
#define jsprvtd_h





#include "jsapi.h"

typedef uintptr_t   jsatomid;

namespace js {

typedef JSNative             Native;
typedef JSParallelNative     ParallelNative;
typedef JSThreadSafeNative   ThreadSafeNative;
typedef JSPropertyOp         PropertyOp;
typedef JSStrictPropertyOp   StrictPropertyOp;
typedef JSPropertyDescriptor PropertyDescriptor;

} 

#endif 
