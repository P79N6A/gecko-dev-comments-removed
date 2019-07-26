





#ifndef jsboolinlines_h___
#define jsboolinlines_h___

#include "mozilla/Assertions.h"
#include "mozilla/Likely.h"

#include "gc/Root.h"

#include "jsobjinlines.h"

#include "vm/BooleanObject-inl.h"

namespace js {

bool BooleanGetPrimitiveValueSlow(JSContext *, HandleObject, Value *);

inline bool
BooleanGetPrimitiveValue(JSContext *cx, HandleObject obj, Value *vp)
{
    if (obj->isBoolean()) {
        *vp = BooleanValue(obj->asBoolean().unbox());
        return true;
    }

    return BooleanGetPrimitiveValueSlow(cx, obj, vp);
}

inline bool
EmulatesUndefined(RawObject obj)
{
    AutoAssertNoGC nogc;
    RawObject actual = MOZ_LIKELY(!obj->isWrapper()) ? obj : UnwrapObject(obj);
    bool emulatesUndefined = actual->getClass()->emulatesUndefined();
    MOZ_ASSERT_IF(emulatesUndefined, obj->type()->flags & types::OBJECT_FLAG_EMULATES_UNDEFINED);
    return emulatesUndefined;
}

} 

#endif 
