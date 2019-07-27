





#include "gc/Marking.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/IntegerRange.h"
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

using namespace js;
using namespace js::gc;

using mozilla::DebugOnly;
using mozilla::IsBaseOf;
using mozilla::IsSame;
using mozilla::MakeRange;

void * const js::NullPtr::constNullValue = nullptr;

JS_PUBLIC_DATA(void * const) JS::NullPtr::constNullValue = nullptr;































static inline void
PushMarkStack(GCMarker* gcmarker, JSObject* thing) {
    gcmarker->traverse(thing);
}
static inline void
PushMarkStack(GCMarker* gcmarker, JSFunction* thing) {
    gcmarker->traverse(static_cast<JSObject*>(thing));
}
static inline void
PushMarkStack(GCMarker* gcmarker, ObjectGroup* thing) {
    gcmarker->traverse(thing);
}
static void
PushMarkStack(GCMarker* gcmarker, jit::JitCode* thing) {
    gcmarker->traverse(thing);
}
static inline void
PushMarkStack(GCMarker* gcmarker, JSScript* thing) {
    gcmarker->traverse(thing);
}
static inline void
PushMarkStack(GCMarker* gcmarker, LazyScript* thing) {
    gcmarker->traverse(thing);
}

static inline void
PushMarkStack(GCMarker* gcmarker, Shape* thing);
static inline void
PushMarkStack(GCMarker* gcmarker, BaseShape* thing);
static inline void
PushMarkStack(GCMarker* gcmarker, JSString* thing);
static inline void
PushMarkStack(GCMarker* gcmarker, JS::Symbol* thing);

namespace js {
namespace gc {

static void MarkChildren(JSTracer* trc, ObjectGroup* group);

} 
} 



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
#endif

static GCMarker*
AsGCMarker(JSTracer* trc)
{
    MOZ_ASSERT(trc->isMarkingTracer());
    return static_cast<GCMarker*>(trc);
}

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
static inline void
CheckMarkedThing(JSTracer* trc, T thing)
{
#ifdef DEBUG
    MOZ_ASSERT(trc);
    MOZ_ASSERT(thing);

    thing = MaybeForwarded(thing);

    
    if (IsInsideNursery(thing))
        return;

    MOZ_ASSERT_IF(!MovingTracer::IsMovingTracer(trc) && !Nursery::IsMinorCollectionTracer(trc),
                  !IsForwarded(thing));

    



    if (ThingIsPermanentAtomOrWellKnownSymbol(thing))
        return;

    Zone* zone = thing->zoneFromAnyThread();
    JSRuntime* rt = trc->runtime();

    MOZ_ASSERT_IF(!MovingTracer::IsMovingTracer(trc), CurrentThreadCanAccessZone(zone));
    MOZ_ASSERT_IF(!MovingTracer::IsMovingTracer(trc), CurrentThreadCanAccessRuntime(rt));

    MOZ_ASSERT(zone->runtimeFromAnyThread() == trc->runtime());

    MOZ_ASSERT(thing->isAligned());
    MOZ_ASSERT(MapTypeToTraceKind<typename mozilla::RemovePointer<T>::Type>::kind ==
               GetGCThingTraceKind(thing));

    



    bool isGcMarkingTracer = trc->isMarkingTracer();

    MOZ_ASSERT_IF(zone->requireGCTracer(), isGcMarkingTracer || IsBufferingGrayRoots(trc));

    if (isGcMarkingTracer) {
        GCMarker* gcMarker = static_cast<GCMarker*>(trc);
        MOZ_ASSERT_IF(gcMarker->shouldCheckCompartments(),
                      zone->isCollecting() || rt->isAtomsZone(zone));

        MOZ_ASSERT_IF(gcMarker->markColor() == GRAY,
                      !zone->isGCMarkingBlack() || rt->isAtomsZone(zone));

        MOZ_ASSERT(!(zone->isGCSweeping() || zone->isGCFinished() || zone->isGCCompacting()));
    }

    







    MOZ_ASSERT_IF(IsThingPoisoned(thing) && rt->isHeapBusy() && !rt->gc.isBackgroundSweeping(),
                  !InFreeList(thing->asTenured().arenaHeader(), thing));
#endif
}

template<>
void
CheckMarkedThing<Value>(JSTracer* trc, Value val)
{
#ifdef DEBUG
    if (val.isString())
        CheckMarkedThing(trc, val.toString());
    else if (val.isObject())
        CheckMarkedThing(trc, &val.toObject());
    else if (val.isSymbol())
        CheckMarkedThing(trc, val.toSymbol());
#endif
}

template <>
void
CheckMarkedThing<jsid>(JSTracer* trc, jsid id)
{
#ifdef DEBUG
    if (JSID_IS_STRING(id))
        CheckMarkedThing(trc, JSID_TO_STRING(id));
    else if (JSID_IS_SYMBOL(id))
        CheckMarkedThing(trc, JSID_TO_SYMBOL(id));
#endif
}

#define JS_ROOT_MARKING_ASSERT(trc) \
    MOZ_ASSERT_IF(trc->isMarkingTracer(), \
                  trc->runtime()->gc.state() == NO_INCREMENTAL || \
                  trc->runtime()->gc.state() == MARK_ROOTS);








template<typename T>
static inline void
SetMaybeAliveFlag(T* thing)
{
}

template<>
void
SetMaybeAliveFlag(JSObject* thing)
{
    thing->compartment()->maybeAlive = true;
}

template<>
void
SetMaybeAliveFlag(NativeObject* thing)
{
    thing->compartment()->maybeAlive = true;
}

template<>
void
SetMaybeAliveFlag(JSScript* thing)
{
    thing->compartment()->maybeAlive = true;
}

#define FOR_EACH_GC_LAYOUT(D) \
    D(Object, JSObject) \
    D(String, JSString) \
    D(Symbol, JS::Symbol) \
    D(Script, JSScript) \
    D(Shape, js::Shape) \
    D(BaseShape, js::BaseShape) \
    D(JitCode, js::jit::JitCode) \
    D(LazyScript, js::LazyScript) \
    D(ObjectGroup, js::ObjectGroup)


enum class TraceKind {
#define NAMES(name, _) name,
FOR_EACH_GC_LAYOUT(NAMES)
#undef NAMES
};

