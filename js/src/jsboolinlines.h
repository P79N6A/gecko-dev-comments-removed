






































#ifndef jsboolinlines_h___
#define jsboolinlines_h___

#include "jsobjinlines.h"

namespace js {

inline bool
BooleanGetPrimitiveValue(JSContext *cx, JSObject &obj, Value *vp)
{
    if (obj.isBoolean()) {
        *vp = obj.getPrimitiveThis();
        return true;
    }

    extern bool BooleanGetPrimitiveValueSlow(JSContext *, JSObject &, Value *);
    return BooleanGetPrimitiveValueSlow(cx, obj, vp);
}

} 

#endif 
