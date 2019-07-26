





#ifndef vm_StringObject_inl_h
#define vm_StringObject_inl_h

#include "vm/StringObject.h"

#include "jsobjinlines.h"

#include "vm/Shape-inl.h"

namespace js {

inline bool
StringObject::init(JSContext *cx, HandleString str)
{
    JS_ASSERT(numFixedSlots() == 2);

    Rooted<StringObject *> self(cx, this);

    if (!EmptyShape::ensureInitialCustomShape<StringObject>(cx, self))
        return false;

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
