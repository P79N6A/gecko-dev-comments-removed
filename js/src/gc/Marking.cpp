





#include "gc/Marking.h"

#include "mozilla/DebugOnly.h"

#include "jsprf.h"

#include "jit/IonCode.h"
#include "js/SliceBudget.h"
#include "vm/ArgumentsObject.h"
#include "vm/ScopeObject.h"
#include "vm/Shape.h"
#include "vm/Symbol.h"
#include "vm/TypedArrayObject.h"

#include "jscompartmentinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"

#ifdef JSGC_GENERATIONAL
# include "gc/Nursery-inl.h"
#endif
#include "vm/String-inl.h"
#include "vm/Symbol-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::DebugOnly;

void * const js::NullPtr::constNullValue = nullptr;

JS_PUBLIC_DATA(void * const) JS::NullPtr::constNullValue = nullptr;































static inline void
PushMarkStack(GCMarker *gcmarker, ObjectImpl *thing);

static inline void
PushMarkStack(GCMarker *gcmarker, JSFunction *thing);

static inline void
PushMarkStack(GCMarker *gcmarker, JSScript *thing);

static inline void
PushMarkStack(GCMarker *gcmarker, Shape *thing);

static inline void
PushMarkStack(GCMarker *gcmarker, JSString *str);

static inline void
PushMarkStack(GCMarker *gcmarker, JS::Symbol *sym);

static inline void
PushMarkStack(GCMarker *gcmarker, types::TypeObject *thing);

namespace js {
namespace gc {

static void MarkChildren(JSTracer *trc, JSString *str);
static void MarkChildren(JSTracer *trc, JS::Symbol *sym);
static void MarkChildren(JSTracer *trc, JSScript *script);
static void MarkChildren(JSTracer *trc, LazyScript *lazy);
static void MarkChildren(JSTracer *trc, Shape *shape);
static void MarkChildren(JSTracer *trc, BaseShape *base);
static void MarkChildren(JSTracer *trc, types::TypeObject *type);
static void MarkChildren(JSTracer *trc, jit::JitCode *code);

} 
} 



