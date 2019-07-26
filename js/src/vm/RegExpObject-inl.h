





#ifndef vm_RegExpObject_inl_h
#define vm_RegExpObject_inl_h

#include "vm/RegExpObject.h"

namespace js {

inline void
RegExpObject::setShared(ExclusiveContext *cx, RegExpShared &shared)
{
    shared.prepareForUse(cx);
    JSObject::setPrivate(&shared);
}

} 

#endif 
