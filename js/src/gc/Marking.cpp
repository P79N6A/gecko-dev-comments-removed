





#include "gc/Marking.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/IntegerRange.h"
#include "mozilla/ReentrancyGuard.h"
#include "mozilla/TypeTraits.h"

#include "jsgc.h"
#include "jsprf.h"

#include "gc/GCInternals.h"
#include "jit/IonCode.h"
#include "js/SliceBudget.h"
#include "vm/ArgumentsObject.h"
#include "vm/ArrayObject.h"
#include "vm/ScopeObject.h"
#include "vm/Shape.h"
#include "vm/Symbol.h"
#include "vm/TypedArrayObject.h"
#include "vm/UnboxedObject.h"

#include "jscompartmentinlines.h"
#include "jsobjinlines.h"

#include "gc/Nursery-inl.h"
#include "vm/String-inl.h"
#include "vm/UnboxedObject-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::ArrayLength;
using mozilla::DebugOnly;
using mozilla::IsBaseOf;
using mozilla::IsSame;
using mozilla::MakeRange;
using mozilla::PodCopy;




































































#if defined(DEBUG)
template<typename T>
static inline bool
IsThingPoisoned(T* thing)
{
    const uint8_t poisonBytes[] = {
        JS_FRESH_NURSERY_PATTERN,
        JS_SWEPT_NURSERY_PATTERN,
        JS_ALLOCATED_NURSERY_PATTERN,
        JS_FRESH_TENURED_PATTERN,
        JS_SWEPT_TENURED_PATTERN,
        JS_ALLOCATED_TENURED_PATTERN,
        JS_SWEPT_CODE_PATTERN,
        JS_SWEPT_FRAME_PATTERN
    };
    const int numPoisonBytes = sizeof(poisonBytes) / sizeof(poisonBytes[0]);
    uint32_t* p = reinterpret_cast<uint32_t*>(reinterpret_cast<FreeSpan*>(thing) + 1);
    
    if ((*p & 1) == 0)
        return false;
    for (int i = 0; i < numPoisonBytes; ++i) {
        const uint8_t pb = poisonBytes[i];
        const uint32_t pw = pb | (pb << 8) | (pb << 16) | (pb << 24);
        if (*p == pw)
            return true;
    }
    return false;
}

static bool
IsMovingTracer(JSTracer *trc)
{
    return trc->isCallbackTracer() &&
           trc->asCallbackTracer()->getTracerKind() == JS::CallbackTracer::TracerKind::Moving;
}
#endif

template <typename T> bool ThingIsPermanentAtomOrWellKnownSymbol(T* thing) { return false; }
template <> bool ThingIsPermanentAtomOrWellKnownSymbol<JSString>(JSString* str) {
    return str->isPermanentAtom();
}
template <> bool ThingIsPermanentAtomOrWellKnownSymbol<JSFlatString>(JSFlatString* str) {
    return str->isPermanentAtom();
}
template <> bool ThingIsPermanentAtomOrWellKnownSymbol<JSLinearString>(JSLinearString* str) {
    return str->isPermanentAtom();
}
template <> bool ThingIsPermanentAtomOrWellKnownSymbol<JSAtom>(JSAtom* atom) {
    return atom->isPermanent();
}
template <> bool ThingIsPermanentAtomOrWellKnownSymbol<PropertyName>(PropertyName* name) {
    return name->isPermanent();
}
template <> bool ThingIsPermanentAtomOrWellKnownSymbol<JS::Symbol>(JS::Symbol* sym) {
    return sym->isWellKnownSymbol();
}

template<typename T>
void
js::CheckTracedThing(JSTracer* trc, T thing)
{
#ifdef DEBUG
    MOZ_ASSERT(trc);
    MOZ_ASSERT(thing);

    thing = MaybeForwarded(thing);

    
    if (IsInsideNursery(thing))
        return;

    MOZ_ASSERT_IF(!IsMovingTracer(trc) && !trc->isTenuringTracer(), !IsForwarded(thing));

    



    if (ThingIsPermanentAtomOrWellKnownSymbol(thing))
        return;

    Zone* zone = thing->zoneFromAnyThread();
    JSRuntime* rt = trc->runtime();

    MOZ_ASSERT_IF(!IsMovingTracer(trc), CurrentThreadCanAccessZone(zone));
    MOZ_ASSERT_IF(!IsMovingTracer(trc), CurrentThreadCanAccessRuntime(rt));

    MOZ_ASSERT(zone->runtimeFromAnyThread() == trc->runtime());

    MOZ_ASSERT(thing->isAligned());
    MOZ_ASSERT(MapTypeToTraceKind<typename mozilla::RemovePointer<T>::Type>::kind ==
               thing->getTraceKind());

    



    bool isGcMarkingTracer = trc->isMarkingTracer();

    MOZ_ASSERT_IF(zone->requireGCTracer(), isGcMarkingTracer || IsBufferGrayRootsTracer(trc));

    if (isGcMarkingTracer) {
        GCMarker* gcMarker = static_cast<GCMarker*>(trc);
        MOZ_ASSERT_IF(gcMarker->shouldCheckCompartments(),
                      zone->isCollecting() || zone->isAtomsZone());

        MOZ_ASSERT_IF(gcMarker->markColor() == GRAY,
                      !zone->isGCMarkingBlack() || zone->isAtomsZone());

        MOZ_ASSERT(!(zone->isGCSweeping() || zone->isGCFinished() || zone->isGCCompacting()));
    }

    







    MOZ_ASSERT_IF(IsThingPoisoned(thing) && rt->isHeapBusy() && !rt->gc.isBackgroundSweeping(),
                  !InFreeList(thing->asTenured().arenaHeader(), thing));
#endif
}

template <typename S>
struct CheckTracedFunctor : public VoidDefaultAdaptor<S> {
    template <typename T> void operator()(T* t, JSTracer* trc) { CheckTracedThing(trc, t); }
};

namespace js {
template<>
void
CheckTracedThing<Value>(JSTracer* trc, Value val)
{
    DispatchValueTyped(CheckTracedFunctor<Value>(), val, trc);
}

template <>
void
CheckTracedThing<jsid>(JSTracer* trc, jsid id)
{
    DispatchIdTyped(CheckTracedFunctor<jsid>(), id, trc);
}

#define IMPL_CHECK_TRACED_THING(_, type, __) \
    template void CheckTracedThing<type*>(JSTracer*, type*);
FOR_EACH_GC_LAYOUT(IMPL_CHECK_TRACED_THING);
#undef IMPL_CHECK_TRACED_THING
} 

static bool
ShouldMarkCrossCompartment(JSTracer* trc, JSObject* src, Cell* cell)
{
    if (!trc->isMarkingTracer())
        return true;

    uint32_t color = static_cast<GCMarker*>(trc)->markColor();
    MOZ_ASSERT(color == BLACK || color == GRAY);

    if (!cell->isTenured()) {
        MOZ_ASSERT(color == BLACK);
        return false;
    }
    TenuredCell& tenured = cell->asTenured();

    JS::Zone* zone = tenured.zone();
    if (color == BLACK) {
        






        if (tenured.isMarked(GRAY)) {
            MOZ_ASSERT(!zone->isCollecting());
            trc->runtime()->gc.setFoundBlackGrayEdges();
        }
        return zone->isGCMarking();
    } else {
        if (zone->isGCMarkingBlack()) {
            




            if (!tenured.isMarked())
                DelayCrossCompartmentGrayMarking(src);
            return false;
        }
        return zone->isGCMarkingGray();
    }
}

static bool
ShouldMarkCrossCompartment(JSTracer* trc, JSObject* src, Value val)
{
    return val.isMarkable() && ShouldMarkCrossCompartment(trc, src, (Cell*)val.toGCThing());
}

static void
AssertZoneIsMarking(Cell* thing)
{
    MOZ_ASSERT(TenuredCell::fromPointer(thing)->zone()->isGCMarking());
}

static void
AssertZoneIsMarking(JSString* str)
{
#ifdef DEBUG
    Zone* zone = TenuredCell::fromPointer(str)->zone();
    MOZ_ASSERT(zone->isGCMarking() || zone->isAtomsZone());
#endif
}

static void
AssertZoneIsMarking(JS::Symbol* sym)
{
#ifdef DEBUG
    Zone* zone = TenuredCell::fromPointer(sym)->zone();
    MOZ_ASSERT(zone->isGCMarking() || zone->isAtomsZone());
#endif
}

static void
AssertRootMarkingPhase(JSTracer* trc)
{
    MOZ_ASSERT_IF(trc->isMarkingTracer(),
                  trc->runtime()->gc.state() == NO_INCREMENTAL ||
                  trc->runtime()->gc.state() == MARK_ROOTS);
}