#if defined(DEBUG)
template<typename T>
static inline bool
IsThingPoisoned(T *thing)
{
    static_assert(sizeof(T) >= sizeof(FreeSpan) + sizeof(uint32_t),
                  "Ensure it is well defined to look past any free span that "
                  "may be embedded in the thing's header when freed.");
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
    uint32_t *p = reinterpret_cast<uint32_t *>(reinterpret_cast<FreeSpan *>(thing) + 1);
    
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

static GCMarker *
AsGCMarker(JSTracer *trc)
{
    JS_ASSERT(IS_GC_MARKING_TRACER(trc));
    return static_cast<GCMarker *>(trc);
}

template <typename T> bool ThingIsPermanentAtom(T *thing) { return false; }
template <> bool ThingIsPermanentAtom<JSString>(JSString *str) { return str->isPermanentAtom(); }
template <> bool ThingIsPermanentAtom<JSFlatString>(JSFlatString *str) { return str->isPermanentAtom(); }
template <> bool ThingIsPermanentAtom<JSLinearString>(JSLinearString *str) { return str->isPermanentAtom(); }
template <> bool ThingIsPermanentAtom<JSAtom>(JSAtom *atom) { return atom->isPermanent(); }
template <> bool ThingIsPermanentAtom<PropertyName>(PropertyName *name) { return name->isPermanent(); }
template <> bool ThingIsPermanentAtom<JS::Symbol>(JS::Symbol *sym) { return sym->isWellKnownSymbol(); }

template<typename T>
static inline void
CheckMarkedThing(JSTracer *trc, T **thingp)
{
#ifdef DEBUG
    JS_ASSERT(trc);
    JS_ASSERT(thingp);

    T *thing = *thingp;
    JS_ASSERT(*thingp);

#ifdef JSGC_COMPACTING
    thing = MaybeForwarded(thing);
#endif

# ifdef JSGC_FJGENERATIONAL
    




    if (ForkJoinContext::current())
        return;
# endif

    
    if (IsInsideNursery(thing))
        return;

#ifdef JSGC_COMPACTING
    JS_ASSERT_IF(!MovingTracer::IsMovingTracer(trc) && !Nursery::IsMinorCollectionTracer(trc),
                 !IsForwarded(*thingp));
#endif

    



    if (ThingIsPermanentAtom(thing))
        return;

    JS_ASSERT(thing->zone());
    JS_ASSERT(thing->zone()->runtimeFromMainThread() == trc->runtime());
    JS_ASSERT(trc->hasTracingDetails());

    DebugOnly<JSRuntime *> rt = trc->runtime();

    bool isGcMarkingTracer = IS_GC_MARKING_TRACER(trc);

    JS_ASSERT(CurrentThreadCanAccessRuntime(rt));

    JS_ASSERT_IF(thing->zone()->requireGCTracer(), isGcMarkingTracer);

    JS_ASSERT(thing->isAligned());

    JS_ASSERT(MapTypeToTraceKind<T>::kind == GetGCThingTraceKind(thing));

    if (isGcMarkingTracer) {
        GCMarker *gcMarker = static_cast<GCMarker *>(trc);
        JS_ASSERT_IF(gcMarker->shouldCheckCompartments(),
                     thing->zone()->isCollecting() || rt->isAtomsZone(thing->zone()));

        JS_ASSERT_IF(gcMarker->getMarkColor() == GRAY,
                     !thing->zone()->isGCMarkingBlack() || rt->isAtomsZone(thing->zone()));

        JS_ASSERT(!(thing->zone()->isGCSweeping() || thing->zone()->isGCFinished()));
    }

    





    JS_ASSERT_IF(IsThingPoisoned(thing) && rt->isHeapBusy(),
                 !InFreeList(thing->asTenured()->arenaHeader(), thing));
#endif
}








template<typename T>
static inline void
SetMaybeAliveFlag(T *thing)
{
}

template<>
void
SetMaybeAliveFlag(JSObject *thing)
{
    thing->compartment()->maybeAlive = true;
}

template<>
void
SetMaybeAliveFlag(JSScript *thing)
{
    thing->compartment()->maybeAlive = true;
}

template<typename T>
static void
MarkInternal(JSTracer *trc, T **thingp)
{
    CheckMarkedThing(trc, thingp);
    T *thing = *thingp;

    if (!trc->callback) {
#ifdef JSGC_FJGENERATIONAL
        




        JS_ASSERT(!ForkJoinContext::current());
        JS_ASSERT(!trc->runtime()->isFJMinorCollecting());
#endif

        





        if (IsInsideNursery(thing))
            return;

        




        if (ThingIsPermanentAtom(thing))
            return;

        



        if (!thing->zone()->isGCMarking())
            return;

        PushMarkStack(AsGCMarker(trc), thing);
        SetMaybeAliveFlag(thing);
    } else {
        trc->callback(trc, (void **)thingp, MapTypeToTraceKind<T>::kind);
        trc->unsetTracingLocation();
    }

    trc->clearTracingDetails();
}

#define JS_ROOT_MARKING_ASSERT(trc)                                     \
    JS_ASSERT_IF(IS_GC_MARKING_TRACER(trc),                             \
                 trc->runtime()->gc.state() == NO_INCREMENTAL ||        \
                 trc->runtime()->gc.state() == MARK_ROOTS);

namespace js {
namespace gc {

template <typename T>
void
MarkUnbarriered(JSTracer *trc, T **thingp, const char *name)
{
    trc->setTracingName(name);
    MarkInternal(trc, thingp);
}

template <typename T>
static void
Mark(JSTracer *trc, BarrieredBase<T*> *thing, const char *name)
{
    trc->setTracingName(name);
    MarkInternal(trc, thing->unsafeGet());
}

void
MarkPermanentAtom(JSTracer *trc, JSAtom *atom, const char *name)
{
    trc->setTracingName(name);

    JS_ASSERT(atom->isPermanent());

    CheckMarkedThing(trc, &atom);

    if (!trc->callback) {
        
        
        atom->markIfUnmarked();
    } else {
        void *thing = atom;
        trc->callback(trc, &thing, JSTRACE_STRING);
        JS_ASSERT(thing == atom);
        trc->unsetTracingLocation();
    }

    trc->clearTracingDetails();
}

void
MarkWellKnownSymbol(JSTracer *trc, JS::Symbol *sym)
{
    if (!sym)
        return;

    trc->setTracingName("wellKnownSymbols");

    MOZ_ASSERT(sym->isWellKnownSymbol());
    CheckMarkedThing(trc, &sym);
    if (!trc->callback) {
        
        MOZ_ASSERT(sym->description()->isMarked());
        sym->markIfUnmarked();
    } else {
        void *thing = sym;
        trc->callback(trc, &thing, JSTRACE_SYMBOL);
        MOZ_ASSERT(thing == sym);
        trc->unsetTracingLocation();
    }

    trc->clearTracingDetails();
}

} 
} 

template <typename T>
static void
MarkRoot(JSTracer *trc, T **thingp, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    trc->setTracingName(name);
    MarkInternal(trc, thingp);
}

template <typename T>
static void
MarkRange(JSTracer *trc, size_t len, HeapPtr<T*> *vec, const char *name)
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
MarkRootRange(JSTracer *trc, size_t len, T **vec, const char *name)
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
static bool
IsMarked(T **thingp)
{
    JS_ASSERT(thingp);
    JS_ASSERT(*thingp);
#ifdef JSGC_GENERATIONAL
    JSRuntime* rt = (*thingp)->runtimeFromAnyThread();
#ifdef JSGC_FJGENERATIONAL
    
    
    if (rt->isFJMinorCollecting()) {
        ForkJoinContext *ctx = ForkJoinContext::current();
        ForkJoinNursery &nursery = ctx->nursery();
        if (nursery.isInsideFromspace(*thingp))
            return nursery.getForwardedPointer(thingp);
    }
    else
#endif
    {
        if (IsInsideNursery(*thingp)) {
            Nursery &nursery = rt->gc.nursery;
            return nursery.getForwardedPointer(thingp);
        }
    }
#endif  
    Zone *zone = (*thingp)->asTenured()->zone();
    if (!zone->isCollecting() || zone->isGCFinished())
        return true;
#ifdef JSGC_COMPACTING
    if (zone->isGCCompacting() && IsForwarded(*thingp))
        *thingp = Forwarded(*thingp);
#endif
    return (*thingp)->asTenured()->isMarked();
}

template <typename T>
static bool
IsAboutToBeFinalized(T **thingp)
{
    JS_ASSERT(thingp);
    JS_ASSERT(*thingp);

    T *thing = *thingp;
    JSRuntime *rt = thing->runtimeFromAnyThread();

    
    if (ThingIsPermanentAtom(thing) && !TlsPerThreadData.get()->associatedWith(rt))
        return false;

#ifdef JSGC_GENERATIONAL
#ifdef JSGC_FJGENERATIONAL
    if (rt->isFJMinorCollecting()) {
        ForkJoinContext *ctx = ForkJoinContext::current();
        ForkJoinNursery &nursery = ctx->nursery();
        if (nursery.isInsideFromspace(thing))
            return !nursery.getForwardedPointer(thingp);
    }
    else
#endif
    {
        Nursery &nursery = rt->gc.nursery;
        JS_ASSERT_IF(!rt->isHeapMinorCollecting(), !IsInsideNursery(thing));
        if (rt->isHeapMinorCollecting()) {
            if (IsInsideNursery(thing))
                return !nursery.getForwardedPointer(thingp);
            return false;
        }
    }
#endif  

    Zone *zone = thing->asTenured()->zone();
    if (zone->isGCSweeping()) {
        






        JS_ASSERT_IF(!rt->isHeapMinorCollecting(),
                     !thing->asTenured()->arenaHeader()->allocatedDuringIncremental);

        return !thing->asTenured()->isMarked();
    }
#ifdef JSGC_COMPACTING
    else if (zone->isGCCompacting() && IsForwarded(thing)) {
        *thingp = Forwarded(thing);
        return false;
    }
#endif

    return false;
}

template <typename T>
T *
UpdateIfRelocated(JSRuntime *rt, T **thingp)
{
    JS_ASSERT(thingp);
    if (!*thingp)
        return nullptr;

#ifdef JSGC_GENERATIONAL

#ifdef JSGC_FJGENERATIONAL
    if (rt->isFJMinorCollecting()) {
        ForkJoinContext *ctx = ForkJoinContext::current();
        ForkJoinNursery &nursery = ctx->nursery();
        if (nursery.isInsideFromspace(*thingp))
            nursery.getForwardedPointer(thingp);
        return *thingp;
    }
#endif

    if (rt->isHeapMinorCollecting() && IsInsideNursery(*thingp)) {
        rt->gc.nursery.getForwardedPointer(thingp);
        return *thingp;
    }
#endif  

#ifdef JSGC_COMPACTING
    Zone *zone = (*thingp)->zone();
    if (zone->isGCCompacting() && IsForwarded(*thingp))
        *thingp = Forwarded(*thingp);
#endif
    return *thingp;
}

#define DeclMarkerImpl(base, type)                                                                \
void                                                                                              \
Mark##base(JSTracer *trc, BarrieredBase<type*> *thing, const char *name)                          \
{                                                                                                 \
    Mark<type>(trc, thing, name);                                                                 \
}                                                                                                 \
                                                                                                  \
void                                                                                              \
Mark##base##Root(JSTracer *trc, type **thingp, const char *name)                                  \
{                                                                                                 \
    MarkRoot<type>(trc, thingp, name);                                                            \
}                                                                                                 \
                                                                                                  \
void                                                                                              \
Mark##base##Unbarriered(JSTracer *trc, type **thingp, const char *name)                           \
{                                                                                                 \
    MarkUnbarriered<type>(trc, thingp, name);                                                     \
}                                                                                                 \
                                                                                                  \
/* Explicitly instantiate MarkUnbarriered<type*>. It is referenced from */                        \
/* other translation units and the instantiation might otherwise get */                           \
/* inlined away. */                                                                               \
template void MarkUnbarriered<type>(JSTracer *, type **, const char *);                           \
                                                                                                  \
void                                                                                              \
Mark##base##Range(JSTracer *trc, size_t len, HeapPtr<type*> *vec, const char *name)               \
{                                                                                                 \
    MarkRange<type>(trc, len, vec, name);                                                         \
}                                                                                                 \
                                                                                                  \
void                                                                                              \
Mark##base##RootRange(JSTracer *trc, size_t len, type **vec, const char *name)                    \
{                                                                                                 \
    MarkRootRange<type>(trc, len, vec, name);                                                     \
}                                                                                                 \
                                                                                                  \
bool                                                                                              \
Is##base##Marked(type **thingp)                                                                   \
{                                                                                                 \
    return IsMarked<type>(thingp);                                                                \
}                                                                                                 \
                                                                                                  \
bool                                                                                              \
Is##base##Marked(BarrieredBase<type*> *thingp)                                                    \
{                                                                                                 \
    return IsMarked<type>(thingp->unsafeGet());                                                   \
}                                                                                                 \
                                                                                                  \
bool                                                                                              \
Is##base##AboutToBeFinalized(type **thingp)                                                       \
{                                                                                                 \
    return IsAboutToBeFinalized<type>(thingp);                                                    \
}                                                                                                 \
                                                                                                  \
bool                                                                                              \
Is##base##AboutToBeFinalized(BarrieredBase<type*> *thingp)                                        \
{                                                                                                 \
    return IsAboutToBeFinalized<type>(thingp->unsafeGet());                                       \
}                                                                                                 \
                                                                                                  \
type *                                                                                            \
Update##base##IfRelocated(JSRuntime *rt, BarrieredBase<type*> *thingp)                            \
{                                                                                                 \
    return UpdateIfRelocated<type>(rt, thingp->unsafeGet());                                      \
}                                                                                                 \
                                                                                                  \
type *                                                                                            \
Update##base##IfRelocated(JSRuntime *rt, type **thingp)                                           \
{                                                                                                 \
    return UpdateIfRelocated<type>(rt, thingp);                                                   \
}


DeclMarkerImpl(BaseShape, BaseShape)
DeclMarkerImpl(BaseShape, UnownedBaseShape)
DeclMarkerImpl(JitCode, jit::JitCode)
DeclMarkerImpl(Object, ArgumentsObject)
DeclMarkerImpl(Object, ArrayBufferObject)
DeclMarkerImpl(Object, ArrayBufferObjectMaybeShared)
DeclMarkerImpl(Object, ArrayBufferViewObject)
DeclMarkerImpl(Object, DebugScopeObject)
DeclMarkerImpl(Object, GlobalObject)
DeclMarkerImpl(Object, JSObject)
DeclMarkerImpl(Object, JSFunction)
DeclMarkerImpl(Object, NestedScopeObject)
DeclMarkerImpl(Object, ObjectImpl)
DeclMarkerImpl(Object, SavedFrame)
DeclMarkerImpl(Object, ScopeObject)
DeclMarkerImpl(Object, SharedArrayBufferObject)
DeclMarkerImpl(Object, SharedTypedArrayObject)
DeclMarkerImpl(Script, JSScript)
DeclMarkerImpl(LazyScript, LazyScript)
DeclMarkerImpl(Shape, Shape)
DeclMarkerImpl(String, JSAtom)
DeclMarkerImpl(String, JSString)
DeclMarkerImpl(String, JSFlatString)
DeclMarkerImpl(String, JSLinearString)
DeclMarkerImpl(String, PropertyName)
DeclMarkerImpl(Symbol, JS::Symbol)
DeclMarkerImpl(TypeObject, js::types::TypeObject)

} 
} 



