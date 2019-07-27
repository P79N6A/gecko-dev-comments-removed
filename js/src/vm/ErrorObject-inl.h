





#ifndef vm_ErrorObject_inl_h
#define vm_ErrorObject_inl_h

#include "vm/ErrorObject.h"

#include "jscntxt.h"

inline JSString*
js::ErrorObject::fileName(JSContext* cx) const
{
    const HeapSlot& slot = getReservedSlotRef(FILENAME_SLOT);
    return slot.isString() ? slot.toString() : cx->names().empty;
}

inline uint32_t
js::ErrorObject::lineNumber() const
{
    const HeapSlot& slot = getReservedSlotRef(LINENUMBER_SLOT);
    return slot.isInt32() ? slot.toInt32() : 0;
}

inline uint32_t
js::ErrorObject::columnNumber() const
{
    const HeapSlot& slot = getReservedSlotRef(COLUMNNUMBER_SLOT);
    return slot.isInt32() ? slot.toInt32() : 0;
}

inline JSObject*
js::ErrorObject::stack() const
{
    return getReservedSlotRef(STACK_SLOT).toObjectOrNull();
}

#endif 
