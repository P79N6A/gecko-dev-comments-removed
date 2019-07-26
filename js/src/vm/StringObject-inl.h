





#ifndef vm_StringObject_inl_h
#define vm_StringObject_inl_h

#include "vm/StringObject.h"

#include "jsobjinlines.h"

namespace js {

inline bool
StringObject::init(JSContext *cx, HandleString str)
{
    JS_ASSERT(numFixedSlots() == 2);

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
StringObject::create(JSContext *cx, HandleString str, NewObjectKind newKind)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &class_, newKind);
    if (!obj)
        return nullptr;
    Rooted<StringObject*> strobj(cx, &obj->as<StringObject>());
    if (!strobj->init(cx, str))
        return nullptr;
    return strobj;
}

} 

#endif 