#define FOR_EACH_GC_POINTER_TYPE(D) \
    D(AccessorShape*) \
    D(BaseShape*) \
    D(UnownedBaseShape*) \
    D(jit::JitCode*) \
    D(NativeObject*) \
    D(ArrayObject*) \
    D(ArgumentsObject*) \
    D(ArrayBufferObject*) \
    D(ArrayBufferObjectMaybeShared*) \
    D(ArrayBufferViewObject*) \
    D(DebugScopeObject*) \
    D(GlobalObject*) \
    D(JSObject*) \
    D(JSFunction*) \
    D(NestedScopeObject*) \
    D(PlainObject*) \
    D(SavedFrame*) \
    D(ScopeObject*) \
    D(ScriptSourceObject*) \
    D(SharedArrayBufferObject*) \
    D(SharedTypedArrayObject*) \
    D(JSScript*) \
    D(LazyScript*) \
    D(Shape*) \
    D(JSAtom*) \
    D(JSString*) \
    D(JSFlatString*) \
    D(JSLinearString*) \
    D(PropertyName*) \
    D(JS::Symbol*) \
    D(js::ObjectGroup*) \
    D(Value) \
    D(jsid)













template <typename T,
          JS::TraceKind = IsBaseOf<JSObject, T>::value     ? JS::TraceKind::Object
                        : IsBaseOf<JSString, T>::value     ? JS::TraceKind::String
                        : IsBaseOf<JS::Symbol, T>::value   ? JS::TraceKind::Symbol
                        : IsBaseOf<JSScript, T>::value     ? JS::TraceKind::Script
                        : IsBaseOf<Shape, T>::value        ? JS::TraceKind::Shape
                        : IsBaseOf<BaseShape, T>::value    ? JS::TraceKind::BaseShape
                        : IsBaseOf<jit::JitCode, T>::value ? JS::TraceKind::JitCode
                        : IsBaseOf<LazyScript, T>::value   ? JS::TraceKind::LazyScript
                        :                                    JS::TraceKind::ObjectGroup>
struct BaseGCType;
#define IMPL_BASE_GC_TYPE(name, type_, _) \
    template <typename T> struct BaseGCType<T, JS::TraceKind:: name> { typedef type_ type; };
FOR_EACH_GC_LAYOUT(IMPL_BASE_GC_TYPE);
#undef IMPL_BASE_GC_TYPE





template <typename T> struct PtrBaseGCType {};
template <> struct PtrBaseGCType<Value> { typedef Value type; };
template <> struct PtrBaseGCType<jsid> { typedef jsid type; };
template <typename T> struct PtrBaseGCType<T*> { typedef typename BaseGCType<T>::type* type; };

template <typename T>
typename PtrBaseGCType<T>::type*
ConvertToBase(T* thingp)
{
    return reinterpret_cast<typename PtrBaseGCType<T>::type*>(thingp);
}

template <typename T> void DispatchToTracer(JSTracer* trc, T* thingp, const char* name);
template <typename T> T DoCallback(JS::CallbackTracer* trc, T* thingp, const char* name);
template <typename T> void DoMarking(GCMarker* gcmarker, T thing);

template <typename T>
void
js::TraceEdge(JSTracer* trc, BarrieredBase<T>* thingp, const char* name)
{
    DispatchToTracer(trc, ConvertToBase(thingp->unsafeGet()), name);
}

template <typename T>
void
js::TraceManuallyBarrieredEdge(JSTracer* trc, T* thingp, const char* name)
{
    DispatchToTracer(trc, ConvertToBase(thingp), name);
}

template <typename T>
void
js::TraceRoot(JSTracer* trc, T* thingp, const char* name)
{
    AssertRootMarkingPhase(trc);
    DispatchToTracer(trc, ConvertToBase(thingp), name);
}

template <typename T>
void
js::TraceRange(JSTracer* trc, size_t len, BarrieredBase<T>* vec, const char* name)
{
    JS::AutoTracingIndex index(trc);
    for (auto i : MakeRange(len)) {
        if (InternalGCMethods<T>::isMarkable(vec[i].get()))
            DispatchToTracer(trc, ConvertToBase(vec[i].unsafeGet()), name);
        ++index;
    }
}

template <typename T>
void
js::TraceRootRange(JSTracer* trc, size_t len, T* vec, const char* name)
{
    AssertRootMarkingPhase(trc);
    JS::AutoTracingIndex index(trc);
    for (auto i : MakeRange(len)) {
        if (InternalGCMethods<T>::isMarkable(vec[i]))
            DispatchToTracer(trc, ConvertToBase(&vec[i]), name);
        ++index;
    }
}


#define INSTANTIATE_ALL_VALID_TRACE_FUNCTIONS(type) \
    template void js::TraceEdge<type>(JSTracer*, BarrieredBase<type>*, const char*); \
    template void js::TraceManuallyBarrieredEdge<type>(JSTracer*, type*, const char*); \
    template void js::TraceRoot<type>(JSTracer*, type*, const char*); \
    template void js::TraceRange<type>(JSTracer*, size_t, BarrieredBase<type>*, const char*); \
    template void js::TraceRootRange<type>(JSTracer*, size_t, type*, const char*);
FOR_EACH_GC_POINTER_TYPE(INSTANTIATE_ALL_VALID_TRACE_FUNCTIONS)
#undef INSTANTIATE_ALL_VALID_TRACE_FUNCTIONS

template <typename T>
void
js::TraceManuallyBarrieredCrossCompartmentEdge(JSTracer* trc, JSObject* src, T* dst,
                                               const char* name)
{
    if (ShouldMarkCrossCompartment(trc, src, *dst))
        DispatchToTracer(trc, dst, name);
}
template void js::TraceManuallyBarrieredCrossCompartmentEdge<JSObject*>(JSTracer*, JSObject*,
                                                                        JSObject**, const char*);
template void js::TraceManuallyBarrieredCrossCompartmentEdge<JSScript*>(JSTracer*, JSObject*,
                                                                        JSScript**, const char*);

template <typename T>
void
js::TraceCrossCompartmentEdge(JSTracer* trc, JSObject* src, BarrieredBase<T>* dst, const char* name)
{
    if (ShouldMarkCrossCompartment(trc, src, dst->get()))
        DispatchToTracer(trc, dst->unsafeGet(), name);
}
template void js::TraceCrossCompartmentEdge<Value>(JSTracer*, JSObject*, BarrieredBase<Value>*,
                                                   const char*);

template <typename T>
void
js::TraceProcessGlobalRoot(JSTracer* trc, T* thing, const char* name)
{
    AssertRootMarkingPhase(trc);
    MOZ_ASSERT(ThingIsPermanentAtomOrWellKnownSymbol(thing));

    
    
    
    
    
    
    CheckTracedThing(trc, *ConvertToBase(&thing));
    if (trc->isMarkingTracer())
        thing->markIfUnmarked(gc::BLACK);
    else
        DoCallback(trc->asCallbackTracer(), ConvertToBase(&thing), name);
}
template void js::TraceProcessGlobalRoot<JSAtom>(JSTracer*, JSAtom*, const char*);
template void js::TraceProcessGlobalRoot<JS::Symbol>(JSTracer*, JS::Symbol*, const char*);


struct TraceRootFunctor {
    template <typename T>
    void operator()(JSTracer* trc, Cell** thingp, const char* name) {
        TraceRoot(trc, reinterpret_cast<T**>(thingp), name);
    }
};

void
js::TraceGenericPointerRoot(JSTracer* trc, Cell** thingp, const char* name)
{
    MOZ_ASSERT(thingp);
    if (!*thingp)
        return;
    TraceRootFunctor f;
    CallTyped(f, (*thingp)->getTraceKind(), trc, thingp, name);
}


struct TraceManuallyBarrieredEdgeFunctor {
    template <typename T>
    void operator()(JSTracer* trc, Cell** thingp, const char* name) {
        TraceManuallyBarrieredEdge(trc, reinterpret_cast<T**>(thingp), name);
    }
};

void
js::TraceManuallyBarrieredGenericPointerEdge(JSTracer* trc, Cell** thingp, const char* name)
{
    MOZ_ASSERT(thingp);
    if (!*thingp)
        return;
    TraceManuallyBarrieredEdgeFunctor f;
    CallTyped(f, (*thingp)->getTraceKind(), trc, thingp, name);
}




template <typename T>
void
DispatchToTracer(JSTracer* trc, T* thingp, const char* name)
{
#define IS_SAME_TYPE_OR(name, type, _) mozilla::IsSame<type*, T>::value ||
    static_assert(
            FOR_EACH_GC_LAYOUT(IS_SAME_TYPE_OR)
            mozilla::IsSame<T, JS::Value>::value ||
            mozilla::IsSame<T, jsid>::value,
            "Only the base cell layout types are allowed into marking/tracing internals");
#undef IS_SAME_TYPE_OR
    if (trc->isMarkingTracer())
        return DoMarking(static_cast<GCMarker*>(trc), *thingp);
    if (trc->isTenuringTracer())
        return static_cast<TenuringTracer*>(trc)->traverse(thingp);
    MOZ_ASSERT(trc->isCallbackTracer());
    DoCallback(trc->asCallbackTracer(), thingp, name);
}




template <typename T>
static inline bool
MustSkipMarking(T thing)
{
    
    return !thing->zone()->isGCMarking();
}

