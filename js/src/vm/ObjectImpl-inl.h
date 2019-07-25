






#ifndef ObjectImpl_inl_h___
#define ObjectImpl_inl_h___

#include "mozilla/Assertions.h"

#include "jscell.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jsgcmark.h"

#include "js/TemplateLib.h"

#include "ObjectImpl.h"

inline bool
js::ObjectImpl::isNative() const
{
    return lastProperty()->isNative();
}

inline js::Class *
js::ObjectImpl::getClass() const
{
    return lastProperty()->getObjectClass();
}

inline JSClass *
js::ObjectImpl::getJSClass() const
{
    return Jsvalify(getClass());
}

inline bool
js::ObjectImpl::hasClass(const Class *c) const
{
    return getClass() == c;
}

inline const js::ObjectOps *
js::ObjectImpl::getOps() const
{
    return &getClass()->ops;
}

inline bool
js::ObjectImpl::isDelegate() const
{
    return lastProperty()->hasObjectFlag(BaseShape::DELEGATE);
}

inline bool
js::ObjectImpl::inDictionaryMode() const
{
    return lastProperty()->inDictionary();
}

 inline size_t
js::ObjectImpl::dynamicSlotsCount(size_t nfixed, size_t span)
{
    if (span <= nfixed)
        return 0;
    span -= nfixed;
    if (span <= SLOT_CAPACITY_MIN)
        return SLOT_CAPACITY_MIN;

    size_t slots = RoundUpPow2(span);
    MOZ_ASSERT(slots >= span);
    return slots;
}

inline size_t
js::ObjectImpl::sizeOfThis() const
{
    return arenaHeader()->getThingSize();
}

 inline void
js::ObjectImpl::readBarrier(ObjectImpl *obj)
{
#ifdef JSGC_INCREMENTAL
    JSCompartment *comp = obj->compartment();
    if (comp->needsBarrier()) {
        MOZ_ASSERT(!comp->rt->gcRunning);
        JSObject *tmp = obj->asObjectPtr();
        MarkObjectUnbarriered(comp->barrierTracer(), &tmp, "read barrier");
        JS_ASSERT(tmp == obj->asObjectPtr());
    }
#endif
}

inline void
js::ObjectImpl::privateWriteBarrierPre(void **old)
{
#ifdef JSGC_INCREMENTAL
    JSCompartment *comp = compartment();
    if (comp->needsBarrier()) {
        if (*old && getClass()->trace)
            getClass()->trace(comp->barrierTracer(), this->asObjectPtr());
    }
#endif
}

inline void
js::ObjectImpl::privateWriteBarrierPost(void **old)
{
}

 inline void
js::ObjectImpl::writeBarrierPre(ObjectImpl *obj)
{
#ifdef JSGC_INCREMENTAL
    



    if (uintptr_t(obj) < 32)
        return;

    JSCompartment *comp = obj->compartment();
    if (comp->needsBarrier()) {
        MOZ_ASSERT(!comp->rt->gcRunning);
        JSObject *tmp = obj->asObjectPtr();
        MarkObjectUnbarriered(comp->barrierTracer(), &tmp, "write barrier");
        JS_ASSERT(tmp == obj->asObjectPtr());
    }
#endif
}

 inline void
js::ObjectImpl::writeBarrierPost(ObjectImpl *obj, void *addr)
{
}

inline bool
js::ObjectImpl::isExtensible() const
{
    return !lastProperty()->hasObjectFlag(BaseShape::NOT_EXTENSIBLE);
}

#endif 