void
gc::MarkKind(JSTracer *trc, void **thingp, JSGCTraceKind kind)
{
    JS_ASSERT(thingp);
    JS_ASSERT(*thingp);
    DebugOnly<Cell *> cell = static_cast<Cell *>(*thingp);
    JS_ASSERT_IF(cell->isTenured(),
                 kind == MapAllocToTraceKind(cell->asTenured()->getAllocKind()));
    switch (kind) {
      case JSTRACE_OBJECT:
        MarkInternal(trc, reinterpret_cast<JSObject **>(thingp));
        break;
      case JSTRACE_STRING:
        MarkInternal(trc, reinterpret_cast<JSString **>(thingp));
        break;
      case JSTRACE_SYMBOL:
        MarkInternal(trc, reinterpret_cast<JS::Symbol **>(thingp));
        break;
      case JSTRACE_SCRIPT:
        MarkInternal(trc, reinterpret_cast<JSScript **>(thingp));
        break;
      case JSTRACE_LAZY_SCRIPT:
        MarkInternal(trc, reinterpret_cast<LazyScript **>(thingp));
        break;
      case JSTRACE_SHAPE:
        MarkInternal(trc, reinterpret_cast<Shape **>(thingp));
        break;
      case JSTRACE_BASE_SHAPE:
        MarkInternal(trc, reinterpret_cast<BaseShape **>(thingp));
        break;
      case JSTRACE_TYPE_OBJECT:
        MarkInternal(trc, reinterpret_cast<types::TypeObject **>(thingp));
        break;
      case JSTRACE_JITCODE:
        MarkInternal(trc, reinterpret_cast<jit::JitCode **>(thingp));
        break;
    }
}

static void
MarkGCThingInternal(JSTracer *trc, void **thingp, const char *name)
{
    trc->setTracingName(name);
    JS_ASSERT(thingp);
    if (!*thingp)
        return;
    MarkKind(trc, thingp, GetGCThingTraceKind(*thingp));
}

void
gc::MarkGCThingRoot(JSTracer *trc, void **thingp, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    MarkGCThingInternal(trc, thingp, name);
}

void
gc::MarkGCThingUnbarriered(JSTracer *trc, void **thingp, const char *name)
{
    MarkGCThingInternal(trc, thingp, name);
}



