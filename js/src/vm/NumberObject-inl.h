






#ifndef NumberObject_inl_h___
#define NumberObject_inl_h___

#include "NumberObject.h"

namespace js {

inline NumberObject *
NumberObject::create(JSContext *cx, double d)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &NumberClass);
    if (!obj)
        return NULL;
    NumberObject &numobj = obj->asNumber();
    numobj.setPrimitiveValue(d);
    return &numobj;
}

} 

#endif 
