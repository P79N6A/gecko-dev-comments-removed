





#ifndef vm_ArrayObject_h
#define vm_ArrayObject_h

#include "jsobj.h"

namespace js {

class ArrayObject : public JSObject
{
  public:
    static Class class_;

    bool lengthIsWritable() const {
        return !getElementsHeader()->hasNonwritableArrayLength();
    }

    uint32_t length() const {
        return getElementsHeader()->length;
    }

    static inline void setLength(ExclusiveContext *cx, Handle<ArrayObject*> arr, uint32_t length);

    
    void setLengthInt32(uint32_t length) {
        JS_ASSERT(lengthIsWritable());
        JS_ASSERT(length <= INT32_MAX);
        getElementsHeader()->length = length;
    }
};

} 

#endif 

