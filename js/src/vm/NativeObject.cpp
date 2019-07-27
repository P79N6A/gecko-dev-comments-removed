





#include "vm/NativeObject-inl.h"

#include "mozilla/CheckedInt.h"

#include "jswatchpoint.h"

#include "gc/Marking.h"
#include "js/Value.h"
#include "vm/Debugger.h"
#include "vm/TypedArrayCommon.h"

#include "jsobjinlines.h"

#include "vm/ArrayObject-inl.h"
#include "vm/Shape-inl.h"

using namespace js;

using JS::GenericNaN;
using mozilla::DebugOnly;
using mozilla::RoundUpPow2;

static const ObjectElements emptyElementsHeader(0, 0);


HeapSlot* const js::emptyObjectElements =
    reinterpret_cast<HeapSlot*>(uintptr_t(&emptyElementsHeader) + sizeof(ObjectElements));

#ifdef DEBUG

bool
NativeObject::canHaveNonEmptyElements()
{
    return !IsAnyTypedArray(this);
}

#endif 

 bool
ObjectElements::ConvertElementsToDoubles(JSContext* cx, uintptr_t elementsPtr)
{
    




    HeapSlot* elementsHeapPtr = (HeapSlot*) elementsPtr;
    MOZ_ASSERT(elementsHeapPtr != emptyObjectElements);

    ObjectElements* header = ObjectElements::fromElements(elementsHeapPtr);
    MOZ_ASSERT(!header->shouldConvertDoubleElements());

    
    
    Value* vp = (Value*) elementsPtr;
    for (size_t i = 0; i < header->initializedLength; i++) {
        if (vp[i].isInt32())
            vp[i].setDouble(vp[i].toInt32());
    }

    header->setShouldConvertDoubleElements();
    return true;
}

 bool
ObjectElements::MakeElementsCopyOnWrite(ExclusiveContext* cx, NativeObject* obj)
{
    static_assert(sizeof(HeapSlot) >= sizeof(HeapPtrObject),
                  "there must be enough room for the owner object pointer at "
                  "the end of the elements");
    if (!obj->ensureElements(cx, obj->getDenseInitializedLength() + 1))
        return false;

    ObjectElements* header = obj->getElementsHeader();

    
    
    MOZ_ASSERT(!header->isCopyOnWrite());
    header->flags |= COPY_ON_WRITE;

    header->ownerObject().init(obj);
    return true;
}

#ifdef DEBUG
void
js::NativeObject::checkShapeConsistency()
{
    static int throttle = -1;
    if (throttle < 0) {
        if (const char* var = getenv("JS_CHECK_SHAPE_THROTTLE"))
            throttle = atoi(var);
        if (throttle < 0)
            throttle = 0;
    }
    if (throttle == 0)
        return;

    MOZ_ASSERT(isNative());

    Shape* shape = lastProperty();
    Shape* prev = nullptr;

    if (inDictionaryMode()) {
        MOZ_ASSERT(shape->hasTable());

        ShapeTable& table = shape->table();
        for (uint32_t fslot = table.freeList();
             fslot != SHAPE_INVALID_SLOT;
             fslot = getSlot(fslot).toPrivateUint32())
        {
            MOZ_ASSERT(fslot < slotSpan());
        }

        for (int n = throttle; --n >= 0 && shape->parent; shape = shape->parent) {
            MOZ_ASSERT_IF(lastProperty() != shape, !shape->hasTable());

            ShapeTable::Entry& entry = table.search(shape->propid(), false);
            MOZ_ASSERT(entry.shape() == shape);
        }

        shape = lastProperty();
        for (int n = throttle; --n >= 0 && shape; shape = shape->parent) {
            MOZ_ASSERT_IF(shape->slot() != SHAPE_INVALID_SLOT, shape->slot() < slotSpan());
            if (!prev) {
                MOZ_ASSERT(lastProperty() == shape);
                MOZ_ASSERT(shape->listp == &shape_);
            } else {
                MOZ_ASSERT(shape->listp == &prev->parent);
            }
            prev = shape;
        }
    } else {
        for (int n = throttle; --n >= 0 && shape->parent; shape = shape->parent) {
            if (shape->hasTable()) {
                ShapeTable& table = shape->table();
                MOZ_ASSERT(shape->parent);
                for (Shape::Range<NoGC> r(shape); !r.empty(); r.popFront()) {
                    ShapeTable::Entry& entry = table.search(r.front().propid(), false);
                    MOZ_ASSERT(entry.shape() == &r.front());
                }
            }
            if (prev) {
                MOZ_ASSERT(prev->maybeSlot() >= shape->maybeSlot());
                shape->kids.checkConsistency(prev);
            }
            prev = shape;
        }
    }
}
#endif

void
js::NativeObject::initializeSlotRange(uint32_t start, uint32_t length)
{
    



    HeapSlot* fixedStart;
    HeapSlot* fixedEnd;
    HeapSlot* slotsStart;
    HeapSlot* slotsEnd;
    getSlotRangeUnchecked(start, length, &fixedStart, &fixedEnd, &slotsStart, &slotsEnd);

    uint32_t offset = start;
    for (HeapSlot* sp = fixedStart; sp < fixedEnd; sp++)
        sp->init(this, HeapSlot::Slot, offset++, UndefinedValue());
    for (HeapSlot* sp = slotsStart; sp < slotsEnd; sp++)
        sp->init(this, HeapSlot::Slot, offset++, UndefinedValue());
}

void
js::NativeObject::initSlotRange(uint32_t start, const Value* vector, uint32_t length)
{
    HeapSlot* fixedStart;
    HeapSlot* fixedEnd;
    HeapSlot* slotsStart;
    HeapSlot* slotsEnd;
    getSlotRange(start, length, &fixedStart, &fixedEnd, &slotsStart, &slotsEnd);
    for (HeapSlot* sp = fixedStart; sp < fixedEnd; sp++)
        sp->init(this, HeapSlot::Slot, start++, *vector++);
    for (HeapSlot* sp = slotsStart; sp < slotsEnd; sp++)
        sp->init(this, HeapSlot::Slot, start++, *vector++);
}

void
js::NativeObject::copySlotRange(uint32_t start, const Value* vector, uint32_t length)
{
    JS::Zone* zone = this->zone();
    HeapSlot* fixedStart;
    HeapSlot* fixedEnd;
    HeapSlot* slotsStart;
    HeapSlot* slotsEnd;
    getSlotRange(start, length, &fixedStart, &fixedEnd, &slotsStart, &slotsEnd);
    for (HeapSlot* sp = fixedStart; sp < fixedEnd; sp++)
        sp->set(zone, this, HeapSlot::Slot, start++, *vector++);
    for (HeapSlot* sp = slotsStart; sp < slotsEnd; sp++)
        sp->set(zone, this, HeapSlot::Slot, start++, *vector++);
}

#ifdef DEBUG
bool
js::NativeObject::slotInRange(uint32_t slot, SentinelAllowed sentinel) const
{
    uint32_t capacity = numFixedSlots() + numDynamicSlots();
    if (sentinel == SENTINEL_ALLOWED)
        return slot <= capacity;
    return slot < capacity;
}
#endif 

Shape*
js::NativeObject::lookup(ExclusiveContext* cx, jsid id)
{
    MOZ_ASSERT(isNative());
    ShapeTable::Entry* entry;
    return Shape::search(cx, lastProperty(), id, &entry);
}

Shape*
js::NativeObject::lookupPure(jsid id)
{
    MOZ_ASSERT(isNative());
    return Shape::searchNoHashify(lastProperty(), id);
}

uint32_t
js::NativeObject::dynamicSlotsCount(uint32_t nfixed, uint32_t span, const Class* clasp)
{
    if (span <= nfixed)
        return 0;
    span -= nfixed;

    
    
    
    if (clasp != &ArrayObject::class_ && span <= SLOT_CAPACITY_MIN)
        return SLOT_CAPACITY_MIN;

    uint32_t slots = mozilla::RoundUpPow2(span);
    MOZ_ASSERT(slots >= span);
    return slots;
}

