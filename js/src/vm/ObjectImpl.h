






#ifndef ObjectImpl_h___
#define ObjectImpl_h___

#include "mozilla/Assertions.h"
#include "mozilla/StandardInteger.h"

#include "jsfriendapi.h"
#include "jsinfer.h"
#include "jsval.h"

#include "gc/Barrier.h"

namespace js {

class ObjectImpl;







class ObjectElements
{
    friend struct ::JSObject;
    friend class ObjectImpl;

    
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

    HeapSlot *elements() { return (HeapSlot *)(uintptr_t(this) + sizeof(ObjectElements)); }
    static ObjectElements * fromElements(HeapSlot *elems) {
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


extern HeapSlot *emptyObjectElements;

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

    HeapSlot *slots;     
    HeapSlot *elements;  

#ifdef DEBUG
    void checkShapeConsistency();
#else
    void checkShapeConsistency() { }
#endif

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

    JSObject * asObjectPtr() { return reinterpret_cast<JSObject *>(this); }

    

  public:
    JSObject * getProto() const {
        return type_->proto;
    }

    inline bool isExtensible() const;

    



    inline bool isDenseArray() const;
    inline bool isSlowArray() const;
    inline bool isArray() const;

    inline HeapSlotArray getDenseArrayElements();
    inline const Value & getDenseArrayElement(unsigned idx);
    inline uint32_t getDenseArrayInitializedLength();

  private:
    



    inline void getSlotRangeUnchecked(size_t start, size_t length,
                                      HeapSlot **fixedStart, HeapSlot **fixedEnd,
                                      HeapSlot **slotsStart, HeapSlot **slotsEnd);
    inline void getSlotRange(size_t start, size_t length,
                             HeapSlot **fixedStart, HeapSlot **fixedEnd,
                             HeapSlot **slotsStart, HeapSlot **slotsEnd);

  protected:
    friend struct GCMarker;
    friend struct Shape;
    friend class NewObjectCache;

    inline void invalidateSlotRange(size_t start, size_t count);
    inline void initializeSlotRange(size_t start, size_t count);

    



    void initSlotRange(size_t start, const Value *vector, size_t length);

    



    void copySlotRange(size_t start, const Value *vector, size_t length);

#ifdef DEBUG
    enum SentinelAllowed {
        SENTINEL_NOT_ALLOWED,
        SENTINEL_ALLOWED
    };

    



    bool slotInRange(unsigned slot, SentinelAllowed sentinel = SENTINEL_NOT_ALLOWED) const;
#endif

    
    static const uint32_t SLOT_CAPACITY_MIN = 8;

    HeapSlot *fixedSlots() const {
        return reinterpret_cast<HeapSlot *>(uintptr_t(this) + sizeof(ObjectImpl));
    }

    




  public:
    Shape * lastProperty() const {
        MOZ_ASSERT(shape_);
        return shape_;
    }

    inline bool isNative() const;

    types::TypeObject *type() const {
        MOZ_ASSERT(!hasLazyType());
        return type_;
    }

    size_t numFixedSlots() const {
        return reinterpret_cast<const shadow::Object *>(this)->numFixedSlots();
    }

    



    bool hasSingletonType() const { return !!type_->singleton; }

    



    bool hasLazyType() const { return type_->lazy(); }

    inline uint32_t slotSpan() const;

    
    inline size_t numDynamicSlots() const;

    const Shape * nativeLookup(JSContext *cx, jsid id);

    inline Class *getClass() const;
    inline JSClass *getJSClass() const;
    inline bool hasClass(const Class *c) const;
    inline const ObjectOps *getOps() const;

    








    inline bool isDelegate() const;

    




    inline bool inDictionaryMode() const;

    const Value &getSlot(unsigned slot) const {
        MOZ_ASSERT(slotInRange(slot));
        size_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots()[slot];
        return slots[slot - fixed];
    }

    HeapSlot *getSlotAddressUnchecked(unsigned slot) {
        size_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots() + slot;
        return slots + (slot - fixed);
    }

    HeapSlot *getSlotAddress(unsigned slot) {
        




        MOZ_ASSERT(slotInRange(slot, SENTINEL_ALLOWED));
        return getSlotAddressUnchecked(slot);
    }

    HeapSlot &getSlotRef(unsigned slot) {
        MOZ_ASSERT(slotInRange(slot));
        return *getSlotAddress(slot);
    }

    inline HeapSlot &nativeGetSlotRef(unsigned slot);
    inline const Value &nativeGetSlot(unsigned slot) const;

    inline void setSlot(unsigned slot, const Value &value);
    inline void initSlot(unsigned slot, const Value &value);
    inline void initSlotUnchecked(unsigned slot, const Value &value);

    

    HeapSlot &getFixedSlotRef(unsigned slot) {
        MOZ_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    const Value &getFixedSlot(unsigned slot) const {
        MOZ_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    inline void setFixedSlot(unsigned slot, const Value &value);
    inline void initFixedSlot(unsigned slot, const Value &value);

    





    static inline size_t dynamicSlotsCount(size_t nfixed, size_t span);

    
    inline size_t sizeOfThis() const;

    

    ObjectElements * getElementsHeader() const {
        return ObjectElements::fromElements(elements);
    }

    inline HeapSlot *fixedElements() const {
        MOZ_STATIC_ASSERT(2 * sizeof(Value) == sizeof(ObjectElements),
                          "when elements are stored inline, the first two "
                          "slots will hold the ObjectElements header");
        return &fixedSlots()[2];
    }

    void setFixedElements() { this->elements = fixedElements(); }

    inline bool hasDynamicElements() const {
        






        return elements != emptyObjectElements && elements != fixedElements();
    }

    
    static inline void readBarrier(ObjectImpl *obj);
    static inline void writeBarrierPre(ObjectImpl *obj);
    static inline void writeBarrierPost(ObjectImpl *obj, void *addr);
    inline void privateWriteBarrierPre(void **oldval);
    inline void privateWriteBarrierPost(void **oldval);
    void markChildren(JSTracer *trc);

    
    static size_t offsetOfShape() { return offsetof(ObjectImpl, shape_); }
    HeapPtrShape *addressOfShape() { return &shape_; }

    static size_t offsetOfType() { return offsetof(ObjectImpl, type_); }
    HeapPtrTypeObject *addressOfType() { return &type_; }

    static size_t offsetOfElements() { return offsetof(ObjectImpl, elements); }
    static size_t offsetOfFixedElements() {
        return sizeof(ObjectImpl) + sizeof(ObjectElements);
    }

    static size_t getFixedSlotOffset(size_t slot) {
        return sizeof(ObjectImpl) + slot * sizeof(Value);
    }
    static size_t getPrivateDataOffset(size_t nfixed) { return getFixedSlotOffset(nfixed); }
    static size_t offsetOfSlots() { return offsetof(ObjectImpl, slots); }
};

} 

#endif 