template <>
bool
MustSkipMarking<JSObject*>(JSObject* obj)
{
    
    
    
    
    if (IsInsideNursery(obj))
        return true;

    
    
    
    return !TenuredCell::fromPointer(obj)->zone()->isGCMarking();
}

template <>
bool
MustSkipMarking<JSString*>(JSString* str)
{
    
    
    
    
    return str->isPermanentAtom() ||
           !str->zone()->isGCMarking();
}

template <>
bool
MustSkipMarking<JS::Symbol*>(JS::Symbol* sym)
{
    
    
    return sym->isWellKnownSymbol() ||
           !sym->zone()->isGCMarking();
}

template <typename T>
void
DoMarking(GCMarker* gcmarker, T thing)
{
    
    if (MustSkipMarking(thing))
        return;

    CheckTracedThing(gcmarker, thing);
    gcmarker->traverse(thing);

    
    SetMaybeAliveFlag(thing);
}

template <typename S>
struct DoMarkingFunctor : public VoidDefaultAdaptor<S> {
    template <typename T> void operator()(T* t, GCMarker* gcmarker) { DoMarking(gcmarker, t); }
};

template <>
void
DoMarking<Value>(GCMarker* gcmarker, Value val)
{
    DispatchValueTyped(DoMarkingFunctor<Value>(), val, gcmarker);
}

template <>
void
DoMarking<jsid>(GCMarker* gcmarker, jsid id)
{
    DispatchIdTyped(DoMarkingFunctor<jsid>(), id, gcmarker);
}






template <typename T>
void
js::GCMarker::markAndTraceChildren(T* thing)
{
    if (ThingIsPermanentAtomOrWellKnownSymbol(thing))
        return;
    if (mark(thing))
        thing->traceChildren(this);
}
namespace js {
template <> void GCMarker::traverse(BaseShape* thing) { markAndTraceChildren(thing); }
template <> void GCMarker::traverse(JS::Symbol* thing) { markAndTraceChildren(thing); }
template <> void GCMarker::traverse(JSScript* thing) { markAndTraceChildren(thing); }
} 




template <typename T>
void
js::GCMarker::markAndScan(T* thing)
{
    if (ThingIsPermanentAtomOrWellKnownSymbol(thing))
        return;
    if (mark(thing))
        eagerlyMarkChildren(thing);
}
namespace js {
template <> void GCMarker::traverse(JSString* thing) { markAndScan(thing); }
template <> void GCMarker::traverse(LazyScript* thing) { markAndScan(thing); }
template <> void GCMarker::traverse(Shape* thing) { markAndScan(thing); }
} 





template <typename T>
void
js::GCMarker::markAndPush(StackTag tag, T* thing)
{
    if (mark(thing))
        pushTaggedPtr(tag, thing);
}
namespace js {
template <> void GCMarker::traverse(JSObject* thing) { markAndPush(ObjectTag, thing); }
template <> void GCMarker::traverse(ObjectGroup* thing) { markAndPush(GroupTag, thing); }
template <> void GCMarker::traverse(jit::JitCode* thing) { markAndPush(JitCodeTag, thing); }
} 

namespace js {
template <>
void
GCMarker::traverse(AccessorShape* thing) {
    MOZ_CRASH("AccessorShape must be marked as a Shape");
}
} 

template <typename S, typename T>
void
js::GCMarker::traverseEdge(S source, T target)
{
    
    MOZ_ASSERT(!ThingIsPermanentAtomOrWellKnownSymbol(source));

    
    MOZ_ASSERT_IF(!ThingIsPermanentAtomOrWellKnownSymbol(target),
                  target->zone()->isAtomsZone() || target->zone() == source->zone());

    
    
    MOZ_ASSERT_IF(ThingIsPermanentAtomOrWellKnownSymbol(target), !target->maybeCompartment());
    MOZ_ASSERT_IF(target->zoneFromAnyThread()->isAtomsZone(), !target->maybeCompartment());
    
    MOZ_ASSERT_IF(source->maybeCompartment() && target->maybeCompartment(),
                  source->maybeCompartment() == target->maybeCompartment());

    traverse(target);
}

template <typename V, typename S> struct TraverseEdgeFunctor : public VoidDefaultAdaptor<V> {
    template <typename T> void operator()(T t, GCMarker* gcmarker, S s) {
        return gcmarker->traverseEdge(s, t);
    }
};

template <typename S>
void
js::GCMarker::traverseEdge(S source, jsid id)
{
    DispatchIdTyped(TraverseEdgeFunctor<jsid, S>(), id, this, source);
}

template <typename S>
void
js::GCMarker::traverseEdge(S source, Value v)
{
    DispatchValueTyped(TraverseEdgeFunctor<Value, S>(), v, this, source);
}

template <typename T>
bool
js::GCMarker::mark(T* thing)
{
    AssertZoneIsMarking(thing);
    MOZ_ASSERT(!IsInsideNursery(gc::TenuredCell::fromPointer(thing)));
    return gc::ParticipatesInCC<T>::value
           ? gc::TenuredCell::fromPointer(thing)->markIfUnmarked(markColor())
           : gc::TenuredCell::fromPointer(thing)->markIfUnmarked(gc::BLACK);
}








void
LazyScript::traceChildren(JSTracer* trc)
{
    if (function_)
        TraceEdge(trc, &function_, "function");

    if (sourceObject_)
        TraceEdge(trc, &sourceObject_, "sourceObject");

    if (enclosingScope_)
        TraceEdge(trc, &enclosingScope_, "enclosingScope");

    
    FreeVariable* freeVariables = this->freeVariables();
    for (auto i : MakeRange(numFreeVariables())) {
        JSAtom* atom = freeVariables[i].atom();
        TraceManuallyBarrieredEdge(trc, &atom, "lazyScriptFreeVariable");
    }

    HeapPtrFunction* innerFunctions = this->innerFunctions();
    for (auto i : MakeRange(numInnerFunctions()))
        TraceEdge(trc, &innerFunctions[i], "lazyScriptInnerFunction");
}
inline void
js::GCMarker::eagerlyMarkChildren(LazyScript *thing)
{
    if (thing->function_)
        traverseEdge(thing, static_cast<JSObject*>(thing->function_));

    if (thing->sourceObject_)
        traverseEdge(thing, static_cast<JSObject*>(thing->sourceObject_));

    if (thing->enclosingScope_)
        traverseEdge(thing, static_cast<JSObject*>(thing->enclosingScope_));

    
    LazyScript::FreeVariable* freeVariables = thing->freeVariables();
    for (auto i : MakeRange(thing->numFreeVariables()))
        traverseEdge(thing, static_cast<JSString*>(freeVariables[i].atom()));

    HeapPtrFunction* innerFunctions = thing->innerFunctions();
    for (auto i : MakeRange(thing->numInnerFunctions()))
        traverseEdge(thing, static_cast<JSObject*>(innerFunctions[i]));
}

void
Shape::traceChildren(JSTracer* trc)
{
    TraceEdge(trc, &base_, "base");
    TraceEdge(trc, &propidRef(), "propid");
    if (parent)
        TraceEdge(trc, &parent, "parent");

    if (hasGetterObject())
        TraceManuallyBarrieredEdge(trc, &asAccessorShape().getterObj, "getter");
    if (hasSetterObject())
        TraceManuallyBarrieredEdge(trc, &asAccessorShape().setterObj, "setter");
}
inline void
js::GCMarker::eagerlyMarkChildren(Shape* shape)
{
    MOZ_ASSERT(shape->isMarked(this->markColor()));
    do {
        traverseEdge(shape, shape->base());
        traverseEdge(shape, shape->propidRef().get());

        
        
        
        if (shape->hasGetterObject() && shape->getterObject()->isTenured())
            traverseEdge(shape, shape->getterObject());
        if (shape->hasSetterObject() && shape->setterObject()->isTenured())
            traverseEdge(shape, shape->setterObject());

        shape = shape->previous();
    } while (shape && mark(shape));
}

void
JSString::traceChildren(JSTracer* trc)
{
    if (hasBase())
        traceBase(trc);
    else if (isRope())
        asRope().traceChildren(trc);
}
inline void
GCMarker::eagerlyMarkChildren(JSString* str)
{
    if (str->isLinear())
        eagerlyMarkChildren(&str->asLinear());
    else
        eagerlyMarkChildren(&str->asRope());
}

void
JSString::traceBase(JSTracer* trc)
{
    MOZ_ASSERT(hasBase());
    TraceManuallyBarrieredEdge(trc, &d.s.u3.base, "base");
}
inline void
js::GCMarker::eagerlyMarkChildren(JSLinearString* linearStr)
{
    AssertZoneIsMarking(linearStr);
    MOZ_ASSERT(linearStr->isMarked());
    MOZ_ASSERT(linearStr->JSString::isLinear());

    
    while (linearStr->hasBase()) {
        linearStr = linearStr->base();
        MOZ_ASSERT(linearStr->JSString::isLinear());
        if (linearStr->isPermanentAtom())
            break;
        AssertZoneIsMarking(linearStr);
        if (!mark(static_cast<JSString*>(linearStr)))
            break;
    }
}