inline bool
NativeObject::updateSlotsForSpan(ExclusiveContext* cx, size_t oldSpan, size_t newSpan)
{
    MOZ_ASSERT(oldSpan != newSpan);

    size_t oldCount = dynamicSlotsCount(numFixedSlots(), oldSpan, getClass());
    size_t newCount = dynamicSlotsCount(numFixedSlots(), newSpan, getClass());

    if (oldSpan < newSpan) {
        if (oldCount < newCount && !growSlots(cx, oldCount, newCount))
            return false;

        if (newSpan == oldSpan + 1)
            initSlotUnchecked(oldSpan, UndefinedValue());
        else
            initializeSlotRange(oldSpan, newSpan - oldSpan);
    } else {
        
        prepareSlotRangeForOverwrite(newSpan, oldSpan);
        invalidateSlotRange(newSpan, oldSpan - newSpan);

        if (oldCount > newCount)
            shrinkSlots(cx, oldCount, newCount);
    }

    return true;
}

bool
NativeObject::setLastProperty(ExclusiveContext* cx, Shape* shape)
{
    MOZ_ASSERT(!inDictionaryMode());
    MOZ_ASSERT(!shape->inDictionary());
    MOZ_ASSERT(shape->compartment() == compartment());
    MOZ_ASSERT(shape->numFixedSlots() == numFixedSlots());
    MOZ_ASSERT(shape->getObjectClass() == getClass());

    size_t oldSpan = lastProperty()->slotSpan();
    size_t newSpan = shape->slotSpan();

    if (oldSpan == newSpan) {
        shape_ = shape;
        return true;
    }

    if (!updateSlotsForSpan(cx, oldSpan, newSpan))
        return false;

    shape_ = shape;
    return true;
}

void
NativeObject::setLastPropertyShrinkFixedSlots(Shape* shape)
{
    MOZ_ASSERT(!inDictionaryMode());
    MOZ_ASSERT(!shape->inDictionary());
    MOZ_ASSERT(shape->compartment() == compartment());
    MOZ_ASSERT(lastProperty()->slotSpan() == shape->slotSpan());
    MOZ_ASSERT(shape->getObjectClass() == getClass());

    DebugOnly<size_t> oldFixed = numFixedSlots();
    DebugOnly<size_t> newFixed = shape->numFixedSlots();
    MOZ_ASSERT(newFixed < oldFixed);
    MOZ_ASSERT(shape->slotSpan() <= oldFixed);
    MOZ_ASSERT(shape->slotSpan() <= newFixed);
    MOZ_ASSERT(dynamicSlotsCount(oldFixed, shape->slotSpan(), getClass()) == 0);
    MOZ_ASSERT(dynamicSlotsCount(newFixed, shape->slotSpan(), getClass()) == 0);

    shape_ = shape;
}

void
NativeObject::setLastPropertyMakeNonNative(Shape* shape)
{
    MOZ_ASSERT(!inDictionaryMode());
    MOZ_ASSERT(!shape->getObjectClass()->isNative());
    MOZ_ASSERT(shape->compartment() == compartment());
    MOZ_ASSERT(shape->slotSpan() == 0);
    MOZ_ASSERT(shape->numFixedSlots() == 0);
    MOZ_ASSERT(!hasDynamicElements());
    MOZ_ASSERT(!hasDynamicSlots());

    shape_ = shape;
}

void
NativeObject::setLastPropertyMakeNative(ExclusiveContext* cx, Shape* shape)
{
    MOZ_ASSERT(getClass()->isNative());
    MOZ_ASSERT(shape->isNative());
    MOZ_ASSERT(!shape->inDictionary());

    
    
    
    shape_.init(shape);

    slots_ = nullptr;
    elements_ = emptyObjectElements;

    size_t oldSpan = shape->numFixedSlots();
    size_t newSpan = shape->slotSpan();

    
    
    if (oldSpan != newSpan && !updateSlotsForSpan(cx, oldSpan, newSpan))
        CrashAtUnhandlableOOM("NativeObject::setLastPropertyMakeNative");
}

bool
NativeObject::setSlotSpan(ExclusiveContext* cx, uint32_t span)
{
    MOZ_ASSERT(inDictionaryMode());

    size_t oldSpan = lastProperty()->base()->slotSpan();
    if (oldSpan == span)
        return true;

    if (!updateSlotsForSpan(cx, oldSpan, span))
        return false;

    lastProperty()->base()->setSlotSpan(span);
    return true;
}



static HeapSlot*
AllocateSlots(ExclusiveContext* cx, JSObject* obj, uint32_t nslots)
{
    if (cx->isJSContext())
        return cx->asJSContext()->runtime()->gc.nursery.allocateSlots(obj, nslots);
    return obj->zone()->pod_malloc<HeapSlot>(nslots);
}





static HeapSlot*
ReallocateSlots(ExclusiveContext* cx, JSObject* obj, HeapSlot* oldSlots,
                uint32_t oldCount, uint32_t newCount)
{
    if (cx->isJSContext()) {
        return cx->asJSContext()->runtime()->gc.nursery.reallocateSlots(obj, oldSlots,
                                                                        oldCount, newCount);
    }
    return obj->zone()->pod_realloc<HeapSlot>(oldSlots, oldCount, newCount);
}

bool
NativeObject::growSlots(ExclusiveContext* cx, uint32_t oldCount, uint32_t newCount)
{
    MOZ_ASSERT(newCount > oldCount);
    MOZ_ASSERT_IF(!is<ArrayObject>(), newCount >= SLOT_CAPACITY_MIN);

    




    NativeObject::slotsSizeMustNotOverflow();
    MOZ_ASSERT(newCount < NELEMENTS_LIMIT);

    if (!oldCount) {
        slots_ = AllocateSlots(cx, this, newCount);
        if (!slots_)
            return false;
        Debug_SetSlotRangeToCrashOnTouch(slots_, newCount);
        return true;
    }

    HeapSlot* newslots = ReallocateSlots(cx, this, slots_, oldCount, newCount);
    if (!newslots)
        return false;  

    slots_ = newslots;

    Debug_SetSlotRangeToCrashOnTouch(slots_ + oldCount, newCount - oldCount);

    return true;
}

static void
FreeSlots(ExclusiveContext* cx, HeapSlot* slots)
{
    
    if (cx->isJSContext())
        return cx->asJSContext()->runtime()->gc.nursery.freeSlots(slots);
    js_free(slots);
}

void
NativeObject::shrinkSlots(ExclusiveContext* cx, uint32_t oldCount, uint32_t newCount)
{
    MOZ_ASSERT(newCount < oldCount);

    if (newCount == 0) {
        FreeSlots(cx, slots_);
        slots_ = nullptr;
        return;
    }

    MOZ_ASSERT_IF(!is<ArrayObject>(), newCount >= SLOT_CAPACITY_MIN);

    HeapSlot* newslots = ReallocateSlots(cx, this, slots_, oldCount, newCount);
    if (!newslots)
        return;  

    slots_ = newslots;
}

 bool
NativeObject::sparsifyDenseElement(ExclusiveContext* cx, HandleNativeObject obj, uint32_t index)
{
    if (!obj->maybeCopyElementsForWrite(cx))
        return false;

    RootedValue value(cx, obj->getDenseElement(index));
    MOZ_ASSERT(!value.isMagic(JS_ELEMENTS_HOLE));

    removeDenseElementForSparseIndex(cx, obj, index);

    uint32_t slot = obj->slotSpan();
    if (!obj->addDataProperty(cx, INT_TO_JSID(index), slot, JSPROP_ENUMERATE)) {
        obj->setDenseElement(index, value);
        return false;
    }

    MOZ_ASSERT(slot == obj->slotSpan() - 1);
    obj->initSlot(slot, value);

    return true;
}

 bool
NativeObject::sparsifyDenseElements(js::ExclusiveContext* cx, HandleNativeObject obj)
{
    if (!obj->maybeCopyElementsForWrite(cx))
        return false;

    uint32_t initialized = obj->getDenseInitializedLength();

    
    for (uint32_t i = 0; i < initialized; i++) {
        if (obj->getDenseElement(i).isMagic(JS_ELEMENTS_HOLE))
            continue;

        if (!sparsifyDenseElement(cx, obj, i))
            return false;
    }

    if (initialized)
        obj->setDenseInitializedLength(0);

    




    if (obj->getDenseCapacity()) {
        obj->shrinkElements(cx, 0);
        obj->getElementsHeader()->capacity = 0;
    }

    return true;
}

