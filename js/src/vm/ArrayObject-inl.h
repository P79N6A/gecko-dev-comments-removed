





#ifndef vm_ArrayObject_inl_h
#define vm_ArrayObject_inl_h

#include "vm/ArrayObject.h"

#include "vm/String.h"

#include "jsinferinlines.h"

namespace js {

inline void
ArrayObject::setLength(ExclusiveContext *cx, uint32_t length)
{
    JS_ASSERT(lengthIsWritable());

    if (length > INT32_MAX) {
        
        types::MarkTypeObjectFlags(cx, this, types::OBJECT_FLAG_LENGTH_OVERFLOW);
    }

    getElementsHeader()->length = length;
}

} 

#endif 