void
JSRope::traceChildren(JSTracer* trc) {
    js::TraceManuallyBarrieredEdge(trc, &d.s.u2.left, "left child");
    js::TraceManuallyBarrieredEdge(trc, &d.s.u3.right, "right child");
}
inline void
js::GCMarker::eagerlyMarkChildren(JSRope* rope)
{
    
    
    
    
    
    
    
    
    ptrdiff_t savedPos = stack.position();
    JS_DIAGNOSTICS_ASSERT(rope->getTraceKind() == JS::TraceKind::String);
    while (true) {
        JS_DIAGNOSTICS_ASSERT(rope->getTraceKind() == JS::TraceKind::String);
        JS_DIAGNOSTICS_ASSERT(rope->JSString::isRope());
        AssertZoneIsMarking(rope);
        MOZ_ASSERT(rope->isMarked());
        JSRope* next = nullptr;

        JSString* right = rope->rightChild();
        if (!right->isPermanentAtom() &&
            mark(right))
        {
            if (right->isLinear())
                eagerlyMarkChildren(&right->asLinear());
            else
                next = &right->asRope();
        }

        JSString* left = rope->leftChild();
        if (!left->isPermanentAtom() &&
            mark(left))
        {
            if (left->isLinear()) {
                eagerlyMarkChildren(&left->asLinear());
            } else {
                
                
                if (next && !stack.push(reinterpret_cast<uintptr_t>(next)))
                    delayMarkingChildren(next);
                next = &left->asRope();
            }
        }
        if (next) {
            rope = next;
        } else if (savedPos != stack.position()) {
            MOZ_ASSERT(savedPos < stack.position());
            rope = reinterpret_cast<JSRope*>(stack.pop());
        } else {
            break;
        }
    }
    MOZ_ASSERT(savedPos == stack.position());
}

void
js::ObjectGroup::traceChildren(JSTracer* trc)
{
    unsigned count = getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        if (ObjectGroup::Property* prop = getProperty(i))
            TraceEdge(trc, &prop->id, "group_property");
    }

    if (proto().isObject())
        TraceEdge(trc, &protoRaw(), "group_proto");

    if (newScript())
        newScript()->trace(trc);

    if (maybePreliminaryObjects())
        maybePreliminaryObjects()->trace(trc);

    if (maybeUnboxedLayout())
        unboxedLayout().trace(trc);

    if (ObjectGroup* unboxedGroup = maybeOriginalUnboxedGroup()) {
        TraceManuallyBarrieredEdge(trc, &unboxedGroup, "group_original_unboxed_group");
        setOriginalUnboxedGroup(unboxedGroup);
    }

    if (JSObject* descr = maybeTypeDescr()) {
        TraceManuallyBarrieredEdge(trc, &descr, "group_type_descr");
        setTypeDescr(&descr->as<TypeDescr>());
    }

    if (JSObject* fun = maybeInterpretedFunction()) {
        TraceManuallyBarrieredEdge(trc, &fun, "group_function");
        setInterpretedFunction(&fun->as<JSFunction>());
    }
}
void
js::GCMarker::lazilyMarkChildren(ObjectGroup* group)
{
    unsigned count = group->getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        if (ObjectGroup::Property* prop = group->getProperty(i))
            traverseEdge(group, prop->id.get());
    }

    if (group->proto().isObject())
        traverseEdge(group, group->proto().toObject());

    group->compartment()->mark();

    if (GlobalObject* global = group->compartment()->unsafeUnbarrieredMaybeGlobal())
        traverseEdge(group, static_cast<JSObject*>(global));

    if (group->newScript())
        group->newScript()->trace(this);

    if (group->maybePreliminaryObjects())
        group->maybePreliminaryObjects()->trace(this);

    if (group->maybeUnboxedLayout())
        group->unboxedLayout().trace(this);

    if (ObjectGroup* unboxedGroup = group->maybeOriginalUnboxedGroup())
        traverseEdge(group, unboxedGroup);

    if (TypeDescr* descr = group->maybeTypeDescr())
        traverseEdge(group, static_cast<JSObject*>(descr));

    if (JSFunction* fun = group->maybeInterpretedFunction())
        traverseEdge(group, static_cast<JSObject*>(fun));
}

struct TraverseObjectFunctor
{
    template <typename T>
    void operator()(T* thing, GCMarker* gcmarker, JSObject* src) {
        gcmarker->traverseEdge(src, *thing);
    }
};



enum class CheckGeneration { DoChecks, NoChecks};
template <typename Functor, typename... Args>
static inline NativeObject*
CallTraceHook(Functor f, JSTracer* trc, JSObject* obj, CheckGeneration check, Args&&... args)
{
    const Class* clasp = obj->getClass();
    MOZ_ASSERT(clasp);
    MOZ_ASSERT(obj->isNative() == clasp->isNative());

    if (!clasp->trace)
        return &obj->as<NativeObject>();

    
    
    
    MOZ_ASSERT_IF(!(clasp->trace == JS_GlobalObjectTraceHook &&
                    (!obj->compartment()->options().getTrace() || !obj->isOwnGlobal())),
                  clasp->flags & JSCLASS_IMPLEMENTS_BARRIERS);

    if (clasp->trace == InlineTypedObject::obj_trace) {
        Shape** pshape = obj->as<InlineTypedObject>().addressOfShapeFromGC();
        f(pshape, mozilla::Forward<Args>(args)...);

        InlineTypedObject& tobj = obj->as<InlineTypedObject>();
        if (tobj.typeDescr().hasTraceList()) {
            VisitTraceList(f, tobj.typeDescr().traceList(), tobj.inlineTypedMem(),
                           mozilla::Forward<Args>(args)...);
        }

        return nullptr;
    }

    if (clasp == &UnboxedPlainObject::class_) {
        JSObject** pexpando = obj->as<UnboxedPlainObject>().addressOfExpando();
        if (*pexpando)
            f(pexpando, mozilla::Forward<Args>(args)...);

        UnboxedPlainObject& unboxed = obj->as<UnboxedPlainObject>();
        const UnboxedLayout& layout = check == CheckGeneration::DoChecks
                                      ? unboxed.layout()
                                      : unboxed.layoutDontCheckGeneration();
        if (layout.traceList()) {
            VisitTraceList(f, layout.traceList(), unboxed.data(),
                           mozilla::Forward<Args>(args)...);
        }

        return nullptr;
    }

    clasp->trace(trc, obj);

    if (!clasp->isNative())
        return nullptr;
    return &obj->as<NativeObject>();
}

template <typename F, typename... Args>
static void
VisitTraceList(F f, const int32_t* traceList, uint8_t* memory, Args&&... args)
{
    while (*traceList != -1) {
        f(reinterpret_cast<JSString**>(memory + *traceList), mozilla::Forward<Args>(args)...);
        traceList++;
    }
    traceList++;
    while (*traceList != -1) {
        JSObject** objp = reinterpret_cast<JSObject**>(memory + *traceList);
        if (*objp)
            f(objp, mozilla::Forward<Args>(args)...);
        traceList++;
    }
    traceList++;
    while (*traceList != -1) {
        f(reinterpret_cast<Value*>(memory + *traceList), mozilla::Forward<Args>(args)...);
        traceList++;
    }
}




bool
GCMarker::drainMarkStack(SliceBudget& budget)
{
#ifdef DEBUG
    struct AutoCheckCompartment {
        bool& flag;
        explicit AutoCheckCompartment(bool& comparmentCheckFlag) : flag(comparmentCheckFlag) {
            MOZ_ASSERT(!flag);
            flag = true;
        }
        ~AutoCheckCompartment() { flag = false; }
    } acc(strictCompartmentChecking);
#endif

    if (budget.isOverBudget())
        return false;

    for (;;) {
        while (!stack.isEmpty()) {
            processMarkStackTop(budget);
            if (budget.isOverBudget()) {
                saveValueRanges();
                return false;
            }
        }

        if (!hasDelayedChildren())
            break;

        




        if (!markDelayedChildren(budget)) {
            saveValueRanges();
            return false;
        }
    }

    return true;
}

