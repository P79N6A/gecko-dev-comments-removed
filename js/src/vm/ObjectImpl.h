





#ifndef vm_ObjectImpl_h
#define vm_ObjectImpl_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

#include <stdint.h>

#include "jsfriendapi.h"
#include "jsinfer.h"
#include "NamespaceImports.h"

#include "gc/Barrier.h"
#include "gc/Heap.h"
#include "gc/Marking.h"
#include "js/Value.h"
#include "vm/NumericConversions.h"
#include "vm/Shape.h"
#include "vm/String.h"

namespace js {

class ObjectImpl;
class Nursery;
class Shape;

namespace gc {
class ForkJoinNursery;
}







static MOZ_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(Value *beg, Value *end)
{
#ifdef DEBUG
    for (Value *v = beg; v != end; ++v)
        v->setObject(*reinterpret_cast<JSObject *>(0x42));
#endif
}

static MOZ_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(Value *vec, size_t len)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch(vec, vec + len);
#endif
}

static MOZ_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(HeapValue *vec, size_t len)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch((Value *) vec, len);
#endif
}

static MOZ_ALWAYS_INLINE void
Debug_SetSlotRangeToCrashOnTouch(HeapSlot *vec, uint32_t len)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch((Value *) vec, len);
#endif
}

static MOZ_ALWAYS_INLINE void
Debug_SetSlotRangeToCrashOnTouch(HeapSlot *begin, HeapSlot *end)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch((Value *) begin, end - begin);
#endif
}

class ArrayObject;









template <ExecutionMode mode>
extern bool
ArraySetLength(typename ExecutionModeTraits<mode>::ContextType cx,
               Handle<ArrayObject*> obj, HandleId id,
               unsigned attrs, HandleValue value, bool setterIsStrict);





































































class ObjectElements
{
  public:
    enum Flags {
        
        CONVERT_DOUBLE_ELEMENTS     = 0x1,

        
        
        NONWRITABLE_ARRAY_LENGTH    = 0x2,

        
        
        
        
        
        
        
        COPY_ON_WRITE               = 0x4
    };

  private:
    friend class ::JSObject;
    friend class ObjectImpl;
    friend class ArrayObject;
    friend class Nursery;
    friend class gc::ForkJoinNursery;

    template <ExecutionMode mode>
    friend bool
    ArraySetLength(typename ExecutionModeTraits<mode>::ContextType cx,
                   Handle<ArrayObject*> obj, HandleId id,
                   unsigned attrs, HandleValue value, bool setterIsStrict);

    
    uint32_t flags;

    





    uint32_t initializedLength;

    
    uint32_t capacity;

    
    uint32_t length;

    bool shouldConvertDoubleElements() const {
        return flags & CONVERT_DOUBLE_ELEMENTS;
    }
    void setShouldConvertDoubleElements() {
        
        flags |= CONVERT_DOUBLE_ELEMENTS;
    }
    void clearShouldConvertDoubleElements() {
        JS_ASSERT(!isCopyOnWrite());
        flags &= ~CONVERT_DOUBLE_ELEMENTS;
    }
    bool hasNonwritableArrayLength() const {
        return flags & NONWRITABLE_ARRAY_LENGTH;
    }
    void setNonwritableArrayLength() {
        JS_ASSERT(!isCopyOnWrite());
        flags |= NONWRITABLE_ARRAY_LENGTH;
    }
    bool isCopyOnWrite() const {
        return flags & COPY_ON_WRITE;
    }
    void clearCopyOnWrite() {
        JS_ASSERT(isCopyOnWrite());
        flags &= ~COPY_ON_WRITE;
    }

  public:
    MOZ_CONSTEXPR ObjectElements(uint32_t capacity, uint32_t length)
      : flags(0), initializedLength(0), capacity(capacity), length(length)
    {}

    HeapSlot *elements() {
        return reinterpret_cast<HeapSlot*>(uintptr_t(this) + sizeof(ObjectElements));
    }
    const HeapSlot *elements() const {
        return reinterpret_cast<const HeapSlot*>(uintptr_t(this) + sizeof(ObjectElements));
    }
    static ObjectElements * fromElements(HeapSlot *elems) {
        return reinterpret_cast<ObjectElements*>(uintptr_t(elems) - sizeof(ObjectElements));
    }

    HeapPtrObject &ownerObject() const {
        JS_ASSERT(isCopyOnWrite());
        return *(HeapPtrObject *)(&elements()[initializedLength]);
    }