#define FOR_EACH_GC_POINTER_TYPE(D) \
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
          TraceKind = IsBaseOf<JSObject, T>::value     ? TraceKind::Object
                    : IsBaseOf<JSString, T>::value     ? TraceKind::String
                    : IsBaseOf<JS::Symbol, T>::value   ? TraceKind::Symbol
                    : IsBaseOf<JSScript, T>::value     ? TraceKind::Script
                    : IsBaseOf<Shape, T>::value        ? TraceKind::Shape
                    : IsBaseOf<BaseShape, T>::value    ? TraceKind::BaseShape
                    : IsBaseOf<jit::JitCode, T>::value ? TraceKind::JitCode
                    : IsBaseOf<LazyScript, T>::value   ? TraceKind::LazyScript
                    :                                    TraceKind::ObjectGroup>
struct BaseGCType;
#define IMPL_BASE_GC_TYPE(name, type_) \
    template <typename T> struct BaseGCType<T, TraceKind:: name> { typedef type_ type; };
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

template <typename T> void DispatchToTracer(JSTracer* trc, T* thingp, const char* name, size_t i);
template <typename T> void DoTracing(JS::CallbackTracer* trc, T* thingp, const char* name, size_t i);
template <typename T> void DoMarking(GCMarker* gcmarker, T thing);
static bool ShouldMarkCrossCompartment(JSTracer* trc, JSObject* src, Cell* cell);
static bool ShouldMarkCrossCompartment(JSTracer* trc, JSObject* src, Value val);

template <typename T>
void
js::TraceEdge(JSTracer* trc, BarrieredBase<T>* thingp, const char* name)
{
    DispatchToTracer(trc, ConvertToBase(thingp->unsafeGet()), name, JSTracer::InvalidIndex);
}

template <typename T>
void
js::TraceManuallyBarrieredEdge(JSTracer* trc, T* thingp, const char* name)
{
    DispatchToTracer(trc, ConvertToBase(thingp), name, JSTracer::InvalidIndex);
}

template <typename T>
void
js::TraceRoot(JSTracer* trc, T* thingp, const char* name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    DispatchToTracer(trc, ConvertToBase(thingp), name, JSTracer::InvalidIndex);
}

template <typename T>
void
js::TraceRange(JSTracer* trc, size_t len, BarrieredBase<T>* vec, const char* name)
{
    for (auto i : MakeRange(len)) {
        if (InternalGCMethods<T>::isMarkable(vec[i].get()))
            DispatchToTracer(trc, ConvertToBase(vec[i].unsafeGet()), name, i);
    }
}

