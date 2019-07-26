





#ifndef NumberObject_inl_h___
#define NumberObject_inl_h___

#include "NumberObject.h"

#include "jsobjinlines.h"

namespace js {

inline NumberObject *
NumberObject::create(JSContext *cx, double d)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &class_);
    if (!obj)
        return NULL;
    NumberObject &numobj = obj->as<NumberObject>();
    numobj.setPrimitiveValue(d);
    return &numobj;
}

} 

#endif 