    static int offsetOfFlags() {
        return int(offsetof(ObjectElements, flags)) - int(sizeof(ObjectElements));
    }
    static int offsetOfInitializedLength() {
        return int(offsetof(ObjectElements, initializedLength)) - int(sizeof(ObjectElements));
    }
    static int offsetOfCapacity() {
        return int(offsetof(ObjectElements, capacity)) - int(sizeof(ObjectElements));
    }
    static int offsetOfLength() {
        return int(offsetof(ObjectElements, length)) - int(sizeof(ObjectElements));
    }

    static bool ConvertElementsToDoubles(JSContext *cx, uintptr_t elements);
    static bool MakeElementsCopyOnWrite(ExclusiveContext *cx, JSObject *obj);

    
    
    static const size_t VALUES_PER_HEADER = 2;
};

static_assert(ObjectElements::VALUES_PER_HEADER * sizeof(HeapSlot) == sizeof(ObjectElements),
              "ObjectElements doesn't fit in the given number of slots");


extern HeapSlot *const emptyObjectElements;

struct Class;
class GCMarker;
struct ObjectOps;
class Shape;

class NewObjectCache;
class TaggedProto;

inline Value
ObjectValue(ObjectImpl &obj);

#ifdef DEBUG
static inline bool
IsObjectValueInCompartment(js::Value v, JSCompartment *comp);
#endif













































class ObjectImpl : public gc::Cell
{
  protected:
    



    HeapPtrShape shape_;

    




    HeapPtrTypeObject type_;

    HeapSlot *slots;     
    HeapSlot *elements;  

    friend bool
    ArraySetLength(JSContext *cx, Handle<ArrayObject*> obj, HandleId id, unsigned attrs,
                   HandleValue value, bool setterIsStrict);

  private:
    static void staticAsserts() {
        static_assert(sizeof(ObjectImpl) == sizeof(shadow::Object),
                      "shadow interface must match actual implementation");
        static_assert(sizeof(ObjectImpl) % sizeof(Value) == 0,
                      "fixed slots after an object must be aligned");

        static_assert(offsetof(ObjectImpl, shape_) == offsetof(shadow::Object, shape),
                      "shadow shape must match actual shape");
        static_assert(offsetof(ObjectImpl, type_) == offsetof(shadow::Object, type),
                      "shadow type must match actual type");
        static_assert(offsetof(ObjectImpl, slots) == offsetof(shadow::Object, slots),
                      "shadow slots must match actual slots");
        static_assert(offsetof(ObjectImpl, elements) == offsetof(shadow::Object, _1),
                      "shadow placeholder must match actual elements");
    }

    JSObject * asObjectPtr() { return reinterpret_cast<JSObject *>(this); }
    const JSObject * asObjectPtr() const { return reinterpret_cast<const JSObject *>(this); }

    friend inline Value ObjectValue(ObjectImpl &obj);

    

  public:
    TaggedProto getTaggedProto() const {
        return type_->proto();
    }

    bool hasTenuredProto() const;

    const Class *getClass() const {
        return type_->clasp();
    }

    static inline bool
    isExtensible(ExclusiveContext *cx, Handle<ObjectImpl*> obj, bool *extensible);

    
    
    
    
    bool nonProxyIsExtensible() const {
        MOZ_ASSERT(!isProxy());

        
        return !lastProperty()->hasObjectFlag(BaseShape::NOT_EXTENSIBLE);
    }

#ifdef DEBUG
    bool isProxy() const;
#endif

    
    
    static bool
    preventExtensions(JSContext *cx, Handle<ObjectImpl*> obj);

    HeapSlotArray getDenseElements() {
        JS_ASSERT(isNative());
        return HeapSlotArray(elements, !getElementsHeader()->isCopyOnWrite());
    }
    HeapSlotArray getDenseElementsAllowCopyOnWrite() {
        
        JS_ASSERT(isNative());
        return HeapSlotArray(elements, true);
    }
    const Value &getDenseElement(uint32_t idx) {
        JS_ASSERT(isNative());
        MOZ_ASSERT(idx < getDenseInitializedLength());
        return elements[idx];
    }
    bool containsDenseElement(uint32_t idx) {
        JS_ASSERT(isNative());
        return idx < getDenseInitializedLength() && !elements[idx].isMagic(JS_ELEMENTS_HOLE);
    }
    uint32_t getDenseInitializedLength() {
        JS_ASSERT(getClass()->isNative());
        return getElementsHeader()->initializedLength;
    }
    uint32_t getDenseCapacity() {
        JS_ASSERT(getClass()->isNative());
        return getElementsHeader()->capacity;
    }

