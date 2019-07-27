





#ifndef vm_ArrayObject_h
#define vm_ArrayObject_h

#include "vm/NativeObject.h"

namespace js {

class ArrayObject : public NativeObject
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

    inline void setLength(ExclusiveContext* cx, uint32_t length);

    
    void setLengthInt32(uint32_t length) {
        MOZ_ASSERT(lengthIsWritable());
        MOZ_ASSERT(length <= INT32_MAX);
        getElementsHeader()->length = length;
    }

    
    static inline ArrayObject*
    createArray(ExclusiveContext* cx,
                gc::AllocKind kind,
                gc::InitialHeap heap,
                HandleShape shape,
                HandleObjectGroup group,
                uint32_t length);

    
    
    static inline ArrayObject*
    createCopyOnWriteArray(ExclusiveContext* cx,
                           gc::InitialHeap heap,
                           HandleArrayObject sharedElementsOwner);

  private:
    
    static inline ArrayObject*
    createArrayInternal(ExclusiveContext* cx,
                        gc::AllocKind kind,
                        gc::InitialHeap heap,
                        HandleShape shape,
                        HandleObjectGroup group);

    static inline ArrayObject*
    finishCreateArray(ArrayObject* obj, HandleShape shape);
};

} 

#endif 

