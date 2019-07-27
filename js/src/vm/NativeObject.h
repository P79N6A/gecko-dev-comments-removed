





#ifndef vm_NativeObject_h
#define vm_NativeObject_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

#include <stdint.h>

#include "jsfriendapi.h"
#include "jsobj.h"
#include "NamespaceImports.h"

#include "gc/Barrier.h"
#include "gc/Heap.h"
#include "gc/Marking.h"
#include "js/Value.h"
#include "vm/Shape.h"
#include "vm/String.h"
#include "vm/TypeInference.h"

namespace js {

class Nursery;
class Shape;







static MOZ_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(Value* beg, Value* end)
{
#ifdef DEBUG
    for (Value* v = beg; v != end; ++v)
        v->setObject(*reinterpret_cast<JSObject*>(0x42));
#endif
}

static MOZ_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(Value* vec, size_t len)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch(vec, vec + len);
#endif
}

static MOZ_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(HeapValue* vec, size_t len)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch((Value*) vec, len);
#endif
}

static MOZ_ALWAYS_INLINE void
Debug_SetSlotRangeToCrashOnTouch(HeapSlot* vec, uint32_t len)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch((Value*) vec, len);
#endif
}

static MOZ_ALWAYS_INLINE void
Debug_SetSlotRangeToCrashOnTouch(HeapSlot* begin, HeapSlot* end)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch((Value*) begin, end - begin);
#endif
}

class ArrayObject;








extern bool
ArraySetLength(JSContext* cx, Handle<ArrayObject*> obj, HandleId id,
               unsigned attrs, HandleValue value, ObjectOpResult& result);





































































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
    friend class NativeObject;
    friend class ArrayObject;
    friend class Nursery;

    friend bool js::SetIntegrityLevel(JSContext* cx, HandleObject obj, IntegrityLevel level);

    friend bool
    ArraySetLength(JSContext* cx, Handle<ArrayObject*> obj, HandleId id,
                   unsigned attrs, HandleValue value, ObjectOpResult& result);

    
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
        MOZ_ASSERT(!isCopyOnWrite());
        flags &= ~CONVERT_DOUBLE_ELEMENTS;
    }
    bool hasNonwritableArrayLength() const {
        return flags & NONWRITABLE_ARRAY_LENGTH;
    }
    void setNonwritableArrayLength() {
        MOZ_ASSERT(!isCopyOnWrite());
        flags |= NONWRITABLE_ARRAY_LENGTH;
    }
    bool isCopyOnWrite() const {
        return flags & COPY_ON_WRITE;
    }
    void clearCopyOnWrite() {
        MOZ_ASSERT(isCopyOnWrite());
        flags &= ~COPY_ON_WRITE;
    }

  public:
    MOZ_CONSTEXPR ObjectElements(uint32_t capacity, uint32_t length)
      : flags(0), initializedLength(0), capacity(capacity), length(length)
    {}

    HeapSlot* elements() {
        return reinterpret_cast<HeapSlot*>(uintptr_t(this) + sizeof(ObjectElements));
    }
    const HeapSlot* elements() const {
        return reinterpret_cast<const HeapSlot*>(uintptr_t(this) + sizeof(ObjectElements));
    }
    static ObjectElements * fromElements(HeapSlot* elems) {
        return reinterpret_cast<ObjectElements*>(uintptr_t(elems) - sizeof(ObjectElements));
    }

    HeapPtrNativeObject& ownerObject() const {
        MOZ_ASSERT(isCopyOnWrite());
        return *(HeapPtrNativeObject*)(&elements()[initializedLength]);
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

    static bool ConvertElementsToDoubles(JSContext* cx, uintptr_t elements);
    static bool MakeElementsCopyOnWrite(ExclusiveContext* cx, NativeObject* obj);

    
    
    static const size_t VALUES_PER_HEADER = 2;
};

static_assert(ObjectElements::VALUES_PER_HEADER * sizeof(HeapSlot) == sizeof(ObjectElements),
              "ObjectElements doesn't fit in the given number of slots");


extern HeapSlot* const emptyObjectElements;

struct Class;
class GCMarker;
struct ObjectOps;
class Shape;

class NewObjectCache;
class TaggedProto;

#ifdef DEBUG
static inline bool
IsObjectValueInCompartment(Value v, JSCompartment* comp);
#endif







inline void
DenseRangeWriteBarrierPost(JSRuntime* rt, NativeObject* obj, uint32_t start, uint32_t count)
{
    if (count > 0) {
        JS::shadow::Runtime* shadowRuntime = JS::shadow::Runtime::asShadowRuntime(rt);
        shadowRuntime->gcStoreBufferPtr()->putSlotFromAnyThread(obj, HeapSlot::Element, start, count);
    }
}






























class NativeObject : public JSObject
{
  protected:
    
    HeapPtrShape shape_;

    
    js::HeapSlot* slots_;

    
    js::HeapSlot* elements_;

