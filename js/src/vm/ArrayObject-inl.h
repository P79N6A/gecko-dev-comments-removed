





#ifndef vm_ArrayObject_inl_h
#define vm_ArrayObject_inl_h

#include "vm/ArrayObject.h"

#include "gc/GCTrace.h"
#include "vm/String.h"

#include "vm/TypeInference-inl.h"

namespace js {

inline void
ArrayObject::setLength(ExclusiveContext* cx, uint32_t length)
{
    MOZ_ASSERT(lengthIsWritable());

    if (length > INT32_MAX) {
        
        MarkObjectGroupFlags(cx, this, OBJECT_FLAG_LENGTH_OVERFLOW);
    }

    getElementsHeader()->length = length;
}

 inline ArrayObject*
ArrayObject::createArrayInternal(ExclusiveContext* cx, gc::AllocKind kind, gc::InitialHeap heap,
                                 HandleShape shape, HandleObjectGroup group)
{
    
    MOZ_ASSERT(shape && group);
    MOZ_ASSERT(group->clasp() == shape->getObjectClass());
    MOZ_ASSERT(group->clasp() == &ArrayObject::class_);
    MOZ_ASSERT_IF(group->clasp()->finalize, heap == gc::TenuredHeap);
    MOZ_ASSERT_IF(group->hasUnanalyzedPreliminaryObjects(),
                  heap == js::gc::TenuredHeap);

    
    
    MOZ_ASSERT(shape->numFixedSlots() == 0);

    size_t nDynamicSlots = dynamicSlotsCount(0, shape->slotSpan(), group->clasp());
    JSObject* obj = Allocate<JSObject>(cx, kind, nDynamicSlots, heap, group->clasp());
    if (!obj)
        return nullptr;

    static_cast<ArrayObject*>(obj)->shape_.init(shape);
    static_cast<ArrayObject*>(obj)->group_.init(group);

    SetNewObjectMetadata(cx, obj);

    return &obj->as<ArrayObject>();
}

 inline ArrayObject*
ArrayObject::finishCreateArray(ArrayObject* obj, HandleShape shape)
{
    size_t span = shape->slotSpan();
    if (span)
        obj->initializeSlotRange(0, span);

    gc::TraceCreateObject(obj);

    return obj;
}

 inline ArrayObject*
ArrayObject::createArray(ExclusiveContext* cx, gc::AllocKind kind, gc::InitialHeap heap,
                         HandleShape shape, HandleObjectGroup group,
                         uint32_t length)
{
    ArrayObject* obj = createArrayInternal(cx, kind, heap, shape, group);
    if (!obj)
        return nullptr;

    uint32_t capacity = gc::GetGCKindSlots(kind) - ObjectElements::VALUES_PER_HEADER;

    obj->setFixedElements();
    new (obj->getElementsHeader()) ObjectElements(capacity, length);

    return finishCreateArray(obj, shape);
}

 inline ArrayObject*
ArrayObject::createArray(ExclusiveContext* cx, gc::InitialHeap heap,
                         HandleShape shape, HandleObjectGroup group,
                         HeapSlot* elements)
{
    
    
    
    gc::AllocKind kind = gc::AllocKind::OBJECT0_BACKGROUND;

    ArrayObject* obj = createArrayInternal(cx, kind, heap, shape, group);
    if (!obj)
        return nullptr;

    obj->elements_ = elements;

    return finishCreateArray(obj, shape);
}

 inline ArrayObject*
ArrayObject::createCopyOnWriteArray(ExclusiveContext* cx, gc::InitialHeap heap,
                                    HandleArrayObject sharedElementsOwner)
{
    MOZ_ASSERT(sharedElementsOwner->getElementsHeader()->isCopyOnWrite());
    MOZ_ASSERT(sharedElementsOwner->getElementsHeader()->ownerObject() == sharedElementsOwner);

    
    
    
    gc::AllocKind kind = gc::AllocKind::OBJECT0_BACKGROUND;

    RootedShape shape(cx, sharedElementsOwner->lastProperty());
    RootedObjectGroup group(cx, sharedElementsOwner->group());
    ArrayObject* obj = createArrayInternal(cx, kind, heap, shape, group);
    if (!obj)
        return nullptr;

    obj->elements_ = sharedElementsOwner->getDenseElementsAllowCopyOnWrite();

    return finishCreateArray(obj, shape);
}

} 

#endif 