bool
NativeObject::willBeSparseElements(uint32_t requiredCapacity, uint32_t newElementsHint)
{
    MOZ_ASSERT(isNative());
    MOZ_ASSERT(requiredCapacity > MIN_SPARSE_INDEX);

    uint32_t cap = getDenseCapacity();
    MOZ_ASSERT(requiredCapacity >= cap);

    if (requiredCapacity >= NELEMENTS_LIMIT)
        return true;

    uint32_t minimalDenseCount = requiredCapacity / SPARSE_DENSITY_RATIO;
    if (newElementsHint >= minimalDenseCount)
        return false;
    minimalDenseCount -= newElementsHint;

    if (minimalDenseCount > cap)
        return true;

    uint32_t len = getDenseInitializedLength();
    const Value* elems = getDenseElements();
    for (uint32_t i = 0; i < len; i++) {
        if (!elems[i].isMagic(JS_ELEMENTS_HOLE) && !--minimalDenseCount)
            return false;
    }
    return true;
}

 NativeObject::EnsureDenseResult
NativeObject::maybeDensifySparseElements(js::ExclusiveContext* cx, HandleNativeObject obj)
{
    




    if (!obj->inDictionaryMode())
        return ED_SPARSE;

    



    uint32_t slotSpan = obj->slotSpan();
    if (slotSpan != RoundUpPow2(slotSpan))
        return ED_SPARSE;

    
    if (!obj->nonProxyIsExtensible() || obj->watched())
        return ED_SPARSE;

    



    uint32_t numDenseElements = 0;
    uint32_t newInitializedLength = 0;

    RootedShape shape(cx, obj->lastProperty());
    while (!shape->isEmptyShape()) {
        uint32_t index;
        if (IdIsIndex(shape->propid(), &index)) {
            if (shape->attributes() == JSPROP_ENUMERATE &&
                shape->hasDefaultGetter() &&
                shape->hasDefaultSetter())
            {
                numDenseElements++;
                newInitializedLength = Max(newInitializedLength, index + 1);
            } else {
                



                return ED_SPARSE;
            }
        }
        shape = shape->previous();
    }

    if (numDenseElements * SPARSE_DENSITY_RATIO < newInitializedLength)
        return ED_SPARSE;

    if (newInitializedLength >= NELEMENTS_LIMIT)
        return ED_SPARSE;

    




    if (!obj->maybeCopyElementsForWrite(cx))
        return ED_FAILED;

    if (newInitializedLength > obj->getDenseCapacity()) {
        if (!obj->growElements(cx, newInitializedLength))
            return ED_FAILED;
    }

    obj->ensureDenseInitializedLength(cx, newInitializedLength, 0);

    RootedValue value(cx);

    shape = obj->lastProperty();
    while (!shape->isEmptyShape()) {
        jsid id = shape->propid();
        uint32_t index;
        if (IdIsIndex(id, &index)) {
            value = obj->getSlot(shape->slot());

            






            if (shape != obj->lastProperty()) {
                shape = shape->previous();
                if (!obj->removeProperty(cx, id))
                    return ED_FAILED;
            } else {
                if (!obj->removeProperty(cx, id))
                    return ED_FAILED;
                shape = obj->lastProperty();
            }

            obj->setDenseElement(index, value);
        } else {
            shape = shape->previous();
        }
    }

    




    if (!obj->clearFlag(cx, BaseShape::INDEXED))
        return ED_FAILED;

    return ED_OK;
}



static ObjectElements*
AllocateElements(ExclusiveContext* cx, JSObject* obj, uint32_t nelems)
{
    if (cx->isJSContext())
        return cx->asJSContext()->runtime()->gc.nursery.allocateElements(obj, nelems);
    return reinterpret_cast<js::ObjectElements*>(obj->zone()->pod_malloc<HeapSlot>(nelems));
}



static ObjectElements*
ReallocateElements(ExclusiveContext* cx, JSObject* obj, ObjectElements* oldHeader,
                   uint32_t oldCount, uint32_t newCount)
{
    if (cx->isJSContext()) {
        return cx->asJSContext()->runtime()->gc.nursery.reallocateElements(obj, oldHeader,
                                                                           oldCount, newCount);
    }
    return reinterpret_cast<js::ObjectElements*>(
            obj->zone()->pod_realloc<HeapSlot>(reinterpret_cast<HeapSlot*>(oldHeader),
                                               oldCount, newCount));
}

























 uint32_t
NativeObject::goodAllocated(uint32_t reqAllocated, uint32_t length = 0)
{
    static const uint32_t Mebi = 1024 * 1024;

    
    
    
    
    
    
    
    
    
    
    static const uint32_t BigBuckets[] = {
        1048576, 2097152, 3145728, 4194304, 5242880, 6291456, 7340032, 8388608,
        9437184, 11534336, 13631488, 15728640, 17825792, 20971520, 24117248,
        27262976, 31457280, 35651584, 40894464, 46137344, 52428800, 59768832,
        68157440, 77594624, 88080384, 99614720, 112197632, 126877696,
        143654912, 162529280, 183500800, 206569472, 232783872, 262144000,
        295698432, 333447168, 375390208, 422576128, 476053504, 535822336,
        602931200, 678428672, 763363328, 858783744, 966787072, 1088421888,
        1224736768, 1377828864, 1550843904, 1744830464, 1962934272, 2208301056,
        2485125120, 2796552192, 3146776576, 3541041152, 3984588800, 0
    };

    
    uint32_t goodAllocated = reqAllocated;
    if (goodAllocated < Mebi) {
        goodAllocated = RoundUpPow2(goodAllocated);

        
        uint32_t goodCapacity = goodAllocated - ObjectElements::VALUES_PER_HEADER;
        uint32_t reqCapacity = reqAllocated - ObjectElements::VALUES_PER_HEADER;
        if (length >= reqCapacity && goodCapacity > (length / 3) * 2)
            goodAllocated = length + ObjectElements::VALUES_PER_HEADER;

        if (goodAllocated < SLOT_CAPACITY_MIN)
            goodAllocated = SLOT_CAPACITY_MIN;

    } else {
        uint32_t i = 0;
        while (true) {
            uint32_t b = BigBuckets[i++];
            if (b >= goodAllocated) {
                
                
                goodAllocated = b;
                break;
            } else if (b == 0) {
                
                goodAllocated = 0xffffffff;
                break;
            }
        }
    }

    return goodAllocated;
}

bool
NativeObject::growElements(ExclusiveContext* cx, uint32_t reqCapacity)
{
    MOZ_ASSERT(nonProxyIsExtensible());
    MOZ_ASSERT(canHaveNonEmptyElements());
    if (denseElementsAreCopyOnWrite())
        MOZ_CRASH();

    uint32_t oldCapacity = getDenseCapacity();
    MOZ_ASSERT(oldCapacity < reqCapacity);

    using mozilla::CheckedInt;

    CheckedInt<uint32_t> checkedOldAllocated =
        CheckedInt<uint32_t>(oldCapacity) + ObjectElements::VALUES_PER_HEADER;
    CheckedInt<uint32_t> checkedReqAllocated =
        CheckedInt<uint32_t>(reqCapacity) + ObjectElements::VALUES_PER_HEADER;
    if (!checkedOldAllocated.isValid() || !checkedReqAllocated.isValid())
        return false;

    uint32_t reqAllocated = checkedReqAllocated.value();
    uint32_t oldAllocated = checkedOldAllocated.value();

    uint32_t newAllocated;
    if (is<ArrayObject>() && !as<ArrayObject>().lengthIsWritable()) {
        MOZ_ASSERT(reqCapacity <= as<ArrayObject>().length());
        
        
        
        newAllocated = reqAllocated;
    } else {
        newAllocated = goodAllocated(reqAllocated, getElementsHeader()->length);
    }

    uint32_t newCapacity = newAllocated - ObjectElements::VALUES_PER_HEADER;
    MOZ_ASSERT(newCapacity > oldCapacity && newCapacity >= reqCapacity);

    
    if (newCapacity >= NELEMENTS_LIMIT)
        return false;

    uint32_t initlen = getDenseInitializedLength();

    ObjectElements* newheader;
    if (hasDynamicElements()) {
        newheader = ReallocateElements(cx, this, getElementsHeader(), oldAllocated, newAllocated);
        if (!newheader)
            return false;   
    } else {
        newheader = AllocateElements(cx, this, newAllocated);
        if (!newheader)
            return false;   
        js_memcpy(newheader, getElementsHeader(),
                  (ObjectElements::VALUES_PER_HEADER + initlen) * sizeof(Value));
    }

    newheader->capacity = newCapacity;
    elements_ = newheader->elements();

    Debug_SetSlotRangeToCrashOnTouch(elements_ + initlen, newCapacity - initlen);

    return true;
}

