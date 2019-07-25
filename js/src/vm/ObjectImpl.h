






#ifndef ObjectImpl_h___
#define ObjectImpl_h___

#include "mozilla/Assertions.h"
#include "mozilla/StdInt.h"

#include "jsinfer.h"
#include "jsval.h"

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

struct Class;
struct GCMarker;
struct ObjectOps;
struct Shape;

class NewObjectCache;

















































class ObjectImpl : public gc::Cell
{
  protected:
    



    HeapPtrShape shape_;

    




    HeapPtrTypeObject type_;

    HeapValue *slots;     
    HeapValue *elements;  

  private:
    static void staticAsserts() {
        MOZ_STATIC_ASSERT(sizeof(ObjectImpl) == sizeof(shadow::Object),
                          "shadow interface must match actual implementation");
        MOZ_STATIC_ASSERT(sizeof(ObjectImpl) % sizeof(Value) == 0,
                          "fixed slots after an object must be aligned");

        MOZ_STATIC_ASSERT(offsetof(ObjectImpl, shape_) == offsetof(shadow::Object, shape),
                          "shadow shape must match actual shape");
        MOZ_STATIC_ASSERT(offsetof(ObjectImpl, type_) == offsetof(shadow::Object, type),
                          "shadow type must match actual type");
        MOZ_STATIC_ASSERT(offsetof(ObjectImpl, slots) == offsetof(shadow::Object, slots),
                          "shadow slots must match actual slots");
        MOZ_STATIC_ASSERT(offsetof(ObjectImpl, elements) == offsetof(shadow::Object, _1),
                          "shadow placeholder must match actual elements");
    }

  protected:
    friend struct GCMarker;
    friend struct Shape;
    friend class NewObjectCache;

    




  public:
    Shape * lastProperty() const {
        MOZ_ASSERT(shape_);
        return shape_;
    }

    types::TypeObject *type() const {
        MOZ_ASSERT(!hasLazyType());
        return type_;
    }

    



    bool hasSingletonType() const { return !!type_->singleton; }

    



    bool hasLazyType() const { return type_->lazy(); }

    inline bool isNative() const;

    inline Class *getClass() const;
    inline JSClass *getJSClass() const;
    inline bool hasClass(const Class *c) const;
    inline const ObjectOps *getOps() const;

    








    inline bool isDelegate() const;

    




    inline bool inDictionaryMode() const;

    
    static inline size_t offsetOfShape() { return offsetof(ObjectImpl, shape_); }
    inline HeapPtrShape *addressOfShape() { return &shape_; }
    static inline size_t offsetOfType() { return offsetof(ObjectImpl, type_); }
    HeapPtrTypeObject *addressOfType() { return &type_; }

    

  public:
    JSObject * getProto() const {
        return type_->proto;
    }
};

} 

#endif 