inline void
GCMarker::processMarkStackTop(SliceBudget& budget)
{
    




    HeapSlot* vp;
    HeapSlot* end;
    JSObject* obj;

    
    uintptr_t addr = stack.pop();
    uintptr_t tag = addr & StackTagMask;
    addr &= ~StackTagMask;

    
    switch (tag) {
      case ValueArrayTag: {
        JS_STATIC_ASSERT(ValueArrayTag == 0);
        MOZ_ASSERT(!(addr & CellMask));
        obj = reinterpret_cast<JSObject*>(addr);
        uintptr_t addr2 = stack.pop();
        uintptr_t addr3 = stack.pop();
        MOZ_ASSERT(addr2 <= addr3);
        MOZ_ASSERT((addr3 - addr2) % sizeof(Value) == 0);
        vp = reinterpret_cast<HeapSlot*>(addr2);
        end = reinterpret_cast<HeapSlot*>(addr3);
        goto scan_value_array;
      }

      case ObjectTag: {
        obj = reinterpret_cast<JSObject*>(addr);
        AssertZoneIsMarking(obj);
        goto scan_obj;
      }

      case GroupTag: {
        return lazilyMarkChildren(reinterpret_cast<ObjectGroup*>(addr));
      }

      case JitCodeTag: {
        return reinterpret_cast<jit::JitCode*>(addr)->traceChildren(this);
      }

      case SavedValueArrayTag: {
        MOZ_ASSERT(!(addr & CellMask));
        JSObject* obj = reinterpret_cast<JSObject*>(addr);
        HeapSlot* vp;
        HeapSlot* end;
        if (restoreValueArray(obj, (void**)&vp, (void**)&end))
            pushValueArray(&obj->as<NativeObject>(), vp, end);
        else
            repush(obj);
        return;
      }

      default: MOZ_CRASH("Invalid tag in mark stack");
    }
    return;

  scan_value_array:
    MOZ_ASSERT(vp <= end);
    while (vp != end) {
        budget.step();
        if (budget.isOverBudget()) {
            pushValueArray(obj, vp, end);
            return;
        }

        const Value& v = *vp++;
        if (v.isString()) {
            traverseEdge(obj, v.toString());
        } else if (v.isObject()) {
            JSObject* obj2 = &v.toObject();
            MOZ_ASSERT(obj->compartment() == obj2->compartment());
            if (mark(obj2)) {
                
                pushValueArray(obj, vp, end);
                obj = obj2;
                goto scan_obj;
            }
        } else if (v.isSymbol()) {
            traverseEdge(obj, v.toSymbol());
        }
    }
    return;

  scan_obj:
    {
        AssertZoneIsMarking(obj);

        budget.step();
        if (budget.isOverBudget()) {
            repush(obj);
            return;
        }

        ObjectGroup* group = obj->groupFromGC();
        traverseEdge(obj, group);

        NativeObject *nobj = CallTraceHook(TraverseObjectFunctor(), this, obj,
                                           CheckGeneration::DoChecks, this, obj);
        if (!nobj)
            return;

        Shape* shape = nobj->lastProperty();
        traverseEdge(obj, shape);

        unsigned nslots = nobj->slotSpan();

        do {
            if (nobj->hasEmptyElements())
                break;

            if (nobj->denseElementsAreCopyOnWrite()) {
                JSObject* owner = nobj->getElementsHeader()->ownerObject();
                if (owner != nobj) {
                    traverseEdge(obj, owner);
                    break;
                }
            }

            vp = nobj->getDenseElementsAllowCopyOnWrite();
            end = vp + nobj->getDenseInitializedLength();
            if (!nslots)
                goto scan_value_array;
            pushValueArray(nobj, vp, end);
        } while (false);

        vp = nobj->fixedSlots();
        if (nobj->slots_) {
            unsigned nfixed = nobj->numFixedSlots();
            if (nslots > nfixed) {
                pushValueArray(nobj, vp, vp + nfixed);
                vp = nobj->slots_;
                end = vp + (nslots - nfixed);
                goto scan_value_array;
            }
        }
        MOZ_ASSERT(nslots <= nobj->numFixedSlots());
        end = vp + nslots;
        goto scan_value_array;
    }
}

struct SlotArrayLayout
{
    union {
        HeapSlot* end;
        uintptr_t kind;
    };
    union {
        HeapSlot* start;
        uintptr_t index;
    };
    NativeObject* obj;

    static void staticAsserts() {
        
        JS_STATIC_ASSERT(sizeof(SlotArrayLayout) == 3 * sizeof(uintptr_t));
    }
};









void
GCMarker::saveValueRanges()
{
    for (uintptr_t* p = stack.tos_; p > stack.stack_; ) {
        uintptr_t tag = *--p & StackTagMask;
        if (tag == ValueArrayTag) {
            *p &= ~StackTagMask;
            p -= 2;
            SlotArrayLayout* arr = reinterpret_cast<SlotArrayLayout*>(p);
            NativeObject* obj = arr->obj;
            MOZ_ASSERT(obj->isNative());

            HeapSlot* vp = obj->getDenseElementsAllowCopyOnWrite();
            if (arr->end == vp + obj->getDenseInitializedLength()) {
                MOZ_ASSERT(arr->start >= vp);
                arr->index = arr->start - vp;
                arr->kind = HeapSlot::Element;
            } else {
                HeapSlot* vp = obj->fixedSlots();
                unsigned nfixed = obj->numFixedSlots();
                if (arr->start == arr->end) {
                    arr->index = obj->slotSpan();
                } else if (arr->start >= vp && arr->start < vp + nfixed) {
                    MOZ_ASSERT(arr->end == vp + Min(nfixed, obj->slotSpan()));
                    arr->index = arr->start - vp;
                } else {
                    MOZ_ASSERT(arr->start >= obj->slots_ &&
                               arr->end == obj->slots_ + obj->slotSpan() - nfixed);
                    arr->index = (arr->start - obj->slots_) + nfixed;
                }
                arr->kind = HeapSlot::Slot;
            }
            p[2] |= SavedValueArrayTag;
        } else if (tag == SavedValueArrayTag) {
            p -= 2;
        }
    }
}

bool
GCMarker::restoreValueArray(JSObject* objArg, void** vpp, void** endp)
{
    uintptr_t start = stack.pop();
    HeapSlot::Kind kind = (HeapSlot::Kind) stack.pop();

    if (!objArg->isNative())
        return false;
    NativeObject* obj = &objArg->as<NativeObject>();

    if (kind == HeapSlot::Element) {
        if (!obj->is<ArrayObject>())
            return false;

        uint32_t initlen = obj->getDenseInitializedLength();
        HeapSlot* vp = obj->getDenseElementsAllowCopyOnWrite();
        if (start < initlen) {
            *vpp = vp + start;
            *endp = vp + initlen;
        } else {
            
            *vpp = *endp = vp;
        }
    } else {
        MOZ_ASSERT(kind == HeapSlot::Slot);
        HeapSlot* vp = obj->fixedSlots();
        unsigned nfixed = obj->numFixedSlots();
        unsigned nslots = obj->slotSpan();
        if (start < nslots) {
            if (start < nfixed) {
                *vpp = vp + start;
                *endp = vp + Min(nfixed, nslots);
            } else {
                *vpp = obj->slots_ + start - nfixed;
                *endp = obj->slots_ + nslots - nfixed;
            }
        } else {
            
            *vpp = *endp = vp;
        }
    }

    MOZ_ASSERT(*vpp <= *endp);
    return true;
}




bool
MarkStack::init(JSGCMode gcMode)
{
    setBaseCapacity(gcMode);

    MOZ_ASSERT(!stack_);
    uintptr_t* newStack = js_pod_malloc<uintptr_t>(baseCapacity_);
    if (!newStack)
        return false;

    setStack(newStack, 0, baseCapacity_);
    return true;
}

void
MarkStack::setBaseCapacity(JSGCMode mode)
{
    switch (mode) {
      case JSGC_MODE_GLOBAL:
      case JSGC_MODE_COMPARTMENT:
        baseCapacity_ = NON_INCREMENTAL_MARK_STACK_BASE_CAPACITY;
        break;
      case JSGC_MODE_INCREMENTAL:
        baseCapacity_ = INCREMENTAL_MARK_STACK_BASE_CAPACITY;
        break;
      default:
        MOZ_CRASH("bad gc mode");
    }

    if (baseCapacity_ > maxCapacity_)
        baseCapacity_ = maxCapacity_;
}

void
MarkStack::setMaxCapacity(size_t maxCapacity)
{
    MOZ_ASSERT(isEmpty());
    maxCapacity_ = maxCapacity;
    if (baseCapacity_ > maxCapacity_)
        baseCapacity_ = maxCapacity_;

    reset();
}

void
MarkStack::reset()
{
    if (capacity() == baseCapacity_) {
        
        setStack(stack_, 0, baseCapacity_);
        return;
    }

    uintptr_t* newStack = (uintptr_t*)js_realloc(stack_, sizeof(uintptr_t) * baseCapacity_);
    if (!newStack) {
        
        
        newStack = stack_;
        baseCapacity_ = capacity();
    }
    setStack(newStack, 0, baseCapacity_);
}

bool
MarkStack::enlarge(unsigned count)
{
    size_t newCapacity = Min(maxCapacity_, capacity() * 2);
    if (newCapacity < capacity() + count)
        return false;

    size_t tosIndex = position();

    uintptr_t* newStack = (uintptr_t*)js_realloc(stack_, sizeof(uintptr_t) * newCapacity);
    if (!newStack)
        return false;

    setStack(newStack, tosIndex, newCapacity);
    return true;
}

void
MarkStack::setGCMode(JSGCMode gcMode)
{
    
    
    setBaseCapacity(gcMode);
}

size_t
MarkStack::sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
    return mallocSizeOf(stack_);
}








GCMarker::GCMarker(JSRuntime* rt)
  : JSTracer(rt, JSTracer::TracerKindTag::Marking, DoNotTraceWeakMaps),
    stack(size_t(-1)),
    color(BLACK),
    unmarkedArenaStackTop(nullptr),
    markLaterArenas(0),
    started(false),
    strictCompartmentChecking(false)
{
}