template <typename T>
void
js::TraceRootRange(JSTracer* trc, size_t len, T* vec, const char* name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    for (auto i : MakeRange(len)) {
        if (InternalGCMethods<T>::isMarkable(vec[i]))
            DispatchToTracer(trc, ConvertToBase(&vec[i]), name, i);
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
        DispatchToTracer(trc, dst, name, -1);
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
        DispatchToTracer(trc, dst->unsafeGet(), name, -1);
}
template void js::TraceCrossCompartmentEdge<Value>(JSTracer*, JSObject*, BarrieredBase<Value>*,
                                                   const char*);




template <typename T>
void
DispatchToTracer(JSTracer* trc, T* thingp, const char* name, size_t i)
{
#define IS_SAME_TYPE_OR(name, type) mozilla::IsSame<type*, T>::value ||
    static_assert(
            FOR_EACH_GC_LAYOUT(IS_SAME_TYPE_OR)
            mozilla::IsSame<T, JS::Value>::value ||
            mozilla::IsSame<T, jsid>::value,
            "Only the base cell layout types are allowed into marking/tracing internals");
#undef IS_SAME_TYPE_OR
    CheckMarkedThing(trc, *thingp);

    if (trc->isMarkingTracer())
        return DoMarking(static_cast<GCMarker*>(trc), *thingp);
    return DoTracing(static_cast<JS::CallbackTracer*>(trc), thingp, name, i);
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

    PushMarkStack(gcmarker, thing);

    
    SetMaybeAliveFlag(thing);
}

template <>
void
DoMarking<Value>(GCMarker* gcmarker, Value val)
{
    if (val.isString())
        DoMarking(gcmarker, val.toString());
    else if (val.isObject())
        DoMarking(gcmarker, &val.toObject());
    else if (val.isSymbol())
        DoMarking(gcmarker, val.toSymbol());
    else
        gcmarker->clearTracingDetails();
}

template <>
void
DoMarking<jsid>(GCMarker* gcmarker, jsid id)
{
    if (JSID_IS_STRING(id))
        DoMarking(gcmarker, JSID_TO_STRING(id));
    else if (JSID_IS_SYMBOL(id))
        DoMarking(gcmarker, JSID_TO_SYMBOL(id));
    else
        gcmarker->clearTracingDetails();
}

template <typename T>
void
DoTracing(JS::CallbackTracer* trc, T* thingp, const char* name, size_t i)
{
    JSGCTraceKind kind = MapTypeToTraceKind<typename mozilla::RemovePointer<T>::Type>::kind;
    trc->setTracingIndex(name, i);
    trc->invoke((void**)thingp, kind);
    trc->unsetTracingLocation();
}

template <>
void
DoTracing<Value>(JS::CallbackTracer* trc, Value* vp, const char* name, size_t i)
{
    if (vp->isObject()) {
        JSObject* prior = &vp->toObject();
        JSObject* obj = prior;
        DoTracing(trc, &obj, name, i);
        if (obj != prior)
            vp->setObjectOrNull(obj);
    } else if (vp->isString()) {
        JSString* prior = vp->toString();
        JSString* str = prior;
        DoTracing(trc, &str, name, i);
        if (str != prior)
            vp->setString(str);
    } else if (vp->isSymbol()) {
        JS::Symbol* prior = vp->toSymbol();
        JS::Symbol* sym = prior;
        DoTracing(trc, &sym, name, i);
        if (sym != prior)
            vp->setSymbol(sym);
    } else {
        
        trc->unsetTracingLocation();
    }
}

template <>
void
DoTracing<jsid>(JS::CallbackTracer* trc, jsid* idp, const char* name, size_t i)
{
    if (JSID_IS_STRING(*idp)) {
        JSString* prior = JSID_TO_STRING(*idp);
        JSString* str = prior;
        DoTracing(trc, &str, name, i);
        if (str != prior)
            *idp = NON_INTEGER_ATOM_TO_JSID(reinterpret_cast<JSAtom*>(str));
    } else if (JSID_IS_SYMBOL(*idp)) {
        JS::Symbol* prior = JSID_TO_SYMBOL(*idp);
        JS::Symbol* sym = prior;
        DoTracing(trc, &sym, name, i);
        if (sym != prior)
            *idp = SYMBOL_TO_JSID(sym);
    } else {
        
        trc->unsetTracingLocation();
    }
}

template<typename T>
static void
MarkInternal(JSTracer* trc, T** thingp)
{
    T* thing = *thingp;
    CheckMarkedThing(trc, thing);

    if (trc->isMarkingTracer()) {
        





        if (IsInsideNursery(thing))
            return;

        




        if (ThingIsPermanentAtomOrWellKnownSymbol(thing))
            return;

        



        if (!thing->zone()->isGCMarking())
            return;

        PushMarkStack(AsGCMarker(trc), thing);
        SetMaybeAliveFlag(thing);
    } else {
        trc->asCallbackTracer()->invoke((void**)thingp, MapTypeToTraceKind<T>::kind);
        trc->unsetTracingLocation();
    }

    trc->clearTracingDetails();
}

namespace js {
namespace gc {

template <typename T>
void
MarkUnbarriered(JSTracer* trc, T** thingp, const char* name)
{
    trc->setTracingName(name);
    MarkInternal(trc, thingp);
}

template <typename T>
static void
Mark(JSTracer* trc, BarrieredBase<T*>* thing, const char* name)
{
    trc->setTracingName(name);
    MarkInternal(trc, thing->unsafeGet());
}

void
MarkPermanentAtom(JSTracer* trc, JSAtom* atom, const char* name)
{
    trc->setTracingName(name);

    MOZ_ASSERT(atom->isPermanent());

    CheckMarkedThing(trc, atom);

    if (trc->isMarkingTracer()) {
        
        
        atom->markIfUnmarked();
    } else {
        void* thing = atom;
        trc->asCallbackTracer()->invoke(&thing, JSTRACE_STRING);
        MOZ_ASSERT(thing == atom);
        trc->unsetTracingLocation();
    }

    trc->clearTracingDetails();
}

void
MarkWellKnownSymbol(JSTracer* trc, JS::Symbol* sym)
{
    if (!sym)
        return;

    trc->setTracingName("wellKnownSymbols");

    MOZ_ASSERT(sym->isWellKnownSymbol());
    CheckMarkedThing(trc, sym);
    if (trc->isMarkingTracer()) {
        
        MOZ_ASSERT(sym->description()->isMarked());
        sym->markIfUnmarked();
    } else {
        void* thing = sym;
        trc->asCallbackTracer()->invoke(&thing, JSTRACE_SYMBOL);
        MOZ_ASSERT(thing == sym);
        trc->unsetTracingLocation();
    }

    trc->clearTracingDetails();
}

} 
} 

template <typename T>
static void
MarkRoot(JSTracer* trc, T** thingp, const char* name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    trc->setTracingName(name);
    MarkInternal(trc, thingp);
}

template <typename T>
static void
MarkRange(JSTracer* trc, size_t len, HeapPtr<T*>* vec, const char* name)
{
    for (size_t i = 0; i < len; ++i) {
        if (vec[i].get()) {
            trc->setTracingIndex(name, i);
            MarkInternal(trc, vec[i].unsafeGet());
        }
    }
}

template <typename T>
static void
MarkRootRange(JSTracer* trc, size_t len, T** vec, const char* name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    for (size_t i = 0; i < len; ++i) {
        if (vec[i]) {
            trc->setTracingIndex(name, i);
            MarkInternal(trc, &vec[i]);
        }
    }
}

namespace js {
namespace gc {

template <typename T>
static inline void
CheckIsMarkedThing(T* thingp)
{
#define IS_SAME_TYPE_OR(name, type) mozilla::IsSame<type*, T>::value ||
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
IsMarkedInternal(T* thingp)
{
    CheckIsMarkedThing(thingp);
    JSRuntime* rt = (*thingp)->runtimeFromAnyThread();

    if (IsInsideNursery(*thingp)) {
        MOZ_ASSERT(CurrentThreadCanAccessRuntime(rt));
        return rt->gc.nursery.getForwardedPointer(thingp);
    }

    Zone* zone = (*thingp)->asTenured().zoneFromAnyThread();
    if (!zone->isCollectingFromAnyThread() || zone->isGCFinished())
        return true;
    if (zone->isGCCompacting() && IsForwarded(*thingp))
        *thingp = Forwarded(*thingp);
    return (*thingp)->asTenured().isMarked();
}

template <>
bool
IsMarkedInternal<Value>(Value* valuep)
{
    bool rv = true;  
    if (valuep->isString()) {
        JSString* str = valuep->toString();
        rv = IsMarkedInternal(&str);
        valuep->setString(str);
    } else if (valuep->isObject()) {
        JSObject* obj = &valuep->toObject();
        rv = IsMarkedInternal(&obj);
        valuep->setObject(*obj);
    } else if (valuep->isSymbol()) {
        JS::Symbol* sym = valuep->toSymbol();
        rv = IsMarkedInternal(&sym);
        valuep->setSymbol(sym);
    }
    return rv;
}

template <>
bool
IsMarkedInternal<jsid>(jsid* idp)
{
    bool rv = true;  
    if (JSID_IS_STRING(*idp)) {
        JSString* str = JSID_TO_STRING(*idp);
        rv = IsMarkedInternal(&str);
        *idp = NON_INTEGER_ATOM_TO_JSID(reinterpret_cast<JSAtom*>(str));
    } else if (JSID_IS_SYMBOL(*idp)) {
        JS::Symbol* sym = JSID_TO_SYMBOL(*idp);
        rv = IsMarkedInternal(&sym);
        *idp = SYMBOL_TO_JSID(sym);
    }
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
            return !nursery.getForwardedPointer(thingp);
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

template <>
bool
IsAboutToBeFinalizedInternal<Value>(Value* valuep)
{
    bool rv = false;  
    if (valuep->isString()) {
        JSString* str = (JSString*)valuep->toGCThing();
        rv = IsAboutToBeFinalizedInternal(&str);
        valuep->setString(str);
    } else if (valuep->isObject()) {
        JSObject* obj = (JSObject*)valuep->toGCThing();
        rv = IsAboutToBeFinalizedInternal(&obj);
        valuep->setObject(*obj);
    } else if (valuep->isSymbol()) {
        JS::Symbol* sym = valuep->toSymbol();
        rv = IsAboutToBeFinalizedInternal(&sym);
        valuep->setSymbol(sym);
    }
    return rv;
}

template <>
bool
IsAboutToBeFinalizedInternal<jsid>(jsid* idp)
{
    bool rv = false;  
    if (JSID_IS_STRING(*idp)) {
        JSString* str = JSID_TO_STRING(*idp);
        rv = IsAboutToBeFinalizedInternal(&str);
        *idp = NON_INTEGER_ATOM_TO_JSID(reinterpret_cast<JSAtom*>(str));
    } else if (JSID_IS_SYMBOL(*idp)) {
        JS::Symbol* sym = JSID_TO_SYMBOL(*idp);
        rv = IsAboutToBeFinalizedInternal(&sym);
        *idp = SYMBOL_TO_JSID(sym);
    }
    return rv;
}

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

template <typename T>
T*
UpdateIfRelocated(JSRuntime* rt, T** thingp)
{
    MOZ_ASSERT(thingp);
    if (!*thingp)
        return nullptr;

    if (rt->isHeapMinorCollecting() && IsInsideNursery(*thingp)) {
        rt->gc.nursery.getForwardedPointer(thingp);
        return *thingp;
    }

    Zone* zone = (*thingp)->zone();
    if (zone->isGCCompacting() && IsForwarded(*thingp))
        *thingp = Forwarded(*thingp);

    return *thingp;
}

#define DeclMarkerImpl(base, type)                                                                \
void                                                                                              \
Mark##base(JSTracer* trc, BarrieredBase<type*>* thing, const char* name)                          \
{                                                                                                 \
    Mark<type>(trc, thing, name);                                                                 \
}                                                                                                 \
                                                                                                  \
void                                                                                              \
Mark##base##Root(JSTracer* trc, type** thingp, const char* name)                                  \
{                                                                                                 \
    MarkRoot<type>(trc, thingp, name);                                                            \
}                                                                                                 \
                                                                                                  \
void                                                                                              \
Mark##base##Unbarriered(JSTracer* trc, type** thingp, const char* name)                           \
{                                                                                                 \
    MarkUnbarriered<type>(trc, thingp, name);                                                     \
}                                                                                                 \
                                                                                                  \
/* Explicitly instantiate MarkUnbarriered<type*>. It is referenced from */                        \
/* other translation units and the instantiation might otherwise get */                           \
/* inlined away. */                                                                               \
template void MarkUnbarriered<type>(JSTracer*, type**, const char*);                           \
                                                                                                  \
void                                                                                              \
Mark##base##Range(JSTracer* trc, size_t len, HeapPtr<type*>* vec, const char* name)               \
{                                                                                                 \
    MarkRange<type>(trc, len, vec, name);                                                         \
}                                                                                                 \
                                                                                                  \
void                                                                                              \
Mark##base##RootRange(JSTracer* trc, size_t len, type** vec, const char* name)                    \
{                                                                                                 \
    MarkRootRange<type>(trc, len, vec, name);                                                     \
}                                                                                                 \
                                                                                                  \
bool                                                                                              \
Is##base##Marked(type** thingp)                                                                   \
{                                                                                                 \
    return IsMarkedUnbarriered<type*>(thingp);                                                    \
}                                                                                                 \
                                                                                                  \
bool                                                                                              \
Is##base##Marked(BarrieredBase<type*>* thingp)                                                    \
{                                                                                                 \
    return IsMarked<type*>(thingp);                                                               \
}                                                                                                 \
                                                                                                  \
bool                                                                                              \
Is##base##AboutToBeFinalized(type** thingp)                                                       \
{                                                                                                 \
    return IsAboutToBeFinalizedUnbarriered<type*>(thingp);                                        \
}                                                                                                 \
                                                                                                  \
bool                                                                                              \
Is##base##AboutToBeFinalized(BarrieredBase<type*>* thingp)                                        \
{                                                                                                 \
    return IsAboutToBeFinalized<type*>(thingp);                                                   \
}                                                                                                 \
                                                                                                  \
type *                                                                                            \
Update##base##IfRelocated(JSRuntime* rt, BarrieredBase<type*>* thingp)                            \
{                                                                                                 \
    return UpdateIfRelocated<type>(rt, thingp->unsafeGet());                                      \
}                                                                                                 \
                                                                                                  \
type *                                                                                            \
Update##base##IfRelocated(JSRuntime* rt, type** thingp)                                           \
{                                                                                                 \
    return UpdateIfRelocated<type>(rt, thingp);                                                   \
}


DeclMarkerImpl(JitCode, jit::JitCode)
DeclMarkerImpl(Object, NativeObject)
DeclMarkerImpl(Object, ArrayObject)
DeclMarkerImpl(Object, ArgumentsObject)
DeclMarkerImpl(Object, ArrayBufferObject)
DeclMarkerImpl(Object, ArrayBufferObjectMaybeShared)
DeclMarkerImpl(Object, ArrayBufferViewObject)
DeclMarkerImpl(Object, DebugScopeObject)
DeclMarkerImpl(Object, GlobalObject)
DeclMarkerImpl(Object, JSObject)
DeclMarkerImpl(Object, JSFunction)
DeclMarkerImpl(Object, NestedScopeObject)
DeclMarkerImpl(Object, PlainObject)
DeclMarkerImpl(Object, SavedFrame)
DeclMarkerImpl(Object, ScopeObject)
DeclMarkerImpl(Object, SharedArrayBufferObject)
DeclMarkerImpl(Object, SharedTypedArrayObject)
DeclMarkerImpl(String, JSAtom)
DeclMarkerImpl(String, JSString)
DeclMarkerImpl(String, JSFlatString)
DeclMarkerImpl(String, JSLinearString)
DeclMarkerImpl(String, PropertyName)

} 
} 



void
gc::MarkKind(JSTracer* trc, void** thingp, JSGCTraceKind kind)
{
    MOZ_ASSERT(thingp);
    MOZ_ASSERT(*thingp);
    DebugOnly<Cell*> cell = static_cast<Cell*>(*thingp);
    MOZ_ASSERT_IF(cell->isTenured(),
                  kind == MapAllocToTraceKind(cell->asTenured().getAllocKind()));
    switch (kind) {
      case JSTRACE_OBJECT:
        MarkInternal(trc, reinterpret_cast<JSObject**>(thingp));
        break;
      case JSTRACE_SCRIPT:
        MarkInternal(trc, reinterpret_cast<JSScript**>(thingp));
        break;
      case JSTRACE_STRING:
        MarkInternal(trc, reinterpret_cast<JSString**>(thingp));
        break;
      case JSTRACE_SYMBOL:
        MarkInternal(trc, reinterpret_cast<JS::Symbol**>(thingp));
        break;
      case JSTRACE_BASE_SHAPE:
        MarkInternal(trc, reinterpret_cast<BaseShape**>(thingp));
        break;
      case JSTRACE_JITCODE:
        MarkInternal(trc, reinterpret_cast<jit::JitCode**>(thingp));
        break;
      case JSTRACE_LAZY_SCRIPT:
        MarkInternal(trc, reinterpret_cast<LazyScript**>(thingp));
        break;
      case JSTRACE_SHAPE:
        MarkInternal(trc, reinterpret_cast<Shape**>(thingp));
        break;
      case JSTRACE_OBJECT_GROUP:
        MarkInternal(trc, reinterpret_cast<ObjectGroup**>(thingp));
        break;
      default:
        MOZ_CRASH("Invalid trace kind in MarkKind.");
    }
}

static void
MarkGCThingInternal(JSTracer* trc, void** thingp, const char* name)
{
    trc->setTracingName(name);
    MOZ_ASSERT(thingp);
    if (!*thingp)
        return;
    MarkKind(trc, thingp, GetGCThingTraceKind(*thingp));
}

void
gc::MarkGCThingRoot(JSTracer* trc, void** thingp, const char* name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    MarkGCThingInternal(trc, thingp, name);
}

void
gc::MarkGCThingUnbarriered(JSTracer* trc, void** thingp, const char* name)
{
    MarkGCThingInternal(trc, thingp, name);
}



static inline void
MarkValueInternal(JSTracer* trc, Value* v)
{
    if (v->isMarkable()) {
        MOZ_ASSERT(v->toGCThing());
        void* thing = v->toGCThing();
        trc->setTracingLocation((void*)v);
        if (v->isString()) {
            JSString* str = static_cast<JSString*>(thing);
            MarkInternal(trc, &str);
            if (str != thing)
                v->setString(str);
        } else if (v->isObject()) {
            JSObject* obj = static_cast<JSObject*>(thing);
            MarkInternal(trc, &obj);
            if (obj != thing)
                v->setObjectOrNull(obj);
        } else {
            MOZ_ASSERT(v->isSymbol());
            JS::Symbol* sym = static_cast<JS::Symbol*>(thing);
            MarkInternal(trc, &sym);
            if (sym != thing)
                v->setSymbol(sym);
        }
    } else {
        
        trc->unsetTracingLocation();
    }
}



void
TypeSet::MarkTypeRoot(JSTracer* trc, TypeSet::Type* v, const char* name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    MarkTypeUnbarriered(trc, v, name);
}

void
TypeSet::MarkTypeUnbarriered(JSTracer* trc, TypeSet::Type* v, const char* name)
{
    trc->setTracingName(name);
    if (v->isSingletonUnchecked()) {
        JSObject* obj = v->singleton();
        MarkInternal(trc, &obj);
        *v = TypeSet::ObjectType(obj);
    } else if (v->isGroupUnchecked()) {
        ObjectGroup* group = v->group();
        MarkInternal(trc, &group);
        *v = TypeSet::ObjectType(group);
    }
}



void
gc::MarkObjectSlots(JSTracer* trc, NativeObject* obj, uint32_t start, uint32_t nslots)
{
    MOZ_ASSERT(obj->isNative());
    for (uint32_t i = start; i < (start + nslots); ++i) {
        trc->setTracingDetails(GetObjectSlotName, obj, i);
        MarkValueInternal(trc, obj->getSlotRef(i).unsafeGet());
    }
}

static bool
ShouldMarkCrossCompartment(JSTracer* trc, JSObject* src, Cell* cell)
{
    if (!trc->isMarkingTracer())
        return true;

    uint32_t color = AsGCMarker(trc)->markColor();
    MOZ_ASSERT(color == BLACK || color == GRAY);

    if (IsInsideNursery(cell)) {
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



void
gc::MarkValueForBarrier(JSTracer* trc, Value* valuep, const char* name)
{
    MOZ_ASSERT(!trc->runtime()->isHeapBusy());
    TraceManuallyBarrieredEdge(trc, valuep, name);
}

void
gc::MarkIdForBarrier(JSTracer* trc, jsid* idp, const char* name)
{
    TraceManuallyBarrieredEdge(trc, idp, name);
}










static void
MaybePushMarkStackBetweenSlices(GCMarker* gcmarker, JSObject* thing)
{
    MOZ_ASSERT_IF(gcmarker->runtime()->isHeapBusy(), !IsInsideNursery(thing));

    if (!IsInsideNursery(thing))
        gcmarker->traverse(thing);
}

static void
ScanShape(GCMarker* gcmarker, Shape* shape);

static void
PushMarkStack(GCMarker* gcmarker, Shape* thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), thing);
    MOZ_ASSERT(!IsInsideNursery(thing));

    
    if (thing->markIfUnmarked(gcmarker->markColor()))
        ScanShape(gcmarker, thing);
}

static inline void
ScanBaseShape(GCMarker* gcmarker, BaseShape* base);

void
BaseShape::markChildren(JSTracer* trc)
{
    if (isOwned())
        TraceEdge(trc, &unowned_, "base");

    JSObject* global = compartment()->unsafeUnbarrieredMaybeGlobal();
    if (global)
        MarkObjectUnbarriered(trc, &global, "global");
}

static void
PushMarkStack(GCMarker* gcmarker, BaseShape* thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), thing);
    MOZ_ASSERT(!IsInsideNursery(thing));

    
    if (thing->markIfUnmarked(gcmarker->markColor()))
        ScanBaseShape(gcmarker, thing);
}

static void
ScanShape(GCMarker* gcmarker, Shape* shape)
{
  restart:
    PushMarkStack(gcmarker, shape->base());

    const BarrieredBase<jsid>& id = shape->propidRef();
    if (JSID_IS_STRING(id))
        PushMarkStack(gcmarker, JSID_TO_STRING(id));
    else if (JSID_IS_SYMBOL(id))
        PushMarkStack(gcmarker, JSID_TO_SYMBOL(id));

    if (shape->hasGetterObject())
        MaybePushMarkStackBetweenSlices(gcmarker, shape->getterObject());

    if (shape->hasSetterObject())
        MaybePushMarkStackBetweenSlices(gcmarker, shape->setterObject());

    shape = shape->previous();
    if (shape && shape->markIfUnmarked(gcmarker->markColor()))
        goto restart;
}

static inline void
ScanBaseShape(GCMarker* gcmarker, BaseShape* base)
{
    base->assertConsistency();

    base->compartment()->mark();

    if (GlobalObject* global = base->compartment()->unsafeUnbarrieredMaybeGlobal())
        gcmarker->traverse(global);

    




    if (base->isOwned()) {
        UnownedBaseShape* unowned = base->baseUnowned();
        MOZ_ASSERT(base->compartment() == unowned->compartment());
        unowned->markIfUnmarked(gcmarker->markColor());
    }
}

static inline void
ScanLinearString(GCMarker* gcmarker, JSLinearString* str)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), str);
    MOZ_ASSERT(str->isMarked());

    



    MOZ_ASSERT(str->JSString::isLinear());
    while (str->hasBase()) {
        str = str->base();
        MOZ_ASSERT(str->JSString::isLinear());
        if (str->isPermanentAtom())
            break;
        JS_COMPARTMENT_ASSERT(gcmarker->runtime(), str);
        if (!str->markIfUnmarked())
            break;
    }
}










