






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
    AssertCanGC();
    JS_ASSERT(gc::GetGCKindSlots(getAllocKind()) == 2);

    Rooted<StringObject *> self(cx, this);

    if (nativeEmpty()) {
        if (isDelegate()) {
            if (!assignInitialShape(cx))
                return false;
        } else {
            RootedShape shape(cx, assignInitialShape(cx));
            if (!shape)
                return false;
            RootedObject proto(cx, self->getProto());
            EmptyShape::insertInitialShape(cx, shape, proto);
        }
    }

    JS_ASSERT(self->nativeLookup(cx, NameToId(cx->names().length))->slot() == LENGTH_SLOT);

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
