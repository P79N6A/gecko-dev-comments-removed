






#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

#include "jsscope.h"
#include "jsobjinlines.h"

#include "ObjectImpl.h"

#include "gc/Barrier-inl.h"

#include "ObjectImpl-inl.h"

using namespace js;

static ObjectElements emptyElementsHeader(0, 0);


HeapSlot *js::emptyObjectElements =
    reinterpret_cast<HeapSlot *>(uintptr_t(&emptyElementsHeader) + sizeof(ObjectElements));

void
js::ObjectImpl::initSlotRange(size_t start, const Value *vector, size_t length)
{
    JSCompartment *comp = compartment();
    HeapSlot *fixedStart, *fixedEnd, *slotsStart, *slotsEnd;
    getSlotRange(start, length, &fixedStart, &fixedEnd, &slotsStart, &slotsEnd);
    for (HeapSlot *sp = fixedStart; sp < fixedEnd; sp++)
        sp->init(comp, this->asObjectPtr(), start++, *vector++);
    for (HeapSlot *sp = slotsStart; sp < slotsEnd; sp++)
        sp->init(comp, this->asObjectPtr(), start++, *vector++);
}

void
js::ObjectImpl::copySlotRange(size_t start, const Value *vector, size_t length)
{
    JSCompartment *comp = compartment();
    HeapSlot *fixedStart, *fixedEnd, *slotsStart, *slotsEnd;
    getSlotRange(start, length, &fixedStart, &fixedEnd, &slotsStart, &slotsEnd);
    for (HeapSlot *sp = fixedStart; sp < fixedEnd; sp++)
        sp->set(comp, this->asObjectPtr(), start++, *vector++);
    for (HeapSlot *sp = slotsStart; sp < slotsEnd; sp++)
        sp->set(comp, this->asObjectPtr(), start++, *vector++);
}

#ifdef DEBUG
bool
js::ObjectImpl::slotInRange(unsigned slot, SentinelAllowed sentinel) const
{
    size_t capacity = numFixedSlots() + numDynamicSlots();
    if (sentinel == SENTINEL_ALLOWED)
        return slot <= capacity;
    return slot < capacity;
}
#endif 

#if defined(_MSC_VER) && _MSC_VER >= 1500





MOZ_NEVER_INLINE
#endif
const Shape *
js::ObjectImpl::nativeLookup(JSContext *cx, jsid id)
{
    MOZ_ASSERT(isNative());
    Shape **spp;
    return Shape::search(cx, lastProperty(), id, &spp);
}

void
js::ObjectImpl::markChildren(JSTracer *trc)
{
    MarkTypeObject(trc, &type_, "type");

    MarkShape(trc, &shape_, "shape");

    Class *clasp = shape_->getObjectClass();
    JSObject *obj = asObjectPtr();
    if (clasp->trace)
        clasp->trace(trc, obj);

    if (shape_->isNative())
        MarkObjectSlots(trc, obj, 0, obj->slotSpan());
}
