





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

enum XDRMode {
    XDR_ENCODE,
    XDR_DECODE
};

struct IdValuePair
{
    jsid id;
    Value value;

    IdValuePair() {}
    IdValuePair(jsid idArg)
      : id(idArg), value(UndefinedValue())
    {}
};

} 

#endif 