static void
ScanRope(GCMarker* gcmarker, JSRope* rope)
{
    ptrdiff_t savedPos = gcmarker->stack.position();
    JS_DIAGNOSTICS_ASSERT(GetGCThingTraceKind(rope) == JSTRACE_STRING);
    for (;;) {
        JS_DIAGNOSTICS_ASSERT(GetGCThingTraceKind(rope) == JSTRACE_STRING);
        JS_DIAGNOSTICS_ASSERT(rope->JSString::isRope());
        JS_COMPARTMENT_ASSERT(gcmarker->runtime(), rope);
        MOZ_ASSERT(rope->isMarked());
        JSRope* next = nullptr;

        JSString* right = rope->rightChild();
        if (!right->isPermanentAtom() && right->markIfUnmarked()) {
            if (right->isLinear())
                ScanLinearString(gcmarker, &right->asLinear());
            else
                next = &right->asRope();
        }

        JSString* left = rope->leftChild();
        if (!left->isPermanentAtom() && left->markIfUnmarked()) {
            if (left->isLinear()) {
                ScanLinearString(gcmarker, &left->asLinear());
            } else {
                



                if (next && !gcmarker->stack.push(reinterpret_cast<uintptr_t>(next)))
                    gcmarker->delayMarkingChildren(next);
                next = &left->asRope();
            }
        }
        if (next) {
            rope = next;
        } else if (savedPos != gcmarker->stack.position()) {
            MOZ_ASSERT(savedPos < gcmarker->stack.position());
            rope = reinterpret_cast<JSRope*>(gcmarker->stack.pop());
        } else {
            break;
        }
    }
    MOZ_ASSERT(savedPos == gcmarker->stack.position());
 }

