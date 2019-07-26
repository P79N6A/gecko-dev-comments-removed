





#include "gc/Barrier.h"

#include "jscompartment.h"
#include "jsobj.h"

#include "gc/Zone.h"

namespace js {

void
ValueReadBarrier(const Value &value)
{
    JS_ASSERT(!CurrentThreadIsIonCompiling());
    if (value.isObject())
        JSObject::readBarrier(&value.toObject());
    else if (value.isString())
        JSString::readBarrier(value.toString());
    else
        JS_ASSERT(!value.isMarkable());
}

#ifdef DEBUG
bool
HeapSlot::preconditionForSet(JSObject *owner, Kind kind, uint32_t slot)
{
    return kind == Slot
         ? &owner->getSlotRef(slot) == this
         : &owner->getDenseElement(slot) == (const Value *)this;
}

bool
HeapSlot::preconditionForSet(Zone *zone, JSObject *owner, Kind kind, uint32_t slot)
{
    bool ok = kind == Slot
            ? &owner->getSlotRef(slot) == this
            : &owner->getDenseElement(slot) == (const Value *)this;
    return ok && owner->zone() == zone;
}

bool
HeapSlot::preconditionForWriteBarrierPost(JSObject *obj, Kind kind, uint32_t slot, Value target) const
{
    return kind == Slot
         ? obj->getSlotAddressUnchecked(slot)->get() == target
         : static_cast<HeapSlot *>(obj->getDenseElements() + slot)->get() == target;
}

bool
RuntimeFromMainThreadIsHeapMajorCollecting(JS::shadow::Zone *shadowZone)
{
    return shadowZone->runtimeFromMainThread()->isHeapMajorCollecting();
}

bool
CurrentThreadIsIonCompiling()
{
    return TlsPerThreadData.get()->ionCompiling;
}
#endif 

bool
StringIsPermanentAtom(JSString *str)
{
    return str->isPermanentAtom();
}

} 