    friend class ::JSObject;

  private:
    static void staticAsserts() {
        static_assert(sizeof(NativeObject) == sizeof(JSObject_Slots0),
                      "native object size must match GC thing size");
        static_assert(sizeof(NativeObject) == sizeof(shadow::Object),
                      "shadow interface must match actual implementation");
        static_assert(sizeof(NativeObject) % sizeof(Value) == 0,
                      "fixed slots after an object must be aligned");

        static_assert(offsetof(NativeObject, shape_) == offsetof(shadow::Object, shape),
                      "shadow shape must match actual shape");
        static_assert(offsetof(NativeObject, group_) == offsetof(shadow::Object, group),
                      "shadow type must match actual type");
        static_assert(offsetof(NativeObject, slots_) == offsetof(shadow::Object, slots),
                      "shadow slots must match actual slots");
        static_assert(offsetof(NativeObject, elements_) == offsetof(shadow::Object, _1),
                      "shadow placeholder must match actual elements");

        static_assert(MAX_FIXED_SLOTS <= Shape::FIXED_SLOTS_MAX,
                      "verify numFixedSlots() bitfield is big enough");
        static_assert(sizeof(NativeObject) + MAX_FIXED_SLOTS * sizeof(Value) == JSObject::MAX_BYTE_SIZE,
                      "inconsistent maximum object size");
    }

  public:
    Shape* lastProperty() const {
        MOZ_ASSERT(shape_);
        return shape_;
    }

    uint32_t propertyCount() const {
        return lastProperty()->entryCount();
    }

    bool hasShapeTable() const {
        return lastProperty()->hasTable();
    }

    HeapSlotArray getDenseElements() {
        return HeapSlotArray(elements_, !getElementsHeader()->isCopyOnWrite());
    }
    HeapSlotArray getDenseElementsAllowCopyOnWrite() {
        
        return HeapSlotArray(elements_, true);
    }
    const Value& getDenseElement(uint32_t idx) {
        MOZ_ASSERT(idx < getDenseInitializedLength());
        return elements_[idx];
    }
    bool containsDenseElement(uint32_t idx) {
        return idx < getDenseInitializedLength() && !elements_[idx].isMagic(JS_ELEMENTS_HOLE);
    }
    uint32_t getDenseInitializedLength() {
        return getElementsHeader()->initializedLength;
    }
    uint32_t getDenseCapacity() {
        return getElementsHeader()->capacity;
    }

    
    
    bool setLastProperty(ExclusiveContext* cx, Shape* shape);

    
    
    
    
    void setLastPropertyShrinkFixedSlots(Shape* shape);

    
    
    
    void setLastPropertyMakeNonNative(Shape* shape);

    
    
    
    void setLastPropertyMakeNative(ExclusiveContext* cx, Shape* shape);

  protected:
#ifdef DEBUG
    void checkShapeConsistency();
#else
    void checkShapeConsistency() { }
#endif

    Shape*
    replaceWithNewEquivalentShape(ExclusiveContext* cx,
                                  Shape* existingShape, Shape* newShape = nullptr,
                                  bool accessorShape = false);

    




    inline void removeLastProperty(ExclusiveContext* cx);
    inline bool canRemoveLastProperty();

    



    bool setSlotSpan(ExclusiveContext* cx, uint32_t span);

    bool toDictionaryMode(ExclusiveContext* cx);

  private:
    friend class Nursery;

    



    void getSlotRangeUnchecked(uint32_t start, uint32_t length,
                               HeapSlot** fixedStart, HeapSlot** fixedEnd,
                               HeapSlot** slotsStart, HeapSlot** slotsEnd)
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
                *slotsStart = &slots_[0];
                *slotsEnd = &slots_[length - localCopy];
            }
        } else {
            *fixedStart = *fixedEnd = nullptr;
            *slotsStart = &slots_[start - fixed];
            *slotsEnd = &slots_[start - fixed + length];
        }
    }

    void getSlotRange(uint32_t start, uint32_t length,
                      HeapSlot** fixedStart, HeapSlot** fixedEnd,
                      HeapSlot** slotsStart, HeapSlot** slotsEnd)
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
        HeapSlot* fixedStart;
        HeapSlot* fixedEnd;
        HeapSlot* slotsStart;
        HeapSlot* slotsEnd;
        getSlotRange(start, length, &fixedStart, &fixedEnd, &slotsStart, &slotsEnd);
        Debug_SetSlotRangeToCrashOnTouch(fixedStart, fixedEnd);
        Debug_SetSlotRangeToCrashOnTouch(slotsStart, slotsEnd);
#endif 
    }

    void initializeSlotRange(uint32_t start, uint32_t count);

    



    void initSlotRange(uint32_t start, const Value* vector, uint32_t length);

    



    void copySlotRange(uint32_t start, const Value* vector, uint32_t length);

