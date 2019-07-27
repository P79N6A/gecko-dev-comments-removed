





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

PropDesc::PropDesc()
{
    setUndefined();
}

void
PropDesc::setUndefined()
{
    value_ = UndefinedValue();
    get_ = UndefinedValue();
    set_ = UndefinedValue();
    attrs = 0;
    hasGet_ = false;
    hasSet_ = false;
    hasValue_ = false;
    hasWritable_ = false;
    hasEnumerable_ = false;
    hasConfigurable_ = false;

    isUndefined_ = true;
}

bool
PropDesc::checkGetter(JSContext *cx)
{
    if (hasGet_) {
        if (!IsCallable(get_) && !get_.isUndefined()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_GET_SET_FIELD,
                                 js_getter_str);
            return false;
        }
    }
    return true;
}

bool
PropDesc::checkSetter(JSContext *cx)
{
    if (hasSet_) {
        if (!IsCallable(set_) && !set_.isUndefined()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_GET_SET_FIELD,
                                 js_setter_str);
            return false;
        }
    }
    return true;
}

static const ObjectElements emptyElementsHeader(0, 0);


HeapSlot *const js::emptyObjectElements =
    reinterpret_cast<HeapSlot *>(uintptr_t(&emptyElementsHeader) + sizeof(ObjectElements));

#ifdef DEBUG

bool
NativeObject::canHaveNonEmptyElements()
{
    return !IsAnyTypedArray(this);
}

#endif 

 bool
ObjectElements::ConvertElementsToDoubles(JSContext *cx, uintptr_t elementsPtr)
{
    




    HeapSlot *elementsHeapPtr = (HeapSlot *) elementsPtr;
    MOZ_ASSERT(elementsHeapPtr != emptyObjectElements);

    ObjectElements *header = ObjectElements::fromElements(elementsHeapPtr);
    MOZ_ASSERT(!header->shouldConvertDoubleElements());

    
    
    Value *vp = (Value *) elementsPtr;
    for (size_t i = 0; i < header->initializedLength; i++) {
        if (vp[i].isInt32())
            vp[i].setDouble(vp[i].toInt32());
    }

    header->setShouldConvertDoubleElements();
    return true;
}

 bool
ObjectElements::MakeElementsCopyOnWrite(ExclusiveContext *cx, NativeObject *obj)
{
    static_assert(sizeof(HeapSlot) >= sizeof(HeapPtrObject),
                  "there must be enough room for the owner object pointer at "
                  "the end of the elements");
    if (!obj->ensureElements(cx, obj->getDenseInitializedLength() + 1))
        return false;

    ObjectElements *header = obj->getElementsHeader();

    
    
    MOZ_ASSERT(!header->isCopyOnWrite());
    header->flags |= COPY_ON_WRITE;

    header->ownerObject().init(obj);
    return true;
}

 bool
JSObject::setImmutablePrototype(ExclusiveContext *cx, HandleObject obj, bool *succeeded)
{
    if (obj->hasLazyPrototype()) {
        if (!cx->shouldBeJSContext())
            return false;
        return Proxy::setImmutablePrototype(cx->asJSContext(), obj, succeeded);
    }

    if (!obj->setFlag(cx, BaseShape::IMMUTABLE_PROTOTYPE))
        return false;
    *succeeded = true;
    return true;
}

