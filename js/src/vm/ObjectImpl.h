






#ifndef ObjectImpl_h___
#define ObjectImpl_h___

#include "mozilla/Assertions.h"
#include "mozilla/StdInt.h"

#include "gc/Barrier.h"

namespace js {







class ObjectElements
{
    friend struct ::JSObject;

    
    uint32_t capacity;

    





    uint32_t initializedLength;

    
    uint32_t length;

    
    uint32_t unused;

    void staticAsserts() {
        MOZ_STATIC_ASSERT(sizeof(ObjectElements) == VALUES_PER_HEADER * sizeof(Value),
                          "Elements size and values-per-Elements mismatch");
    }

  public:

    ObjectElements(uint32_t capacity, uint32_t length)
      : capacity(capacity), initializedLength(0), length(length)
    {}

    HeapValue * elements() { return (HeapValue *)(uintptr_t(this) + sizeof(ObjectElements)); }
    static ObjectElements * fromElements(HeapValue *elems) {
        return (ObjectElements *)(uintptr_t(elems) - sizeof(ObjectElements));
    }

    static int offsetOfCapacity() {
        return (int)offsetof(ObjectElements, capacity) - (int)sizeof(ObjectElements);
    }
    static int offsetOfInitializedLength() {
        return (int)offsetof(ObjectElements, initializedLength) - (int)sizeof(ObjectElements);
    }
    static int offsetOfLength() {
        return (int)offsetof(ObjectElements, length) - (int)sizeof(ObjectElements);
    }

    static const size_t VALUES_PER_HEADER = 2;
};


extern HeapValue *emptyObjectElements;

struct Shape;
struct GCMarker;
class NewObjectCache;

















































class ObjectImpl : public gc::Cell
{
  protected:
    



    HeapPtrShape shape_;

    




    HeapPtrTypeObject type_;

    HeapValue *slots;     
    HeapValue *elements;  

  protected:
    friend struct Shape;
    friend struct GCMarker;
    friend class  NewObjectCache;

};

} 

#endif 
