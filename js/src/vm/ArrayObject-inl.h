





#ifndef vm_ArrayObject_inl_h
#define vm_ArrayObject_inl_h

#include "vm/ArrayObject.h"
#include "vm/String.h"

#include "jsinferinlines.h"

namespace js {

 inline void
ArrayObject::setLength(JSContext *cx, Handle<ArrayObject*> arr, uint32_t length)
{
    JS_ASSERT(arr->lengthIsWritable());

    if (length > INT32_MAX) {
        
        js::types::MarkTypeObjectFlags(cx, arr, js::types::OBJECT_FLAG_LENGTH_OVERFLOW);
        jsid lengthId = js::NameToId(cx->names().length);
        js::types::AddTypePropertyId(cx, arr, lengthId, js::types::Type::DoubleType());
    }

    arr->getElementsHeader()->length = length;
}

} 

#endif 