#ifdef DEBUG
void
js::NativeObject::checkShapeConsistency()
{
    static int throttle = -1;
    if (throttle < 0) {
        if (const char *var = getenv("JS_CHECK_SHAPE_THROTTLE"))
            throttle = atoi(var);
        if (throttle < 0)
            throttle = 0;
    }
    if (throttle == 0)
        return;

    MOZ_ASSERT(isNative());

    Shape *shape = lastProperty();
    Shape *prev = nullptr;

    if (inDictionaryMode()) {
        MOZ_ASSERT(shape->hasTable());

        ShapeTable &table = shape->table();
        for (uint32_t fslot = table.freelist; fslot != SHAPE_INVALID_SLOT;
             fslot = getSlot(fslot).toPrivateUint32()) {
            MOZ_ASSERT(fslot < slotSpan());
        }

        for (int n = throttle; --n >= 0 && shape->parent; shape = shape->parent) {
            MOZ_ASSERT_IF(lastProperty() != shape, !shape->hasTable());

            Shape **spp = table.search(shape->propid(), false);
            MOZ_ASSERT(SHAPE_FETCH(spp) == shape);
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
                ShapeTable &table = shape->table();
                MOZ_ASSERT(shape->parent);
                for (Shape::Range<NoGC> r(shape); !r.empty(); r.popFront()) {
                    Shape **spp = table.search(r.front().propid(), false);
                    MOZ_ASSERT(SHAPE_FETCH(spp) == &r.front());
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
    



    HeapSlot *fixedStart, *fixedEnd, *slotsStart, *slotsEnd;
    getSlotRangeUnchecked(start, length, &fixedStart, &fixedEnd, &slotsStart, &slotsEnd);

    uint32_t offset = start;
    for (HeapSlot *sp = fixedStart; sp < fixedEnd; sp++)
        sp->init(this, HeapSlot::Slot, offset++, UndefinedValue());
    for (HeapSlot *sp = slotsStart; sp < slotsEnd; sp++)
        sp->init(this, HeapSlot::Slot, offset++, UndefinedValue());
}

void
js::NativeObject::initSlotRange(uint32_t start, const Value *vector, uint32_t length)
{
    HeapSlot *fixedStart, *fixedEnd, *slotsStart, *slotsEnd;
    getSlotRange(start, length, &fixedStart, &fixedEnd, &slotsStart, &slotsEnd);
    for (HeapSlot *sp = fixedStart; sp < fixedEnd; sp++)
        sp->init(this, HeapSlot::Slot, start++, *vector++);
    for (HeapSlot *sp = slotsStart; sp < slotsEnd; sp++)
        sp->init(this, HeapSlot::Slot, start++, *vector++);
}

void
js::NativeObject::copySlotRange(uint32_t start, const Value *vector, uint32_t length)
{
    JS::Zone *zone = this->zone();
    HeapSlot *fixedStart, *fixedEnd, *slotsStart, *slotsEnd;
    getSlotRange(start, length, &fixedStart, &fixedEnd, &slotsStart, &slotsEnd);
    for (HeapSlot *sp = fixedStart; sp < fixedEnd; sp++)
        sp->set(zone, this, HeapSlot::Slot, start++, *vector++);
    for (HeapSlot *sp = slotsStart; sp < slotsEnd; sp++)
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

#if defined(_MSC_VER) && _MSC_VER >= 1500





MOZ_NEVER_INLINE
#endif
Shape *
js::NativeObject::lookup(ExclusiveContext *cx, jsid id)
{
    MOZ_ASSERT(isNative());
    Shape **spp;
    return Shape::search(cx, lastProperty(), id, &spp);
}

Shape *
js::NativeObject::lookupPure(jsid id)
{
    MOZ_ASSERT(isNative());
    return Shape::searchNoHashify(lastProperty(), id);
}

uint32_t
js::NativeObject::dynamicSlotsCount(uint32_t nfixed, uint32_t span, const Class *clasp)
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

void
PropDesc::trace(JSTracer *trc)
{
    gc::MarkValueRoot(trc, &value_, "PropDesc value");
    gc::MarkValueRoot(trc, &get_, "PropDesc get");
    gc::MarkValueRoot(trc, &set_, "PropDesc set");
}

 inline bool
NativeObject::updateSlotsForSpan(ThreadSafeContext *cx,
                                 HandleNativeObject obj, size_t oldSpan, size_t newSpan)
{
    MOZ_ASSERT(cx->isThreadLocal(obj));
    MOZ_ASSERT(oldSpan != newSpan);

    size_t oldCount = dynamicSlotsCount(obj->numFixedSlots(), oldSpan, obj->getClass());
    size_t newCount = dynamicSlotsCount(obj->numFixedSlots(), newSpan, obj->getClass());

    if (oldSpan < newSpan) {
        if (oldCount < newCount && !growSlots(cx, obj, oldCount, newCount))
            return false;

        if (newSpan == oldSpan + 1)
            obj->initSlotUnchecked(oldSpan, UndefinedValue());
        else
            obj->initializeSlotRange(oldSpan, newSpan - oldSpan);
    } else {
        
        obj->prepareSlotRangeForOverwrite(newSpan, oldSpan);
        obj->invalidateSlotRange(newSpan, oldSpan - newSpan);

        if (oldCount > newCount)
            shrinkSlots(cx, obj, oldCount, newCount);
    }

    return true;
}

 bool
NativeObject::setLastProperty(ThreadSafeContext *cx, HandleNativeObject obj, HandleShape shape)
{
    MOZ_ASSERT(cx->isThreadLocal(obj));
    MOZ_ASSERT(!obj->inDictionaryMode());
    MOZ_ASSERT(!shape->inDictionary());
    MOZ_ASSERT(shape->compartment() == obj->compartment());
    MOZ_ASSERT(shape->numFixedSlots() == obj->numFixedSlots());

    size_t oldSpan = obj->lastProperty()->slotSpan();
    size_t newSpan = shape->slotSpan();

    if (oldSpan == newSpan) {
        obj->shape_ = shape;
        return true;
    }

    if (!updateSlotsForSpan(cx, obj, oldSpan, newSpan))
        return false;

    obj->shape_ = shape;
    return true;
}

void
NativeObject::setLastPropertyShrinkFixedSlots(Shape *shape)
{
    MOZ_ASSERT(!inDictionaryMode());
    MOZ_ASSERT(!shape->inDictionary());
    MOZ_ASSERT(shape->compartment() == compartment());
    MOZ_ASSERT(lastProperty()->slotSpan() == shape->slotSpan());

    DebugOnly<size_t> oldFixed = numFixedSlots();
    DebugOnly<size_t> newFixed = shape->numFixedSlots();
    MOZ_ASSERT(newFixed < oldFixed);
    MOZ_ASSERT(shape->slotSpan() <= oldFixed);
    MOZ_ASSERT(shape->slotSpan() <= newFixed);
    MOZ_ASSERT(dynamicSlotsCount(oldFixed, shape->slotSpan(), getClass()) == 0);
    MOZ_ASSERT(dynamicSlotsCount(newFixed, shape->slotSpan(), getClass()) == 0);

    shape_ = shape;
}

 bool
NativeObject::setSlotSpan(ThreadSafeContext *cx, HandleNativeObject obj, uint32_t span)
{
    MOZ_ASSERT(cx->isThreadLocal(obj));
    MOZ_ASSERT(obj->inDictionaryMode());

    size_t oldSpan = obj->lastProperty()->base()->slotSpan();
    if (oldSpan == span)
        return true;

    if (!updateSlotsForSpan(cx, obj, oldSpan, span))
        return false;

    obj->lastProperty()->base()->setSlotSpan(span);
    return true;
}



static HeapSlot *
AllocateSlots(ThreadSafeContext *cx, JSObject *obj, uint32_t nslots)
{
    if (cx->isJSContext())
        return cx->asJSContext()->runtime()->gc.nursery.allocateSlots(obj, nslots);
#ifdef JSGC_FJGENERATIONAL
    if (cx->isForkJoinContext())
        return cx->asForkJoinContext()->nursery().allocateSlots(obj, nslots);
#endif
    return obj->zone()->pod_malloc<HeapSlot>(nslots);
}





static HeapSlot *
ReallocateSlots(ThreadSafeContext *cx, JSObject *obj, HeapSlot *oldSlots,
                uint32_t oldCount, uint32_t newCount)
{
    if (cx->isJSContext()) {
        return cx->asJSContext()->runtime()->gc.nursery.reallocateSlots(obj, oldSlots,
                                                                        oldCount, newCount);
    }
#ifdef JSGC_FJGENERATIONAL
    if (cx->isForkJoinContext()) {
        return cx->asForkJoinContext()->nursery().reallocateSlots(obj, oldSlots,
                                                                  oldCount, newCount);
    }
#endif
    return obj->zone()->pod_realloc<HeapSlot>(oldSlots, oldCount, newCount);
}

 bool
NativeObject::growSlots(ThreadSafeContext *cx, HandleNativeObject obj, uint32_t oldCount, uint32_t newCount)
{
    MOZ_ASSERT(cx->isThreadLocal(obj));
    MOZ_ASSERT(newCount > oldCount);
    MOZ_ASSERT_IF(!obj->is<ArrayObject>(), newCount >= SLOT_CAPACITY_MIN);

    




    NativeObject::slotsSizeMustNotOverflow();
    MOZ_ASSERT(newCount < NELEMENTS_LIMIT);

    if (!oldCount) {
        obj->slots_ = AllocateSlots(cx, obj, newCount);
        if (!obj->slots_)
            return false;
        Debug_SetSlotRangeToCrashOnTouch(obj->slots_, newCount);
        return true;
    }

    HeapSlot *newslots = ReallocateSlots(cx, obj, obj->slots_, oldCount, newCount);
    if (!newslots)
        return false;  

    obj->slots_ = newslots;

    Debug_SetSlotRangeToCrashOnTouch(obj->slots_ + oldCount, newCount - oldCount);

    return true;
}

static void
FreeSlots(ThreadSafeContext *cx, HeapSlot *slots)
{
    
    if (cx->isJSContext())
        return cx->asJSContext()->runtime()->gc.nursery.freeSlots(slots);
#ifdef JSGC_FJGENERATIONAL
    if (cx->isForkJoinContext())
        return cx->asForkJoinContext()->nursery().freeSlots(slots);
#endif
    js_free(slots);
}

 void
NativeObject::shrinkSlots(ThreadSafeContext *cx, HandleNativeObject obj,
                          uint32_t oldCount, uint32_t newCount)
{
    MOZ_ASSERT(cx->isThreadLocal(obj));
    MOZ_ASSERT(newCount < oldCount);

    if (newCount == 0) {
        FreeSlots(cx, obj->slots_);
        obj->slots_ = nullptr;
        return;
    }

    MOZ_ASSERT_IF(!obj->is<ArrayObject>(), newCount >= SLOT_CAPACITY_MIN);

    HeapSlot *newslots = ReallocateSlots(cx, obj, obj->slots_, oldCount, newCount);
    if (!newslots)
        return;  

    obj->slots_ = newslots;
}

 bool
NativeObject::sparsifyDenseElement(ExclusiveContext *cx, HandleNativeObject obj, uint32_t index)
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
NativeObject::sparsifyDenseElements(js::ExclusiveContext *cx, HandleNativeObject obj)
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
    const Value *elems = getDenseElements();
    for (uint32_t i = 0; i < len; i++) {
        if (!elems[i].isMagic(JS_ELEMENTS_HOLE) && !--minimalDenseCount)
            return false;
    }
    return true;
}

 NativeObject::EnsureDenseResult
NativeObject::maybeDensifySparseElements(js::ExclusiveContext *cx, HandleNativeObject obj)
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
        if (js_IdIsIndex(shape->propid(), &index)) {
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
        if (js_IdIsIndex(id, &index)) {
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



static ObjectElements *
AllocateElements(ThreadSafeContext *cx, JSObject *obj, uint32_t nelems)
{
    if (cx->isJSContext())
        return cx->asJSContext()->runtime()->gc.nursery.allocateElements(obj, nelems);
#ifdef JSGC_FJGENERATIONAL
    if (cx->isForkJoinContext())
        return cx->asForkJoinContext()->nursery().allocateElements(obj, nelems);
#endif

    return reinterpret_cast<js::ObjectElements *>(obj->zone()->pod_malloc<HeapSlot>(nelems));
}



static ObjectElements *
ReallocateElements(ThreadSafeContext *cx, JSObject *obj, ObjectElements *oldHeader,
                   uint32_t oldCount, uint32_t newCount)
{
    if (cx->isJSContext()) {
        return cx->asJSContext()->runtime()->gc.nursery.reallocateElements(obj, oldHeader,
                                                                           oldCount, newCount);
    }
#ifdef JSGC_FJGENERATIONAL
    if (cx->isForkJoinContext()) {
        return cx->asForkJoinContext()->nursery().reallocateElements(obj, oldHeader,
                                                                     oldCount, newCount);
    }
#endif

    return reinterpret_cast<js::ObjectElements *>(
            obj->zone()->pod_realloc<HeapSlot>(reinterpret_cast<HeapSlot *>(oldHeader),
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
NativeObject::growElements(ThreadSafeContext *cx, uint32_t reqCapacity)
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

    ObjectElements *newheader;
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
NativeObject::shrinkElements(ThreadSafeContext *cx, uint32_t reqCapacity)
{
    MOZ_ASSERT(cx->isThreadLocal(this));
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

    ObjectElements *newheader = ReallocateElements(cx, this, getElementsHeader(),
                                                   oldAllocated, newAllocated);
    if (!newheader) {
        cx->recoverFromOutOfMemory();
        return;  
    }

    newheader->capacity = newCapacity;
    elements_ = newheader->elements();
}

 bool
NativeObject::CopyElementsForWrite(ThreadSafeContext *cx, NativeObject *obj)
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

    ObjectElements *newheader = AllocateElements(cx, obj, newAllocated);
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
NativeObject::allocSlot(ThreadSafeContext *cx, HandleNativeObject obj, uint32_t *slotp)
{
    MOZ_ASSERT(cx->isThreadLocal(obj));

    uint32_t slot = obj->slotSpan();
    MOZ_ASSERT(slot >= JSSLOT_FREE(obj->getClass()));

    



    if (obj->inDictionaryMode()) {
        ShapeTable &table = obj->lastProperty()->table();
        uint32_t last = table.freelist;
        if (last != SHAPE_INVALID_SLOT) {
#ifdef DEBUG
            MOZ_ASSERT(last < slot);
            uint32_t next = obj->getSlot(last).toPrivateUint32();
            MOZ_ASSERT_IF(next != SHAPE_INVALID_SLOT, next < slot);
#endif

            *slotp = last;

            const Value &vref = obj->getSlot(last);
            table.freelist = vref.toPrivateUint32();
            obj->setSlot(last, UndefinedValue());
            return true;
        }
    }

    if (slot >= SHAPE_MAXIMUM_SLOT) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    *slotp = slot;

    if (obj->inDictionaryMode() && !setSlotSpan(cx, obj, slot + 1))
        return false;

    return true;
}

void
NativeObject::freeSlot(uint32_t slot)
{
    MOZ_ASSERT(slot < slotSpan());

    if (inDictionaryMode()) {
        uint32_t &last = lastProperty()->table().freelist;

        
        MOZ_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan() && last != slot);

        



        if (JSSLOT_FREE(getClass()) <= slot) {
            MOZ_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan());
            setSlot(slot, PrivateUint32Value(last));
            last = slot;
            return;
        }
    }
    setSlot(slot, UndefinedValue());
}

Shape *
NativeObject::addDataProperty(ExclusiveContext *cx, jsid idArg, uint32_t slot, unsigned attrs)
{
    MOZ_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
    RootedNativeObject self(cx, this);
    RootedId id(cx, idArg);
    return addProperty(cx, self, id, nullptr, nullptr, slot, attrs, 0);
}

Shape *
NativeObject::addDataProperty(ExclusiveContext *cx, HandlePropertyName name,
                          uint32_t slot, unsigned attrs)
{
    MOZ_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
    RootedNativeObject self(cx, this);
    RootedId id(cx, NameToId(name));
    return addProperty(cx, self, id, nullptr, nullptr, slot, attrs, 0);
}







template <ExecutionMode mode>
static inline bool
CallAddPropertyHook(typename ExecutionModeTraits<mode>::ExclusiveContextType cxArg,
                    HandleNativeObject obj, HandleShape shape, HandleValue nominal)
{
    if (JSPropertyOp addProperty = obj->getClass()->addProperty) {
        MOZ_ASSERT(addProperty != JS_PropertyStub);

        if (mode == ParallelExecution)
            return false;

        ExclusiveContext *cx = cxArg->asExclusiveContext();
        if (!cx->shouldBeJSContext())
            return false;

        
        RootedValue value(cx, nominal);

        Rooted<jsid> id(cx, shape->propid());
        if (!CallJSPropertyOp(cx->asJSContext(), addProperty, obj, id, &value)) {
            obj->removeProperty(cx, shape->propid());
            return false;
        }
        if (value.get() != nominal) {
            if (shape->hasSlot())
                obj->setSlotWithType(cx, shape, value);
        }
    }
    return true;
}

template <ExecutionMode mode>
static inline bool
CallAddPropertyHookDense(typename ExecutionModeTraits<mode>::ExclusiveContextType cxArg,
                         HandleNativeObject obj, uint32_t index, HandleValue nominal)
{
    
    if (obj->is<ArrayObject>()) {
        ArrayObject *arr = &obj->as<ArrayObject>();
        uint32_t length = arr->length();
        if (index >= length) {
            if (mode == ParallelExecution) {
                
                if (length > INT32_MAX)
                    return false;
                arr->setLengthInt32(index + 1);
            } else {
                arr->setLength(cxArg->asExclusiveContext(), index + 1);
            }
        }
        return true;
    }

    if (JSPropertyOp addProperty = obj->getClass()->addProperty) {
        MOZ_ASSERT(addProperty != JS_PropertyStub);

        if (mode == ParallelExecution)
            return false;

        ExclusiveContext *cx = cxArg->asExclusiveContext();
        if (!cx->shouldBeJSContext())
            return false;

        if (!obj->maybeCopyElementsForWrite(cx))
            return false;

        
        RootedValue value(cx, nominal);

        Rooted<jsid> id(cx, INT_TO_JSID(index));
        if (!CallJSPropertyOp(cx->asJSContext(), addProperty, obj, id, &value)) {
            obj->setDenseElementHole(cx, index);
            return false;
        }
        if (value.get() != nominal)
            obj->setDenseElementWithType(cx, index, value);
    }

    return true;
}

template <ExecutionMode mode>
static bool
UpdateShapeTypeAndValue(typename ExecutionModeTraits<mode>::ExclusiveContextType cx,
                        NativeObject *obj, Shape *shape, const Value &value)
{
    jsid id = shape->propid();
    if (shape->hasSlot()) {
        if (mode == ParallelExecution) {
            if (!obj->setSlotIfHasType(shape, value,  false))
                return false;
        } else {
            obj->setSlotWithType(cx->asExclusiveContext(), shape, value,  false);
        }

        
        
        
        if (types::TypeNewScript *newScript = obj->typeRaw()->newScript()) {
            if (newScript->initializedShape() == shape)
                obj->setType(newScript->initializedType());
        }
    }
    if (!shape->hasSlot() || !shape->hasDefaultGetter() || !shape->hasDefaultSetter()) {
        if (mode == ParallelExecution) {
            if (!types::IsTypePropertyIdMarkedNonData(obj, id))
                return false;
        } else {
            types::MarkTypePropertyNonData(cx->asExclusiveContext(), obj, id);
        }
    }
    if (!shape->writable()) {
        if (mode == ParallelExecution) {
            if (!types::IsTypePropertyIdMarkedNonWritable(obj, id))
                return false;
        } else {
            types::MarkTypePropertyNonWritable(cx->asExclusiveContext(), obj, id);
        }
    }
    return true;
}

template <ExecutionMode mode>
static bool
NativeSet(typename ExecutionModeTraits<mode>::ContextType cx,
          HandleNativeObject obj, HandleObject receiver,
          HandleShape shape, bool strict, MutableHandleValue vp);

template <ExecutionMode mode>
static inline bool
DefinePropertyOrElement(typename ExecutionModeTraits<mode>::ExclusiveContextType cx,
                        HandleNativeObject obj, HandleId id,
                        PropertyOp getter, StrictPropertyOp setter,
                        unsigned attrs, HandleValue value,
                        bool callSetterAfterwards, bool setterIsStrict)
{
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);

    
    if (JSID_IS_INT(id) &&
        !getter &&
        !setter &&
        attrs == JSPROP_ENUMERATE &&
        (!obj->isIndexed() || !obj->containsPure(id)) &&
        !IsAnyTypedArray(obj))
    {
        uint32_t index = JSID_TO_INT(id);
        bool definesPast;
        if (!WouldDefinePastNonwritableLength(cx, obj, index, setterIsStrict, &definesPast))
            return false;
        if (definesPast)
            return true;

        NativeObject::EnsureDenseResult result;
        if (mode == ParallelExecution) {
            if (obj->writeToIndexWouldMarkNotPacked(index))
                return false;
            result = obj->ensureDenseElementsPreservePackedFlag(cx, index, 1);
        } else {
            result = obj->ensureDenseElements(cx->asExclusiveContext(), index, 1);
        }

        if (result == NativeObject::ED_FAILED)
            return false;
        if (result == NativeObject::ED_OK) {
            if (mode == ParallelExecution) {
                if (!obj->setDenseElementIfHasType(index, value))
                    return false;
            } else {
                obj->setDenseElementWithType(cx->asExclusiveContext(), index, value);
            }
            return CallAddPropertyHookDense<mode>(cx, obj, index, value);
        }
    }

    if (obj->is<ArrayObject>()) {
        Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
        if (id == NameToId(cx->names().length)) {
            if (mode == SequentialExecution && !cx->shouldBeJSContext())
                return false;
            return ArraySetLength<mode>(ExecutionModeTraits<mode>::toContextType(cx), arr, id,
                                        attrs, value, setterIsStrict);
        }

        uint32_t index;
        if (js_IdIsIndex(id, &index)) {
            bool definesPast;
            if (!WouldDefinePastNonwritableLength(cx, arr, index, setterIsStrict, &definesPast))
                return false;
            if (definesPast)
                return true;
        }
    }

    
    if (IsAnyTypedArray(obj)) {
        uint64_t index;
        if (IsTypedArrayIndex(id, &index))
            return true;
    }

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);
    RootedShape shape(cx, NativeObject::putProperty<mode>(cx, obj, id, getter, setter,
                                                          SHAPE_INVALID_SLOT, attrs, 0));
    if (!shape)
        return false;

    if (!UpdateShapeTypeAndValue<mode>(cx, obj, shape, value))
        return false;

    



    if (JSID_IS_INT(id)) {
        if (mode == ParallelExecution)
            return false;

        if (!obj->maybeCopyElementsForWrite(cx))
            return false;

        ExclusiveContext *ncx = cx->asExclusiveContext();
        uint32_t index = JSID_TO_INT(id);
        NativeObject::removeDenseElementForSparseIndex(ncx, obj, index);
        NativeObject::EnsureDenseResult result = NativeObject::maybeDensifySparseElements(ncx, obj);
        if (result == NativeObject::ED_FAILED)
            return false;
        if (result == NativeObject::ED_OK) {
            MOZ_ASSERT(!setter);
            return CallAddPropertyHookDense<mode>(cx, obj, index, value);
        }
    }

    if (!CallAddPropertyHook<mode>(cx, obj, shape, value))
        return false;

    if (callSetterAfterwards && setter) {
        if (!cx->shouldBeJSContext())
            return false;
        RootedValue nvalue(cx, value);
        return NativeSet<mode>(ExecutionModeTraits<mode>::toContextType(cx),
                               obj, obj, shape, setterIsStrict, &nvalue);
    }
    return true;
}

static bool
NativeLookupOwnProperty(ExclusiveContext *cx, HandleNativeObject obj, HandleId id,
                        MutableHandle<Shape*> shapep);

static unsigned
ApplyOrDefaultAttributes(unsigned attrs, const Shape *shape = nullptr)
{
    bool enumerable = shape ? shape->enumerable() : false;
    bool writable = shape ? shape->writable() : false;
    bool configurable = shape ? shape->configurable() : false;
    return ApplyAttributes(attrs, enumerable, writable, configurable);
}

static bool
PurgeProtoChain(ExclusiveContext *cx, JSObject *objArg, HandleId id)
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
PurgeScopeChainHelper(ExclusiveContext *cx, HandleObject objArg, HandleId id)
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
PurgeScopeChain(ExclusiveContext *cx, HandleObject obj, HandleId id)
{
    if (obj->isDelegate() && obj->isNative())
        return PurgeScopeChainHelper(cx, obj, id);
    return true;
}





static inline bool
CheckAccessorRedefinition(ExclusiveContext *cx, HandleObject obj, HandleShape shape,
                          PropertyOp getter, StrictPropertyOp setter, HandleId id, unsigned attrs)
{
    MOZ_ASSERT(shape->isAccessorDescriptor());
    if (shape->configurable() || (getter == shape->getter() && setter == shape->setter()))
        return true;

    





    if ((attrs & JSPROP_REDEFINE_NONCONFIGURABLE) &&
        obj->is<GlobalObject>() &&
        !obj->getClass()->isDOMClass())
    {
        return true;
    }

    if (!cx->isJSContext())
        return false;

    return Throw(cx->asJSContext(), id, JSMSG_CANT_REDEFINE_PROP);
}

bool
js::DefineNativeProperty(ExclusiveContext *cx, HandleNativeObject obj, HandleId id, HandleValue value,
                         PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);
    MOZ_ASSERT(!(attrs & JSPROP_PROPOP_ACCESSORS));

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    RootedShape shape(cx);
    RootedValue updateValue(cx, value);
    bool shouldDefine = true;

    




    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        if (!NativeLookupOwnProperty(cx, obj, id, &shape))
            return false;
        if (shape) {
            



            if (IsImplicitDenseOrTypedArrayElement(shape)) {
                if (IsAnyTypedArray(obj)) {
                    
                    return true;
                }
                if (!NativeObject::sparsifyDenseElement(cx, obj, JSID_TO_INT(id)))
                    return false;
                shape = obj->lookup(cx, id);
            }
            if (shape->isAccessorDescriptor()) {
                if (!CheckAccessorRedefinition(cx, obj, shape, getter, setter, id, attrs))
                    return false;
                attrs = ApplyOrDefaultAttributes(attrs, shape);
                shape = NativeObject::changeProperty<SequentialExecution>(cx, obj, shape, attrs,
                                                                          JSPROP_GETTER | JSPROP_SETTER,
                                                                          (attrs & JSPROP_GETTER)
                                                                          ? getter
                                                                          : shape->getter(),
                                                                          (attrs & JSPROP_SETTER)
                                                                          ? setter
                                                                          : shape->setter());
                if (!shape)
                    return false;
                shouldDefine = false;
            }
        }
    } else if (!(attrs & JSPROP_IGNORE_VALUE)) {
        
        
        
        
        
        
        
        
        
        
        
        NativeLookupOwnPropertyNoResolve(cx, obj, id, &shape);

        if (shape) {
            if (shape->isAccessorDescriptor() &&
                !CheckAccessorRedefinition(cx, obj, shape, getter, setter, id, attrs))
            {
                return false;
            }

            
            
            if (IsImplicitDenseOrTypedArrayElement(shape))
                attrs = ApplyAttributes(attrs, true, true, !IsAnyTypedArray(obj));
            else
                attrs = ApplyOrDefaultAttributes(attrs, shape);
        }
    } else {
        
        
        
        if (!NativeLookupOwnProperty(cx, obj, id, &shape))
            return false;

        if (shape) {
            
            if (IsImplicitDenseOrTypedArrayElement(shape)) {
                if (obj->is<TypedArrayObject>()) {
                    




                    return true;
                }
                if (!NativeObject::sparsifyDenseElement(cx, obj, JSID_TO_INT(id)))
                    return false;
                shape = obj->lookup(cx, id);
            }

            if (shape->isAccessorDescriptor() &&
                !CheckAccessorRedefinition(cx, obj, shape, getter, setter, id, attrs))
            {
                return false;
            }

            attrs = ApplyOrDefaultAttributes(attrs, shape);

            
            unsigned attrMask = ~(JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
            shape = NativeObject::changeProperty<SequentialExecution>(cx, obj, shape, attrs, attrMask,
                                                                      shape->getter(), shape->setter());
            if (!shape)
                return false;
            if (shape->hasSlot())
                updateValue = obj->getSlot(shape->slot());
            shouldDefine = false;
        }
    }

    



    if (!PurgeScopeChain(cx, obj, id))
        return false;

    if (shouldDefine) {
        
        
        
        attrs = ApplyOrDefaultAttributes(attrs) & ~JSPROP_IGNORE_VALUE;
        return DefinePropertyOrElement<SequentialExecution>(cx, obj, id, getter, setter,
                                                            attrs, value, false, false);
    }

    MOZ_ASSERT(shape);

    JS_ALWAYS_TRUE(UpdateShapeTypeAndValue<SequentialExecution>(cx, obj, shape, updateValue));

    return CallAddPropertyHook<SequentialExecution>(cx, obj, shape, updateValue);
}

static bool
NativeLookupOwnProperty(ExclusiveContext *cx, HandleNativeObject obj, HandleId id,
                        MutableHandle<Shape*> shapep)
{
    RootedObject pobj(cx);
    bool done;

    if (!LookupOwnPropertyInline<CanGC>(cx, obj, id, &pobj, shapep, &done))
        return false;
    if (!done || pobj != obj)
        shapep.set(nullptr);
    return true;
}

template <AllowGC allowGC>
bool
baseops::LookupProperty(ExclusiveContext *cx,
                        typename MaybeRooted<NativeObject*, allowGC>::HandleType obj,
                        typename MaybeRooted<jsid, allowGC>::HandleType id,
                        typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
                        typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp)
{
    return LookupPropertyInline<allowGC>(cx, obj, id, objp, propp);
}

template bool
baseops::LookupProperty<CanGC>(ExclusiveContext *cx, HandleNativeObject obj, HandleId id,
                               MutableHandleObject objp, MutableHandleShape propp);

template bool
baseops::LookupProperty<NoGC>(ExclusiveContext *cx, NativeObject *obj, jsid id,
                              FakeMutableHandle<JSObject*> objp,
                              FakeMutableHandle<Shape*> propp);

bool
baseops::LookupElement(JSContext *cx, HandleNativeObject obj, uint32_t index,
                       MutableHandleObject objp, MutableHandleShape propp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;

    return LookupPropertyInline<CanGC>(cx, obj, id, objp, propp);
}

bool
js::LookupNativeProperty(ExclusiveContext *cx, HandleNativeObject obj, HandleId id,
                         MutableHandleObject objp, MutableHandleShape propp)
{
    return LookupPropertyInline<CanGC>(cx, obj, id, objp, propp);
}

bool
baseops::DefineGeneric(ExclusiveContext *cx, HandleNativeObject obj, HandleId id, HandleValue value,
                       PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    return DefineNativeProperty(cx, obj, id, value, getter, setter, attrs);
}

bool
baseops::DefineElement(ExclusiveContext *cx, HandleNativeObject obj, uint32_t index, HandleValue value,
                       PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    RootedId id(cx);
    if (index <= JSID_INT_MAX) {
        id = INT_TO_JSID(index);
        return DefineNativeProperty(cx, obj, id, value, getter, setter, attrs);
    }

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    if (!IndexToId(cx, index, &id))
        return false;

    return DefineNativeProperty(cx, obj, id, value, getter, setter, attrs);
}

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE bool
NativeGetInline(JSContext *cx,
                typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
                typename MaybeRooted<JSObject*, allowGC>::HandleType receiver,
                typename MaybeRooted<NativeObject*, allowGC>::HandleType pobj,
                typename MaybeRooted<Shape*, allowGC>::HandleType shape,
                typename MaybeRooted<Value, allowGC>::MutableHandleType vp)
{
    if (shape->hasSlot()) {
        vp.set(pobj->getSlot(shape->slot()));
        MOZ_ASSERT_IF(!vp.isMagic(JS_UNINITIALIZED_LEXICAL) &&
                      !pobj->hasSingletonType() &&
                      !pobj->template is<ScopeObject>() &&
                      shape->hasDefaultGetter(),
                      js::types::TypeHasProperty(cx, pobj->type(), shape->propid(), vp));
    } else {
        vp.setUndefined();
    }
    if (shape->hasDefaultGetter())
        return true;

    {
        jsbytecode *pc;
        JSScript *script = cx->currentScript(&pc);
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

    if (!shape->get(cx,
                    MaybeRooted<JSObject*, allowGC>::toHandle(receiver),
                    MaybeRooted<JSObject*, allowGC>::toHandle(obj),
                    MaybeRooted<JSObject*, allowGC>::toHandle(pobj),
                    MaybeRooted<Value, allowGC>::toMutableHandle(vp)))
    {
        return false;
    }

    
    if (shape->hasSlot() && pobj->contains(cx, shape))
        pobj->setSlot(shape->slot(), vp);

    return true;
}

bool
js::NativeGet(JSContext *cx, HandleObject obj, HandleNativeObject pobj, HandleShape shape,
              MutableHandleValue vp)
{
    return NativeGetInline<CanGC>(cx, obj, obj, pobj, shape, vp);
}

template <ExecutionMode mode>
static bool
NativeSet(typename ExecutionModeTraits<mode>::ContextType cxArg, HandleNativeObject obj,
          HandleObject receiver, HandleShape shape, bool strict, MutableHandleValue vp)
{
    MOZ_ASSERT(cxArg->isThreadLocal(obj));
    MOZ_ASSERT(obj->isNative());

    if (shape->hasSlot()) {
        
        if (shape->hasDefaultSetter()) {
            if (mode == ParallelExecution) {
                if (!obj->setSlotIfHasType(shape, vp))
                    return false;
            } else {
                
                
                
                bool overwriting = !obj->is<GlobalObject>() || !obj->getSlot(shape->slot()).isUndefined();
                obj->setSlotWithType(cxArg->asExclusiveContext(), shape, vp, overwriting);
            }

            return true;
        }
    }

    if (mode == ParallelExecution)
        return false;
    JSContext *cx = cxArg->asJSContext();

    if (!shape->hasSlot()) {
        





        if (!shape->hasGetterValue() && shape->hasDefaultSetter())
            return js_ReportGetterOnlyAssignment(cx, strict);
    }

    RootedValue ovp(cx, vp);

    uint32_t sample = cx->runtime()->propertyRemovals;
    if (!shape->set(cx, obj, receiver, strict, vp))
        return false;

    



    if (shape->hasSlot() &&
        (MOZ_LIKELY(cx->runtime()->propertyRemovals == sample) ||
         obj->contains(cx, shape)))
    {
        obj->setSlot(shape->slot(), vp);
    }

    return true;
}






static bool
Detecting(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    MOZ_ASSERT(script->containsPC(pc));

    
    JSOp op = JSOp(*pc);
    if (js_CodeSpec[op].format & JOF_DETECTING)
        return true;

    jsbytecode *endpc = script->codeEnd();

    if (op == JSOP_NULL) {
        



        if (++pc < endpc) {
            op = JSOp(*pc);
            return op == JSOP_EQ || op == JSOP_NE;
        }
        return false;
    }

    if (op == JSOP_GETGNAME || op == JSOP_GETNAME) {
        




        JSAtom *atom = script->getAtom(GET_UINT32_INDEX(pc));
        if (atom == cx->names().undefined &&
            (pc += js_CodeSpec[op].length) < endpc) {
            op = JSOp(*pc);
            return op == JSOP_EQ || op == JSOP_NE || op == JSOP_STRICTEQ || op == JSOP_STRICTNE;
        }
    }

    return false;
}

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE bool
GetPropertyHelperInline(JSContext *cx,
                        typename MaybeRooted<NativeObject*, allowGC>::HandleType obj,
                        typename MaybeRooted<JSObject*, allowGC>::HandleType receiver,
                        typename MaybeRooted<jsid, allowGC>::HandleType id,
                        typename MaybeRooted<Value, allowGC>::MutableHandleType vp)
{
    
    typename MaybeRooted<JSObject*, allowGC>::RootType obj2(cx);
    typename MaybeRooted<Shape*, allowGC>::RootType shape(cx);
    if (!LookupPropertyInline<allowGC>(cx, obj, id, &obj2, &shape))
        return false;

    if (!shape) {
        if (!allowGC)
            return false;

        vp.setUndefined();

        if (JSPropertyOp getProperty = obj->getClass()->getProperty) {
            if (!CallJSPropertyOp(cx, getProperty,
                                  MaybeRooted<JSObject*, allowGC>::toHandle(obj),
                                  MaybeRooted<jsid, allowGC>::toHandle(id),
                                  MaybeRooted<Value, allowGC>::toMutableHandle(vp)))
            {
                return false;
            }
        }

        



        if (vp.isUndefined()) {
            jsbytecode *pc = nullptr;
            RootedScript script(cx, cx->currentScript(&pc));
            if (!pc)
                return true;
            JSOp op = (JSOp) *pc;

            if (op == JSOP_GETXPROP) {
                
                JSAutoByteString printable;
                if (js_ValueToPrintable(cx, IdToValue(id), &printable))
                    js_ReportIsNotDefined(cx, printable.ptr());
                return false;
            }

            
            if (!cx->compartment()->options().extraWarnings(cx) || (op != JSOP_GETPROP && op != JSOP_GETELEM))
                return true;

            
            if (!script || script->warnedAboutUndefinedProp())
                return true;

            




            if (script->selfHosted())
                return true;

            
            if (JSID_IS_ATOM(id, cx->names().iteratorIntrinsic))
                return true;

            
            pc += js_CodeSpec[op].length;
            if (Detecting(cx, script, pc))
                return true;

            unsigned flags = JSREPORT_WARNING | JSREPORT_STRICT;
            script->setWarnedAboutUndefinedProp();

            
            RootedValue val(cx, IdToValue(id));
            if (!js_ReportValueErrorFlags(cx, flags, JSMSG_UNDEFINED_PROP,
                                          JSDVG_IGNORE_STACK, val, js::NullPtr(),
                                          nullptr, nullptr))
            {
                return false;
            }
        }
        return true;
    }

    if (!obj2->isNative()) {
        if (!allowGC)
            return false;
        HandleObject obj2Handle = MaybeRooted<JSObject*, allowGC>::toHandle(obj2);
        HandleObject receiverHandle = MaybeRooted<JSObject*, allowGC>::toHandle(receiver);
        HandleId idHandle = MaybeRooted<jsid, allowGC>::toHandle(id);
        MutableHandleValue vpHandle = MaybeRooted<Value, allowGC>::toMutableHandle(vp);
        return obj2->template is<ProxyObject>()
               ? Proxy::get(cx, obj2Handle, receiverHandle, idHandle, vpHandle)
               : JSObject::getGeneric(cx, obj2Handle, obj2Handle, idHandle, vpHandle);
    }

    typename MaybeRooted<NativeObject*, allowGC>::HandleType nobj2 =
        MaybeRooted<JSObject*, allowGC>::template downcastHandle<NativeObject>(obj2);

    if (IsImplicitDenseOrTypedArrayElement(shape)) {
        vp.set(nobj2->getDenseOrTypedArrayElement(JSID_TO_INT(id)));
        return true;
    }

    
    if (!NativeGetInline<allowGC>(cx, obj, receiver, nobj2, shape, vp))
        return false;

    return true;
}

bool
baseops::GetProperty(JSContext *cx, HandleNativeObject obj, HandleObject receiver, HandleId id, MutableHandleValue vp)
{
    
    return GetPropertyHelperInline<CanGC>(cx, obj, receiver, id, vp);
}

bool
baseops::GetPropertyNoGC(JSContext *cx, NativeObject *obj, JSObject *receiver, jsid id, Value *vp)
{
    AutoAssertNoException nogc(cx);
    return GetPropertyHelperInline<NoGC>(cx, obj, receiver, id, vp);
}

bool
baseops::GetElement(JSContext *cx, HandleNativeObject obj, HandleObject receiver, uint32_t index,
                    MutableHandleValue vp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;

    
    return GetPropertyHelperInline<CanGC>(cx, obj, receiver, id, vp);
}

static bool
MaybeReportUndeclaredVarAssignment(JSContext *cx, JSString *propname)
{
    {
        jsbytecode *pc;
        JSScript *script = cx->currentScript(&pc, JSContext::ALLOW_CROSS_COMPARTMENT);
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
                                        js_GetErrorMessage, nullptr,
                                        JSMSG_UNDECLARED_VAR, bytes.ptr());
}









template <ExecutionMode mode>
static bool
SetPropertyByDefining(typename ExecutionModeTraits<mode>::ContextType cxArg,
                      HandleObject receiver, HandleId id, HandleValue v, bool strict)
{
    
    
    
    
    
    bool existing;
    if (mode == ParallelExecution) {
        
        
        NativeObject *npobj;
        Shape *shape;
        if (!LookupPropertyPure(cxArg, receiver, id, &npobj, &shape))
            return false;
        existing = (npobj == receiver);
    } else {
        if (!HasOwnProperty(cxArg->asJSContext(), receiver, id, &existing))
            return false;
    }

    
    
    
    if (!existing) {
        bool extensible;
        if (mode == ParallelExecution) {
            if (receiver->is<ProxyObject>())
                return false;
            extensible = receiver->nonProxyIsExtensible();
        } else {
            if (!JSObject::isExtensible(cxArg->asJSContext(), receiver, &extensible))
                return false;
        }
        if (!extensible) {
            
            
            if (strict)
                return receiver->reportNotExtensible(cxArg);
            if (mode == SequentialExecution &&
                cxArg->asJSContext()->compartment()->options().extraWarnings(cxArg->asJSContext()))
            {
                return receiver->reportNotExtensible(cxArg, JSREPORT_STRICT | JSREPORT_WARNING);
            }
            return true;
        }
    }

    
    const Class *clasp = receiver->getClass();
    if (mode == ParallelExecution) {
        if (receiver->isDelegate())
            return false;

        if (clasp->getProperty || !types::HasTypePropertyId(receiver, id, v))
            return false;
    } else {
        
        if (!PurgeScopeChain(cxArg->asJSContext(), receiver, id))
            return false;
    }

    
    unsigned attrs =
        existing
        ? JSPROP_IGNORE_ENUMERATE | JSPROP_IGNORE_READONLY | JSPROP_IGNORE_PERMANENT
        : JSPROP_ENUMERATE;
    JSPropertyOp getter = clasp->getProperty;
    JSStrictPropertyOp setter = clasp->setProperty;
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);
    if (!receiver->is<NativeObject>()) {
        if (mode == ParallelExecution)
            return false;
        return JSObject::defineGeneric(cxArg->asJSContext(), receiver, id, v, getter, setter,
                                       attrs);
    }
    Rooted<NativeObject*> nativeReceiver(cxArg, &receiver->as<NativeObject>());
    return DefinePropertyOrElement<mode>(cxArg, nativeReceiver, id, getter, setter,
                                         attrs, v, true, strict);
}










template <ExecutionMode mode>
static bool
SetNonexistentProperty(typename ExecutionModeTraits<mode>::ContextType cxArg,
                       HandleObject receiver, HandleId id, baseops::QualifiedBool qualified,
                       HandleValue v, bool strict)
{
    
    MOZ_ASSERT(!receiver->is<BlockObject>());

    if (receiver->isUnqualifiedVarObj() && !qualified) {
        if (mode == ParallelExecution)
            return false;

        if (!MaybeReportUndeclaredVarAssignment(cxArg->asJSContext(), JSID_TO_STRING(id)))
            return false;
    }

    return SetPropertyByDefining<mode>(cxArg, receiver, id, v, strict);
}

template <ExecutionMode mode>
static bool
SetDenseOrTypedArrayElement(typename ExecutionModeTraits<mode>::ContextType cxArg,
                            HandleNativeObject obj, uint32_t index, MutableHandleValue vp,
                            bool strict);








template <ExecutionMode mode>
static bool
SetExistingProperty(typename ExecutionModeTraits<mode>::ContextType cxArg,
                    HandleNativeObject obj, HandleObject receiver, HandleId id,
                    HandleObject pobj, HandleShape foundShape, MutableHandleValue vp, bool strict)
{
    RootedShape shape(cxArg, foundShape);
    if (IsImplicitDenseOrTypedArrayElement(shape)) {
        
        if (pobj != receiver)
            shape = nullptr;
    } else {
        
        if (shape->isAccessorDescriptor()) {
            if (shape->hasDefaultSetter()) {
                
                if (mode == ParallelExecution)
                    return !strict;

                return js_ReportGetterOnlyAssignment(cxArg->asJSContext(), strict);
            }
        } else {
            MOZ_ASSERT(shape->isDataDescriptor());

            if (!shape->writable()) {
                





                if (mode == ParallelExecution)
                    return !strict;

                JSContext *cx = cxArg->asJSContext();
                if (strict)
                    return JSObject::reportReadOnly(cx, id, JSREPORT_ERROR);
                if (cx->compartment()->options().extraWarnings(cx))
                    return JSObject::reportReadOnly(cx, id, JSREPORT_STRICT | JSREPORT_WARNING);
                return true;
            }
        }

        if (pobj != receiver) {
            

            if (!shape->shadowable() &&
                !(pobj->is<ArrayObject>() && id == NameToId(cxArg->names().length)))
            {
                
                if (shape->hasDefaultSetter() && !shape->hasGetterValue())
                    return true;

                
                if (mode == ParallelExecution)
                    return false;
                return shape->set(cxArg->asJSContext(), obj, receiver, strict, vp);
            }

            
            
            shape = nullptr;
        }
    }

    if (IsImplicitDenseOrTypedArrayElement(shape))
        return SetDenseOrTypedArrayElement<mode>(cxArg, obj, JSID_TO_INT(id), vp, strict);

    if (shape) {
        if (obj->is<ArrayObject>() && id == NameToId(cxArg->names().length)) {
            Rooted<ArrayObject*> arr(cxArg, &obj->as<ArrayObject>());
            return ArraySetLength<mode>(cxArg, arr, id, shape->attributes(), vp, strict);
        }

        return NativeSet<mode>(cxArg, obj, receiver, shape, strict, vp);
    }
    return SetPropertyByDefining<mode>(cxArg, receiver, id, vp, strict);
}

template <ExecutionMode mode>
static bool
SetDenseOrTypedArrayElement(typename ExecutionModeTraits<mode>::ContextType cxArg,
                            HandleNativeObject obj, uint32_t index, MutableHandleValue vp,
                            bool strict)
{
    if (IsAnyTypedArray(obj)) {
        double d;
        if (mode == ParallelExecution) {
            
            
            if (vp.isObject())
                return false;
            if (!NonObjectToNumber(cxArg, vp, &d))
                return false;
        } else {
            if (!ToNumber(cxArg->asJSContext(), vp, &d))
                return false;
        }

        
        
        
        uint32_t len = AnyTypedArrayLength(obj);
        if (index < len) {
            if (obj->is<TypedArrayObject>())
                TypedArrayObject::setElement(obj->as<TypedArrayObject>(), index, d);
            else
                SharedTypedArrayObject::setElement(obj->as<SharedTypedArrayObject>(), index, d);
        }
        return true;
    }

    bool definesPast;
    if (!WouldDefinePastNonwritableLength(cxArg, obj, index, strict, &definesPast))
        return false;
    if (definesPast) {
        
        if (mode == ParallelExecution)
            return !strict;
        return true;
    }

    if (!obj->maybeCopyElementsForWrite(cxArg))
        return false;

    if (mode == ParallelExecution)
        return obj->setDenseElementIfHasType(index, vp);

    obj->setDenseElementWithType(cxArg->asJSContext(), index, vp);
    return true;
}

template <ExecutionMode mode>
bool
baseops::SetPropertyHelper(typename ExecutionModeTraits<mode>::ContextType cxArg,
                           HandleNativeObject obj, HandleObject receiver, HandleId id,
                           QualifiedBool qualified, MutableHandleValue vp, bool strict)
{
    MOZ_ASSERT(cxArg->isThreadLocal(obj));

    if (MOZ_UNLIKELY(obj->watched())) {
        if (mode == ParallelExecution)
            return false;

        
        JSContext *cx = cxArg->asJSContext();
        WatchpointMap *wpmap = cx->compartment()->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, obj, id, vp))
            return false;
    }

    RootedObject pobj(cxArg);
    RootedShape shape(cxArg);
    if (mode == ParallelExecution) {
        NativeObject *npobj;
        if (!LookupPropertyPure(cxArg, obj, id, &npobj, shape.address()))
            return false;
        pobj = npobj;
    } else {
        JSContext *cx = cxArg->asJSContext();
        if (!LookupNativeProperty(cx, obj, id, &pobj, &shape))
            return false;
    }

    if (!shape)
        return SetNonexistentProperty<mode>(cxArg, receiver, id, qualified, vp, strict);

    if (pobj->isNative())
        return SetExistingProperty<mode>(cxArg, obj, receiver, id, pobj, shape, vp, strict);

    if (pobj->is<ProxyObject>()) {
        if (mode == ParallelExecution)
            return false;

        JSContext *cx = cxArg->asJSContext();
        Rooted<PropertyDescriptor> pd(cx);
        if (!Proxy::getPropertyDescriptor(cx, pobj, id, &pd))
            return false;

        if ((pd.attributes() & (JSPROP_SHARED | JSPROP_SHADOWABLE)) == JSPROP_SHARED) {
            return !pd.setter() ||
                   CallSetter(cx, receiver, id, pd.setter(), pd.attributes(), strict, vp);
        }

        if (pd.isReadonly()) {
            if (strict)
                return JSObject::reportReadOnly(cx, id, JSREPORT_ERROR);
            if (cx->compartment()->options().extraWarnings(cx))
                return JSObject::reportReadOnly(cx, id, JSREPORT_STRICT | JSREPORT_WARNING);
            return true;
        }
    }

    return SetPropertyByDefining<mode>(cxArg, receiver, id, vp, strict);
}

template bool
baseops::SetPropertyHelper<SequentialExecution>(JSContext *cx, HandleNativeObject obj,
                                                HandleObject receiver, HandleId id,
                                                QualifiedBool qualified,
                                                MutableHandleValue vp, bool strict);
template bool
baseops::SetPropertyHelper<ParallelExecution>(ForkJoinContext *cx, HandleNativeObject obj,
                                              HandleObject receiver, HandleId id,
                                              QualifiedBool qualified,
                                              MutableHandleValue vp, bool strict);

bool
baseops::SetElementHelper(JSContext *cx, HandleNativeObject obj, HandleObject receiver, uint32_t index,
                          MutableHandleValue vp, bool strict)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return baseops::SetPropertyHelper<SequentialExecution>(cx, obj, receiver, id, Qualified, vp,
                                                           strict);
}

