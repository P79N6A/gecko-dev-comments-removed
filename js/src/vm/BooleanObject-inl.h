






#ifndef BooleanObject_inl_h___
#define BooleanObject_inl_h___

#include "jsobjinlines.h"

#include "vm/BooleanObject.h"

inline js::BooleanObject &
JSObject::asBoolean()
{
    JS_ASSERT(isBoolean());
    return *static_cast<js::BooleanObject *>(this);
}

namespace js {

inline BooleanObject *
BooleanObject::create(JSContext *cx, bool b)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &BooleanClass);
    if (!obj)
        return NULL;
    BooleanObject &boolobj = obj->asBoolean();
    boolobj.setPrimitiveValue(b);
    return &boolobj;
}

} 

#endif 