static inline void
MarkIdInternal(JSTracer *trc, jsid *id)
{
    if (JSID_IS_STRING(*id)) {
        JSString *str = JSID_TO_STRING(*id);
        trc->setTracingLocation((void *)id);
        MarkInternal(trc, &str);
        *id = NON_INTEGER_ATOM_TO_JSID(reinterpret_cast<JSAtom *>(str));
    } else if (JSID_IS_SYMBOL(*id)) {
        JS::Symbol *sym = JSID_TO_SYMBOL(*id);
        trc->setTracingLocation((void *)id);
        MarkInternal(trc, &sym);
        *id = SYMBOL_TO_JSID(sym);
    } else {
        
        trc->unsetTracingLocation();
    }
}

void
gc::MarkId(JSTracer *trc, BarrieredBase<jsid> *id, const char *name)
{
    trc->setTracingName(name);
    MarkIdInternal(trc, id->unsafeGet());
}

void
gc::MarkIdRoot(JSTracer *trc, jsid *id, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    trc->setTracingName(name);
    MarkIdInternal(trc, id);
}

void
gc::MarkIdUnbarriered(JSTracer *trc, jsid *id, const char *name)
{
    trc->setTracingName(name);
    MarkIdInternal(trc, id);
}

void
gc::MarkIdRange(JSTracer *trc, size_t len, HeapId *vec, const char *name)
{
    for (size_t i = 0; i < len; ++i) {
        trc->setTracingIndex(name, i);
        MarkIdInternal(trc, vec[i].unsafeGet());
    }
}

void
gc::MarkIdRootRange(JSTracer *trc, size_t len, jsid *vec, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    for (size_t i = 0; i < len; ++i) {
        trc->setTracingIndex(name, i);
        MarkIdInternal(trc, &vec[i]);
    }
}



static inline void
MarkValueInternal(JSTracer *trc, Value *v)
{
    if (v->isMarkable()) {
        JS_ASSERT(v->toGCThing());
        void *thing = v->toGCThing();
        trc->setTracingLocation((void *)v);
        MarkKind(trc, &thing, v->gcKind());
        if (v->isString()) {
            v->setString((JSString *)thing);
        } else if (v->isObject()) {
            v->setObjectOrNull((JSObject *)thing);
        } else {
            JS_ASSERT(v->isSymbol());
            v->setSymbol((JS::Symbol *)thing);
        }
    } else {
        
        trc->unsetTracingLocation();
    }
}

void
gc::MarkValue(JSTracer *trc, BarrieredBase<Value> *v, const char *name)
{
    trc->setTracingName(name);
    MarkValueInternal(trc, v->unsafeGet());
}

void
gc::MarkValueRoot(JSTracer *trc, Value *v, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    trc->setTracingName(name);
    MarkValueInternal(trc, v);
}

void
gc::MarkTypeRoot(JSTracer *trc, types::Type *v, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    trc->setTracingName(name);
    if (v->isSingleObject()) {
        JSObject *obj = v->singleObject();
        MarkInternal(trc, &obj);
        *v = types::Type::ObjectType(obj);
    } else if (v->isTypeObject()) {
        types::TypeObject *typeObj = v->typeObject();
        MarkInternal(trc, &typeObj);
        *v = types::Type::ObjectType(typeObj);
    }
}

void
gc::MarkValueRange(JSTracer *trc, size_t len, BarrieredBase<Value> *vec, const char *name)
{
    for (size_t i = 0; i < len; ++i) {
        trc->setTracingIndex(name, i);
        MarkValueInternal(trc, vec[i].unsafeGet());
    }
}

void
gc::MarkValueRootRange(JSTracer *trc, size_t len, Value *vec, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    for (size_t i = 0; i < len; ++i) {
        trc->setTracingIndex(name, i);
        MarkValueInternal(trc, &vec[i]);
    }
}

bool
gc::IsValueMarked(Value *v)
{
    JS_ASSERT(v->isMarkable());
    bool rv;
    if (v->isString()) {
        JSString *str = (JSString *)v->toGCThing();
        rv = IsMarked<JSString>(&str);
        v->setString(str);
    } else if (v->isObject()) {
        JSObject *obj = (JSObject *)v->toGCThing();
        rv = IsMarked<JSObject>(&obj);
        v->setObject(*obj);
    } else {
        JS_ASSERT(v->isSymbol());
        JS::Symbol *sym = v->toSymbol();
        rv = IsMarked<JS::Symbol>(&sym);
        v->setSymbol(sym);
    }
    return rv;
}

bool
gc::IsValueAboutToBeFinalized(Value *v)
{
    JS_ASSERT(v->isMarkable());
    bool rv;
    if (v->isString()) {
        JSString *str = (JSString *)v->toGCThing();
        rv = IsAboutToBeFinalized<JSString>(&str);
        v->setString(str);
    } else if (v->isObject()) {
        JSObject *obj = (JSObject *)v->toGCThing();
        rv = IsAboutToBeFinalized<JSObject>(&obj);
        v->setObject(*obj);
    } else {
        JS_ASSERT(v->isSymbol());
        JS::Symbol *sym = v->toSymbol();
        rv = IsAboutToBeFinalized<JS::Symbol>(&sym);
        v->setSymbol(sym);
    }
    return rv;
}



bool
gc::IsSlotMarked(HeapSlot *s)
{
    return IsMarked(s);
}

void
gc::MarkSlot(JSTracer *trc, HeapSlot *s, const char *name)
{
    trc->setTracingName(name);
    MarkValueInternal(trc, s->unsafeGet());
}

void
gc::MarkArraySlots(JSTracer *trc, size_t len, HeapSlot *vec, const char *name)
{
    for (size_t i = 0; i < len; ++i) {
        trc->setTracingIndex(name, i);
        MarkValueInternal(trc, vec[i].unsafeGet());
    }
}

void
gc::MarkObjectSlots(JSTracer *trc, JSObject *obj, uint32_t start, uint32_t nslots)
{
    JS_ASSERT(obj->isNative());
    for (uint32_t i = start; i < (start + nslots); ++i) {
        trc->setTracingDetails(js_GetObjectSlotName, obj, i);
        MarkValueInternal(trc, obj->nativeGetSlotRef(i).unsafeGet());
    }
}

static bool
ShouldMarkCrossCompartment(JSTracer *trc, JSObject *src, Cell *cell)
{
    if (!IS_GC_MARKING_TRACER(trc))
        return true;

    uint32_t color = AsGCMarker(trc)->getMarkColor();
    JS_ASSERT(color == BLACK || color == GRAY);

    if (IsInsideNursery(cell)) {
        JS_ASSERT(color == BLACK);
        return false;
    }
    TenuredCell *tenured = cell->asTenured();

    JS::Zone *zone = tenured->zone();
    if (color == BLACK) {
        






        if (tenured->isMarked(GRAY)) {
            JS_ASSERT(!zone->isCollecting());
            trc->runtime()->gc.setFoundBlackGrayEdges();
        }
        return zone->isGCMarking();
    } else {
        if (zone->isGCMarkingBlack()) {
            




            if (!tenured->isMarked())
                DelayCrossCompartmentGrayMarking(src);
            return false;
        }
        return zone->isGCMarkingGray();
    }
}