void
NativeObject::shrinkElements(ExclusiveContext* cx, uint32_t reqCapacity)
{
    MOZ_ASSERT(canHaveNonEmptyElements());
    if (denseElementsAreCopyOnWrite())
        MOZ_CRASH();

    if (!hasDynamicElements())
        return;

    uint32_t oldCapacity = getDenseCapacity();
    MOZ_ASSERT(reqCapacity < oldCapacity);

    uint32_t oldAllocated = oldCapacity + ObjectElements::VALUES_PER_HEADER;
    uint32_t reqAllocated = reqCapacity + ObjectElements::VALUES_PER_HEADER;
    uint32_t newAllocated = goodAllocated(reqAllocated);
    if (newAllocated == oldAllocated)
        return;  

    MOZ_ASSERT(newAllocated > ObjectElements::VALUES_PER_HEADER);
    uint32_t newCapacity = newAllocated - ObjectElements::VALUES_PER_HEADER;

    ObjectElements* newheader = ReallocateElements(cx, this, getElementsHeader(),
                                                   oldAllocated, newAllocated);
    if (!newheader) {
        cx->recoverFromOutOfMemory();
        return;  
    }

    newheader->capacity = newCapacity;
    elements_ = newheader->elements();
}

 bool
NativeObject::CopyElementsForWrite(ExclusiveContext* cx, NativeObject* obj)
{
    MOZ_ASSERT(obj->denseElementsAreCopyOnWrite());

    
    MOZ_ASSERT(obj->getElementsHeader()->ownerObject() != obj);

    uint32_t initlen = obj->getDenseInitializedLength();
    uint32_t allocated = initlen + ObjectElements::VALUES_PER_HEADER;
    uint32_t newAllocated = goodAllocated(allocated);

    uint32_t newCapacity = newAllocated - ObjectElements::VALUES_PER_HEADER;

    if (newCapacity >= NELEMENTS_LIMIT)
        return false;

    JSObject::writeBarrierPre(obj->getElementsHeader()->ownerObject());

    ObjectElements* newheader = AllocateElements(cx, obj, newAllocated);
    if (!newheader)
        return false;
    js_memcpy(newheader, obj->getElementsHeader(),
              (ObjectElements::VALUES_PER_HEADER + initlen) * sizeof(Value));

    newheader->capacity = newCapacity;
    newheader->clearCopyOnWrite();
    obj->elements_ = newheader->elements();

    Debug_SetSlotRangeToCrashOnTouch(obj->elements_ + initlen, newCapacity - initlen);

    return true;
}

 bool
NativeObject::allocSlot(ExclusiveContext* cx, HandleNativeObject obj, uint32_t* slotp)
{
    uint32_t slot = obj->slotSpan();
    MOZ_ASSERT(slot >= JSSLOT_FREE(obj->getClass()));

    



    if (obj->inDictionaryMode()) {
        ShapeTable& table = obj->lastProperty()->table();
        uint32_t last = table.freeList();
        if (last != SHAPE_INVALID_SLOT) {
#ifdef DEBUG
            MOZ_ASSERT(last < slot);
            uint32_t next = obj->getSlot(last).toPrivateUint32();
            MOZ_ASSERT_IF(next != SHAPE_INVALID_SLOT, next < slot);
#endif

            *slotp = last;

            const Value& vref = obj->getSlot(last);
            table.setFreeList(vref.toPrivateUint32());
            obj->setSlot(last, UndefinedValue());
            return true;
        }
    }

    if (slot >= SHAPE_MAXIMUM_SLOT) {
        ReportOutOfMemory(cx);
        return false;
    }

    *slotp = slot;

    if (obj->inDictionaryMode() && !obj->setSlotSpan(cx, slot + 1))
        return false;

    return true;
}

void
NativeObject::freeSlot(uint32_t slot)
{
    MOZ_ASSERT(slot < slotSpan());

    if (inDictionaryMode()) {
        ShapeTable& table = lastProperty()->table();
        uint32_t last = table.freeList();

        
        MOZ_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan() && last != slot);

        



        if (JSSLOT_FREE(getClass()) <= slot) {
            MOZ_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan());
            setSlot(slot, PrivateUint32Value(last));
            table.setFreeList(slot);
            return;
        }
    }
    setSlot(slot, UndefinedValue());
}

Shape*
NativeObject::addDataProperty(ExclusiveContext* cx, jsid idArg, uint32_t slot, unsigned attrs)
{
    MOZ_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
    RootedNativeObject self(cx, this);
    RootedId id(cx, idArg);
    return addProperty(cx, self, id, nullptr, nullptr, slot, attrs, 0);
}

Shape*
NativeObject::addDataProperty(ExclusiveContext* cx, HandlePropertyName name,
                          uint32_t slot, unsigned attrs)
{
    MOZ_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
    RootedNativeObject self(cx, this);
    RootedId id(cx, NameToId(name));
    return addProperty(cx, self, id, nullptr, nullptr, slot, attrs, 0);
}

template <AllowGC allowGC>
bool
js::NativeLookupOwnProperty(ExclusiveContext* cx,
                            typename MaybeRooted<NativeObject*, allowGC>::HandleType obj,
                            typename MaybeRooted<jsid, allowGC>::HandleType id,
                            typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp)
{
    bool done;
    return LookupOwnPropertyInline<allowGC>(cx, obj, id, propp, &done);
}

template bool
js::NativeLookupOwnProperty<CanGC>(ExclusiveContext* cx, HandleNativeObject obj, HandleId id,
                                   MutableHandleShape propp);

template bool
js::NativeLookupOwnProperty<NoGC>(ExclusiveContext* cx, NativeObject* obj, jsid id,
                                  FakeMutableHandle<Shape*> propp);

template <AllowGC allowGC>
bool
js::NativeLookupProperty(ExclusiveContext* cx,
                         typename MaybeRooted<NativeObject*, allowGC>::HandleType obj,
                         typename MaybeRooted<jsid, allowGC>::HandleType id,
                         typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
                         typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp)
{
    return LookupPropertyInline<allowGC>(cx, obj, id, objp, propp);
}

template bool
js::NativeLookupProperty<CanGC>(ExclusiveContext* cx, HandleNativeObject obj, HandleId id,
                                MutableHandleObject objp, MutableHandleShape propp);

template bool
js::NativeLookupProperty<NoGC>(ExclusiveContext* cx, NativeObject* obj, jsid id,
                               FakeMutableHandle<JSObject*> objp,
                               FakeMutableHandle<Shape*> propp);

bool
js::NativeLookupElement(JSContext* cx, HandleNativeObject obj, uint32_t index,
                        MutableHandleObject objp, MutableHandleShape propp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;

    return LookupPropertyInline<CanGC>(cx, obj, id, objp, propp);
}




static inline bool
CallAddPropertyHook(ExclusiveContext* cx, HandleNativeObject obj, HandleShape shape,
                    HandleValue value)
{
    if (JSAddPropertyOp addProperty = obj->getClass()->addProperty) {
        if (!cx->shouldBeJSContext())
            return false;

        RootedId id(cx, shape->propid());
        if (!CallJSAddPropertyOp(cx->asJSContext(), addProperty, obj, id, value)) {
            obj->removeProperty(cx, shape->propid());
            return false;
        }
    }
    return true;
}

static inline bool
CallAddPropertyHookDense(ExclusiveContext* cx, HandleNativeObject obj, uint32_t index,
                         HandleValue value)
{
    
    if (obj->is<ArrayObject>()) {
        ArrayObject* arr = &obj->as<ArrayObject>();
        uint32_t length = arr->length();
        if (index >= length)
            arr->setLength(cx, index + 1);
        return true;
    }

    if (JSAddPropertyOp addProperty = obj->getClass()->addProperty) {
        if (!cx->shouldBeJSContext())
            return false;

        if (!obj->maybeCopyElementsForWrite(cx))
            return false;

        RootedId id(cx, INT_TO_JSID(index));
        if (!CallJSAddPropertyOp(cx->asJSContext(), addProperty, obj, id, value)) {
            obj->setDenseElementHole(cx, index);
            return false;
        }
    }
    return true;
}

