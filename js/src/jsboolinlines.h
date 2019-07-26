





#ifndef jsboolinlines_h
#define jsboolinlines_h

#include "jsbool.h"

#include "vm/BooleanObject.h"
#include "vm/WrapperObject.h"

namespace js {

bool
BooleanGetPrimitiveValueSlow(HandleObject);

inline bool
BooleanGetPrimitiveValue(HandleObject obj)
{
    if (obj->is<BooleanObject>())
        return obj->as<BooleanObject>().unbox();

    return BooleanGetPrimitiveValueSlow(obj);
}

inline bool
EmulatesUndefined(JSObject *obj)
{
    JSObject *actual = MOZ_LIKELY(!obj->is<WrapperObject>()) ? obj : UncheckedUnwrap(obj);
    return actual->getClass()->emulatesUndefined();
}

} 

#endif 
