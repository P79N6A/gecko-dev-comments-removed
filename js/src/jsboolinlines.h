






































#ifndef jsboolinlines_h___
#define jsboolinlines_h___

#include "jsobjinlines.h"

#include "vm/BooleanObject-inl.h"

namespace js {

inline bool
BooleanGetPrimitiveValue(JSContext *cx, JSObject &obj, Value *vp)
{
    if (obj.isBoolean()) {
        *vp = BooleanValue(obj.asBoolean().unbox());
        return true;
    }

    extern bool BooleanGetPrimitiveValueSlow(JSContext *, JSObject &, Value *);
    return BooleanGetPrimitiveValueSlow(cx, obj, vp);
}

} 

#endif 
