





#include "vm/ReceiverGuard.h"

#include "vm/UnboxedObject.h"

ReceiverGuard::ReceiverGuard(JSObject* obj)
  : group(nullptr), shape(nullptr)
{
    if (obj) {
        if (obj->is<UnboxedPlainObject>()) {
            group = obj->group();
            if (UnboxedExpandoObject* expando = obj->as<UnboxedPlainObject>().maybeExpando())
                shape = expando->lastProperty();
        } else if (obj->is<UnboxedArrayObject>() || obj->is<TypedObject>()) {
            group = obj->group();
        } else {
            shape = obj->maybeShape();
        }
    }
}

ReceiverGuard::ReceiverGuard(ObjectGroup* group, Shape* shape)
  : group(group), shape(shape)
{
    if (group) {
        const Class* clasp = group->clasp();
        if (clasp == &UnboxedPlainObject::class_) {
            
        } else if (clasp == &UnboxedArrayObject::class_ || IsTypedObjectClass(clasp)) {
            this->shape = nullptr;
        } else {
            this->group = nullptr;
        }
    }
}

 int32_t
HeapReceiverGuard::keyBits(JSObject* obj)
{
    if (obj->is<UnboxedPlainObject>()) {
        
        return obj->as<UnboxedPlainObject>().maybeExpando() ? 0 : 1;
    }
    if (obj->is<UnboxedArrayObject>() || obj->is<TypedObject>()) {
        
        return 2;
    }
    
    return 3;
}

void
HeapReceiverGuard::trace(JSTracer* trc)
{
    if (shape_)
        TraceEdge(trc, &shape_, "receiver_guard_shape");
    else
        TraceEdge(trc, &group_, "receiver_guard_group");
}