bool
GCMarker::init(JSGCMode gcMode)
{
    return stack.init(gcMode);
}

void
GCMarker::start()
{
    MOZ_ASSERT(!started);
    started = true;
    color = BLACK;

    MOZ_ASSERT(!unmarkedArenaStackTop);
    MOZ_ASSERT(markLaterArenas == 0);

}

void
GCMarker::stop()
{
    MOZ_ASSERT(isDrained());

    MOZ_ASSERT(started);
    started = false;

    MOZ_ASSERT(!unmarkedArenaStackTop);
    MOZ_ASSERT(markLaterArenas == 0);

    
    stack.reset();
}

void
GCMarker::reset()
{
    color = BLACK;

    stack.reset();
    MOZ_ASSERT(isMarkStackEmpty());

    while (unmarkedArenaStackTop) {
        ArenaHeader* aheader = unmarkedArenaStackTop;
        MOZ_ASSERT(aheader->hasDelayedMarking);
        MOZ_ASSERT(markLaterArenas);
        unmarkedArenaStackTop = aheader->getNextDelayedMarking();
        aheader->unsetDelayedMarking();
        aheader->markOverflow = 0;
        aheader->allocatedDuringIncremental = 0;
        markLaterArenas--;
    }
    MOZ_ASSERT(isDrained());
    MOZ_ASSERT(!markLaterArenas);
}

void
GCMarker::markDelayedChildren(ArenaHeader* aheader)
{
    if (aheader->markOverflow) {
        bool always = aheader->allocatedDuringIncremental;
        aheader->markOverflow = 0;

        for (ArenaCellIterUnderGC i(aheader); !i.done(); i.next()) {
            TenuredCell* t = i.getCell();
            if (always || t->isMarked()) {
                t->markIfUnmarked();
                JS_TraceChildren(this, t, MapAllocToTraceKind(aheader->getAllocKind()));
            }
        }
    } else {
        MOZ_ASSERT(aheader->allocatedDuringIncremental);
        PushArena(this, aheader);
    }
    aheader->allocatedDuringIncremental = 0;
    




}

bool
GCMarker::markDelayedChildren(SliceBudget& budget)
{
    GCRuntime& gc = runtime()->gc;
    gcstats::AutoPhase ap(gc.stats, gc.state() == MARK, gcstats::PHASE_MARK_DELAYED);

    MOZ_ASSERT(unmarkedArenaStackTop);
    do {
        




        ArenaHeader* aheader = unmarkedArenaStackTop;
        MOZ_ASSERT(aheader->hasDelayedMarking);
        MOZ_ASSERT(markLaterArenas);
        unmarkedArenaStackTop = aheader->getNextDelayedMarking();
        aheader->unsetDelayedMarking();
        markLaterArenas--;
        markDelayedChildren(aheader);

        budget.step(150);
        if (budget.isOverBudget())
            return false;
    } while (unmarkedArenaStackTop);
    MOZ_ASSERT(!markLaterArenas);

    return true;
}

template<typename T>
static void
PushArenaTyped(GCMarker* gcmarker, ArenaHeader* aheader)
{
    for (ArenaCellIterUnderGC i(aheader); !i.done(); i.next())
        gcmarker->traverse(i.get<T>());
}

struct PushArenaFunctor {
    template <typename T> void operator()(GCMarker* gcmarker, ArenaHeader* aheader) {
        PushArenaTyped<T>(gcmarker, aheader);
    }
};

void
gc::PushArena(GCMarker* gcmarker, ArenaHeader* aheader)
{
    CallTyped(PushArenaFunctor(), MapAllocToTraceKind(aheader->getAllocKind()), gcmarker, aheader);
}

#ifdef DEBUG
void
GCMarker::checkZone(void* p)
{
    MOZ_ASSERT(started);
    DebugOnly<Cell*> cell = static_cast<Cell*>(p);
    MOZ_ASSERT_IF(cell->isTenured(), cell->asTenured().zone()->isCollecting());
}
#endif

size_t
GCMarker::sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
    size_t size = stack.sizeOfExcludingThis(mallocSizeOf);
    for (ZonesIter zone(runtime(), WithAtoms); !zone.done(); zone.next())
        size += zone->gcGrayRoots.sizeOfExcludingThis(mallocSizeOf);
    return size;
}




namespace js {
template <>
void
TenuringTracer::traverse(JSObject** objp)
{
    
    MOZ_ASSERT(!nursery().isInside(objp));

    if (IsInsideNursery(*objp) && !nursery().getForwardedPointer(objp))
        *objp = moveToTenured(*objp);
}

template <>
void
TenuringTracer::traverse(Value* valp)
{
    if (!valp->isObject())
        return;

    JSObject *obj = &valp->toObject();
    traverse(&obj);
    valp->setObject(*obj);
}

template <> void js::TenuringTracer::traverse(js::BaseShape**) {}
template <> void js::TenuringTracer::traverse(js::jit::JitCode**) {}
template <> void js::TenuringTracer::traverse(JSScript**) {}
template <> void js::TenuringTracer::traverse(js::LazyScript**) {}
template <> void js::TenuringTracer::traverse(js::Shape**) {}
template <> void js::TenuringTracer::traverse(JSString**) {}
template <> void js::TenuringTracer::traverse(JS::Symbol**) {}
template <> void js::TenuringTracer::traverse(js::ObjectGroup**) {}
template <> void js::TenuringTracer::traverse(jsid*) {}
} 

template <typename T>
void
js::gc::StoreBuffer::MonoTypeBuffer<T>::trace(StoreBuffer* owner, TenuringTracer& mover)
{
    mozilla::ReentrancyGuard g(*owner);
    MOZ_ASSERT(owner->isEnabled());
    MOZ_ASSERT(stores_.initialized());
    for (typename StoreSet::Range r = stores_.all(); !r.empty(); r.popFront())
        r.front().trace(mover);
}

namespace js {
namespace gc {
template void
StoreBuffer::MonoTypeBuffer<StoreBuffer::WholeCellEdges>::trace(StoreBuffer*, TenuringTracer&);
template void
StoreBuffer::MonoTypeBuffer<StoreBuffer::ValueEdge>::trace(StoreBuffer*, TenuringTracer&);
template void
StoreBuffer::MonoTypeBuffer<StoreBuffer::SlotsEdge>::trace(StoreBuffer*, TenuringTracer&);
template void
StoreBuffer::MonoTypeBuffer<StoreBuffer::CellPtrEdge>::trace(StoreBuffer*, TenuringTracer&);
} 
} 

void
js::gc::StoreBuffer::SlotsEdge::trace(TenuringTracer& mover) const
{
    NativeObject* obj = object();

    
    if (!obj->isNative())
        return;

    if (IsInsideNursery(obj))
        return;

    if (kind() == ElementKind) {
        int32_t initLen = obj->getDenseInitializedLength();
        int32_t clampedStart = Min(start_, initLen);
        int32_t clampedEnd = Min(start_ + count_, initLen);
        mover.traceSlots(static_cast<HeapSlot*>(obj->getDenseElements() + clampedStart)->unsafeGet(),
                         clampedEnd - clampedStart);
    } else {
        int32_t start = Min(uint32_t(start_), obj->slotSpan());
        int32_t end = Min(uint32_t(start_) + count_, obj->slotSpan());
        MOZ_ASSERT(end >= start);
        mover.traceObjectSlots(obj, start, end - start);
    }
}

void
js::gc::StoreBuffer::WholeCellEdges::trace(TenuringTracer& mover) const
{
    MOZ_ASSERT(edge->isTenured());
    JS::TraceKind kind = edge->getTraceKind();
    if (kind == JS::TraceKind::Object) {
        JSObject *object = static_cast<JSObject*>(edge);
        mover.traceObject(object);

        
        
        
        
        
        
        if (object->is<UnboxedPlainObject>()) {
            if (UnboxedExpandoObject* expando = object->as<UnboxedPlainObject>().maybeExpando())
                expando->traceChildren(&mover);
        }

        return;
    }
    if (kind == JS::TraceKind::Script)
        static_cast<JSScript*>(edge)->traceChildren(&mover);
    else if (kind == JS::TraceKind::JitCode)
        static_cast<jit::JitCode*>(edge)->traceChildren(&mover);
    else
        MOZ_CRASH();
}

void
js::gc::StoreBuffer::CellPtrEdge::trace(TenuringTracer& mover) const
{
    if (!*edge)
        return;

    MOZ_ASSERT((*edge)->getTraceKind() == JS::TraceKind::Object);
    mover.traverse(reinterpret_cast<JSObject**>(edge));
}

void
js::gc::StoreBuffer::ValueEdge::trace(TenuringTracer& mover) const
{
    if (deref())
        mover.traverse(edge);
}


void
js::TenuringTracer::insertIntoFixupList(RelocationOverlay* entry) {
    *tail = entry;
    tail = &entry->next_;
    *tail = nullptr;
}