static inline void
ScanString(GCMarker* gcmarker, JSString* str)
{
    if (str->isLinear())
        ScanLinearString(gcmarker, &str->asLinear());
    else
        ScanRope(gcmarker, &str->asRope());
}

static inline void
PushMarkStack(GCMarker* gcmarker, JSString* str)
{
    
    if (str->isPermanentAtom())
        return;

    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), str);

    




    if (str->markIfUnmarked())
        ScanString(gcmarker, str);
}

static inline void
ScanSymbol(GCMarker* gcmarker, JS::Symbol* sym)
{
    if (JSString* desc = sym->description())
        PushMarkStack(gcmarker, desc);
}

static inline void
PushMarkStack(GCMarker* gcmarker, JS::Symbol* sym)
{
    
    if (sym->isWellKnownSymbol())
        return;

    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), sym);
    MOZ_ASSERT(!IsInsideNursery(sym));

    if (sym->markIfUnmarked())
        ScanSymbol(gcmarker, sym);
}








void
gc::MarkCycleCollectorChildren(JSTracer* trc, Shape* shape)
{
    




    JSObject* global = shape->compartment()->unsafeUnbarrieredMaybeGlobal();
    MOZ_ASSERT(global);
    MarkObjectUnbarriered(trc, &global, "global");

    do {
        MOZ_ASSERT(global == shape->compartment()->unsafeUnbarrieredMaybeGlobal());

        MOZ_ASSERT(shape->base());
        shape->base()->assertConsistency();

        TraceEdge(trc, &shape->propidRef(), "propid");

        if (shape->hasGetterObject()) {
            JSObject* tmp = shape->getterObject();
            MarkObjectUnbarriered(trc, &tmp, "getter");
            MOZ_ASSERT(tmp == shape->getterObject());
        }

        if (shape->hasSetterObject()) {
            JSObject* tmp = shape->setterObject();
            MarkObjectUnbarriered(trc, &tmp, "setter");
            MOZ_ASSERT(tmp == shape->setterObject());
        }

        shape = shape->previous();
    } while (shape);
}

