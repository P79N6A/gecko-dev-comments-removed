





#ifndef vm_BooleanObject_inl_h
#define vm_BooleanObject_inl_h

#include "vm/BooleanObject.h"

#include "jsobjinlines.h"

namespace js {

inline BooleanObject *
BooleanObject::create(JSContext *cx, bool b)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &class_);
    if (!obj)
        return nullptr;
    BooleanObject &boolobj = obj->as<BooleanObject>();
    boolobj.setPrimitiveValue(b);
    return &boolobj;
}

} 

#endif 