bool
baseops::GetAttributes(JSContext *cx, HandleNativeObject obj, HandleId id, unsigned *attrsp)
{
    RootedObject nobj(cx);
    RootedShape shape(cx);
    if (!baseops::LookupProperty<CanGC>(cx, obj, id, &nobj, &shape))
        return false;
    if (!shape) {
        *attrsp = 0;
        return true;
    }
    if (!nobj->isNative())
        return JSObject::getGenericAttributes(cx, nobj, id, attrsp);

    *attrsp = GetShapeAttributes(nobj, shape);
    return true;
}

bool
baseops::SetAttributes(JSContext *cx, HandleNativeObject obj, HandleId id, unsigned *attrsp)
{
    RootedObject nobj(cx);
    RootedShape shape(cx);
    if (!baseops::LookupProperty<CanGC>(cx, obj, id, &nobj, &shape))
        return false;
    if (!shape)
        return true;
    if (nobj->isNative() && IsImplicitDenseOrTypedArrayElement(shape)) {
        if (IsAnyTypedArray(nobj.get())) {
            if (*attrsp == (JSPROP_ENUMERATE | JSPROP_PERMANENT))
                return true;
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_SET_ARRAY_ATTRS);
            return false;
        }
        if (!NativeObject::sparsifyDenseElement(cx, nobj.as<NativeObject>(), JSID_TO_INT(id)))
            return false;
        shape = nobj->as<NativeObject>().lookup(cx, id);
    }
    if (nobj->isNative()) {
        if (!NativeObject::changePropertyAttributes(cx, nobj.as<NativeObject>(), shape, *attrsp))
            return false;
        if (*attrsp & JSPROP_READONLY)
            types::MarkTypePropertyNonWritable(cx, nobj, id);
        return true;
    } else {
        return JSObject::setGenericAttributes(cx, nobj, id, attrsp);
    }
}