#ifdef DEBUG
    enum SentinelAllowed {
        SENTINEL_NOT_ALLOWED,
        SENTINEL_ALLOWED
    };

    



    bool slotInRange(uint32_t slot, SentinelAllowed sentinel = SENTINEL_NOT_ALLOWED) const;
#endif

    




    static const uint32_t SLOT_CAPACITY_MIN = 8;

    HeapSlot* fixedSlots() const {
        return reinterpret_cast<HeapSlot*>(uintptr_t(this) + sizeof(NativeObject));
    }

  public:
    bool generateOwnShape(ExclusiveContext* cx, Shape* newShape = nullptr) {
        return replaceWithNewEquivalentShape(cx, lastProperty(), newShape);
    }

    bool shadowingShapeChange(ExclusiveContext* cx, const Shape& shape);
    bool clearFlag(ExclusiveContext* cx, BaseShape::Flag flag);

    static void slotsSizeMustNotOverflow() {
        static_assert((NativeObject::NELEMENTS_LIMIT - 1) <= INT32_MAX / sizeof(JS::Value),
                      "every caller of this method requires that a slot "
                      "number (or slot count) count multiplied by "
                      "sizeof(Value) can't overflow uint32_t (and sometimes "
                      "int32_t, too)");
    }

    uint32_t numFixedSlots() const {
        return reinterpret_cast<const shadow::Object*>(this)->numFixedSlots();
    }
    uint32_t numUsedFixedSlots() const {
        uint32_t nslots = lastProperty()->slotSpan(getClass());
        return Min(nslots, numFixedSlots());
    }

    uint32_t slotSpan() const {
        if (inDictionaryMode())
            return lastProperty()->base()->slotSpan();
        return lastProperty()->slotSpan();
    }

    
    bool isFixedSlot(size_t slot) {
        return slot < numFixedSlots();
    }

    
    size_t dynamicSlotIndex(size_t slot) {
        MOZ_ASSERT(slot >= numFixedSlots());
        return slot - numFixedSlots();
    }

    




    bool growSlots(ExclusiveContext* cx, uint32_t oldCount, uint32_t newCount);
    void shrinkSlots(ExclusiveContext* cx, uint32_t oldCount, uint32_t newCount);

    bool hasDynamicSlots() const { return !!slots_; }

    
    uint32_t numDynamicSlots() const {
        return dynamicSlotsCount(numFixedSlots(), slotSpan(), getClass());
    }

    bool empty() const {
        return lastProperty()->isEmptyShape();
    }

    Shape* lookup(ExclusiveContext* cx, jsid id);
    Shape* lookup(ExclusiveContext* cx, PropertyName* name) {
        return lookup(cx, NameToId(name));
    }

    bool contains(ExclusiveContext* cx, jsid id) {
        return lookup(cx, id) != nullptr;
    }
    bool contains(ExclusiveContext* cx, PropertyName* name) {
        return lookup(cx, name) != nullptr;
    }
    bool contains(ExclusiveContext* cx, Shape* shape) {
        return lookup(cx, shape->propid()) == shape;
    }

    bool containsShapeOrElement(ExclusiveContext* cx, jsid id) {
        if (JSID_IS_INT(id) && containsDenseElement(JSID_TO_INT(id)))
            return true;
        return contains(cx, id);
    }

    
    Shape* lookupPure(jsid id);
    Shape* lookupPure(PropertyName* name) {
        return lookupPure(NameToId(name));
    }

    bool containsPure(jsid id) {
        return lookupPure(id) != nullptr;
    }
    bool containsPure(PropertyName* name) {
        return containsPure(NameToId(name));
    }
    bool containsPure(Shape* shape) {
        return lookupPure(shape->propid()) == shape;
    }

    






    static bool allocSlot(ExclusiveContext* cx, HandleNativeObject obj, uint32_t* slotp);
    void freeSlot(uint32_t slot);

  private:
    static Shape* getChildPropertyOnDictionary(ExclusiveContext* cx, HandleNativeObject obj,
                                               HandleShape parent, StackShape& child);
    static Shape* getChildProperty(ExclusiveContext* cx, HandleNativeObject obj,
                                   HandleShape parent, StackShape& child);

  public:
    
    static Shape* addProperty(ExclusiveContext* cx, HandleNativeObject obj, HandleId id,
                              JSGetterOp getter, JSSetterOp setter,
                              uint32_t slot, unsigned attrs, unsigned flags,
                              bool allowDictionary = true);

    
    Shape* addDataProperty(ExclusiveContext* cx,
                           jsid id_, uint32_t slot, unsigned attrs);
    Shape* addDataProperty(ExclusiveContext* cx, HandlePropertyName name,
                           uint32_t slot, unsigned attrs);

    
    static Shape*
    putProperty(ExclusiveContext* cx, HandleNativeObject obj, HandleId id,
                JSGetterOp getter, JSSetterOp setter,
                uint32_t slot, unsigned attrs,
                unsigned flags);
    static inline Shape*
    putProperty(ExclusiveContext* cx, HandleObject obj, PropertyName* name,
                JSGetterOp getter, JSSetterOp setter,
                uint32_t slot, unsigned attrs,
                unsigned flags);

    
    static Shape*
    changeProperty(ExclusiveContext* cx, HandleNativeObject obj, HandleShape shape,
                   unsigned attrs, JSGetterOp getter, JSSetterOp setter);

    
    bool removeProperty(ExclusiveContext* cx, jsid id);

    
    static void clear(ExclusiveContext* cx, HandleNativeObject obj);

  protected:
    






    static Shape*
    addPropertyInternal(ExclusiveContext* cx, HandleNativeObject obj, HandleId id,
                        JSGetterOp getter, JSSetterOp setter, uint32_t slot, unsigned attrs,
                        unsigned flags, ShapeTable::Entry* entry, bool allowDictionary);

    void fillInAfterSwap(JSContext* cx, const Vector<Value>& values, void* priv);

  public:
    
    
    
    bool inDictionaryMode() const {
        return lastProperty()->inDictionary();
    }

    const Value& getSlot(uint32_t slot) const {
        MOZ_ASSERT(slotInRange(slot));
        uint32_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots()[slot];
        return slots_[slot - fixed];
    }

    const HeapSlot* getSlotAddressUnchecked(uint32_t slot) const {
        uint32_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots() + slot;
        return slots_ + (slot - fixed);
    }

    HeapSlot* getSlotAddressUnchecked(uint32_t slot) {
        uint32_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots() + slot;
        return slots_ + (slot - fixed);
    }

    HeapSlot* getSlotAddress(uint32_t slot) {
        




        MOZ_ASSERT(slotInRange(slot, SENTINEL_ALLOWED));
        return getSlotAddressUnchecked(slot);
    }

    const HeapSlot* getSlotAddress(uint32_t slot) const {
        




        MOZ_ASSERT(slotInRange(slot, SENTINEL_ALLOWED));
        return getSlotAddressUnchecked(slot);
    }

    HeapSlot& getSlotRef(uint32_t slot) {
        MOZ_ASSERT(slotInRange(slot));
        return *getSlotAddress(slot);
    }

    const HeapSlot& getSlotRef(uint32_t slot) const {
        MOZ_ASSERT(slotInRange(slot));
        return *getSlotAddress(slot);
    }

    void setSlot(uint32_t slot, const Value& value) {
        MOZ_ASSERT(slotInRange(slot));
        MOZ_ASSERT(IsObjectValueInCompartment(value, compartment()));
        getSlotRef(slot).set(this, HeapSlot::Slot, slot, value);
    }

    void initSlot(uint32_t slot, const Value& value) {
        MOZ_ASSERT(getSlot(slot).isUndefined());
        MOZ_ASSERT(slotInRange(slot));
        MOZ_ASSERT(IsObjectValueInCompartment(value, compartment()));
        initSlotUnchecked(slot, value);
    }

    void initSlotUnchecked(uint32_t slot, const Value& value) {
        getSlotAddressUnchecked(slot)->init(this, HeapSlot::Slot, slot, value);
    }

    
    
    static const uint32_t MAX_FIXED_SLOTS = 16;

  protected:
    inline bool updateSlotsForSpan(ExclusiveContext* cx, size_t oldSpan, size_t newSpan);

  public:
    



    void prepareSlotRangeForOverwrite(size_t start, size_t end) {
        for (size_t i = start; i < end; i++)
            getSlotAddressUnchecked(i)->HeapSlot::~HeapSlot();
    }

    void prepareElementRangeForOverwrite(size_t start, size_t end) {
        MOZ_ASSERT(end <= getDenseInitializedLength());
        MOZ_ASSERT(!denseElementsAreCopyOnWrite());
        for (size_t i = start; i < end; i++)
            elements_[i].HeapSlot::~HeapSlot();
    }

    static bool rollbackProperties(ExclusiveContext* cx, HandleNativeObject obj,
                                   uint32_t slotSpan);

    inline void setSlotWithType(ExclusiveContext* cx, Shape* shape,
                                const Value& value, bool overwriting = true);

    inline const Value& getReservedSlot(uint32_t index) const {
        MOZ_ASSERT(index < JSSLOT_FREE(getClass()));
        return getSlot(index);
    }

    const HeapSlot& getReservedSlotRef(uint32_t index) const {
        MOZ_ASSERT(index < JSSLOT_FREE(getClass()));
        return getSlotRef(index);
    }

    HeapSlot& getReservedSlotRef(uint32_t index) {
        MOZ_ASSERT(index < JSSLOT_FREE(getClass()));
        return getSlotRef(index);
    }

    void initReservedSlot(uint32_t index, const Value& v) {
        MOZ_ASSERT(index < JSSLOT_FREE(getClass()));
        initSlot(index, v);
    }

    void setReservedSlot(uint32_t index, const Value& v) {
        MOZ_ASSERT(index < JSSLOT_FREE(getClass()));
        setSlot(index, v);
    }

    

    HeapSlot& getFixedSlotRef(uint32_t slot) {
        MOZ_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    const Value& getFixedSlot(uint32_t slot) const {
        MOZ_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    void setFixedSlot(uint32_t slot, const Value& value) {
        MOZ_ASSERT(slot < numFixedSlots());
        fixedSlots()[slot].set(this, HeapSlot::Slot, slot, value);
    }

    void initFixedSlot(uint32_t slot, const Value& value) {
        MOZ_ASSERT(slot < numFixedSlots());
        fixedSlots()[slot].init(this, HeapSlot::Slot, slot, value);
    }

    





    static uint32_t dynamicSlotsCount(uint32_t nfixed, uint32_t span, const Class* clasp);
    static uint32_t dynamicSlotsCount(Shape* shape) {
        return dynamicSlotsCount(shape->numFixedSlots(), shape->slotSpan(), shape->getObjectClass());
    }

    

    
    static const uint32_t NELEMENTS_LIMIT = JS_BIT(28);

    static void elementsSizeMustNotOverflow() {
        static_assert((NativeObject::NELEMENTS_LIMIT - 1) <= INT32_MAX / sizeof(JS::Value),
                      "every caller of this method require that an element "
                      "count multiplied by sizeof(Value) can't overflow "
                      "uint32_t (and sometimes int32_t ,too)");
    }

    ObjectElements * getElementsHeader() const {
        return ObjectElements::fromElements(elements_);
    }

    
    bool ensureElements(ExclusiveContext* cx, uint32_t capacity) {
        MOZ_ASSERT(!denseElementsAreCopyOnWrite());
        if (capacity > getDenseCapacity())
            return growElements(cx, capacity);
        return true;
    }

    static uint32_t goodAllocated(uint32_t n, uint32_t length);
    bool growElements(ExclusiveContext* cx, uint32_t newcap);
    void shrinkElements(ExclusiveContext* cx, uint32_t cap);
    void setDynamicElements(ObjectElements* header) {
        MOZ_ASSERT(!hasDynamicElements());
        elements_ = header->elements();
        MOZ_ASSERT(hasDynamicElements());
    }

    static bool CopyElementsForWrite(ExclusiveContext* cx, NativeObject* obj);

    bool maybeCopyElementsForWrite(ExclusiveContext* cx) {
        if (denseElementsAreCopyOnWrite())
            return CopyElementsForWrite(cx, this);
        return true;
    }

  private:
    inline void ensureDenseInitializedLengthNoPackedCheck(ExclusiveContext* cx,
                                                          uint32_t index, uint32_t extra);

  public:
    void setDenseInitializedLength(uint32_t length) {
        MOZ_ASSERT(length <= getDenseCapacity());
        MOZ_ASSERT(!denseElementsAreCopyOnWrite());
        prepareElementRangeForOverwrite(length, getElementsHeader()->initializedLength);
        getElementsHeader()->initializedLength = length;
    }

    inline void ensureDenseInitializedLength(ExclusiveContext* cx,
                                             uint32_t index, uint32_t extra);
    void setDenseElement(uint32_t index, const Value& val) {
        MOZ_ASSERT(index < getDenseInitializedLength());
        MOZ_ASSERT(!denseElementsAreCopyOnWrite());
        elements_[index].set(this, HeapSlot::Element, index, val);
    }

    void initDenseElement(uint32_t index, const Value& val) {
        MOZ_ASSERT(index < getDenseInitializedLength());
        MOZ_ASSERT(!denseElementsAreCopyOnWrite());
        elements_[index].init(this, HeapSlot::Element, index, val);
    }

    void setDenseElementMaybeConvertDouble(uint32_t index, const Value& val) {
        if (val.isInt32() && shouldConvertDoubleElements())
            setDenseElement(index, DoubleValue(val.toInt32()));
        else
            setDenseElement(index, val);
    }

    inline void setDenseElementWithType(ExclusiveContext* cx, uint32_t index,
                                        const Value& val);
    inline void initDenseElementWithType(ExclusiveContext* cx, uint32_t index,
                                         const Value& val);
    inline void setDenseElementHole(ExclusiveContext* cx, uint32_t index);
    static inline void removeDenseElementForSparseIndex(ExclusiveContext* cx,
                                                        HandleNativeObject obj, uint32_t index);

    inline Value getDenseOrTypedArrayElement(uint32_t idx);

    void copyDenseElements(uint32_t dstStart, const Value* src, uint32_t count) {
        MOZ_ASSERT(dstStart + count <= getDenseCapacity());
        MOZ_ASSERT(!denseElementsAreCopyOnWrite());
        JSRuntime* rt = runtimeFromMainThread();
        if (JS::IsIncrementalBarrierNeeded(rt)) {
            Zone* zone = this->zone();
            for (uint32_t i = 0; i < count; ++i)
                elements_[dstStart + i].set(zone, this, HeapSlot::Element, dstStart + i, src[i]);
        } else {
            memcpy(&elements_[dstStart], src, count * sizeof(HeapSlot));
            DenseRangeWriteBarrierPost(rt, this, dstStart, count);
        }
    }

    void initDenseElements(uint32_t dstStart, const Value* src, uint32_t count) {
        MOZ_ASSERT(dstStart + count <= getDenseCapacity());
        MOZ_ASSERT(!denseElementsAreCopyOnWrite());
        memcpy(&elements_[dstStart], src, count * sizeof(HeapSlot));
        DenseRangeWriteBarrierPost(runtimeFromMainThread(), this, dstStart, count);
    }

    void initDenseElementsUnbarriered(uint32_t dstStart, const Value* src, uint32_t count);

    void moveDenseElements(uint32_t dstStart, uint32_t srcStart, uint32_t count) {
        MOZ_ASSERT(dstStart + count <= getDenseCapacity());
        MOZ_ASSERT(srcStart + count <= getDenseInitializedLength());
        MOZ_ASSERT(!denseElementsAreCopyOnWrite());

        











        Zone* zone = this->zone();
        JS::shadow::Zone* shadowZone = JS::shadow::Zone::asShadowZone(zone);
        if (shadowZone->needsIncrementalBarrier()) {
            if (dstStart < srcStart) {
                HeapSlot* dst = elements_ + dstStart;
                HeapSlot* src = elements_ + srcStart;
                for (uint32_t i = 0; i < count; i++, dst++, src++)
                    dst->set(zone, this, HeapSlot::Element, dst - elements_, *src);
            } else {
                HeapSlot* dst = elements_ + dstStart + count - 1;
                HeapSlot* src = elements_ + srcStart + count - 1;
                for (uint32_t i = 0; i < count; i++, dst--, src--)
                    dst->set(zone, this, HeapSlot::Element, dst - elements_, *src);
            }
        } else {
            memmove(elements_ + dstStart, elements_ + srcStart, count * sizeof(HeapSlot));
            DenseRangeWriteBarrierPost(runtimeFromMainThread(), this, dstStart, count);
        }
    }

    void moveDenseElementsNoPreBarrier(uint32_t dstStart, uint32_t srcStart, uint32_t count) {
        MOZ_ASSERT(!shadowZone()->needsIncrementalBarrier());

        MOZ_ASSERT(dstStart + count <= getDenseCapacity());
        MOZ_ASSERT(srcStart + count <= getDenseCapacity());
        MOZ_ASSERT(!denseElementsAreCopyOnWrite());

        memmove(elements_ + dstStart, elements_ + srcStart, count * sizeof(Value));
        DenseRangeWriteBarrierPost(runtimeFromMainThread(), this, dstStart, count);
    }

    bool shouldConvertDoubleElements() {
        return getElementsHeader()->shouldConvertDoubleElements();
    }

    inline void setShouldConvertDoubleElements();
    inline void clearShouldConvertDoubleElements();

    bool denseElementsAreCopyOnWrite() {
        return getElementsHeader()->isCopyOnWrite();
    }

    
    inline bool writeToIndexWouldMarkNotPacked(uint32_t index);
    inline void markDenseElementsNotPacked(ExclusiveContext* cx);

    






    enum EnsureDenseResult { ED_OK, ED_FAILED, ED_SPARSE };

  public:
    inline EnsureDenseResult ensureDenseElements(ExclusiveContext* cx,
                                                 uint32_t index, uint32_t extra);

    inline EnsureDenseResult extendDenseElements(ExclusiveContext* cx,
                                                 uint32_t requiredCapacity, uint32_t extra);

    
    static bool sparsifyDenseElement(ExclusiveContext* cx,
                                     HandleNativeObject obj, uint32_t index);

    
    static bool sparsifyDenseElements(ExclusiveContext* cx, HandleNativeObject obj);

    
    static const uint32_t MIN_SPARSE_INDEX = 1000;

    



    static const unsigned SPARSE_DENSITY_RATIO = 8;

    



    bool willBeSparseElements(uint32_t requiredCapacity, uint32_t newElementsHint);

    



    static EnsureDenseResult maybeDensifySparseElements(ExclusiveContext* cx,
                                                        HandleNativeObject obj);

    inline HeapSlot* fixedElements() const {
        static_assert(2 * sizeof(Value) == sizeof(ObjectElements),
                      "when elements are stored inline, the first two "
                      "slots will hold the ObjectElements header");
        return &fixedSlots()[2];
    }

#ifdef DEBUG
    bool canHaveNonEmptyElements();
#endif

    void setFixedElements() {
        MOZ_ASSERT(canHaveNonEmptyElements());
        elements_ = fixedElements();
    }

    inline bool hasDynamicElements() const {
        






        return !hasEmptyElements() && elements_ != fixedElements();
    }

    inline bool hasFixedElements() const {
        return elements_ == fixedElements();
    }

    inline bool hasEmptyElements() const {
        return elements_ == emptyObjectElements;
    }

    




    inline uint8_t* fixedData(size_t nslots) const;

    inline void privateWriteBarrierPre(void** oldval);

    void privateWriteBarrierPost(void** pprivate) {
        gc::Cell** cellp = reinterpret_cast<gc::Cell**>(pprivate);
        MOZ_ASSERT(cellp);
        MOZ_ASSERT(*cellp);
        gc::StoreBuffer* storeBuffer = (*cellp)->storeBuffer();
        if (storeBuffer)
            storeBuffer->putCellFromAnyThread(cellp);
    }

    

    inline void*& privateRef(uint32_t nfixed) const { 
        




        MOZ_ASSERT(nfixed == numFixedSlots());
        MOZ_ASSERT(hasPrivate());
        HeapSlot* end = &fixedSlots()[nfixed];
        return *reinterpret_cast<void**>(end);
    }

    bool hasPrivate() const {
        return getClass()->hasPrivate();
    }
    void* getPrivate() const {
        return privateRef(numFixedSlots());
    }
    void setPrivate(void* data) {
        void** pprivate = &privateRef(numFixedSlots());
        privateWriteBarrierPre(pprivate);
        *pprivate = data;
    }

    void setPrivateGCThing(gc::Cell* cell) {
        void** pprivate = &privateRef(numFixedSlots());
        privateWriteBarrierPre(pprivate);
        *pprivate = reinterpret_cast<void*>(cell);
        privateWriteBarrierPost(pprivate);
    }

    void setPrivateUnbarriered(void* data) {
        void** pprivate = &privateRef(numFixedSlots());
        *pprivate = data;
    }
    void initPrivate(void* data) {
        privateRef(numFixedSlots()) = data;
    }

    
    inline void* getPrivate(uint32_t nfixed) const {
        return privateRef(nfixed);
    }

    static inline NativeObject*
    copy(ExclusiveContext* cx, gc::AllocKind kind, gc::InitialHeap heap,
         HandleNativeObject templateObject);

    
    static size_t offsetOfElements() { return offsetof(NativeObject, elements_); }
    static size_t offsetOfFixedElements() {
        return sizeof(NativeObject) + sizeof(ObjectElements);
    }

    static size_t getFixedSlotOffset(size_t slot) {
        return sizeof(NativeObject) + slot * sizeof(Value);
    }
    static size_t getPrivateDataOffset(size_t nfixed) { return getFixedSlotOffset(nfixed); }
    static size_t offsetOfSlots() { return offsetof(NativeObject, slots_); }
};



class PlainObject : public NativeObject
{
  public:
    static const js::Class class_;
};

inline void
NativeObject::privateWriteBarrierPre(void** oldval)
{
    JS::shadow::Zone* shadowZone = this->shadowZoneFromAnyThread();
    if (shadowZone->needsIncrementalBarrier()) {
        if (*oldval && getClass()->trace)
            getClass()->trace(shadowZone->barrierTracer(), this);
    }
}

#ifdef DEBUG
static inline bool
IsObjectValueInCompartment(Value v, JSCompartment* comp)
{
    if (!v.isObject())
        return true;
    return v.toObject().compartment() == comp;
}
#endif













extern bool
NativeDefineProperty(ExclusiveContext* cx, HandleNativeObject obj, HandleId id,
                     Handle<PropertyDescriptor> desc,
                     ObjectOpResult& result);

extern bool
NativeDefineProperty(ExclusiveContext* cx, HandleNativeObject obj, HandleId id, HandleValue value,
                     JSGetterOp getter, JSSetterOp setter, unsigned attrs,
                     ObjectOpResult& result);

extern bool
NativeDefineProperty(ExclusiveContext* cx, HandleNativeObject obj, PropertyName* name,
                     HandleValue value, GetterOp getter, SetterOp setter,
                     unsigned attrs, ObjectOpResult& result);

extern bool
NativeDefineElement(ExclusiveContext* cx, HandleNativeObject obj, uint32_t index, HandleValue value,
                    JSGetterOp getter, JSSetterOp setter, unsigned attrs,
                    ObjectOpResult& result);


extern bool
NativeDefineProperty(ExclusiveContext* cx, HandleNativeObject obj, HandleId id, HandleValue value,
                     JSGetterOp getter, JSSetterOp setter, unsigned attrs);

extern bool
NativeDefineProperty(ExclusiveContext* cx, HandleNativeObject obj, PropertyName* name,
                     HandleValue value, JSGetterOp getter, JSSetterOp setter,
                     unsigned attrs);

extern bool
NativeHasProperty(JSContext* cx, HandleNativeObject obj, HandleId id, bool* foundp);

extern bool
NativeGetProperty(JSContext* cx, HandleNativeObject obj, HandleObject receiver, HandleId id,
                  MutableHandleValue vp);

extern bool
NativeGetPropertyNoGC(JSContext* cx, NativeObject* obj, JSObject* receiver, jsid id, Value* vp);

extern bool
NativeGetElement(JSContext* cx, HandleNativeObject obj, HandleObject receiver, uint32_t index,
                 MutableHandleValue vp);

inline bool
NativeGetProperty(JSContext* cx, HandleNativeObject obj, HandleId id, MutableHandleValue vp)
{
    return NativeGetProperty(cx, obj, obj, id, vp);
}

inline bool
NativeGetElement(JSContext* cx, HandleNativeObject obj, uint32_t index, MutableHandleValue vp)
{
    return NativeGetElement(cx, obj, obj, index, vp);
}

bool
SetPropertyByDefining(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                      HandleValue receiver, bool objHasOwn, ObjectOpResult& result);

bool
SetPropertyOnProto(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                   HandleValue receiver, ObjectOpResult& result);








enum QualifiedBool {
    Unqualified = 0,
    Qualified = 1
};

extern bool
NativeSetProperty(JSContext* cx, HandleNativeObject obj, HandleId id, HandleValue v,
                  HandleValue receiver, QualifiedBool qualified, ObjectOpResult& result);

extern bool
NativeSetElement(JSContext* cx, HandleNativeObject obj, uint32_t index, HandleValue v,
                 HandleValue receiver, ObjectOpResult& result);

extern bool
NativeDeleteProperty(JSContext* cx, HandleNativeObject obj, HandleId id, ObjectOpResult& result);




template <AllowGC allowGC>
extern bool
NativeLookupOwnProperty(ExclusiveContext* cx,
                        typename MaybeRooted<NativeObject*, allowGC>::HandleType obj,
                        typename MaybeRooted<jsid, allowGC>::HandleType id,
                        typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp);






template <AllowGC allowGC>
extern bool
NativeLookupProperty(ExclusiveContext* cx,
                     typename MaybeRooted<NativeObject*, allowGC>::HandleType obj,
                     typename MaybeRooted<jsid, allowGC>::HandleType id,
                     typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
                     typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp);

inline bool
NativeLookupProperty(ExclusiveContext* cx, HandleNativeObject obj, PropertyName* name,
                     MutableHandleObject objp, MutableHandleShape propp);

extern bool
NativeLookupElement(JSContext* cx, HandleNativeObject obj, uint32_t index,
                    MutableHandleObject objp, MutableHandleShape propp);








extern bool
NativeGetExistingProperty(JSContext* cx, HandleObject receiver, HandleNativeObject obj,
                          HandleShape shape, MutableHandle<Value> vp);







extern bool
HasDataProperty(JSContext* cx, NativeObject* obj, jsid id, Value* vp);

inline bool
HasDataProperty(JSContext* cx, NativeObject* obj, PropertyName* name, Value* vp)
{
    return HasDataProperty(cx, obj, NameToId(name), vp);
}

extern bool
GetPropertyForNameLookup(JSContext* cx, HandleObject obj, HandleId id, MutableHandleValue vp);

} 