static bool
UpdateShapeTypeAndValue(ExclusiveContext* cx, NativeObject* obj, Shape* shape, const Value& value)
{
    jsid id = shape->propid();
    if (shape->hasSlot()) {
        obj->setSlotWithType(cx, shape, value,  false);

        
        
        
        if (TypeNewScript* newScript = obj->groupRaw()->newScript()) {
            if (newScript->initializedShape() == shape)
                obj->setGroup(newScript->initializedGroup());
        }
    }
    if (!shape->hasSlot() || !shape->hasDefaultGetter() || !shape->hasDefaultSetter())
        MarkTypePropertyNonData(cx, obj, id);
    if (!shape->writable())
        MarkTypePropertyNonWritable(cx, obj, id);
    return true;
}

static unsigned
ApplyOrDefaultAttributes(unsigned attrs, const Shape* shape = nullptr)
{
    bool enumerable = shape ? shape->enumerable() : false;
    bool writable = shape ? shape->writable() : false;
    bool configurable = shape ? shape->configurable() : false;
    return ApplyAttributes(attrs, enumerable, writable, configurable);
}

static bool
PurgeProtoChain(ExclusiveContext* cx, JSObject* objArg, HandleId id)
{
    
    RootedObject obj(cx, objArg);

    RootedShape shape(cx);
    while (obj) {
        
        if (!obj->isNative())
            break;

        shape = obj->as<NativeObject>().lookup(cx, id);
        if (shape)
            return obj->as<NativeObject>().shadowingShapeChange(cx, *shape);

        obj = obj->getProto();
    }

    return true;
}

static bool
PurgeScopeChainHelper(ExclusiveContext* cx, HandleObject objArg, HandleId id)
{
    
    RootedObject obj(cx, objArg);

    MOZ_ASSERT(obj->isNative());
    MOZ_ASSERT(obj->isDelegate());

    
    if (JSID_IS_INT(id))
        return true;

    if (!PurgeProtoChain(cx, obj->getProto(), id))
        return false;

    





    if (obj->is<CallObject>()) {
        while ((obj = obj->enclosingScope()) != nullptr) {
            if (!PurgeProtoChain(cx, obj, id))
                return false;
        }
    }

    return true;
}







static inline bool
PurgeScopeChain(ExclusiveContext* cx, HandleObject obj, HandleId id)
{
    if (obj->isDelegate() && obj->isNative())
        return PurgeScopeChainHelper(cx, obj, id);
    return true;
}





static inline bool
CheckAccessorRedefinition(ExclusiveContext* cx, HandleObject obj, HandleShape shape,
                          Handle<PropertyDescriptor> desc)
{
    MOZ_ASSERT(shape->isAccessorDescriptor());
    if (shape->configurable())
        return true;
    if (desc.getter() == shape->getter() && desc.setter() == shape->setter())
        return true;

    





    if ((desc.attributes() & JSPROP_REDEFINE_NONCONFIGURABLE) &&
        obj->is<GlobalObject>() &&
        !obj->getClass()->isDOMClass())
    {
        return true;
    }

    if (!cx->isJSContext())
        return false;

    return Throw(cx->asJSContext(), shape->propid(), JSMSG_CANT_REDEFINE_PROP);
}

static bool
AddOrChangeProperty(ExclusiveContext *cx, HandleNativeObject obj, HandleId id,
                    Handle<PropertyDescriptor> desc)
{
    desc.assertComplete();

    if (!PurgeScopeChain(cx, obj, id))
        return false;

    
    if (JSID_IS_INT(id) &&
        !desc.getter() &&
        !desc.setter() &&
        desc.attributes() == JSPROP_ENUMERATE &&
        (!obj->isIndexed() || !obj->containsPure(id)) &&
        !IsAnyTypedArray(obj))
    {
        uint32_t index = JSID_TO_INT(id);
        NativeObject::EnsureDenseResult edResult = obj->ensureDenseElements(cx, index, 1);
        if (edResult == NativeObject::ED_FAILED)
            return false;
        if (edResult == NativeObject::ED_OK) {
            obj->setDenseElementWithType(cx, index, desc.value());
            if (!CallAddPropertyHookDense(cx, obj, index, desc.value()))
                return false;
            return true;
        }
    }

    RootedShape shape(cx, NativeObject::putProperty(cx, obj, id, desc.getter(), desc.setter(),
                                                    SHAPE_INVALID_SLOT, desc.attributes(), 0));
    if (!shape)
        return false;

    if (!UpdateShapeTypeAndValue(cx, obj, shape, desc.value()))
        return false;

    
    
    if (JSID_IS_INT(id)) {
        if (!obj->maybeCopyElementsForWrite(cx))
            return false;

        uint32_t index = JSID_TO_INT(id);
        NativeObject::removeDenseElementForSparseIndex(cx, obj, index);
        NativeObject::EnsureDenseResult edResult =
            NativeObject::maybeDensifySparseElements(cx, obj);
        if (edResult == NativeObject::ED_FAILED)
            return false;
        if (edResult == NativeObject::ED_OK) {
            MOZ_ASSERT(!desc.setter());
            return CallAddPropertyHookDense(cx, obj, index, desc.value());
        }
    }

    return CallAddPropertyHook(cx, obj, shape, desc.value());
}

bool
js::NativeDefineProperty(ExclusiveContext* cx, HandleNativeObject obj, HandleId id,
                         Handle<PropertyDescriptor> desc_,
                         ObjectOpResult& result)
{
    desc_.assertValid();

    
    
    
    
    
    

    
    if (obj->is<ArrayObject>()) {
        
        Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
        if (id == NameToId(cx->names().length)) {
            if (!cx->shouldBeJSContext())
                return false;
            return ArraySetLength(cx->asJSContext(), arr, id, desc_.attributes(), desc_.value(),
                                  result);
        }

        
        uint32_t index;
        if (IdIsIndex(id, &index)) {
            if (WouldDefinePastNonwritableLength(obj, index))
                return result.fail(JSMSG_CANT_DEFINE_PAST_ARRAY_LENGTH);
        }
    } else if (IsAnyTypedArray(obj)) {
        
        uint64_t index;
        if (IsTypedArrayIndex(id, &index))
            return result.succeed();
    }

    Rooted<PropertyDescriptor> desc(cx, desc_);

    RootedShape shape(cx);

    
    
    
    if (desc.isAccessorDescriptor()) {
        if (!NativeLookupOwnProperty<CanGC>(cx, obj, id, &shape))
            return false;
        if (shape) {
            
            
            if (IsImplicitDenseOrTypedArrayElement(shape)) {
                if (IsAnyTypedArray(obj)) {
                    
                    return result.succeed();
                }
                if (!NativeObject::sparsifyDenseElement(cx, obj, JSID_TO_INT(id)))
                    return false;
                shape = obj->lookup(cx, id);
            }
            if (shape->isAccessorDescriptor()) {
                if (!CheckAccessorRedefinition(cx, obj, shape, desc))
                    return false;

                desc.setAttributes(ApplyOrDefaultAttributes(desc.attributes(), shape));
                if (!desc.hasGetterObject())
                    desc.setGetter(shape->getter());
                if (!desc.hasSetterObject())
                    desc.setSetter(shape->setter());
                desc.attributesRef() |= JSPROP_GETTER | JSPROP_SETTER;
                desc.assertComplete();

                shape = NativeObject::changeProperty(cx, obj, shape, desc.attributes(),
                                                     desc.getter(), desc.setter());
                if (!shape)
                    return false;
                if (!PurgeScopeChain(cx, obj, id))
                    return false;

                JS_ALWAYS_TRUE(UpdateShapeTypeAndValue(cx, obj, shape, desc.value()));
                if (!CallAddPropertyHook(cx, obj, shape, desc.value()))
                    return false;
                return result.succeed();
            }
        }

        
        
        
        desc.attributesRef() |= JSPROP_GETTER | JSPROP_SETTER;
    } else if (desc.hasValue()) {
        
        
        
        
        
        
        
        
        
        
        
        
        NativeLookupOwnPropertyNoResolve(cx, obj, id, &shape);

        if (shape) {
            
            
            if (IsImplicitDenseOrTypedArrayElement(shape)) {
                desc.setAttributes(ApplyAttributes(desc.attributes(), true, true,
                                                   !IsAnyTypedArray(obj)));
            } else {
                desc.setAttributes(ApplyOrDefaultAttributes(desc.attributes(), shape));

                
                if (shape->isAccessorDescriptor()) {
                    if (!CheckAccessorRedefinition(cx, obj, shape, desc))
                        return false;
                }
            }
        }
    } else {
        
        if (!NativeLookupOwnProperty<CanGC>(cx, obj, id, &shape))
            return false;

        if (shape) {
            
            if (IsImplicitDenseOrTypedArrayElement(shape)) {
                if (IsAnyTypedArray(obj)) {
                    
                    
                    
                    return result.succeed();
                }
                if (!NativeObject::sparsifyDenseElement(cx, obj, JSID_TO_INT(id)))
                    return false;
                shape = obj->lookup(cx, id);
            }

            if (shape->isAccessorDescriptor() &&
                !CheckAccessorRedefinition(cx, obj, shape, desc))
            {
                return false;
            }

            desc.setAttributes(ApplyOrDefaultAttributes(desc.attributes(), shape));

            if (shape->isAccessorDescriptor() && desc.hasWritable()) {
                
                
                
                desc.value().setUndefined();
            } else {
                
                
                
                uint32_t propMask = JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT;
                desc.setAttributes((shape->attributes() & ~propMask) |
                                   (desc.attributes() & propMask));
                desc.setGetter(shape->getter());
                desc.setSetter(shape->setter());
                if (shape->hasSlot())
                    desc.value().set(obj->getSlot(shape->slot()));
            }
        }
    }

    
    
    
    
    desc.setAttributes(ApplyOrDefaultAttributes(desc.attributes()) & ~JSPROP_IGNORE_VALUE);

    
    
    if (!AddOrChangeProperty(cx, obj, id, desc))
        return false;
    return result.succeed();
}