  protected:
#ifdef DEBUG
    void checkShapeConsistency();
#else
    void checkShapeConsistency() { }
#endif

    Shape *
    replaceWithNewEquivalentShape(ThreadSafeContext *cx,
                                  Shape *existingShape, Shape *newShape = nullptr);

    enum GenerateShape {
        GENERATE_NONE,
        GENERATE_SHAPE
    };

    bool setFlag(ExclusiveContext *cx,  uint32_t flag,
                 GenerateShape generateShape = GENERATE_NONE);
    bool clearFlag(ExclusiveContext *cx,  uint32_t flag);

    bool toDictionaryMode(ThreadSafeContext *cx);

  private:
    friend class Nursery;
    friend class gc::ForkJoinNursery;

    



    void getSlotRangeUnchecked(uint32_t start, uint32_t length,
                               HeapSlot **fixedStart, HeapSlot **fixedEnd,
                               HeapSlot **slotsStart, HeapSlot **slotsEnd)
    {
        MOZ_ASSERT(start + length >= start);

        uint32_t fixed = numFixedSlots();
        if (start < fixed) {
            if (start + length < fixed) {
                *fixedStart = &fixedSlots()[start];
                *fixedEnd = &fixedSlots()[start + length];
                *slotsStart = *slotsEnd = nullptr;
            } else {
                uint32_t localCopy = fixed - start;
                *fixedStart = &fixedSlots()[start];
                *fixedEnd = &fixedSlots()[start + localCopy];
                *slotsStart = &slots[0];
                *slotsEnd = &slots[length - localCopy];
            }
        } else {
            *fixedStart = *fixedEnd = nullptr;
            *slotsStart = &slots[start - fixed];
            *slotsEnd = &slots[start - fixed + length];
        }
    }

    void getSlotRange(uint32_t start, uint32_t length,
                      HeapSlot **fixedStart, HeapSlot **fixedEnd,
                      HeapSlot **slotsStart, HeapSlot **slotsEnd)
    {
        MOZ_ASSERT(slotInRange(start + length, SENTINEL_ALLOWED));
        getSlotRangeUnchecked(start, length, fixedStart, fixedEnd, slotsStart, slotsEnd);
    }

  protected:
    friend class GCMarker;
    friend class Shape;
    friend class NewObjectCache;

    void invalidateSlotRange(uint32_t start, uint32_t length) {
#ifdef DEBUG
        HeapSlot *fixedStart, *fixedEnd, *slotsStart, *slotsEnd;
        getSlotRange(start, length, &fixedStart, &fixedEnd, &slotsStart, &slotsEnd);
        Debug_SetSlotRangeToCrashOnTouch(fixedStart, fixedEnd);
        Debug_SetSlotRangeToCrashOnTouch(slotsStart, slotsEnd);
#endif 
    }

    void initializeSlotRange(uint32_t start, uint32_t count);

    



    void initSlotRange(uint32_t start, const Value *vector, uint32_t length);

    



    void copySlotRange(uint32_t start, const Value *vector, uint32_t length);

#ifdef DEBUG
    enum SentinelAllowed {
        SENTINEL_NOT_ALLOWED,
        SENTINEL_ALLOWED
    };

    



    bool slotInRange(uint32_t slot, SentinelAllowed sentinel = SENTINEL_NOT_ALLOWED) const;
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

    bool generateOwnShape(ThreadSafeContext *cx, js::Shape *newShape = nullptr) {
        return replaceWithNewEquivalentShape(cx, lastProperty(), newShape);
    }

    JSCompartment *compartment() const {
        return lastProperty()->base()->compartment();
    }

    bool isNative() const {
        return lastProperty()->isNative();
    }

    types::TypeObject *type() const {
        MOZ_ASSERT(!hasLazyType());
        return typeRaw();
    }

    types::TypeObject *typeRaw() const {
        return type_;
    }

    uint32_t numFixedSlots() const {
        return reinterpret_cast<const shadow::Object *>(this)->numFixedSlots();
    }
    uint32_t numUsedFixedSlots() const {
        uint32_t nslots = lastProperty()->slotSpan(getClass());
        return Min(nslots, numFixedSlots());
    }

    



    bool hasSingletonType() const {
        return !!type_->singleton();
    }

    



    bool hasLazyType() const {
        return type_->lazy();
    }