template <>
inline bool
JSObject::is<js::NativeObject>() const { return isNative(); }

namespace js {


inline NativeObject*
MaybeNativeObject(JSObject* obj)
{
    return obj ? &obj->as<NativeObject>() : nullptr;
}

} 




inline bool
js::HasProperty(JSContext* cx, HandleObject obj, HandleId id, bool* foundp)
{
    if (HasPropertyOp op = obj->getOps()->hasProperty)
        return op(cx, obj, id, foundp);
    return NativeHasProperty(cx, obj.as<NativeObject>(), id, foundp);
}

inline bool
js::GetProperty(JSContext* cx, HandleObject obj, HandleObject receiver, HandleId id,
                MutableHandleValue vp)
{
    if (GetPropertyOp op = obj->getOps()->getProperty)
        return op(cx, obj, receiver, id, vp);
    return NativeGetProperty(cx, obj.as<NativeObject>(), receiver, id, vp);
}

inline bool
js::GetPropertyNoGC(JSContext* cx, JSObject* obj, JSObject* receiver, jsid id, Value* vp)
{
    if (obj->getOps()->getProperty)
        return false;
    return NativeGetPropertyNoGC(cx, &obj->as<NativeObject>(), receiver, id, vp);
}

inline bool
js::SetProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                HandleValue receiver, ObjectOpResult& result)
{
    if (obj->getOps()->setProperty)
        return JSObject::nonNativeSetProperty(cx, obj, id, v, receiver, result);
    return NativeSetProperty(cx, obj.as<NativeObject>(), id, v, receiver, Qualified, result);
}

inline bool
js::SetElement(JSContext* cx, HandleObject obj, uint32_t index, HandleValue v,
               HandleValue receiver, ObjectOpResult& result)
{
    if (obj->getOps()->setProperty)
        return JSObject::nonNativeSetElement(cx, obj, index, v, receiver, result);
    return NativeSetElement(cx, obj.as<NativeObject>(), index, v, receiver, result);
}

#endif