JSObject*
js::TenuringTracer::moveToTenured(JSObject* src)
{
    MOZ_ASSERT(IsInsideNursery(src));

    AllocKind dstKind = src->allocKindForTenure(nursery());
    Zone* zone = src->zone();
    TenuredCell* t = zone->arenas.allocateFromFreeList(dstKind, Arena::thingSize(dstKind));
    if (!t) {
        zone->arenas.checkEmptyFreeList(dstKind);
        AutoMaybeStartBackgroundAllocation maybeStartBackgroundAllocation;
        t = zone->arenas.allocateFromArena(zone, dstKind, maybeStartBackgroundAllocation);
        if (!t)
            CrashAtUnhandlableOOM("Failed to allocate object while tenuring.");
    }
    JSObject* dst = reinterpret_cast<JSObject*>(t);

    tenuredSize += moveObjectToTenured(dst, src, dstKind);

    RelocationOverlay* overlay = RelocationOverlay::fromCell(src);
    overlay->forwardTo(dst);
    insertIntoFixupList(overlay);

    TracePromoteToTenured(src, dst);
    return dst;
}

void
js::Nursery::collectToFixedPoint(TenuringTracer& mover, TenureCountCache& tenureCounts)
{
    for (RelocationOverlay* p = mover.head; p; p = p->next()) {
        JSObject* obj = static_cast<JSObject*>(p->forwardingAddress());
        mover.traceObject(obj);

        TenureCount& entry = tenureCounts.findEntry(obj->groupRaw());
        if (entry.group == obj->groupRaw()) {
            entry.count++;
        } else if (!entry.group) {
            entry.group = obj->groupRaw();
            entry.count = 1;
        }
    }
}

struct TenuringFunctor
{
    template <typename T>
    void operator()(T* thing, TenuringTracer& mover) {
        mover.traverse(thing);
    }
};


void
js::TenuringTracer::traceObject(JSObject* obj)
{
    NativeObject *nobj = CallTraceHook(TenuringFunctor(), this, obj,
                                       CheckGeneration::NoChecks, *this);
    if (!nobj)
        return;

    
    
    if (!nobj->hasEmptyElements() && !nobj->denseElementsAreCopyOnWrite()) {
        Value* elems = static_cast<HeapSlot*>(nobj->getDenseElements())->unsafeGet();
        traceSlots(elems, elems + nobj->getDenseInitializedLength());
    }

    traceObjectSlots(nobj, 0, nobj->slotSpan());
}

void
js::TenuringTracer::traceObjectSlots(NativeObject* nobj, uint32_t start, uint32_t length)
{
    HeapSlot* fixedStart;
    HeapSlot* fixedEnd;
    HeapSlot* dynStart;
    HeapSlot* dynEnd;
    nobj->getSlotRange(start, length, &fixedStart, &fixedEnd, &dynStart, &dynEnd);
    if (fixedStart)
        traceSlots(fixedStart->unsafeGet(), fixedEnd->unsafeGet());
    if (dynStart)
        traceSlots(dynStart->unsafeGet(), dynEnd->unsafeGet());
}

void
js::TenuringTracer::traceSlots(Value* vp, Value* end)
{
    for (; vp != end; ++vp)
        traverse(vp);
}

size_t
js::TenuringTracer::moveObjectToTenured(JSObject* dst, JSObject* src, AllocKind dstKind)
{
    size_t srcSize = Arena::thingSize(dstKind);
    size_t tenuredSize = srcSize;

    








    if (src->is<ArrayObject>())
        tenuredSize = srcSize = sizeof(NativeObject);

    js_memcpy(dst, src, srcSize);
    if (src->isNative()) {
        NativeObject* ndst = &dst->as<NativeObject>();
        NativeObject* nsrc = &src->as<NativeObject>();
        tenuredSize += moveSlotsToTenured(ndst, nsrc, dstKind);
        tenuredSize += moveElementsToTenured(ndst, nsrc, dstKind);

        
        
        if (&nsrc->shape_ == ndst->shape_->listp) {
            MOZ_ASSERT(nsrc->shape_->inDictionary());
            ndst->shape_->listp = &ndst->shape_;
        }
    }

    if (src->is<InlineTypedObject>()) {
        InlineTypedObject::objectMovedDuringMinorGC(this, dst, src);
    } else if (src->is<UnboxedArrayObject>()) {
        tenuredSize += UnboxedArrayObject::objectMovedDuringMinorGC(this, dst, src, dstKind);
    } else {
        
        
        MOZ_ASSERT(!(src->getClass()->flags & JSCLASS_SKIP_NURSERY_FINALIZE));
    }

    return tenuredSize;
}

size_t
js::TenuringTracer::moveSlotsToTenured(NativeObject* dst, NativeObject* src, AllocKind dstKind)
{
    
    if (!src->hasDynamicSlots())
        return 0;

    if (!nursery().isInside(src->slots_)) {
        nursery().removeMallocedBuffer(src->slots_);
        return 0;
    }

    Zone* zone = src->zone();
    size_t count = src->numDynamicSlots();
    dst->slots_ = zone->pod_malloc<HeapSlot>(count);
    if (!dst->slots_)
        CrashAtUnhandlableOOM("Failed to allocate slots while tenuring.");
    PodCopy(dst->slots_, src->slots_, count);
    nursery().setSlotsForwardingPointer(src->slots_, dst->slots_, count);
    return count * sizeof(HeapSlot);
}

size_t
js::TenuringTracer::moveElementsToTenured(NativeObject* dst, NativeObject* src, AllocKind dstKind)
{
    if (src->hasEmptyElements() || src->denseElementsAreCopyOnWrite())
        return 0;

    Zone* zone = src->zone();
    ObjectElements* srcHeader = src->getElementsHeader();
    ObjectElements* dstHeader;

    
    if (!nursery().isInside(srcHeader)) {
        MOZ_ASSERT(src->elements_ == dst->elements_);
        nursery().removeMallocedBuffer(srcHeader);
        return 0;
    }

    size_t nslots = ObjectElements::VALUES_PER_HEADER + srcHeader->capacity;

    
    if (src->is<ArrayObject>() && nslots <= GetGCKindSlots(dstKind)) {
        dst->as<ArrayObject>().setFixedElements();
        dstHeader = dst->as<ArrayObject>().getElementsHeader();
        js_memcpy(dstHeader, srcHeader, nslots * sizeof(HeapSlot));
        nursery().setElementsForwardingPointer(srcHeader, dstHeader, nslots);
        return nslots * sizeof(HeapSlot);
    }

    MOZ_ASSERT(nslots >= 2);
    dstHeader = reinterpret_cast<ObjectElements*>(zone->pod_malloc<HeapSlot>(nslots));
    if (!dstHeader)
        CrashAtUnhandlableOOM("Failed to allocate elements while tenuring.");
    js_memcpy(dstHeader, srcHeader, nslots * sizeof(HeapSlot));
    nursery().setElementsForwardingPointer(srcHeader, dstHeader, nslots);
    dst->elements_ = dstHeader->elements();
    return nslots * sizeof(HeapSlot);
}




template <typename T>
static inline void
CheckIsMarkedThing(T* thingp)
{
#define IS_SAME_TYPE_OR(name, type, _) mozilla::IsSame<type*, T>::value ||
    static_assert(
            FOR_EACH_GC_LAYOUT(IS_SAME_TYPE_OR)
            false, "Only the base cell layout types are allowed into marking/tracing internals");
#undef IS_SAME_TYPE_OR

#ifdef DEBUG
    MOZ_ASSERT(thingp);
    MOZ_ASSERT(*thingp);
    JSRuntime* rt = (*thingp)->runtimeFromAnyThread();
    MOZ_ASSERT_IF(!ThingIsPermanentAtomOrWellKnownSymbol(*thingp),
                  CurrentThreadCanAccessRuntime(rt) ||
                  (rt->isHeapCollecting() && rt->gc.state() == SWEEP));
#endif
}

template <typename T>
static bool
IsMarkedInternalCommon(T* thingp)
{
    CheckIsMarkedThing(thingp);
    MOZ_ASSERT(!IsInsideNursery(*thingp));

    Zone* zone = (*thingp)->asTenured().zoneFromAnyThread();
    if (!zone->isCollectingFromAnyThread() || zone->isGCFinished())
        return true;
    if (zone->isGCCompacting() && IsForwarded(*thingp))
        *thingp = Forwarded(*thingp);
    return (*thingp)->asTenured().isMarked();
}

template <typename T>
static bool
IsMarkedInternal(T* thingp)
{
    return IsMarkedInternalCommon(thingp);
}

template <typename T>
static bool
IsMarkedInternal(JSObject** thingp)
{
    if (IsInsideNursery(*thingp)) {
        JSRuntime* rt = (*thingp)->runtimeFromAnyThread();
        MOZ_ASSERT(CurrentThreadCanAccessRuntime(rt));
        return rt->gc.nursery.getForwardedPointer(thingp);
    }
    return IsMarkedInternalCommon(thingp);
}

template <typename S>
struct IsMarkedFunctor : public IdentityDefaultAdaptor<S> {
    template <typename T> S operator()(T* t, bool* rv) {
        *rv = IsMarkedInternal(&t);
        return js::gc::RewrapValueOrId<S, T*>::wrap(t);
    }
};