bool
js::NativeDefineProperty(ExclusiveContext* cx, HandleNativeObject obj, HandleId id,
                         HandleValue value, GetterOp getter, SetterOp setter, unsigned attrs,
                         ObjectOpResult& result)
{
    Rooted<PropertyDescriptor> desc(cx);
    desc.initFields(obj, value, attrs, getter, setter);
    return NativeDefineProperty(cx, obj, id, desc, result);
}

bool
js::NativeDefineProperty(ExclusiveContext* cx, HandleNativeObject obj, PropertyName* name,
                         HandleValue value, GetterOp getter, SetterOp setter, unsigned attrs,
                         ObjectOpResult& result)
{
    RootedId id(cx, NameToId(name));
    return NativeDefineProperty(cx, obj, id, value, getter, setter, attrs, result);
}

bool
js::NativeDefineElement(ExclusiveContext* cx, HandleNativeObject obj, uint32_t index,
                        HandleValue value, GetterOp getter, SetterOp setter, unsigned attrs,
                        ObjectOpResult& result)
{
    RootedId id(cx);
    if (index <= JSID_INT_MAX) {
        id = INT_TO_JSID(index);
        return NativeDefineProperty(cx, obj, id, value, getter, setter, attrs, result);
    }

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    if (!IndexToId(cx, index, &id))
        return false;

    return NativeDefineProperty(cx, obj, id, value, getter, setter, attrs, result);
}

bool
js::NativeDefineProperty(ExclusiveContext* cx, HandleNativeObject obj, HandleId id,
                         HandleValue value, JSGetterOp getter, JSSetterOp setter,
                         unsigned attrs)
{
    ObjectOpResult result;
    if (!NativeDefineProperty(cx, obj, id, value, getter, setter, attrs, result))
        return false;
    if (!result) {
        
        
        
        if (!cx->shouldBeJSContext())
            return false;
        result.reportError(cx->asJSContext(), obj, id);
        return false;
    }
    return true;
}

bool
js::NativeDefineProperty(ExclusiveContext* cx, HandleNativeObject obj, PropertyName* name,
                         HandleValue value, JSGetterOp getter, JSSetterOp setter,
                         unsigned attrs)
{
    RootedId id(cx, NameToId(name));
    return NativeDefineProperty(cx, obj, id, value, getter, setter, attrs);
}





bool
js::NativeHasProperty(JSContext* cx, HandleNativeObject obj, HandleId id, bool* foundp)
{
    RootedNativeObject pobj(cx, obj);
    RootedShape shape(cx);

    
    
    for (;;) {
        
        bool done;
        if (!LookupOwnPropertyInline<CanGC>(cx, pobj, id, &shape, &done))
            return false;

        
        if (shape) {
            *foundp = true;
            return true;
        }

        
        
        
        
        
        
        
        
        
        RootedObject proto(cx, done ? nullptr : pobj->getProto());

        
        if (!proto) {
            *foundp = false;
            return true;
        }

        
        
        
        
        
        if (!proto->isNative())
            return HasProperty(cx, proto, id, foundp);

        pobj = &proto->as<NativeObject>();
    }
}




static inline bool
CallGetter(JSContext* cx, HandleObject receiver, HandleShape shape, MutableHandleValue vp)
{
    MOZ_ASSERT(!shape->hasDefaultGetter());

    if (shape->hasGetterValue()) {
        Value fval = shape->getterValue();
        return InvokeGetter(cx, receiver, fval, vp);
    }

    RootedId id(cx, shape->propid());
    return CallJSGetterOp(cx, shape->getterOp(), receiver, id, vp);
}

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE bool
GetExistingProperty(JSContext* cx,
                    typename MaybeRooted<JSObject*, allowGC>::HandleType receiver,
                    typename MaybeRooted<NativeObject*, allowGC>::HandleType obj,
                    typename MaybeRooted<Shape*, allowGC>::HandleType shape,
                    typename MaybeRooted<Value, allowGC>::MutableHandleType vp)
{
    if (shape->hasSlot()) {
        vp.set(obj->getSlot(shape->slot()));
        MOZ_ASSERT_IF(!vp.isMagic(JS_UNINITIALIZED_LEXICAL) &&
                      !obj->isSingleton() &&
                      !obj->template is<ScopeObject>() &&
                      shape->hasDefaultGetter(),
                      ObjectGroupHasProperty(cx, obj->group(), shape->propid(), vp));
    } else {
        vp.setUndefined();
    }
    if (shape->hasDefaultGetter())
        return true;

    {
        jsbytecode* pc;
        JSScript* script = cx->currentScript(&pc);
        if (script && script->hasBaselineScript()) {
            switch (JSOp(*pc)) {
              case JSOP_GETPROP:
              case JSOP_CALLPROP:
              case JSOP_LENGTH:
                script->baselineScript()->noteAccessedGetter(script->pcToOffset(pc));
                break;
              default:
                break;
            }
        }
    }

    if (!allowGC)
        return false;

    if (!CallGetter(cx,
                    MaybeRooted<JSObject*, allowGC>::toHandle(receiver),
                    MaybeRooted<Shape*, allowGC>::toHandle(shape),
                    MaybeRooted<Value, allowGC>::toMutableHandle(vp)))
    {
        return false;
    }

    
    
    
    if (shape->hasSlot() && obj->contains(cx, shape))
        obj->setSlot(shape->slot(), vp);

    return true;
}

bool
js::NativeGetExistingProperty(JSContext* cx, HandleObject receiver, HandleNativeObject obj,
                              HandleShape shape, MutableHandleValue vp)
{
    return GetExistingProperty<CanGC>(cx, receiver, obj, shape, vp);
}






static bool
Detecting(JSContext* cx, JSScript* script, jsbytecode* pc)
{
    MOZ_ASSERT(script->containsPC(pc));

    
    JSOp op = JSOp(*pc);
    if (js_CodeSpec[op].format & JOF_DETECTING)
        return true;

    jsbytecode* endpc = script->codeEnd();

    if (op == JSOP_NULL) {
        
        if (++pc < endpc) {
            op = JSOp(*pc);
            return op == JSOP_EQ || op == JSOP_NE;
        }
        return false;
    }

    if (op == JSOP_GETGNAME || op == JSOP_GETNAME) {
        
        JSAtom* atom = script->getAtom(GET_UINT32_INDEX(pc));
        if (atom == cx->names().undefined &&
            (pc += js_CodeSpec[op].length) < endpc) {
            op = JSOp(*pc);
            return op == JSOP_EQ || op == JSOP_NE || op == JSOP_STRICTEQ || op == JSOP_STRICTNE;
        }
    }

    return false;
}

