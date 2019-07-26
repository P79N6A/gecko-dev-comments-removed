





#ifndef vm_ArrayObject_inl_h
#define vm_ArrayObject_inl_h

#include "vm/ArrayObject.h"
#include "vm/String.h"

#include "jsinferinlines.h"

namespace js {

 inline void
ArrayObject::setLength(ExclusiveContext *cx, Handle<ArrayObject*> arr, uint32_t length)
{
    JS_ASSERT(arr->lengthIsWritable());

    if (length > INT32_MAX) {
        
        types::MarkTypeObjectFlags(cx, arr, types::OBJECT_FLAG_LENGTH_OVERFLOW);
        jsid lengthId = NameToId(cx->names().length);
        types::AddTypePropertyId(cx, arr, lengthId, types::Type::DoubleType());
    }

    arr->getElementsHeader()->length = length;
}

} 

#endif 

