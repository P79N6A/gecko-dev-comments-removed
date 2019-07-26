





#ifndef jsboolinlines_h
#define jsboolinlines_h

#include "jsbool.h"

#include "mozilla/Assertions.h"
#include "mozilla/Likely.h"

#include "jswrapper.h"

#include "js/RootingAPI.h"
#include "vm/WrapperObject.h"

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
    JSObject *actual = MOZ_LIKELY(!obj->is<WrapperObject>()) ? obj : UncheckedUnwrap(obj);
    bool emulatesUndefined = actual->getClass()->emulatesUndefined();
    MOZ_ASSERT_IF(emulatesUndefined, obj->type()->flags & types::OBJECT_FLAG_EMULATES_UNDEFINED);
    return emulatesUndefined;
}

} 

#endif 