    uint32_t slotSpan() const {
        if (inDictionaryMode())
            return lastProperty()->base()->slotSpan();
        return lastProperty()->slotSpan();
    }

    
    uint32_t numDynamicSlots() const {
        return dynamicSlotsCount(numFixedSlots(), slotSpan(), getClass());
    }


    Shape *nativeLookup(ExclusiveContext *cx, jsid id);
    Shape *nativeLookup(ExclusiveContext *cx, PropertyName *name) {
        return nativeLookup(cx, NameToId(name));
    }

    bool nativeContains(ExclusiveContext *cx, jsid id) {
        return nativeLookup(cx, id) != nullptr;
    }
    bool nativeContains(ExclusiveContext *cx, PropertyName* name) {
        return nativeLookup(cx, name) != nullptr;
    }
    bool nativeContains(ExclusiveContext *cx, Shape* shape) {
        return nativeLookup(cx, shape->propid()) == shape;
    }

    
    Shape *nativeLookupPure(jsid id);
    Shape *nativeLookupPure(PropertyName *name) {
        return nativeLookupPure(NameToId(name));
    }

    bool nativeContainsPure(jsid id) {
        return nativeLookupPure(id) != nullptr;
    }
    bool nativeContainsPure(PropertyName* name) {
        return nativeContainsPure(NameToId(name));
    }
    bool nativeContainsPure(Shape* shape) {
        return nativeLookupPure(shape->propid()) == shape;
    }

    const JSClass *getJSClass() const {
        return Jsvalify(getClass());
    }
    bool hasClass(const Class *c) const {
        return getClass() == c;
    }
    const ObjectOps *getOps() const {
        return &getClass()->ops;
    }

    








    bool isDelegate() const {
        return lastProperty()->hasObjectFlag(BaseShape::DELEGATE);
    }

    




    bool inDictionaryMode() const {
        return lastProperty()->inDictionary();
    }

    const Value &getSlot(uint32_t slot) const {
        MOZ_ASSERT(slotInRange(slot));
        uint32_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots()[slot];
        return slots[slot - fixed];
    }

    const HeapSlot *getSlotAddressUnchecked(uint32_t slot) const {
        uint32_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots() + slot;
        return slots + (slot - fixed);
    }

    HeapSlot *getSlotAddressUnchecked(uint32_t slot) {
        const ObjectImpl *obj = static_cast<const ObjectImpl*>(this);
        return const_cast<HeapSlot*>(obj->getSlotAddressUnchecked(slot));
    }

    HeapSlot *getSlotAddress(uint32_t slot) {
        




        MOZ_ASSERT(slotInRange(slot, SENTINEL_ALLOWED));
        return getSlotAddressUnchecked(slot);
    }

    const HeapSlot *getSlotAddress(uint32_t slot) const {
        




        MOZ_ASSERT(slotInRange(slot, SENTINEL_ALLOWED));
        return getSlotAddressUnchecked(slot);
    }

    HeapSlot &getSlotRef(uint32_t slot) {
        MOZ_ASSERT(slotInRange(slot));
        return *getSlotAddress(slot);
    }

    const HeapSlot &getSlotRef(uint32_t slot) const {
        MOZ_ASSERT(slotInRange(slot));
        return *getSlotAddress(slot);
    }

    HeapSlot &nativeGetSlotRef(uint32_t slot) {
        JS_ASSERT(isNative() && slot < slotSpan());
        return getSlotRef(slot);
    }
    const Value &nativeGetSlot(uint32_t slot) const {
        JS_ASSERT(isNative() && slot < slotSpan());
        return getSlot(slot);
    }

    void setSlot(uint32_t slot, const Value &value) {
        MOZ_ASSERT(slotInRange(slot));
        MOZ_ASSERT(IsObjectValueInCompartment(value, compartment()));
        getSlotRef(slot).set(this->asObjectPtr(), HeapSlot::Slot, slot, value);
    }

    MOZ_ALWAYS_INLINE void setCrossCompartmentSlot(uint32_t slot, const Value &value) {
        MOZ_ASSERT(slotInRange(slot));
        getSlotRef(slot).set(this->asObjectPtr(), HeapSlot::Slot, slot, value);
    }

    void initSlot(uint32_t slot, const Value &value) {
        MOZ_ASSERT(getSlot(slot).isUndefined());
        MOZ_ASSERT(slotInRange(slot));
        MOZ_ASSERT(IsObjectValueInCompartment(value, compartment()));
        initSlotUnchecked(slot, value);
    }