void
gc::MarkCrossCompartmentObjectUnbarriered(JSTracer *trc, JSObject *src, JSObject **dst, const char *name)
{
    if (ShouldMarkCrossCompartment(trc, src, *dst))
        MarkObjectUnbarriered(trc, dst, name);
}

void
gc::MarkCrossCompartmentScriptUnbarriered(JSTracer *trc, JSObject *src, JSScript **dst,
                                          const char *name)
{
    if (ShouldMarkCrossCompartment(trc, src, *dst))
        MarkScriptUnbarriered(trc, dst, name);
}

void
gc::MarkCrossCompartmentSlot(JSTracer *trc, JSObject *src, HeapSlot *dst, const char *name)
{
    if (dst->isMarkable() && ShouldMarkCrossCompartment(trc, src, (Cell *)dst->toGCThing()))
        MarkSlot(trc, dst, name);
}



void
gc::MarkValueUnbarriered(JSTracer *trc, Value *v, const char *name)
{
    trc->setTracingName(name);
    MarkValueInternal(trc, v);
}

bool
gc::IsCellMarked(Cell **thingp)
{
    return IsMarked<Cell>(thingp);
}

bool
gc::IsCellAboutToBeFinalized(Cell **thingp)
{
    return IsAboutToBeFinalized<Cell>(thingp);
}



#define JS_COMPARTMENT_ASSERT(rt, thing)                                \
    JS_ASSERT((thing)->zone()->isGCMarking())

#define JS_COMPARTMENT_ASSERT_STR(rt, thing)                            \
    JS_ASSERT((thing)->zone()->isGCMarking() ||                         \
              (rt)->isAtomsZone((thing)->zone()));


#define JS_COMPARTMENT_ASSERT_SYM(rt, sym)                              \
    JS_COMPARTMENT_ASSERT_STR(rt, sym)

static void
PushMarkStack(GCMarker *gcmarker, ObjectImpl *thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), thing);
    JS_ASSERT(!IsInsideNursery(thing));

    if (thing->asTenured()->markIfUnmarked(gcmarker->getMarkColor()))
        gcmarker->pushObject(thing);
}








static void
MaybePushMarkStackBetweenSlices(GCMarker *gcmarker, JSObject *thing)
{
    DebugOnly<JSRuntime *> rt = gcmarker->runtime();
    JS_COMPARTMENT_ASSERT(rt, thing);
    JS_ASSERT_IF(rt->isHeapBusy(), !IsInsideNursery(thing));

    if (!IsInsideNursery(thing) && thing->asTenured()->markIfUnmarked(gcmarker->getMarkColor()))
        gcmarker->pushObject(thing);
}

static void
PushMarkStack(GCMarker *gcmarker, JSFunction *thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), thing);
    JS_ASSERT(!IsInsideNursery(thing));

    if (thing->asTenured()->markIfUnmarked(gcmarker->getMarkColor()))
        gcmarker->pushObject(thing);
}

static void
PushMarkStack(GCMarker *gcmarker, types::TypeObject *thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), thing);
    JS_ASSERT(!IsInsideNursery(thing));

    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        gcmarker->pushType(thing);
}

static void
PushMarkStack(GCMarker *gcmarker, JSScript *thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), thing);
    JS_ASSERT(!IsInsideNursery(thing));

    




    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        MarkChildren(gcmarker, thing);
}

static void
PushMarkStack(GCMarker *gcmarker, LazyScript *thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), thing);
    JS_ASSERT(!IsInsideNursery(thing));

    



    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        MarkChildren(gcmarker, thing);
}

static void
ScanShape(GCMarker *gcmarker, Shape *shape);

static void
PushMarkStack(GCMarker *gcmarker, Shape *thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), thing);
    JS_ASSERT(!IsInsideNursery(thing));

    
    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        ScanShape(gcmarker, thing);
}

static void
PushMarkStack(GCMarker *gcmarker, jit::JitCode *thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), thing);
    JS_ASSERT(!IsInsideNursery(thing));

    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        gcmarker->pushJitCode(thing);
}

static inline void
ScanBaseShape(GCMarker *gcmarker, BaseShape *base);

static void
PushMarkStack(GCMarker *gcmarker, BaseShape *thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime(), thing);
    JS_ASSERT(!IsInsideNursery(thing));

    
    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        ScanBaseShape(gcmarker, thing);
}

static void
ScanShape(GCMarker *gcmarker, Shape *shape)
{
  restart:
    PushMarkStack(gcmarker, shape->base());

    const BarrieredBase<jsid> &id = shape->propidRef();
    if (JSID_IS_STRING(id))
        PushMarkStack(gcmarker, JSID_TO_STRING(id));
    else if (JSID_IS_SYMBOL(id))
        PushMarkStack(gcmarker, JSID_TO_SYMBOL(id));

    shape = shape->previous();
    if (shape && shape->markIfUnmarked(gcmarker->getMarkColor()))
        goto restart;
}

static inline void
ScanBaseShape(GCMarker *gcmarker, BaseShape *base)
{
    base->assertConsistency();

    base->compartment()->mark();

    if (base->hasGetterObject())
        MaybePushMarkStackBetweenSlices(gcmarker, base->getterObject());

    if (base->hasSetterObject())
        MaybePushMarkStackBetweenSlices(gcmarker, base->setterObject());

    if (JSObject *parent = base->getObjectParent()) {
        MaybePushMarkStackBetweenSlices(gcmarker, parent);
    } else if (GlobalObject *global = base->compartment()->unsafeUnbarrieredMaybeGlobal()) {
        PushMarkStack(gcmarker, global);
    }

    if (JSObject *metadata = base->getObjectMetadata())
        MaybePushMarkStackBetweenSlices(gcmarker, metadata);

    




    if (base->isOwned()) {
        UnownedBaseShape *unowned = base->baseUnowned();
        JS_ASSERT(base->compartment() == unowned->compartment());
        unowned->markIfUnmarked(gcmarker->getMarkColor());
    }
}

static inline void
ScanLinearString(GCMarker *gcmarker, JSLinearString *str)
{
    JS_COMPARTMENT_ASSERT_STR(gcmarker->runtime(), str);
    JS_ASSERT(str->isMarked());

    



    JS_ASSERT(str->JSString::isLinear());
    while (str->hasBase()) {
        str = str->base();
        JS_ASSERT(str->JSString::isLinear());
        if (str->isPermanentAtom())
            break;
        JS_COMPARTMENT_ASSERT_STR(gcmarker->runtime(), str);
        if (!str->markIfUnmarked())
            break;
    }
}