enum IsNameLookup { NotNameLookup = false, NameLookup = true };


















static bool
GetNonexistentProperty(JSContext* cx, HandleNativeObject obj, HandleId id,
                       HandleObject receiver, IsNameLookup nameLookup, MutableHandleValue vp)
{
    vp.setUndefined();

    
    
    
    if (JSGetterOp getProperty = obj->getClass()->getProperty) {
        if (!CallJSGetterOp(cx, getProperty, obj, id, vp))
            return false;

        if (!vp.isUndefined())
            return true;
    }

    
    if (nameLookup)
        return ReportIsNotDefined(cx, id);

    
    
    
    
    
    if (!cx->compartment()->options().extraWarnings(cx))
        return true;

    jsbytecode* pc;
    RootedScript script(cx, cx->currentScript(&pc));
    if (!script)
        return true;

    if (*pc != JSOP_GETPROP && *pc != JSOP_GETELEM)
        return true;

    
    if (script->warnedAboutUndefinedProp())
        return true;

    
    
    
    if (script->selfHosted())
        return true;

    
    if (JSID_IS_ATOM(id, cx->names().iteratorIntrinsic))
        return true;

    
    pc += js_CodeSpec[*pc].length;
    if (Detecting(cx, script, pc))
        return true;

    unsigned flags = JSREPORT_WARNING | JSREPORT_STRICT;
    script->setWarnedAboutUndefinedProp();

    
    RootedValue val(cx, IdToValue(id));
    return ReportValueErrorFlags(cx, flags, JSMSG_UNDEFINED_PROP, JSDVG_IGNORE_STACK, val,
                                    js::NullPtr(), nullptr, nullptr);
}


bool
GetNonexistentProperty(JSContext* cx, NativeObject* obj, jsid id, JSObject* receiver,
                       IsNameLookup nameLookup, FakeMutableHandle<Value> vp)
{
    return false;
}

static inline bool
GeneralizedGetProperty(JSContext* cx, HandleObject obj, HandleId id, HandleObject receiver,
                       IsNameLookup nameLookup, MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);
    if (nameLookup) {
        
        
        
        
        
        
        
        

        bool found;
        if (!HasProperty(cx, obj, id, &found))
            return false;
        if (!found)
            return ReportIsNotDefined(cx, id);
    }

    return GetProperty(cx, obj, receiver, id, vp);
}

static inline bool
GeneralizedGetProperty(JSContext* cx, JSObject* obj, jsid id, JSObject* receiver,
                       IsNameLookup nameLookup, FakeMutableHandle<Value> vp)
{
    JS_CHECK_RECURSION_DONT_REPORT(cx, return false);
    if (nameLookup)
        return false;
    return GetPropertyNoGC(cx, obj, receiver, id, vp.address());
}

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE bool
NativeGetPropertyInline(JSContext* cx,
                        typename MaybeRooted<NativeObject*, allowGC>::HandleType obj,
                        typename MaybeRooted<JSObject*, allowGC>::HandleType receiver,
                        typename MaybeRooted<jsid, allowGC>::HandleType id,
                        IsNameLookup nameLookup,
                        typename MaybeRooted<Value, allowGC>::MutableHandleType vp)
{
    typename MaybeRooted<NativeObject*, allowGC>::RootType pobj(cx, obj);
    typename MaybeRooted<Shape*, allowGC>::RootType shape(cx);

    
    
    for (;;) {
        
        bool done;
        if (!LookupOwnPropertyInline<allowGC>(cx, pobj, id, &shape, &done))
            return false;

        if (shape) {
            
            
            if (IsImplicitDenseOrTypedArrayElement(shape)) {
                vp.set(pobj->getDenseOrTypedArrayElement(JSID_TO_INT(id)));
                return true;
            }
            return GetExistingProperty<allowGC>(cx, receiver, pobj, shape, vp);
        }

        
        
        
        
        
        
        
        
        RootedObject proto(cx, done ? nullptr : pobj->getProto());

        
        
        if (!proto)
            return GetNonexistentProperty(cx, obj, id, receiver, nameLookup, vp);

        
        
        
        
        
        if (proto->getOps()->getProperty)
            return GeneralizedGetProperty(cx, proto, id, receiver, nameLookup, vp);

        pobj = &proto->as<NativeObject>();
    }
}

bool
js::NativeGetProperty(JSContext* cx, HandleNativeObject obj, HandleObject receiver, HandleId id,
                      MutableHandleValue vp)
{
    return NativeGetPropertyInline<CanGC>(cx, obj, receiver, id, NotNameLookup, vp);
}

bool
js::NativeGetPropertyNoGC(JSContext* cx, NativeObject* obj, JSObject* receiver, jsid id, Value* vp)
{
    AutoAssertNoException noexc(cx);
    return NativeGetPropertyInline<NoGC>(cx, obj, receiver, id, NotNameLookup, vp);
}

bool
js::GetPropertyForNameLookup(JSContext* cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    if (obj->getOps()->getProperty)
        return GeneralizedGetProperty(cx, obj, id, obj, NameLookup, vp);
    return NativeGetPropertyInline<CanGC>(cx, obj.as<NativeObject>(), obj, id, NameLookup, vp);
}




static bool
MaybeReportUndeclaredVarAssignment(JSContext* cx, JSString* propname)
{
    {
        jsbytecode* pc;
        JSScript* script = cx->currentScript(&pc, JSContext::ALLOW_CROSS_COMPARTMENT);
        if (!script)
            return true;

        
        
        if (!IsStrictSetPC(pc) && !cx->compartment()->options().extraWarnings(cx))
            return true;
    }

    JSAutoByteString bytes(cx, propname);
    return !!bytes &&
           JS_ReportErrorFlagsAndNumber(cx,
                                        (JSREPORT_WARNING | JSREPORT_STRICT
                                         | JSREPORT_STRICT_MODE_ERROR),
                                        GetErrorMessage, nullptr,
                                        JSMSG_UNDECLARED_VAR, bytes.ptr());
}





static bool
NativeSetExistingDataProperty(JSContext* cx, HandleNativeObject obj, HandleShape shape,
                              HandleValue v, HandleValue receiver, ObjectOpResult& result)
{
    MOZ_ASSERT(obj->isNative());
    MOZ_ASSERT(shape->isDataDescriptor());

    if (shape->hasDefaultSetter()) {
        if (shape->hasSlot()) {
            

            
            
            
            bool overwriting = !obj->is<GlobalObject>() || !obj->getSlot(shape->slot()).isUndefined();
            obj->setSlotWithType(cx, shape, v, overwriting);
            return result.succeed();
        }

        
        
        
        return result.fail(JSMSG_GETTER_ONLY);
    }

    MOZ_ASSERT(!obj->is<DynamicWithObject>());  

    uint32_t sample = cx->runtime()->propertyRemovals;
    RootedId id(cx, shape->propid());
    RootedValue value(cx, v);
    if (!CallJSSetterOp(cx, shape->setterOp(), obj, id, &value, result))
        return false;

    
    
    if (shape->hasSlot() &&
        (MOZ_LIKELY(cx->runtime()->propertyRemovals == sample) ||
         obj->contains(cx, shape)))
    {
        obj->setSlot(shape->slot(), value);
    }

    return true;  
}