    void initCrossCompartmentSlot(uint32_t slot, const Value &value) {
        MOZ_ASSERT(getSlot(slot).isUndefined());
        MOZ_ASSERT(slotInRange(slot));
        initSlotUnchecked(slot, value);
    }

    void initSlotUnchecked(uint32_t slot, const Value &value) {
        getSlotAddressUnchecked(slot)->init(this->asObjectPtr(), HeapSlot::Slot, slot, value);
    }

    

    HeapSlot &getFixedSlotRef(uint32_t slot) {
        MOZ_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    const Value &getFixedSlot(uint32_t slot) const {
        MOZ_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    void setFixedSlot(uint32_t slot, const Value &value) {
        MOZ_ASSERT(slot < numFixedSlots());
        fixedSlots()[slot].set(this->asObjectPtr(), HeapSlot::Slot, slot, value);
    }

    void initFixedSlot(uint32_t slot, const Value &value) {
        MOZ_ASSERT(slot < numFixedSlots());
        fixedSlots()[slot].init(this->asObjectPtr(), HeapSlot::Slot, slot, value);
    }

    





    static uint32_t dynamicSlotsCount(uint32_t nfixed, uint32_t span, const Class *clasp);

    
    size_t tenuredSizeOfThis() const {
        MOZ_ASSERT(isTenured());
        return js::gc::Arena::thingSize(asTenured()->getAllocKind());
    }

    

    ObjectElements * getElementsHeader() const {
        return ObjectElements::fromElements(elements);
    }

    inline HeapSlot *fixedElements() const {
        static_assert(2 * sizeof(Value) == sizeof(ObjectElements),
                      "when elements are stored inline, the first two "
                      "slots will hold the ObjectElements header");
        return &fixedSlots()[2];
    }

#ifdef DEBUG
    bool canHaveNonEmptyElements();
#endif

    void setFixedElements() {
        JS_ASSERT(canHaveNonEmptyElements());
        this->elements = fixedElements();
    }

    inline bool hasDynamicElements() const {
        






        return !hasEmptyElements() && elements != fixedElements();
    }

    inline bool hasFixedElements() const {
        return elements == fixedElements();
    }

    inline bool hasEmptyElements() const {
        return elements == emptyObjectElements;
    }

    




    inline void *fixedData(size_t nslots) const;

    
    static ThingRootKind rootKind() { return THING_ROOT_OBJECT; }

    inline void privateWriteBarrierPre(void **oldval);

    void privateWriteBarrierPost(void **pprivate) {
#ifdef JSGC_GENERATIONAL
        js::gc::Cell **cellp = reinterpret_cast<js::gc::Cell **>(pprivate);
        JS_ASSERT(cellp);
        JS_ASSERT(*cellp);
        js::gc::StoreBuffer *storeBuffer = (*cellp)->storeBuffer();
        if (storeBuffer)
            storeBuffer->putCellFromAnyThread(cellp);
#endif
    }

    void markChildren(JSTracer *trc);

    

    inline void *&privateRef(uint32_t nfixed) const { 
        




        MOZ_ASSERT(nfixed == numFixedSlots());
        MOZ_ASSERT(hasPrivate());
        HeapSlot *end = &fixedSlots()[nfixed];
        return *reinterpret_cast<void**>(end);
    }

    bool hasPrivate() const {
        return getClass()->hasPrivate();
    }
    void *getPrivate() const {
        return privateRef(numFixedSlots());
    }
    void setPrivate(void *data) {
        void **pprivate = &privateRef(numFixedSlots());
        privateWriteBarrierPre(pprivate);
        *pprivate = data;
    }

    void setPrivateGCThing(gc::Cell *cell) {
        void **pprivate = &privateRef(numFixedSlots());
        privateWriteBarrierPre(pprivate);
        *pprivate = reinterpret_cast<void *>(cell);
        privateWriteBarrierPost(pprivate);
    }

    void setPrivateUnbarriered(void *data) {
        void **pprivate = &privateRef(numFixedSlots());
        *pprivate = data;
    }
    void initPrivate(void *data) {
        privateRef(numFixedSlots()) = data;
    }

    
    inline void *getPrivate(uint32_t nfixed) const {
        return privateRef(nfixed);
    }

    
    static const size_t MaxTagBits = 3;
    void setInitialSlots(HeapSlot *newSlots) { slots = newSlots; }
    static bool isNullLike(const ObjectImpl *obj) { return uintptr_t(obj) < (1 << MaxTagBits); }
    MOZ_ALWAYS_INLINE JS::Zone *zone() const {
        return shape_->zone();
    }
    MOZ_ALWAYS_INLINE JS::shadow::Zone *shadowZone() const {
        return JS::shadow::Zone::asShadowZone(zone());
    }
    MOZ_ALWAYS_INLINE JS::Zone *zoneFromAnyThread() const {
        return shape_->zoneFromAnyThread();
    }
    MOZ_ALWAYS_INLINE JS::shadow::Zone *shadowZoneFromAnyThread() const {
        return JS::shadow::Zone::asShadowZone(zoneFromAnyThread());
    }
    static MOZ_ALWAYS_INLINE void readBarrier(ObjectImpl *obj);
    static MOZ_ALWAYS_INLINE void writeBarrierPre(ObjectImpl *obj);
    static MOZ_ALWAYS_INLINE void writeBarrierPost(ObjectImpl *obj, void *cellp);
    static MOZ_ALWAYS_INLINE void writeBarrierPostRelocate(ObjectImpl *obj, void *cellp);
    static MOZ_ALWAYS_INLINE void writeBarrierPostRemove(ObjectImpl *obj, void *cellp);

    
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

 MOZ_ALWAYS_INLINE void
ObjectImpl::readBarrier(ObjectImpl *obj)
{
    if (!isNullLike(obj) && obj->isTenured())
        obj->asTenured()->readBarrier(obj->asTenured());
}

 MOZ_ALWAYS_INLINE void
ObjectImpl::writeBarrierPre(ObjectImpl *obj)
{
    if (!isNullLike(obj) && obj->isTenured())
        obj->asTenured()->writeBarrierPre(obj->asTenured());
}

 MOZ_ALWAYS_INLINE void
ObjectImpl::writeBarrierPost(ObjectImpl *obj, void *cellp)
{
    JS_ASSERT(cellp);
#ifdef JSGC_GENERATIONAL
    if (IsNullTaggedPointer(obj))
        return;
    JS_ASSERT(obj == *static_cast<ObjectImpl **>(cellp));
    gc::StoreBuffer *storeBuffer = obj->storeBuffer();
    if (storeBuffer)
        storeBuffer->putCellFromAnyThread(static_cast<gc::Cell **>(cellp));
#endif
}

 MOZ_ALWAYS_INLINE void
ObjectImpl::writeBarrierPostRelocate(ObjectImpl *obj, void *cellp)
{
    JS_ASSERT(cellp);
    JS_ASSERT(obj);
    JS_ASSERT(obj == *static_cast<ObjectImpl **>(cellp));
#ifdef JSGC_GENERATIONAL
    gc::StoreBuffer *storeBuffer = obj->storeBuffer();
    if (storeBuffer)
        storeBuffer->putRelocatableCellFromAnyThread(static_cast<gc::Cell **>(cellp));
#endif
}

 MOZ_ALWAYS_INLINE void
ObjectImpl::writeBarrierPostRemove(ObjectImpl *obj, void *cellp)
{
    JS_ASSERT(cellp);
    JS_ASSERT(obj);
    JS_ASSERT(obj == *static_cast<ObjectImpl **>(cellp));
#ifdef JSGC_GENERATIONAL
    obj->shadowRuntimeFromAnyThread()->gcStoreBufferPtr()->removeRelocatableCellFromAnyThread(
        static_cast<gc::Cell **>(cellp));
#endif
}

inline void
ObjectImpl::privateWriteBarrierPre(void **oldval)
{
#ifdef JSGC_INCREMENTAL
    JS::shadow::Zone *shadowZone = this->shadowZoneFromAnyThread();
    if (shadowZone->needsIncrementalBarrier()) {
        if (*oldval && getClass()->trace)
            getClass()->trace(shadowZone->barrierTracer(), this->asObjectPtr());
    }
#endif
}

inline Value
ObjectValue(ObjectImpl &obj)
{
    Value v;
    v.setObject(*obj.asObjectPtr());
    return v;
}

inline Handle<JSObject*>
Downcast(Handle<ObjectImpl*> obj)
{
    return Handle<JSObject*>::fromMarkedLocation(reinterpret_cast<JSObject* const*>(obj.address()));
}

#ifdef DEBUG
static inline bool
IsObjectValueInCompartment(js::Value v, JSCompartment *comp)
{
    if (!v.isObject())
        return true;
    return reinterpret_cast<ObjectImpl*>(&v.toObject())->compartment() == comp;
}
#endif

} 

#endif 