static void
ScanRope(GCMarker *gcmarker, JSRope *rope)
{
    ptrdiff_t savedPos = gcmarker->stack.position();
    JS_DIAGNOSTICS_ASSERT(GetGCThingTraceKind(rope) == JSTRACE_STRING);
    for (;;) {
        JS_DIAGNOSTICS_ASSERT(GetGCThingTraceKind(rope) == JSTRACE_STRING);
        JS_DIAGNOSTICS_ASSERT(rope->JSString::isRope());
        JS_COMPARTMENT_ASSERT_STR(gcmarker->runtime(), rope);
        JS_ASSERT(rope->isMarked());
        JSRope *next = nullptr;

        JSString *right = rope->rightChild();
        if (!right->isPermanentAtom() && right->markIfUnmarked()) {
            if (right->isLinear())
                ScanLinearString(gcmarker, &right->asLinear());
            else
                next = &right->asRope();
        }

        JSString *left = rope->leftChild();
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
            JS_ASSERT(savedPos < gcmarker->stack.position());
            rope = reinterpret_cast<JSRope *>(gcmarker->stack.pop());
        } else {
            break;
        }
    }
    JS_ASSERT(savedPos == gcmarker->stack.position());
 }

static inline void
ScanString(GCMarker *gcmarker, JSString *str)
{
    if (str->isLinear())
        ScanLinearString(gcmarker, &str->asLinear());
    else
        ScanRope(gcmarker, &str->asRope());
}

static inline void
PushMarkStack(GCMarker *gcmarker, JSString *str)
{
    
    if (str->isPermanentAtom())
        return;

    JS_COMPARTMENT_ASSERT_STR(gcmarker->runtime(), str);

    




    if (str->markIfUnmarked())
        ScanString(gcmarker, str);
}

static inline void
ScanSymbol(GCMarker *gcmarker, JS::Symbol *sym)
{
    if (JSString *desc = sym->description())
        PushMarkStack(gcmarker, desc);
}

static inline void
PushMarkStack(GCMarker *gcmarker, JS::Symbol *sym)
{
    
    if (sym->isWellKnownSymbol())
        return;

    JS_COMPARTMENT_ASSERT_SYM(gcmarker->runtime(), sym);
    JS_ASSERT(!IsInsideNursery(sym));

    if (sym->markIfUnmarked())
        ScanSymbol(gcmarker, sym);
}

void
gc::MarkChildren(JSTracer *trc, JSObject *obj)
{
    obj->markChildren(trc);
}

static void
gc::MarkChildren(JSTracer *trc, JSString *str)
{
    if (str->hasBase())
        str->markBase(trc);
    else if (str->isRope())
        str->asRope().markChildren(trc);
}

static void
gc::MarkChildren(JSTracer *trc, JS::Symbol *sym)
{
    sym->markChildren(trc);
}

static void
gc::MarkChildren(JSTracer *trc, JSScript *script)
{
    script->markChildren(trc);
}

static void
gc::MarkChildren(JSTracer *trc, LazyScript *lazy)
{
    lazy->markChildren(trc);
}

static void
gc::MarkChildren(JSTracer *trc, Shape *shape)
{
    shape->markChildren(trc);
}

static void
gc::MarkChildren(JSTracer *trc, BaseShape *base)
{
    base->markChildren(trc);
}









static inline void
MarkCycleCollectorChildren(JSTracer *trc, BaseShape *base, JSObject **prevParent)
{
    JS_ASSERT(base);

    




    base->assertConsistency();

    if (base->hasGetterObject()) {
        JSObject *tmp = base->getterObject();
        MarkObjectUnbarriered(trc, &tmp, "getter");
        JS_ASSERT(tmp == base->getterObject());
    }

    if (base->hasSetterObject()) {
        JSObject *tmp = base->setterObject();
        MarkObjectUnbarriered(trc, &tmp, "setter");
        JS_ASSERT(tmp == base->setterObject());
    }

    JSObject *parent = base->getObjectParent();
    if (parent && parent != *prevParent) {
        MarkObjectUnbarriered(trc, &parent, "parent");
        JS_ASSERT(parent == base->getObjectParent());
        *prevParent = parent;
    }
}









void
gc::MarkCycleCollectorChildren(JSTracer *trc, Shape *shape)
{
    JSObject *prevParent = nullptr;
    do {
        MarkCycleCollectorChildren(trc, shape->base(), &prevParent);
        MarkId(trc, &shape->propidRef(), "propid");
        shape = shape->previous();
    } while (shape);
}

static void
ScanTypeObject(GCMarker *gcmarker, types::TypeObject *type)
{
    unsigned count = type->getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        if (types::Property *prop = type->getProperty(i))
            MarkId(gcmarker, &prop->id, "TypeObject property id");
    }

    if (type->proto().isObject())
        PushMarkStack(gcmarker, type->proto().toObject());

    if (type->singleton() && !type->lazy())
        PushMarkStack(gcmarker, type->singleton());

    if (type->newScript())
        type->newScript()->trace(gcmarker);

    if (type->interpretedFunction)
        PushMarkStack(gcmarker, type->interpretedFunction);
}

static void
gc::MarkChildren(JSTracer *trc, types::TypeObject *type)
{
    unsigned count = type->getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        types::Property *prop = type->getProperty(i);
        if (prop)
            MarkId(trc, &prop->id, "type_prop");
    }

    if (type->proto().isObject())
        MarkObject(trc, &type->protoRaw(), "type_proto");

    if (type->singleton() && !type->lazy())
        MarkObject(trc, &type->singletonRaw(), "type_singleton");

    if (type->newScript())
        type->newScript()->trace(trc);

    if (type->interpretedFunction)
        MarkObject(trc, &type->interpretedFunction, "type_function");
}

static void
gc::MarkChildren(JSTracer *trc, jit::JitCode *code)
{
    code->trace(trc);
}

template<typename T>
static void
PushArenaTyped(GCMarker *gcmarker, ArenaHeader *aheader)
{
    for (ArenaCellIterUnderGC i(aheader); !i.done(); i.next())
        PushMarkStack(gcmarker, i.get<T>());
}

void
gc::PushArena(GCMarker *gcmarker, ArenaHeader *aheader)
{
    switch (MapAllocToTraceKind(aheader->getAllocKind())) {
      case JSTRACE_OBJECT:
        PushArenaTyped<JSObject>(gcmarker, aheader);
        break;

      case JSTRACE_STRING:
        PushArenaTyped<JSString>(gcmarker, aheader);
        break;

      case JSTRACE_SYMBOL:
        PushArenaTyped<JS::Symbol>(gcmarker, aheader);
        break;

      case JSTRACE_SCRIPT:
        PushArenaTyped<JSScript>(gcmarker, aheader);
        break;

      case JSTRACE_LAZY_SCRIPT:
        PushArenaTyped<LazyScript>(gcmarker, aheader);
        break;

      case JSTRACE_SHAPE:
        PushArenaTyped<js::Shape>(gcmarker, aheader);
        break;

      case JSTRACE_BASE_SHAPE:
        PushArenaTyped<js::BaseShape>(gcmarker, aheader);
        break;

      case JSTRACE_TYPE_OBJECT:
        PushArenaTyped<js::types::TypeObject>(gcmarker, aheader);
        break;

      case JSTRACE_JITCODE:
        PushArenaTyped<js::jit::JitCode>(gcmarker, aheader);
        break;
    }
}

