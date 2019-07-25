







































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

inline bool
StringObject::init(JSContext *cx, JSString *str)
{
    JS_ASSERT(nativeEmpty());
    JS_ASSERT(gc::GetGCKindSlots(getAllocKind()) == 2);

    if (isDelegate()) {
        if (!assignInitialShape(cx))
            return false;
    } else {
        Shape *shape = assignInitialShape(cx);
        if (!shape)
            return false;
        EmptyShape::insertInitialShape(cx, shape, getProto());
    }
    JS_ASSERT(!nativeEmpty());
    JS_ASSERT(nativeLookup(cx, ATOM_TO_JSID(cx->runtime->atomState.lengthAtom))->slot() == LENGTH_SLOT);

    setStringThis(str);
    return true;
}

inline StringObject *
StringObject::create(JSContext *cx, JSString *str)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &StringClass);
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
    JS_ASSERT(gc::FINALIZE_OBJECT2 == gc::GetGCObjectKind(JSCLASS_RESERVED_SLOTS(&StringClass)));
    JSObject *obj = NewObjectWithClassProto(cx, &StringClass, &proto, NULL, gc::FINALIZE_OBJECT2);
    if (!obj)
        return NULL;
    StringObject *strobj = obj->asString();
    if (!strobj->init(cx, str))
        return NULL;
    return strobj;
}

} 

#endif 