template <>
bool
IsMarkedInternal<Value>(Value* valuep)
{
    bool rv = true;
    *valuep = DispatchValueTyped(IsMarkedFunctor<Value>(), *valuep, &rv);
    return rv;
}

template <>
bool
IsMarkedInternal<jsid>(jsid* idp)
{
    bool rv = true;
    *idp = DispatchIdTyped(IsMarkedFunctor<jsid>(), *idp, &rv);
    return rv;
}

template <typename T>
static bool
IsAboutToBeFinalizedInternal(T* thingp)
{
    CheckIsMarkedThing(thingp);
    T thing = *thingp;
    JSRuntime* rt = thing->runtimeFromAnyThread();

    
    if (ThingIsPermanentAtomOrWellKnownSymbol(thing) && !TlsPerThreadData.get()->associatedWith(rt))
        return false;

    Nursery& nursery = rt->gc.nursery;
    MOZ_ASSERT_IF(!rt->isHeapMinorCollecting(), !IsInsideNursery(thing));
    if (rt->isHeapMinorCollecting()) {
        if (IsInsideNursery(thing))
            return !nursery.getForwardedPointer(reinterpret_cast<JSObject**>(thingp));
        return false;
    }

    Zone* zone = thing->asTenured().zoneFromAnyThread();
    if (zone->isGCSweeping()) {
        if (thing->asTenured().arenaHeader()->allocatedDuringIncremental)
            return false;
        return !thing->asTenured().isMarked();
    }
    else if (zone->isGCCompacting() && IsForwarded(thing)) {
        *thingp = Forwarded(thing);
        return false;
    }

    return false;
}

template <typename S>
struct IsAboutToBeFinalizedFunctor : public IdentityDefaultAdaptor<S> {
    template <typename T> S operator()(T* t, bool* rv) {
        *rv = IsAboutToBeFinalizedInternal(&t);
        return js::gc::RewrapValueOrId<S, T*>::wrap(t);
    }
};

template <>
bool
IsAboutToBeFinalizedInternal<Value>(Value* valuep)
{
    bool rv = false;
    *valuep = DispatchValueTyped(IsAboutToBeFinalizedFunctor<Value>(), *valuep, &rv);
    return rv;
}

template <>
bool
IsAboutToBeFinalizedInternal<jsid>(jsid* idp)
{
    bool rv = false;
    *idp = DispatchIdTyped(IsAboutToBeFinalizedFunctor<jsid>(), *idp, &rv);
    return rv;
}

namespace js {
namespace gc {

template <typename T>
bool
IsMarkedUnbarriered(T* thingp)
{
    return IsMarkedInternal(ConvertToBase(thingp));
}

template <typename T>
bool
IsMarked(BarrieredBase<T>* thingp)
{
    return IsMarkedInternal(ConvertToBase(thingp->unsafeGet()));
}

template <typename T>
bool
IsMarked(ReadBarriered<T>* thingp)
{
    return IsMarkedInternal(ConvertToBase(thingp->unsafeGet()));
}

template <typename T>
bool
IsAboutToBeFinalizedUnbarriered(T* thingp)
{
    return IsAboutToBeFinalizedInternal(ConvertToBase(thingp));
}

template <typename T>
bool
IsAboutToBeFinalized(BarrieredBase<T>* thingp)
{
    return IsAboutToBeFinalizedInternal(ConvertToBase(thingp->unsafeGet()));
}

template <typename T>
bool
IsAboutToBeFinalized(ReadBarriered<T>* thingp)
{
    return IsAboutToBeFinalizedInternal(ConvertToBase(thingp->unsafeGet()));
}


#define INSTANTIATE_ALL_VALID_TRACE_FUNCTIONS(type) \
    template bool IsMarkedUnbarriered<type>(type*); \
    template bool IsMarked<type>(BarrieredBase<type>*); \
    template bool IsMarked<type>(ReadBarriered<type>*); \
    template bool IsAboutToBeFinalizedUnbarriered<type>(type*); \
    template bool IsAboutToBeFinalized<type>(BarrieredBase<type>*); \
    template bool IsAboutToBeFinalized<type>(ReadBarriered<type>*);
FOR_EACH_GC_POINTER_TYPE(INSTANTIATE_ALL_VALID_TRACE_FUNCTIONS)
#undef INSTANTIATE_ALL_VALID_TRACE_FUNCTIONS

} 
} 




void
TypeSet::MarkTypeRoot(JSTracer* trc, TypeSet::Type* v, const char* name)
{
    AssertRootMarkingPhase(trc);
    MarkTypeUnbarriered(trc, v, name);
}

void
TypeSet::MarkTypeUnbarriered(JSTracer* trc, TypeSet::Type* v, const char* name)
{
    if (v->isSingletonUnchecked()) {
        JSObject* obj = v->singleton();
        DispatchToTracer(trc, &obj, name);
        *v = TypeSet::ObjectType(obj);
    } else if (v->isGroupUnchecked()) {
        ObjectGroup* group = v->group();
        DispatchToTracer(trc, &group, name);
        *v = TypeSet::ObjectType(group);
    }
}




#ifdef DEBUG
struct AssertNonGrayTracer : public JS::CallbackTracer {
    explicit AssertNonGrayTracer(JSRuntime* rt) : JS::CallbackTracer(rt) {}
    void onChild(const JS::GCCellPtr& thing) override {
        MOZ_ASSERT_IF(thing.asCell()->isTenured(),
                      !thing.asCell()->asTenured().isMarked(js::gc::GRAY));
    }
};
#endif

struct UnmarkGrayTracer : public JS::CallbackTracer
{
    



    explicit UnmarkGrayTracer(JSRuntime* rt)
      : JS::CallbackTracer(rt, DoNotTraceWeakMaps),
        tracingShape(false),
        previousShape(nullptr),
        unmarkedAny(false)
    {}

    UnmarkGrayTracer(JSTracer* trc, bool tracingShape)
      : JS::CallbackTracer(trc->runtime(), DoNotTraceWeakMaps),
        tracingShape(tracingShape),
        previousShape(nullptr),
        unmarkedAny(false)
    {}

    void onChild(const JS::GCCellPtr& thing) override;

    
    bool tracingShape;

    
    Shape* previousShape;

    
    bool unmarkedAny;
};































void
UnmarkGrayTracer::onChild(const JS::GCCellPtr& thing)
{
    int stackDummy;
    if (!JS_CHECK_STACK_SIZE(runtime()->mainThread.nativeStackLimit[StackForSystemCode],
                             &stackDummy))
    {
        



        runtime()->gc.setGrayBitsInvalid();
        return;
    }

    Cell* cell = thing.asCell();

    
    
    if (!cell->isTenured()) {
#ifdef DEBUG
        AssertNonGrayTracer nongray(runtime());
        TraceChildren(&nongray, cell, thing.kind());
#endif
        return;
    }

    TenuredCell& tenured = cell->asTenured();
    if (!tenured.isMarked(js::gc::GRAY))
        return;
    tenured.unmark(js::gc::GRAY);

    unmarkedAny = true;

    
    
    
    
    
    UnmarkGrayTracer childTracer(this, thing.kind() == JS::TraceKind::Shape);

    if (thing.kind() != JS::TraceKind::Shape) {
        TraceChildren(&childTracer, &tenured, thing.kind());
        MOZ_ASSERT(!childTracer.previousShape);
        unmarkedAny |= childTracer.unmarkedAny;
        return;
    }

    MOZ_ASSERT(thing.kind() == JS::TraceKind::Shape);
    Shape* shape = static_cast<Shape*>(&tenured);
    if (tracingShape) {
        MOZ_ASSERT(!previousShape);
        previousShape = shape;
        return;
    }

    do {
        MOZ_ASSERT(!shape->isMarked(js::gc::GRAY));
        TraceChildren(&childTracer, shape, JS::TraceKind::Shape);
        shape = childTracer.previousShape;
        childTracer.previousShape = nullptr;
    } while (shape);
    unmarkedAny |= childTracer.unmarkedAny;
}

bool
js::UnmarkGrayCellRecursively(gc::Cell* cell, JS::TraceKind kind)
{
    MOZ_ASSERT(cell);

    JSRuntime* rt = cell->runtimeFromMainThread();
    MOZ_ASSERT(!rt->isHeapBusy());

    bool unmarkedArg = false;
    if (cell->isTenured()) {
        if (!cell->asTenured().isMarked(GRAY))
            return false;

        cell->asTenured().unmark(GRAY);
        unmarkedArg = true;
    }

    UnmarkGrayTracer trc(rt);
    TraceChildren(&trc, cell, kind);

    return unmarkedArg || trc.unmarkedAny;
}

bool
js::UnmarkGrayShapeRecursively(Shape* shape)
{
    return js::UnmarkGrayCellRecursively(shape, JS::TraceKind::Shape);
}

JS_FRIEND_API(bool)
JS::UnmarkGrayGCThingRecursively(JS::GCCellPtr thing)
{
    return js::UnmarkGrayCellRecursively(thing.asCell(), thing.kind());
}