struct SlotArrayLayout
{
    union {
        HeapSlot *end;
        uintptr_t kind;
    };
    union {
        HeapSlot *start;
        uintptr_t index;
    };
    JSObject *obj;

    static void staticAsserts() {
        
        JS_STATIC_ASSERT(sizeof(SlotArrayLayout) == 3 * sizeof(uintptr_t));
    }
};









void
GCMarker::saveValueRanges()
{
    for (uintptr_t *p = stack.tos_; p > stack.stack_; ) {
        uintptr_t tag = *--p & StackTagMask;
        if (tag == ValueArrayTag) {
            *p &= ~StackTagMask;
            p -= 2;
            SlotArrayLayout *arr = reinterpret_cast<SlotArrayLayout *>(p);
            JSObject *obj = arr->obj;
            JS_ASSERT(obj->isNative());

            HeapSlot *vp = obj->getDenseElements();
            if (arr->end == vp + obj->getDenseInitializedLength()) {
                JS_ASSERT(arr->start >= vp);
                arr->index = arr->start - vp;
                arr->kind = HeapSlot::Element;
            } else {
                HeapSlot *vp = obj->fixedSlots();
                unsigned nfixed = obj->numFixedSlots();
                if (arr->start == arr->end) {
                    arr->index = obj->slotSpan();
                } else if (arr->start >= vp && arr->start < vp + nfixed) {
                    JS_ASSERT(arr->end == vp + Min(nfixed, obj->slotSpan()));
                    arr->index = arr->start - vp;
                } else {
                    JS_ASSERT(arr->start >= obj->slots &&
                              arr->end == obj->slots + obj->slotSpan() - nfixed);
                    arr->index = (arr->start - obj->slots) + nfixed;
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
GCMarker::restoreValueArray(JSObject *obj, void **vpp, void **endp)
{
    uintptr_t start = stack.pop();
    HeapSlot::Kind kind = (HeapSlot::Kind) stack.pop();

    if (kind == HeapSlot::Element) {
        if (!obj->is<ArrayObject>())
            return false;

        uint32_t initlen = obj->getDenseInitializedLength();
        HeapSlot *vp = obj->getDenseElements();
        if (start < initlen) {
            *vpp = vp + start;
            *endp = vp + initlen;
        } else {
            
            *vpp = *endp = vp;
        }
    } else {
        JS_ASSERT(kind == HeapSlot::Slot);
        HeapSlot *vp = obj->fixedSlots();
        unsigned nfixed = obj->numFixedSlots();
        unsigned nslots = obj->slotSpan();
        if (start < nslots) {
            if (start < nfixed) {
                *vpp = vp + start;
                *endp = vp + Min(nfixed, nslots);
            } else {
                *vpp = obj->slots + start - nfixed;
                *endp = obj->slots + nslots - nfixed;
            }
        } else {
            
            *vpp = *endp = vp;
        }
    }

    JS_ASSERT(*vpp <= *endp);
    return true;
}

void
GCMarker::processMarkStackOther(uintptr_t tag, uintptr_t addr)
{
    if (tag == TypeTag) {
        ScanTypeObject(this, reinterpret_cast<types::TypeObject *>(addr));
    } else if (tag == SavedValueArrayTag) {
        JS_ASSERT(!(addr & CellMask));
        JSObject *obj = reinterpret_cast<JSObject *>(addr);
        HeapValue *vp, *end;
        if (restoreValueArray(obj, (void **)&vp, (void **)&end))
            pushValueArray(obj, vp, end);
        else
            pushObject(obj);
    } else if (tag == JitCodeTag) {
        MarkChildren(this, reinterpret_cast<jit::JitCode *>(addr));
    }
}

inline void
GCMarker::processMarkStackTop(SliceBudget &budget)
{
    




    HeapSlot *vp, *end;
    JSObject *obj;

    uintptr_t addr = stack.pop();
    uintptr_t tag = addr & StackTagMask;
    addr &= ~StackTagMask;

    if (tag == ValueArrayTag) {
        JS_STATIC_ASSERT(ValueArrayTag == 0);
        JS_ASSERT(!(addr & CellMask));
        obj = reinterpret_cast<JSObject *>(addr);
        uintptr_t addr2 = stack.pop();
        uintptr_t addr3 = stack.pop();
        JS_ASSERT(addr2 <= addr3);
        JS_ASSERT((addr3 - addr2) % sizeof(Value) == 0);
        vp = reinterpret_cast<HeapSlot *>(addr2);
        end = reinterpret_cast<HeapSlot *>(addr3);
        goto scan_value_array;
    }

    if (tag == ObjectTag) {
        obj = reinterpret_cast<JSObject *>(addr);
        JS_COMPARTMENT_ASSERT(runtime(), obj);
        goto scan_obj;
    }

    processMarkStackOther(tag, addr);
    return;

  scan_value_array:
    JS_ASSERT(vp <= end);
    while (vp != end) {
        const Value &v = *vp++;
        if (v.isString()) {
            JSString *str = v.toString();
            if (!str->isPermanentAtom()) {
                JS_COMPARTMENT_ASSERT_STR(runtime(), str);
                JS_ASSERT(runtime()->isAtomsZone(str->zone()) || str->zone() == obj->zone());
                if (str->markIfUnmarked())
                    ScanString(this, str);
            }
        } else if (v.isObject()) {
            JSObject *obj2 = &v.toObject();
            JS_COMPARTMENT_ASSERT(runtime(), obj2);
            JS_ASSERT(obj->compartment() == obj2->compartment());
            if (obj2->asTenured()->markIfUnmarked(getMarkColor())) {
                pushValueArray(obj, vp, end);
                obj = obj2;
                goto scan_obj;
            }
        } else if (v.isSymbol()) {
            JS::Symbol *sym = v.toSymbol();
            if (!sym->isWellKnownSymbol()) {
                JS_COMPARTMENT_ASSERT_SYM(runtime(), sym);
                MOZ_ASSERT(runtime()->isAtomsZone(sym->zone()) || sym->zone() == obj->zone());
                if (sym->markIfUnmarked())
                    ScanSymbol(this, sym);
            }
        }
    }
    return;

  scan_obj:
    {
        JS_COMPARTMENT_ASSERT(runtime(), obj);

        budget.step();
        if (budget.isOverBudget()) {
            pushObject(obj);
            return;
        }

        types::TypeObject *type = obj->typeFromGC();
        PushMarkStack(this, type);

        Shape *shape = obj->lastProperty();
        PushMarkStack(this, shape);

        
        const Class *clasp = type->clasp();
        if (clasp->trace) {
            
            
            
            JS_ASSERT_IF(runtime()->gc.isIncrementalGCEnabled() &&
                         !(clasp->trace == JS_GlobalObjectTraceHook &&
                           (!obj->compartment()->options().getTrace() ||
                            !obj->isOwnGlobal())),
                         clasp->flags & JSCLASS_IMPLEMENTS_BARRIERS);
            clasp->trace(this, obj);
        }

        if (!shape->isNative())
            return;

        unsigned nslots = obj->slotSpan();

        do {
            if (obj->hasEmptyElements())
                break;

            if (obj->denseElementsAreCopyOnWrite()) {
                JSObject *owner = obj->getElementsHeader()->ownerObject();
                if (owner != obj) {
                    PushMarkStack(this, owner);
                    break;
                }
            }

            vp = obj->getDenseElementsAllowCopyOnWrite();
            end = vp + obj->getDenseInitializedLength();
            if (!nslots)
                goto scan_value_array;
            pushValueArray(obj, vp, end);
        } while (false);

        vp = obj->fixedSlots();
        if (obj->slots) {
            unsigned nfixed = obj->numFixedSlots();
            if (nslots > nfixed) {
                pushValueArray(obj, vp, vp + nfixed);
                vp = obj->slots;
                end = vp + (nslots - nfixed);
                goto scan_value_array;
            }
        }
        JS_ASSERT(nslots <= obj->numFixedSlots());
        end = vp + nslots;
        goto scan_value_array;
    }
}

bool
GCMarker::drainMarkStack(SliceBudget &budget)
{
#ifdef DEBUG
    struct AutoCheckCompartment {
        bool &flag;
        explicit AutoCheckCompartment(bool &comparmentCheckFlag) : flag(comparmentCheckFlag) {
            JS_ASSERT(!flag);
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

void
js::TraceChildren(JSTracer *trc, void *thing, JSGCTraceKind kind)
{
    switch (kind) {
      case JSTRACE_OBJECT:
        MarkChildren(trc, static_cast<JSObject *>(thing));
        break;

      case JSTRACE_STRING:
        MarkChildren(trc, static_cast<JSString *>(thing));
        break;

      case JSTRACE_SYMBOL:
        MarkChildren(trc, static_cast<JS::Symbol *>(thing));
        break;

      case JSTRACE_SCRIPT:
        MarkChildren(trc, static_cast<JSScript *>(thing));
        break;

      case JSTRACE_LAZY_SCRIPT:
        MarkChildren(trc, static_cast<LazyScript *>(thing));
        break;

      case JSTRACE_SHAPE:
        MarkChildren(trc, static_cast<Shape *>(thing));
        break;

      case JSTRACE_JITCODE:
        MarkChildren(trc, (js::jit::JitCode *)thing);
        break;

      case JSTRACE_BASE_SHAPE:
        MarkChildren(trc, static_cast<BaseShape *>(thing));
        break;

      case JSTRACE_TYPE_OBJECT:
        MarkChildren(trc, (types::TypeObject *)thing);
        break;
    }
}

#ifdef DEBUG
static void
AssertNonGrayGCThing(JSTracer *trc, void **thingp, JSGCTraceKind kind)
{
    MOZ_ASSERT(!JS::GCThingIsMarkedGray(*thingp));
}
#endif

static void
UnmarkGrayChildren(JSTracer *trc, void **thingp, JSGCTraceKind kind);

struct UnmarkGrayTracer : public JSTracer
{
    



    explicit UnmarkGrayTracer(JSRuntime *rt)
      : JSTracer(rt, UnmarkGrayChildren, DoNotTraceWeakMaps),
        tracingShape(false),
        previousShape(nullptr),
        unmarkedAny(false)
    {}

    UnmarkGrayTracer(JSTracer *trc, bool tracingShape)
      : JSTracer(trc->runtime(), UnmarkGrayChildren, DoNotTraceWeakMaps),
        tracingShape(tracingShape),
        previousShape(nullptr),
        unmarkedAny(false)
    {}

    
    bool tracingShape;

    
    void *previousShape;

    
    bool unmarkedAny;
};































static void
UnmarkGrayChildren(JSTracer *trc, void **thingp, JSGCTraceKind kind)
{
    void *thing = *thingp;
    int stackDummy;
    if (!JS_CHECK_STACK_SIZE(trc->runtime()->mainThread.nativeStackLimit[StackForSystemCode], &stackDummy)) {
        



        trc->runtime()->gc.setGrayBitsInvalid();
        return;
    }

#ifdef DEBUG
    if (gc::IsInsideNursery(static_cast<gc::Cell *>(thing))) {
        JSTracer nongray(trc->runtime(), AssertNonGrayGCThing);
        JS_TraceChildren(&nongray, thing, kind);
    }
#endif

    if (!JS::GCThingIsMarkedGray(thing))
        return;

    UnmarkGrayTracer *tracer = static_cast<UnmarkGrayTracer *>(trc);
    TenuredCell::fromPointer(thing)->unmark(js::gc::GRAY);
    tracer->unmarkedAny = true;

    






    UnmarkGrayTracer childTracer(tracer, kind == JSTRACE_SHAPE);

    if (kind != JSTRACE_SHAPE) {
        JS_TraceChildren(&childTracer, thing, kind);
        JS_ASSERT(!childTracer.previousShape);
        tracer->unmarkedAny |= childTracer.unmarkedAny;
        return;
    }

    if (tracer->tracingShape) {
        JS_ASSERT(!tracer->previousShape);
        tracer->previousShape = thing;
        return;
    }

    do {
        JS_ASSERT(!JS::GCThingIsMarkedGray(thing));
        JS_TraceChildren(&childTracer, thing, JSTRACE_SHAPE);
        thing = childTracer.previousShape;
        childTracer.previousShape = nullptr;
    } while (thing);
    tracer->unmarkedAny |= childTracer.unmarkedAny;
}

JS_FRIEND_API(bool)
JS::UnmarkGrayGCThingRecursively(void *thing, JSGCTraceKind kind)
{
    JSRuntime *rt = static_cast<Cell *>(thing)->runtimeFromMainThread();

    
    
    if (rt->isHeapBusy())
        return false;

    bool unmarkedArg = false;
    if (!IsInsideNursery(static_cast<Cell *>(thing))) {
        if (!JS::GCThingIsMarkedGray(thing))
            return false;

        TenuredCell::fromPointer(thing)->unmark(js::gc::GRAY);
        unmarkedArg = true;
    }

    UnmarkGrayTracer trc(rt);
    JS_TraceChildren(&trc, thing, kind);

    return unmarkedArg || trc.unmarkedAny;
}
