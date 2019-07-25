







































#ifndef NumberObject_inl_h___
#define NumberObject_inl_h___

#include "NumberObject.h"

inline js::NumberObject *
JSObject::asNumber()
{
    JS_ASSERT(isNumber());
    return static_cast<js::NumberObject *>(const_cast<JSObject *>(this));
}

namespace js {

inline NumberObject *
NumberObject::create(JSContext *cx, jsdouble d)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &NumberClass);
    if (!obj)
        return NULL;
    NumberObject *numobj = obj->asNumber();
    numobj->setPrimitiveValue(d);
    return numobj;
}

inline NumberObject *
NumberObject::createWithProto(JSContext *cx, jsdouble d, JSObject &proto)
{
    JSObject *obj = NewObjectWithClassProto(cx, &NumberClass, &proto, NULL,
                                            gc::GetGCObjectKind(RESERVED_SLOTS));
    if (!obj)
        return NULL;
    NumberObject *numobj = obj->asNumber();
    numobj->setPrimitiveValue(d);
    return numobj;
}

} 

#endif 
