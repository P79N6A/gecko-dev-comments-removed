





#ifndef BooleanObject_inl_h___
#define BooleanObject_inl_h___

#include "vm/BooleanObject.h"

#include "jsobjinlines.h"

namespace js {

inline BooleanObject *
BooleanObject::create(JSContext *cx, bool b)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &class_);
    if (!obj)
        return NULL;
    BooleanObject &boolobj = obj->as<BooleanObject>();
    boolobj.setPrimitiveValue(b);
    return &boolobj;
}

} 

#endif 