bool
js::SetPropertyByDefining(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                          HandleValue receiverValue, bool objHasOwn, ObjectOpResult& result)
{
    
    if (!receiverValue.isObject())
        return result.fail(JSMSG_SET_NON_OBJECT_RECEIVER);
    RootedObject receiver(cx, &receiverValue.toObject());

    
    
    
    
    bool existing;
    if (receiver == obj) {
        
        
#ifdef DEBUG
        
        
        
        if (!HasOwnProperty(cx, receiver, id, &existing))
            return false;
        MOZ_ASSERT(existing == objHasOwn);
#endif
        existing = objHasOwn;
    } else {
        if (!HasOwnProperty(cx, receiver, id, &existing))
            return false;
    }

    
    
    
    if (!existing) {
        bool extensible;
        if (!IsExtensible(cx, receiver, &extensible))
            return false;
        if (!extensible)
            return result.fail(JSMSG_OBJECT_NOT_EXTENSIBLE);
    }

    
    const Class* clasp = receiver->getClass();

    
    if (!PurgeScopeChain(cx, receiver, id))
        return false;

    
    unsigned attrs =
        existing
        ? JSPROP_IGNORE_ENUMERATE | JSPROP_IGNORE_READONLY | JSPROP_IGNORE_PERMANENT
        : JSPROP_ENUMERATE;
    JSGetterOp getter = clasp->getProperty;
    JSSetterOp setter = clasp->setProperty;
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);
    if (!DefineProperty(cx, receiver, id, v, getter, setter, attrs, result))
        return false;

    
    
    if (setter && receiver->is<NativeObject>()) {
        if (!result)
            return true;

        Rooted<NativeObject*> nativeReceiver(cx, &receiver->as<NativeObject>());
        if (!cx->shouldBeJSContext())
            return false;
        RootedValue receiverValue(cx, ObjectValue(*receiver));

        
        
        
        
        
        RootedShape shape(cx, nativeReceiver->lookup(cx, id));
        MOZ_ASSERT(shape);
        return NativeSetExistingDataProperty(cx->asJSContext(), nativeReceiver, shape, v,
                                             receiverValue, result);
    }

    return true;
}



bool
js::SetPropertyOnProto(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                       HandleValue receiver, ObjectOpResult& result)
{
    MOZ_ASSERT(!obj->is<ProxyObject>());

    RootedObject proto(cx, obj->getProto());
    if (proto)
        return SetProperty(cx, proto, id, v, receiver, result);
    return SetPropertyByDefining(cx, obj, id, v, receiver, false, result);
}








static bool
SetNonexistentProperty(JSContext* cx, HandleNativeObject obj, HandleId id, HandleValue v,
                       HandleValue receiver, QualifiedBool qualified, ObjectOpResult& result)
{
    
    MOZ_ASSERT_IF(receiver.isObject(), !receiver.toObject().is<BlockObject>());

    if (!qualified && receiver.isObject() && receiver.toObject().isUnqualifiedVarObj()) {
        if (!MaybeReportUndeclaredVarAssignment(cx, JSID_TO_STRING(id)))
            return false;
    }

    return SetPropertyByDefining(cx, obj, id, v, receiver, false, result);
}





static bool
SetDenseOrTypedArrayElement(JSContext* cx, HandleNativeObject obj, uint32_t index, HandleValue v,
                            ObjectOpResult& result)
{
    if (IsAnyTypedArray(obj)) {
        double d;
        if (!ToNumber(cx, v, &d))
            return false;

        
        
        
        uint32_t len = AnyTypedArrayLength(obj);
        if (index < len) {
            if (obj->is<TypedArrayObject>())
                TypedArrayObject::setElement(obj->as<TypedArrayObject>(), index, d);
            else
                SharedTypedArrayObject::setElement(obj->as<SharedTypedArrayObject>(), index, d);
        }
        return result.succeed();
    }

    if (WouldDefinePastNonwritableLength(obj, index))
        return result.fail(JSMSG_CANT_DEFINE_PAST_ARRAY_LENGTH);

    if (!obj->maybeCopyElementsForWrite(cx))
        return false;

    obj->setDenseElementWithType(cx, index, v);
    return result.succeed();
}









static bool
SetExistingProperty(JSContext* cx, HandleNativeObject obj, HandleId id, HandleValue v,
                    HandleValue receiver, HandleNativeObject pobj, HandleShape shape,
                    ObjectOpResult& result)
{
    
    if (IsImplicitDenseOrTypedArrayElement(shape)) {
        

        
        if (receiver.isObject() && pobj == &receiver.toObject())
            return SetDenseOrTypedArrayElement(cx, pobj, JSID_TO_INT(id), v, result);

        
        return SetPropertyByDefining(cx, obj, id, v, receiver, obj == pobj, result);
    }

    
    if (shape->isDataDescriptor()) {
        
        if (!shape->writable())
            return result.fail(JSMSG_READ_ONLY);

        
        if (receiver.isObject() && pobj == &receiver.toObject()) {
            
            
            

            
            if (pobj->is<ArrayObject>() && id == NameToId(cx->names().length)) {
                Rooted<ArrayObject*> arr(cx, &pobj->as<ArrayObject>());
                return ArraySetLength(cx, arr, id, shape->attributes(), v, result);
            }
            return NativeSetExistingDataProperty(cx, obj, shape, v, receiver, result);
        }

        
        
        
        
        if (!shape->hasSlot() &&
            !shape->hasShadowable() &&
            !(pobj->is<ArrayObject>() && id == NameToId(cx->names().length)))
        {
            
            
            if (shape->hasDefaultSetter())
                return result.succeed();

            RootedValue valCopy(cx, v);
            return CallJSSetterOp(cx, shape->setterOp(), obj, id, &valCopy, result);
        }

        
        
        return SetPropertyByDefining(cx, obj, id, v, receiver, obj == pobj, result);
    }

    
    MOZ_ASSERT(shape->isAccessorDescriptor());
    MOZ_ASSERT_IF(!shape->hasSetterObject(), shape->hasDefaultSetter());
    if (shape->hasDefaultSetter())
        return result.fail(JSMSG_GETTER_ONLY);
    Value setter = ObjectValue(*shape->setterObject());
    if (!InvokeSetter(cx, receiver, setter, v))
        return false;
    return result.succeed();
}

bool
js::NativeSetProperty(JSContext* cx, HandleNativeObject obj, HandleId id, HandleValue value,
                      HandleValue receiver, QualifiedBool qualified, ObjectOpResult& result)
{
    
    RootedValue v(cx, value);
    if (MOZ_UNLIKELY(obj->watched())) {
        WatchpointMap* wpmap = cx->compartment()->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, obj, id, &v))
            return false;
    }

    
    
    
    RootedShape shape(cx);
    RootedNativeObject pobj(cx, obj);

    
    
    
    for (;;) {
        
        bool done;
        if (!LookupOwnPropertyInline<CanGC>(cx, pobj, id, &shape, &done))
            return false;

        if (shape) {
            
            return SetExistingProperty(cx, obj, id, v, receiver, pobj, shape, result);
        }

        
        
        
        
        
        
        
        
        RootedObject proto(cx, done ? nullptr : pobj->getProto());
        if (!proto) {
            
            return SetNonexistentProperty(cx, obj, id, v, receiver, qualified, result);
        }

        
        
        
        
        
        if (!proto->isNative()) {
            
            
            
            if (!qualified) {
                bool found;
                if (!HasProperty(cx, proto, id, &found))
                    return false;
                if (!found)
                    return SetNonexistentProperty(cx, obj, id, v, receiver, qualified, result);
            }

            return SetProperty(cx, proto, id, v, receiver, result);
        }
        pobj = &proto->as<NativeObject>();
    }
}

bool
js::NativeSetElement(JSContext* cx, HandleNativeObject obj, uint32_t index, HandleValue v,
                     HandleValue receiver, ObjectOpResult& result)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return NativeSetProperty(cx, obj, id, v, receiver, Qualified, result);
}




bool
js::NativeDeleteProperty(JSContext* cx, HandleNativeObject obj, HandleId id,
                         ObjectOpResult& result)
{
    
    RootedShape shape(cx);
    if (!NativeLookupOwnProperty<CanGC>(cx, obj, id, &shape))
        return false;

    
    if (!shape) {
        
        
        return CallJSDeletePropertyOp(cx, obj->getClass()->delProperty, obj, id, result);
    }

    cx->runtime()->gc.poke();

    
    if (GetShapeAttributes(obj, shape) & JSPROP_PERMANENT)
        return result.failCantDelete();

    if (!CallJSDeletePropertyOp(cx, obj->getClass()->delProperty, obj, id, result))
        return false;
    if (!result)
        return true;

    
    if (IsImplicitDenseOrTypedArrayElement(shape)) {
        
        MOZ_ASSERT(!IsAnyTypedArray(obj));

        if (!obj->maybeCopyElementsForWrite(cx))
            return false;

        obj->setDenseElementHole(cx, JSID_TO_INT(id));
    } else {
        if (!obj->removeProperty(cx, id))
            return false;
    }

    return SuppressDeletedProperty(cx, obj, id);
}
