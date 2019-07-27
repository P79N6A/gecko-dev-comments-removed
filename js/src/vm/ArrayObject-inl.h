





#ifndef vm_ArrayObject_inl_h
#define vm_ArrayObject_inl_h

#include "vm/ArrayObject.h"

#include "vm/String.h"

#include "jsinferinlines.h"

namespace js {

inline void
ArrayObject::setLength(ExclusiveContext *cx, uint32_t length)
{
    MOZ_ASSERT(lengthIsWritable());

    if (length > INT32_MAX) {
        
        types::MarkTypeObjectFlags(cx, this, types::OBJECT_FLAG_LENGTH_OVERFLOW);
    }

    getElementsHeader()->length = length;
}

 inline ArrayObject *
ArrayObject::createArrayInternal(ExclusiveContext *cx, gc::AllocKind kind, gc::InitialHeap heap,
                                 HandleShape shape, HandleTypeObject type)
{
    
    MOZ_ASSERT(shape && type);
    MOZ_ASSERT(type->clasp() == shape->getObjectClass());
    MOZ_ASSERT(type->clasp() == &ArrayObject::class_);
    MOZ_ASSERT_IF(type->clasp()->finalize, heap == gc::TenuredHeap);

    
    
    MOZ_ASSERT(shape->numFixedSlots() == 0);

    size_t nDynamicSlots = dynamicSlotsCount(0, shape->slotSpan(), type->clasp());
    JSObject *obj = NewGCObject<CanGC>(cx, kind, nDynamicSlots, heap);
    if (!obj)
        return nullptr;

    static_cast<ArrayObject *>(obj)->shape_.init(shape);
    static_cast<ArrayObject *>(obj)->type_.init(type);

    return &obj->as<ArrayObject>();
}

 inline ArrayObject *
ArrayObject::finishCreateArray(ArrayObject *obj, HandleShape shape)
{
    size_t span = shape->slotSpan();
    if (span)
        obj->initializeSlotRange(0, span);

    gc::TraceCreateObject(obj);

    return obj;
}

 inline ArrayObject *
ArrayObject::createArray(ExclusiveContext *cx, gc::AllocKind kind, gc::InitialHeap heap,
                         HandleShape shape, HandleTypeObject type,
                         uint32_t length)
{
    ArrayObject *obj = createArrayInternal(cx, kind, heap, shape, type);
    if (!obj)
        return nullptr;

    uint32_t capacity = gc::GetGCKindSlots(kind) - ObjectElements::VALUES_PER_HEADER;

    obj->setFixedElements();
    new (obj->getElementsHeader()) ObjectElements(capacity, length);

    return finishCreateArray(obj, shape);
}

 inline ArrayObject *
ArrayObject::createArray(ExclusiveContext *cx, gc::InitialHeap heap,
                         HandleShape shape, HandleTypeObject type,
                         HeapSlot *elements)
{
    
    
    
    gc::AllocKind kind = gc::FINALIZE_OBJECT0_BACKGROUND;

    ArrayObject *obj = createArrayInternal(cx, kind, heap, shape, type);
    if (!obj)
        return nullptr;

    obj->elements_ = elements;

    return finishCreateArray(obj, shape);
}

 inline ArrayObject *
ArrayObject::createCopyOnWriteArray(ExclusiveContext *cx, gc::InitialHeap heap,
                                    HandleShape shape,
                                    HandleNativeObject sharedElementsOwner)
{
    MOZ_ASSERT(sharedElementsOwner->getElementsHeader()->isCopyOnWrite());
    MOZ_ASSERT(sharedElementsOwner->getElementsHeader()->ownerObject() == sharedElementsOwner);

    
    
    
    gc::AllocKind kind = gc::FINALIZE_OBJECT0_BACKGROUND;

    RootedTypeObject type(cx, sharedElementsOwner->type());
    ArrayObject *obj = createArrayInternal(cx, kind, heap, shape, type);
    if (!obj)
        return nullptr;

    obj->elements_ = sharedElementsOwner->getDenseElementsAllowCopyOnWrite();

    return finishCreateArray(obj, shape);
}

} 

#endif 
