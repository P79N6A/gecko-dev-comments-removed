





#include "gc/Barrier.h"

#include "jscompartment.h"
#include "jsobj.h"

#include "gc/Zone.h"
#include "js/Value.h"
#include "vm/Symbol.h"

#ifdef DEBUG

bool
js::HeapSlot::preconditionForSet(NativeObject* owner, Kind kind, uint32_t slot)
{
    return kind == Slot
         ? &owner->getSlotRef(slot) == this
         : &owner->getDenseElement(slot) == (const Value*)this;
}

bool
js::HeapSlot::preconditionForWriteBarrierPost(NativeObject* obj, Kind kind, uint32_t slot,
                                              Value target) const
{
    return kind == Slot
         ? obj->getSlotAddressUnchecked(slot)->get() == target
         : static_cast<HeapSlot*>(obj->getDenseElements() + slot)->get() == target;
}

bool
js::RuntimeFromMainThreadIsHeapMajorCollecting(JS::shadow::Zone* shadowZone)
{
    return shadowZone->runtimeFromMainThread()->isHeapMajorCollecting();
}

bool
js::CurrentThreadIsIonCompiling()
{
    return TlsPerThreadData.get()->ionCompiling;
}

bool
js::CurrentThreadIsIonCompilingSafeForMinorGC()
{
    return TlsPerThreadData.get()->ionCompilingSafeForMinorGC;
}

bool
js::CurrentThreadIsGCSweeping()
{
    return js::TlsPerThreadData.get()->gcSweeping;
}

bool
js::CurrentThreadIsHandlingInitFailure()
{
    JSRuntime* rt = js::TlsPerThreadData.get()->runtimeIfOnOwnerThread();
    return rt && rt->handlingInitFailure;
}

#endif 

template <typename S>
template <typename T>
void
js::ReadBarrierFunctor<S>::operator()(T* t)
{
    InternalGCMethods<T*>::readBarrier(t);
}
template void js::ReadBarrierFunctor<JS::Value>::operator()<JS::Symbol>(JS::Symbol*);
template void js::ReadBarrierFunctor<JS::Value>::operator()<JSObject>(JSObject*);
template void js::ReadBarrierFunctor<JS::Value>::operator()<JSString>(JSString*);

template <typename S>
template <typename T>
void
js::PreBarrierFunctor<S>::operator()(T* t)
{
    InternalGCMethods<T*>::preBarrier(t);
}
template void js::PreBarrierFunctor<JS::Value>::operator()<JS::Symbol>(JS::Symbol*);
template void js::PreBarrierFunctor<JS::Value>::operator()<JSObject>(JSObject*);
template void js::PreBarrierFunctor<JS::Value>::operator()<JSString>(JSString*);
template void js::PreBarrierFunctor<jsid>::operator()<JS::Symbol>(JS::Symbol*);
template void js::PreBarrierFunctor<jsid>::operator()<JSString>(JSString*);

JS_PUBLIC_API(void)
JS::HeapObjectPostBarrier(JSObject** objp, JSObject* prev, JSObject* next)
{
    MOZ_ASSERT(objp);
    js::InternalGCMethods<JSObject*>::postBarrier(objp, prev, next);
}

JS_PUBLIC_API(void)
JS::HeapValuePostBarrier(JS::Value* valuep, const Value& prev, const Value& next)
{
    MOZ_ASSERT(valuep);
    js::InternalGCMethods<JS::Value>::postBarrier(valuep, prev, next);
}
