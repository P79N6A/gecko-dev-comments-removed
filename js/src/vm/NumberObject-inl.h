





#ifndef vm_NumberObject_inl_h
#define vm_NumberObject_inl_h

#include "vm/NumberObject.h"

#include "jsobjinlines.h"

namespace js {

inline NumberObject *
NumberObject::create(JSContext *cx, double d)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &class_);
    if (!obj)
        return nullptr;
    NumberObject &numobj = obj->as<NumberObject>();
    numobj.setPrimitiveValue(d);
    return &numobj;
}

} 

#endif 