static void
ScanObjectGroup(GCMarker* gcmarker, ObjectGroup* group)
{
    unsigned count = group->getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        if (ObjectGroup::Property* prop = group->getProperty(i))
            DoMarking(gcmarker, prop->id.get());
    }

    if (group->proto().isObject())
        gcmarker->traverse(group->proto().toObject());

    group->compartment()->mark();

    if (GlobalObject* global = group->compartment()->unsafeUnbarrieredMaybeGlobal())
        PushMarkStack(gcmarker, global);

    if (group->newScript())
        group->newScript()->trace(gcmarker);

    if (group->maybePreliminaryObjects())
        group->maybePreliminaryObjects()->trace(gcmarker);

    if (group->maybeUnboxedLayout())
        group->unboxedLayout().trace(gcmarker);

    if (ObjectGroup* unboxedGroup = group->maybeOriginalUnboxedGroup())
        PushMarkStack(gcmarker, unboxedGroup);

    if (TypeDescr* descr = group->maybeTypeDescr())
        gcmarker->traverse(descr);

    if (JSFunction* fun = group->maybeInterpretedFunction())
        gcmarker->traverse(fun);
}

static void
gc::MarkChildren(JSTracer* trc, ObjectGroup* group)
{
    unsigned count = group->getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        if (ObjectGroup::Property* prop = group->getProperty(i))
            TraceEdge(trc, &prop->id, "group_property");
    }

    if (group->proto().isObject())
        MarkObject(trc, &group->protoRaw(), "group_proto");

    if (group->newScript())
        group->newScript()->trace(trc);

    if (group->maybePreliminaryObjects())
        group->maybePreliminaryObjects()->trace(trc);

    if (group->maybeUnboxedLayout())
        group->unboxedLayout().trace(trc);

    if (ObjectGroup* unboxedGroup = group->maybeOriginalUnboxedGroup()) {
        TraceManuallyBarrieredEdge(trc, &unboxedGroup, "group_original_unboxed_group");
        group->setOriginalUnboxedGroup(unboxedGroup);
    }

    if (JSObject* descr = group->maybeTypeDescr()) {
        MarkObjectUnbarriered(trc, &descr, "group_type_descr");
        group->setTypeDescr(&descr->as<TypeDescr>());
    }

    if (JSObject* fun = group->maybeInterpretedFunction()) {
        MarkObjectUnbarriered(trc, &fun, "group_function");
        group->setInterpretedFunction(&fun->as<JSFunction>());
    }
}

template<typename T>
static void
PushArenaTyped(GCMarker* gcmarker, ArenaHeader* aheader)
{
    for (ArenaCellIterUnderGC i(aheader); !i.done(); i.next())
        PushMarkStack(gcmarker, i.get<T>());
}