bool
baseops::DeleteGeneric(JSContext *cx, HandleNativeObject obj, HandleId id, bool *succeeded)
{
    RootedObject proto(cx);
    RootedShape shape(cx);
    if (!baseops::LookupProperty<CanGC>(cx, obj, id, &proto, &shape))
        return false;
    if (!shape || proto != obj) {
        



        return CallJSDeletePropertyOp(cx, obj->getClass()->delProperty, obj, id, succeeded);
    }

    cx->runtime()->gc.poke();

    if (IsImplicitDenseOrTypedArrayElement(shape)) {
        if (IsAnyTypedArray(obj)) {
            
            *succeeded = false;
            return true;
        }

        if (!CallJSDeletePropertyOp(cx, obj->getClass()->delProperty, obj, id, succeeded))
            return false;
        if (!*succeeded)
            return true;

        NativeObject *nobj = &obj->as<NativeObject>();
        if (!nobj->maybeCopyElementsForWrite(cx))
            return false;

        nobj->setDenseElementHole(cx, JSID_TO_INT(id));
        return SuppressDeletedProperty(cx, obj, id);
    }

    if (!shape->configurable()) {
        *succeeded = false;
        return true;
    }

    RootedId propid(cx, shape->propid());
    if (!CallJSDeletePropertyOp(cx, obj->getClass()->delProperty, obj, propid, succeeded))
        return false;
    if (!*succeeded)
        return true;

    return obj->removeProperty(cx, id) && SuppressDeletedProperty(cx, obj, id);
}
