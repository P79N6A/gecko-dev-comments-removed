





#ifndef vm_ArrayObject_h
#define vm_ArrayObject_h

#include "jsobj.h"

namespace js {

class ArrayObject : public JSObject
{
  public:
    
    
    
    static const uint32_t EagerAllocationMaxLength = 2048 - ObjectElements::VALUES_PER_HEADER;

    static const Class class_;

    bool lengthIsWritable() const {
        return !getElementsHeader()->hasNonwritableArrayLength();
    }

    uint32_t length() const {
        return getElementsHeader()->length;
    }

    inline void setLength(ExclusiveContext *cx, uint32_t length);

    
    void setLengthInt32(uint32_t length) {
        JS_ASSERT(lengthIsWritable());
        JS_ASSERT(length <= INT32_MAX);
        getElementsHeader()->length = length;
    }
};

} 

#endif 

