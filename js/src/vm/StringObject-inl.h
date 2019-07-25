







































#ifndef StringObject_inl_h___
#define StringObject_inl_h___

#include "StringObject.h"

inline js::StringObject *
JSObject::asString()
{
    JS_ASSERT(isString());
    return static_cast<js::StringObject *>(const_cast<JSObject *>(this));
}

namespace js {

inline StringObject *
StringObject::create(JSContext *cx, JSString *str)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &js_StringClass);
    if (!obj)
        return NULL;
    StringObject *strobj = obj->asString();
    if (!strobj->init(cx, str))
        return NULL;
    return strobj;
}

inline StringObject *
StringObject::createWithProto(JSContext *cx, JSString *str, JSObject &proto)
{
    JS_ASSERT(gc::FINALIZE_OBJECT2 == gc::GetGCObjectKind(JSCLASS_RESERVED_SLOTS(&js_StringClass)));
    JSObject *obj = NewObjectWithClassProto(cx, &js_StringClass, &proto, gc::FINALIZE_OBJECT2);
    if (!obj)
        return NULL;
    StringObject *strobj = obj->asString();
    if (!strobj->init(cx, str))
        return NULL;
    return strobj;
}

} 

#endif 