void
gc::PushArena(GCMarker* gcmarker, ArenaHeader* aheader)
{
    switch (MapAllocToTraceKind(aheader->getAllocKind())) {
      case JSTRACE_OBJECT:
        PushArenaTyped<JSObject>(gcmarker, aheader);
        break;

      case JSTRACE_SCRIPT:
        PushArenaTyped<JSScript>(gcmarker, aheader);
        break;

      case JSTRACE_STRING:
        PushArenaTyped<JSString>(gcmarker, aheader);
        break;

      case JSTRACE_SYMBOL:
        PushArenaTyped<JS::Symbol>(gcmarker, aheader);
        break;

      case JSTRACE_BASE_SHAPE:
        PushArenaTyped<js::BaseShape>(gcmarker, aheader);
        break;

      case JSTRACE_JITCODE:
        PushArenaTyped<js::jit::JitCode>(gcmarker, aheader);
        break;

      case JSTRACE_LAZY_SCRIPT:
        PushArenaTyped<LazyScript>(gcmarker, aheader);
        break;

      case JSTRACE_SHAPE:
        PushArenaTyped<js::Shape>(gcmarker, aheader);
        break;

      case JSTRACE_OBJECT_GROUP:
        PushArenaTyped<js::ObjectGroup>(gcmarker, aheader);
        break;

      default:
        MOZ_CRASH("Invalid trace kind in PushArena.");
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
GCMarker::restoreValueArray(NativeObject* obj, void** vpp, void** endp)
{
    uintptr_t start = stack.pop();
    HeapSlot::Kind kind = (HeapSlot::Kind) stack.pop();

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

void
GCMarker::processMarkStackOther(uintptr_t tag, uintptr_t addr)
{
    if (tag == GroupTag) {
        ScanObjectGroup(this, reinterpret_cast<ObjectGroup*>(addr));
    } else if (tag == SavedValueArrayTag) {
        MOZ_ASSERT(!(addr & CellMask));
        NativeObject* obj = reinterpret_cast<NativeObject*>(addr);
        HeapValue* vp;
        HeapValue* end;
        if (restoreValueArray(obj, (void**)&vp, (void**)&end))
            pushValueArray(obj, vp, end);
        else
            repush(obj);
    } else if (tag == JitCodeTag) {
        reinterpret_cast<jit::JitCode*>(addr)->trace(this);
    }
}

MOZ_ALWAYS_INLINE void
GCMarker::markAndScanString(JSObject* source, JSString* str)
{
    if (!str->isPermanentAtom()) {
        JS_COMPARTMENT_ASSERT(runtime(), str);
        MOZ_ASSERT(runtime()->isAtomsZone(str->zone()) || str->zone() == source->zone());
        if (str->markIfUnmarked())
            ScanString(this, str);
    }
}

MOZ_ALWAYS_INLINE void
GCMarker::markAndScanSymbol(JSObject* source, JS::Symbol* sym)
{
    if (!sym->isWellKnownSymbol()) {
        JS_COMPARTMENT_ASSERT(runtime(), sym);
        MOZ_ASSERT(runtime()->isAtomsZone(sym->zone()) || sym->zone() == source->zone());
        if (sym->markIfUnmarked())
            ScanSymbol(this, sym);
    }
}

inline void
GCMarker::processMarkStackTop(SliceBudget& budget)
{
    




    HeapSlot* vp;
    HeapSlot* end;
    JSObject* obj;

    const int32_t* unboxedTraceList;
    uint8_t* unboxedMemory;

    uintptr_t addr = stack.pop();
    uintptr_t tag = addr & StackTagMask;
    addr &= ~StackTagMask;

    if (tag == ValueArrayTag) {
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

    if (tag == ObjectTag) {
        obj = reinterpret_cast<JSObject*>(addr);
        JS_COMPARTMENT_ASSERT(runtime(), obj);
        goto scan_obj;
    }

    processMarkStackOther(tag, addr);
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
            markAndScanString(obj, v.toString());
        } else if (v.isObject()) {
            JSObject* obj2 = &v.toObject();
            MOZ_ASSERT(obj->compartment() == obj2->compartment());
            if (mark(obj2)) {
                
                pushValueArray(obj, vp, end);
                obj = obj2;
                goto scan_obj;
            }
        } else if (v.isSymbol()) {
            markAndScanSymbol(obj, v.toSymbol());
        }
    }
    return;

  scan_unboxed:
    {
        while (*unboxedTraceList != -1) {
            JSString* str = *reinterpret_cast<JSString**>(unboxedMemory + *unboxedTraceList);
            markAndScanString(obj, str);
            unboxedTraceList++;
        }
        unboxedTraceList++;
        while (*unboxedTraceList != -1) {
            JSObject* obj2 = *reinterpret_cast<JSObject**>(unboxedMemory + *unboxedTraceList);
            MOZ_ASSERT_IF(obj2, obj->compartment() == obj2->compartment());
            if (obj2 && mark(obj2))
                repush(obj2);
            unboxedTraceList++;
        }
        unboxedTraceList++;
        while (*unboxedTraceList != -1) {
            const Value& v = *reinterpret_cast<Value*>(unboxedMemory + *unboxedTraceList);
            if (v.isString()) {
                markAndScanString(obj, v.toString());
            } else if (v.isObject()) {
                JSObject* obj2 = &v.toObject();
                MOZ_ASSERT(obj->compartment() == obj2->compartment());
                if (mark(obj2))
                    repush(obj2);
            } else if (v.isSymbol()) {
                markAndScanSymbol(obj, v.toSymbol());
            }
            unboxedTraceList++;
        }
        return;
    }

  scan_obj:
    {
        JS_COMPARTMENT_ASSERT(runtime(), obj);

        budget.step();
        if (budget.isOverBudget()) {
            repush(obj);
            return;
        }

        ObjectGroup* group = obj->groupFromGC();
        traverse(group);

        
        const Class* clasp = group->clasp();
        if (clasp->trace) {
            
            
            
            MOZ_ASSERT_IF(!(clasp->trace == JS_GlobalObjectTraceHook &&
                            (!obj->compartment()->options().getTrace() || !obj->isOwnGlobal())),
                          clasp->flags & JSCLASS_IMPLEMENTS_BARRIERS);
            if (clasp->trace == InlineTypedObject::obj_trace) {
                Shape* shape = obj->as<InlineTypedObject>().shapeFromGC();
                PushMarkStack(this, shape);
                TypeDescr* descr = &obj->as<InlineTypedObject>().typeDescr();
                if (!descr->hasTraceList())
                    return;
                unboxedTraceList = descr->traceList();
                unboxedMemory = obj->as<InlineTypedObject>().inlineTypedMem();
                goto scan_unboxed;
            }
            if (clasp == &UnboxedPlainObject::class_) {
                JSObject* expando = obj->as<UnboxedPlainObject>().maybeExpando();
                if (expando && mark(expando))
                    repush(expando);
                const UnboxedLayout& layout = obj->as<UnboxedPlainObject>().layout();
                unboxedTraceList = layout.traceList();
                if (!unboxedTraceList)
                    return;
                unboxedMemory = obj->as<UnboxedPlainObject>().data();
                goto scan_unboxed;
            }
            clasp->trace(this, obj);
        }

        if (!clasp->isNative())
            return;

        NativeObject* nobj = &obj->as<NativeObject>();

        Shape* shape = nobj->lastProperty();
        PushMarkStack(this, shape);

        unsigned nslots = nobj->slotSpan();

        do {
            if (nobj->hasEmptyElements())
                break;

            if (nobj->denseElementsAreCopyOnWrite()) {
                JSObject* owner = nobj->getElementsHeader()->ownerObject();
                if (owner != nobj) {
                    traverse(owner);
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

template <typename T>
void
GCMarker::markChildren(T* thing)
{
    thing->markChildren(this);
}

void
js::TraceChildren(JSTracer* trc, void* thing, JSGCTraceKind kind)
{
    switch (kind) {
      case JSTRACE_OBJECT:
        static_cast<JSObject*>(thing)->markChildren(trc);
        break;

      case JSTRACE_SCRIPT:
        static_cast<JSScript*>(thing)->markChildren(trc);
        break;

      case JSTRACE_STRING:
        static_cast<JSString*>(thing)->markChildren(trc);
        break;

      case JSTRACE_SYMBOL:
        static_cast<JS::Symbol*>(thing)->markChildren(trc);
        break;

      case JSTRACE_BASE_SHAPE:
        static_cast<BaseShape*>(thing)->markChildren(trc);
        break;

      case JSTRACE_JITCODE:
        static_cast<jit::JitCode*>(thing)->trace(trc);
        break;

      case JSTRACE_LAZY_SCRIPT:
        static_cast<LazyScript*>(thing)->markChildren(trc);
        break;

      case JSTRACE_SHAPE:
        static_cast<Shape*>(thing)->markChildren(trc);
        break;

      case JSTRACE_OBJECT_GROUP:
        MarkChildren(trc, (ObjectGroup*)thing);
        break;

      default:
        MOZ_CRASH("Invalid trace kind in TraceChildren.");
    }
}

#ifdef DEBUG
static void
AssertNonGrayGCThing(JS::CallbackTracer* trc, void** thingp, JSGCTraceKind kind)
{
    DebugOnly<Cell*> thing(static_cast<Cell*>(*thingp));
    MOZ_ASSERT_IF(thing->isTenured(), !thing->asTenured().isMarked(js::gc::GRAY));
}

template <typename T>
bool
gc::ZoneIsGCMarking(T* thing)
{
    return thing->zone()->isGCMarking();
}

template <typename T>
bool
js::gc::ZoneIsAtomsZoneForString(JSRuntime* rt, T* thing)
{
    JSGCTraceKind kind = GetGCThingTraceKind(thing);
    if (kind == JSTRACE_STRING || kind == JSTRACE_SYMBOL)
        return rt->isAtomsZone(thing->zone());
    return false;
}
#endif

static void
UnmarkGrayChildren(JS::CallbackTracer* trc, void** thingp, JSGCTraceKind kind);

struct UnmarkGrayTracer : public JS::CallbackTracer
{
    



    explicit UnmarkGrayTracer(JSRuntime* rt)
      : JS::CallbackTracer(rt, UnmarkGrayChildren, DoNotTraceWeakMaps),
        tracingShape(false),
        previousShape(nullptr),
        unmarkedAny(false)
    {}

    UnmarkGrayTracer(JSTracer* trc, bool tracingShape)
      : JS::CallbackTracer(trc->runtime(), UnmarkGrayChildren, DoNotTraceWeakMaps),
        tracingShape(tracingShape),
        previousShape(nullptr),
        unmarkedAny(false)
    {}

    
    bool tracingShape;

    
    Shape* previousShape;

    
    bool unmarkedAny;
};































static void
UnmarkGrayChildren(JS::CallbackTracer* trc, void** thingp, JSGCTraceKind kind)
{
    int stackDummy;
    if (!JS_CHECK_STACK_SIZE(trc->runtime()->mainThread.nativeStackLimit[StackForSystemCode],
                             &stackDummy))
    {
        



        trc->runtime()->gc.setGrayBitsInvalid();
        return;
    }

    Cell* cell = static_cast<Cell*>(*thingp);

    
    
    if (!cell->isTenured()) {
#ifdef DEBUG
        JS::CallbackTracer nongray(trc->runtime(), AssertNonGrayGCThing);
        TraceChildren(&nongray, cell, kind);
#endif
        return;
    }

    TenuredCell& tenured = cell->asTenured();
    if (!tenured.isMarked(js::gc::GRAY))
        return;
    tenured.unmark(js::gc::GRAY);

    UnmarkGrayTracer* tracer = static_cast<UnmarkGrayTracer*>(trc);
    tracer->unmarkedAny = true;

    
    
    
    
    
    UnmarkGrayTracer childTracer(tracer, kind == JSTRACE_SHAPE);

    if (kind != JSTRACE_SHAPE) {
        TraceChildren(&childTracer, &tenured, kind);
        MOZ_ASSERT(!childTracer.previousShape);
        tracer->unmarkedAny |= childTracer.unmarkedAny;
        return;
    }

    MOZ_ASSERT(kind == JSTRACE_SHAPE);
    Shape* shape = static_cast<Shape*>(&tenured);
    if (tracer->tracingShape) {
        MOZ_ASSERT(!tracer->previousShape);
        tracer->previousShape = shape;
        return;
    }

    do {
        MOZ_ASSERT(!shape->isMarked(js::gc::GRAY));
        TraceChildren(&childTracer, shape, JSTRACE_SHAPE);
        shape = childTracer.previousShape;
        childTracer.previousShape = nullptr;
    } while (shape);
    tracer->unmarkedAny |= childTracer.unmarkedAny;
}

bool
js::UnmarkGrayCellRecursively(gc::Cell* cell, JSGCTraceKind kind)
{
    MOZ_ASSERT(cell);

    JSRuntime* rt = cell->runtimeFromMainThread();

    
    
    if (rt->isHeapBusy())
        return false;

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
    return js::UnmarkGrayCellRecursively(shape, JSTRACE_SHAPE);
}

JS_FRIEND_API(bool)
JS::UnmarkGrayGCThingRecursively(JS::GCCellPtr thing)
{
    return js::UnmarkGrayCellRecursively(thing.asCell(), thing.kind());
}
