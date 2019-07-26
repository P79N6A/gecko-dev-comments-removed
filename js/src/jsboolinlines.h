





#ifndef jsboolinlines_h___
#define jsboolinlines_h___

#include "mozilla/Assertions.h"
#include "mozilla/Likely.h"

#include "js/RootingAPI.h"

#include "vm/BooleanObject-inl.h"

namespace js {

bool
BooleanGetPrimitiveValueSlow(HandleObject, JSContext *);

inline bool
BooleanGetPrimitiveValue(HandleObject obj, JSContext *cx)
{
    if (obj->is<BooleanObject>())
        return obj->as<BooleanObject>().unbox();

    return BooleanGetPrimitiveValueSlow(obj, cx);
}

inline bool
EmulatesUndefined(JSObject *obj)
{
    JSObject *actual = MOZ_LIKELY(!obj->isWrapper()) ? obj : UncheckedUnwrap(obj);
    bool emulatesUndefined = actual->getClass()->emulatesUndefined();
    MOZ_ASSERT_IF(emulatesUndefined, obj->type()->flags & types::OBJECT_FLAG_EMULATES_UNDEFINED);
    return emulatesUndefined;
}

} 

#endif 
