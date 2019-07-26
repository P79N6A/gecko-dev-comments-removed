






#ifndef StringObject_inl_h___
#define StringObject_inl_h___

#include "StringObject.h"

inline js::StringObject &
JSObject::asString()
{
    JS_ASSERT(isString());
    return *static_cast<js::StringObject *>(this);
}

namespace js {

inline bool
StringObject::init(JSContext *cx, HandleString str)
{
    JS_ASSERT(gc::GetGCKindSlots(getAllocKind()) == 2);

    Rooted<StringObject *> self(cx, this);

    if (nativeEmpty()) {
        if (isDelegate()) {
            if (!assignInitialShape(cx))
                return false;
        } else {
            Shape *shape = assignInitialShape(cx);
            if (!shape)
                return false;
            EmptyShape::insertInitialShape(cx, shape, self->getProto());
        }
    }

    JS_ASSERT(self->nativeLookupNoAllocation(NameToId(cx->names().length))->slot() == LENGTH_SLOT);

    self->setStringThis(str);

    return true;
}

inline StringObject *
StringObject::create(JSContext *cx, HandleString str)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &StringClass);
    if (!obj)
        return NULL;
    Rooted<StringObject*> strobj(cx, &obj->asString());
    if (!strobj->init(cx, str))
        return NULL;
    return strobj;
}

inline StringObject *
StringObject::createWithProto(JSContext *cx, HandleString str, JSObject &proto)
{
    JSObject *obj = NewObjectWithClassProto(cx, &StringClass, &proto, NULL);
    if (!obj)
        return NULL;
    Rooted<StringObject*> strobj(cx, &obj->asString());
    if (!strobj->init(cx, str))
        return NULL;
    return strobj;
}

} 

#endif 
