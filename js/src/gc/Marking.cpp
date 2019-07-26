






#include "mozilla/DebugOnly.h"

#include "jsprf.h"
#include "jsstr.h"

#include "gc/Marking.h"
#include "methodjit/MethodJIT.h"
#include "vm/Shape.h"

#include "jsobjinlines.h"

#include "ion/IonCode.h"
#include "vm/Shape-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::DebugOnly;

void * const js::NullPtr::constNullValue = NULL;































static inline void
PushMarkStack(GCMarker *gcmarker, JSObject *thing);

static inline void
PushMarkStack(GCMarker *gcmarker, JSFunction *thing);

static inline void
PushMarkStack(GCMarker *gcmarker, RawScript thing);

static inline void
PushMarkStack(GCMarker *gcmarker, RawShape thing);

static inline void
PushMarkStack(GCMarker *gcmarker, JSString *thing);

static inline void
PushMarkStack(GCMarker *gcmarker, types::TypeObject *thing);

namespace js {
namespace gc {

static void MarkChildren(JSTracer *trc, JSString *str);
static void MarkChildren(JSTracer *trc, RawScript script);
static void MarkChildren(JSTracer *trc, RawShape shape);
static void MarkChildren(JSTracer *trc, RawBaseShape base);
static void MarkChildren(JSTracer *trc, types::TypeObject *type);
static void MarkChildren(JSTracer *trc, ion::IonCode *code);

} 
} 



#ifdef DEBUG
template<typename T>
static inline bool
IsThingPoisoned(T *thing)
{
    const uint8_t pb = JS_FREE_PATTERN;
    const uint32_t pw = pb | (pb << 8) | (pb << 16) | (pb << 24);
    JS_STATIC_ASSERT(sizeof(T) >= sizeof(FreeSpan) + sizeof(uint32_t));
    uint32_t *p =
        reinterpret_cast<uint32_t *>(reinterpret_cast<FreeSpan *>(thing) + 1);
    return *p == pw;
}
#endif

template<typename T>
static inline void
CheckMarkedThing(JSTracer *trc, T *thing)
{
    JS_ASSERT(trc);
    JS_ASSERT(thing);
    JS_ASSERT(thing->zone());
    JS_ASSERT(thing->zone()->rt == trc->runtime);
    JS_ASSERT(trc->debugPrinter || trc->debugPrintArg);

    DebugOnly<JSRuntime *> rt = trc->runtime;

    JS_ASSERT_IF(IS_GC_MARKING_TRACER(trc) && rt->gcManipulatingDeadZones,
                 !thing->zone()->scheduledForDestruction);

#ifdef DEBUG
    rt->assertValidThread();
#endif

    JS_ASSERT_IF(thing->zone()->requireGCTracer(), IS_GC_MARKING_TRACER(trc));

    JS_ASSERT(thing->isAligned());

    JS_ASSERT_IF(rt->gcStrictCompartmentChecking,
                 thing->zone()->isCollecting() ||
                 thing->zone() == rt->atomsCompartment->zone());

    JS_ASSERT_IF(IS_GC_MARKING_TRACER(trc) && ((GCMarker *)trc)->getMarkColor() == GRAY,
                 thing->zone()->isGCMarkingGray() ||
                 thing->zone() == rt->atomsCompartment->zone());

    





    JS_ASSERT_IF(IsThingPoisoned(thing) && rt->isHeapBusy(),
                 !InFreeList(thing->arenaHeader(), thing));
}

static GCMarker *
AsGCMarker(JSTracer *trc)
{
    JS_ASSERT(IS_GC_MARKING_TRACER(trc));
    return static_cast<GCMarker *>(trc);
}

template<typename T>
static void
MarkInternal(JSTracer *trc, T **thingp)
{
    AutoAssertNoGC nogc;
    JS_ASSERT(thingp);
    T *thing = *thingp;

    CheckMarkedThing(trc, thing);

    



    if (!trc->callback) {
        if (thing->zone()->isGCMarking()) {
            PushMarkStack(AsGCMarker(trc), thing);
            thing->zone()->maybeAlive = true;
        }
    } else {
        trc->callback(trc, (void **)thingp, GetGCThingTraceKind(thing));
        JS_UNSET_TRACING_LOCATION(trc);
    }

    trc->debugPrinter = NULL;
    trc->debugPrintArg = NULL;
}

#define JS_ROOT_MARKING_ASSERT(trc)                                     \
    JS_ASSERT_IF(IS_GC_MARKING_TRACER(trc),                             \
                 trc->runtime->gcIncrementalState == NO_INCREMENTAL ||  \
                 trc->runtime->gcIncrementalState == MARK_ROOTS);

template <typename T>
static void
MarkUnbarriered(JSTracer *trc, T **thingp, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkInternal(trc, thingp);
}

namespace js {
namespace gc {

template <typename T>
static void
Mark(JSTracer *trc, EncapsulatedPtr<T> *thing, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkInternal(trc, thing->unsafeGet());
}

} 
} 

template <typename T>
static void
MarkRoot(JSTracer *trc, T **thingp, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    JS_SET_TRACING_NAME(trc, name);
    MarkInternal(trc, thingp);
}

template <typename T>
static void
MarkRange(JSTracer *trc, size_t len, HeapPtr<T> *vec, const char *name)
{
    for (size_t i = 0; i < len; ++i) {
        if (vec[i].get()) {
            JS_SET_TRACING_INDEX(trc, name, i);
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
            JS_SET_TRACING_INDEX(trc, name, i);
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
    Zone *zone = (*thingp)->zone();
    if (!zone->isCollecting() || zone->isGCFinished())
        return true;
    return (*thingp)->isMarked();
}

template <typename T>
static bool
IsAboutToBeFinalized(T **thingp)
{
    JS_ASSERT(thingp);
    JS_ASSERT(*thingp);
    if (!(*thingp)->zone()->isGCSweeping())
        return false;
    return !(*thingp)->isMarked();
}

#define DeclMarkerImpl(base, type)                                                                \
void                                                                                              \
Mark##base(JSTracer *trc, EncapsulatedPtr<type> *thing, const char *name)                         \
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
void                                                                                              \
Mark##base##Range(JSTracer *trc, size_t len, HeapPtr<type> *vec, const char *name)                \
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
Is##base##Marked(EncapsulatedPtr<type> *thingp)                                                   \
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
Is##base##AboutToBeFinalized(EncapsulatedPtr<type> *thingp)                                       \
{                                                                                                 \
    return IsAboutToBeFinalized<type>(thingp->unsafeGet());                                       \
}

DeclMarkerImpl(BaseShape, BaseShape)
DeclMarkerImpl(BaseShape, UnownedBaseShape)
DeclMarkerImpl(IonCode, ion::IonCode)
DeclMarkerImpl(Object, ArgumentsObject)
DeclMarkerImpl(Object, DebugScopeObject)
DeclMarkerImpl(Object, GlobalObject)
DeclMarkerImpl(Object, JSObject)
DeclMarkerImpl(Object, JSFunction)
DeclMarkerImpl(Object, ScopeObject)
DeclMarkerImpl(Script, JSScript)
DeclMarkerImpl(Shape, Shape)
DeclMarkerImpl(String, JSAtom)
DeclMarkerImpl(String, JSString)
DeclMarkerImpl(String, JSFlatString)
DeclMarkerImpl(String, JSLinearString)
DeclMarkerImpl(String, PropertyName)
DeclMarkerImpl(TypeObject, js::types::TypeObject)

} 
} 



void
gc::MarkKind(JSTracer *trc, void **thingp, JSGCTraceKind kind)
{
    JS_ASSERT(thingp);
    JS_ASSERT(*thingp);
    JS_ASSERT(kind == GetGCThingTraceKind(*thingp));
    switch (kind) {
      case JSTRACE_OBJECT:
        MarkInternal(trc, reinterpret_cast<RawObject *>(thingp));
        break;
      case JSTRACE_STRING:
        MarkInternal(trc, reinterpret_cast<RawString *>(thingp));
        break;
      case JSTRACE_SCRIPT:
        MarkInternal(trc, reinterpret_cast<RawScript *>(thingp));
        break;
      case JSTRACE_SHAPE:
        MarkInternal(trc, reinterpret_cast<RawShape *>(thingp));
        break;
      case JSTRACE_BASE_SHAPE:
        MarkInternal(trc, reinterpret_cast<RawBaseShape *>(thingp));
        break;
      case JSTRACE_TYPE_OBJECT:
        MarkInternal(trc, reinterpret_cast<types::TypeObject **>(thingp));
        break;
      case JSTRACE_IONCODE:
        MarkInternal(trc, reinterpret_cast<ion::IonCode **>(thingp));
        break;
    }
}

static void
MarkGCThingInternal(JSTracer *trc, void **thingp, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
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
        JS_SET_TRACING_LOCATION(trc, (void *)id);
        MarkInternal(trc, &str);
        *id = NON_INTEGER_ATOM_TO_JSID(reinterpret_cast<JSAtom *>(str));
    } else if (JS_UNLIKELY(JSID_IS_OBJECT(*id))) {
        JSObject *obj = JSID_TO_OBJECT(*id);
        JS_SET_TRACING_LOCATION(trc, (void *)id);
        MarkInternal(trc, &obj);
        *id = OBJECT_TO_JSID(obj);
    } else {
        
        JS_UNSET_TRACING_LOCATION(trc);
    }
}

void
gc::MarkId(JSTracer *trc, EncapsulatedId *id, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkIdInternal(trc, id->unsafeGet());
}

void
gc::MarkIdRoot(JSTracer *trc, jsid *id, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    JS_SET_TRACING_NAME(trc, name);
    MarkIdInternal(trc, id);
}

void
gc::MarkIdUnbarriered(JSTracer *trc, jsid *id, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkIdInternal(trc, id);
}

void
gc::MarkIdRange(JSTracer *trc, size_t len, HeapId *vec, const char *name)
{
    for (size_t i = 0; i < len; ++i) {
        JS_SET_TRACING_INDEX(trc, name, i);
        MarkIdInternal(trc, vec[i].unsafeGet());
    }
}

void
gc::MarkIdRootRange(JSTracer *trc, size_t len, jsid *vec, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    for (size_t i = 0; i < len; ++i) {
        JS_SET_TRACING_INDEX(trc, name, i);
        MarkIdInternal(trc, &vec[i]);
    }
}



static inline void
MarkValueInternal(JSTracer *trc, Value *v)
{
    if (v->isMarkable()) {
        JS_ASSERT(v->toGCThing());
        void *thing = v->toGCThing();
        JS_SET_TRACING_LOCATION(trc, (void *)v);
        MarkKind(trc, &thing, v->gcKind());
        if (v->isString())
            v->setString((JSString *)thing);
        else
            v->setObjectOrNull((JSObject *)thing);
    } else {
        
        JS_UNSET_TRACING_LOCATION(trc);
    }
}

static inline void
MarkValueInternalMaybeNullPayload(JSTracer *trc, Value *v)
{
    if (v->isMarkable()) {
        void *thing = v->toGCThing();
        if (thing) {
            JS_SET_TRACING_LOCATION(trc, (void *)v);
            MarkKind(trc, &thing, v->gcKind());
            if (v->isString())
                v->setString((JSString *)thing);
            else
                v->setObjectOrNull((JSObject *)thing);
            return;
        }
    }
    JS_UNSET_TRACING_LOCATION(trc);
}

void
gc::MarkValue(JSTracer *trc, EncapsulatedValue *v, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkValueInternal(trc, v->unsafeGet());
}

void
gc::MarkValueRoot(JSTracer *trc, Value *v, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    JS_SET_TRACING_NAME(trc, name);
    MarkValueInternal(trc, v);
}

void
gc::MarkTypeRoot(JSTracer *trc, types::Type *v, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    JS_SET_TRACING_NAME(trc, name);
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
gc::MarkValueRange(JSTracer *trc, size_t len, EncapsulatedValue *vec, const char *name)
{
    for (size_t i = 0; i < len; ++i) {
        JS_SET_TRACING_INDEX(trc, name, i);
        MarkValueInternal(trc, vec[i].unsafeGet());
    }
}

void
gc::MarkValueRootRange(JSTracer *trc, size_t len, Value *vec, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    for (size_t i = 0; i < len; ++i) {
        JS_SET_TRACING_INDEX(trc, name, i);
        MarkValueInternal(trc, &vec[i]);
    }
}

void
gc::MarkValueRootRangeMaybeNullPayload(JSTracer *trc, size_t len, Value *vec, const char *name)
{
    JS_ROOT_MARKING_ASSERT(trc);
    for (size_t i = 0; i < len; ++i) {
        JS_SET_TRACING_INDEX(trc, name, i);
        MarkValueInternalMaybeNullPayload(trc, &vec[i]);
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
    } else {
        JSObject *obj = (JSObject *)v->toGCThing();
        rv = IsMarked<JSObject>(&obj);
        v->setObject(*obj);
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
    } else {
        JSObject *obj = (JSObject *)v->toGCThing();
        rv = IsAboutToBeFinalized<JSObject>(&obj);
        v->setObject(*obj);
    }
    return rv;
}



void
gc::MarkSlot(JSTracer *trc, HeapSlot *s, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkValueInternal(trc, s->unsafeGet());
}

void
gc::MarkArraySlots(JSTracer *trc, size_t len, HeapSlot *vec, const char *name)
{
    for (size_t i = 0; i < len; ++i) {
        JS_SET_TRACING_INDEX(trc, name, i);
        MarkValueInternal(trc, vec[i].unsafeGet());
    }
}

void
gc::MarkObjectSlots(JSTracer *trc, JSObject *obj, uint32_t start, uint32_t nslots)
{
    JS_ASSERT(obj->isNative());
    for (uint32_t i = start; i < (start + nslots); ++i) {
        JS_SET_TRACING_DETAILS(trc, js_GetObjectSlotName, obj, i);
        MarkValueInternal(trc, obj->nativeGetSlotRef(i).unsafeGet());
    }
}

static bool
ShouldMarkCrossCompartment(JSTracer *trc, RawObject src, Cell *cell)
{
    if (!IS_GC_MARKING_TRACER(trc))
        return true;

    JS::Zone *zone = cell->zone();
    uint32_t color = AsGCMarker(trc)->getMarkColor();

    JS_ASSERT(color == BLACK || color == GRAY);
    if (color == BLACK) {
        






        if (cell->isMarked(GRAY)) {
            JS_ASSERT(!zone->isCollecting());
            trc->runtime->gcFoundBlackGrayEdges = true;
        }
        return zone->isGCMarking();
    } else {
        if (zone->isGCMarkingBlack()) {
            




            if (!cell->isMarked())
                DelayCrossCompartmentGrayMarking(src);
            return false;
        }
        return zone->isGCMarkingGray();
    }
}

void
gc::MarkCrossCompartmentObjectUnbarriered(JSTracer *trc, RawObject src, JSObject **dst, const char *name)
{
    if (ShouldMarkCrossCompartment(trc, src, *dst))
        MarkObjectUnbarriered(trc, dst, name);
}

void
gc::MarkCrossCompartmentScriptUnbarriered(JSTracer *trc, RawObject src, JSScript **dst,
                                      const char *name)
{
    if (ShouldMarkCrossCompartment(trc, src, *dst))
        MarkScriptUnbarriered(trc, dst, name);
}

void
gc::MarkCrossCompartmentSlot(JSTracer *trc, RawObject src, HeapSlot *dst, const char *name)
{
    if (dst->isMarkable() && ShouldMarkCrossCompartment(trc, src, (Cell *)dst->toGCThing()))
        MarkSlot(trc, dst, name);
}



void
gc::MarkObject(JSTracer *trc, HeapPtr<GlobalObject, RawScript> *thingp, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkInternal(trc, thingp->unsafeGet());
}

void
gc::MarkValueUnbarriered(JSTracer *trc, Value *v, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
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
              (thing)->zone() == (rt)->atomsCompartment->zone());

static void
PushMarkStack(GCMarker *gcmarker, JSObject *thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime, thing);

    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        gcmarker->pushObject(thing);
}

static void
PushMarkStack(GCMarker *gcmarker, JSFunction *thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime, thing);

    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        gcmarker->pushObject(thing);
}

static void
PushMarkStack(GCMarker *gcmarker, types::TypeObject *thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime, thing);

    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        gcmarker->pushType(thing);
}

static void
PushMarkStack(GCMarker *gcmarker, RawScript thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime, thing);

    




    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        MarkChildren(gcmarker, thing);
}

static void
ScanShape(GCMarker *gcmarker, RawShape shape);

static void
PushMarkStack(GCMarker *gcmarker, RawShape thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime, thing);

    
    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        ScanShape(gcmarker, thing);
}

static void
PushMarkStack(GCMarker *gcmarker, ion::IonCode *thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime, thing);

    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        gcmarker->pushIonCode(thing);
}

static inline void
ScanBaseShape(GCMarker *gcmarker, RawBaseShape base);

static void
PushMarkStack(GCMarker *gcmarker, RawBaseShape thing)
{
    JS_COMPARTMENT_ASSERT(gcmarker->runtime, thing);

    
    if (thing->markIfUnmarked(gcmarker->getMarkColor()))
        ScanBaseShape(gcmarker, thing);
}

static void
ScanShape(GCMarker *gcmarker, RawShape shape)
{
  restart:
    PushMarkStack(gcmarker, shape->base());

    const EncapsulatedId &id = shape->propidRef();
    if (JSID_IS_STRING(id))
        PushMarkStack(gcmarker, JSID_TO_STRING(id));
    else if (JS_UNLIKELY(JSID_IS_OBJECT(id)))
        PushMarkStack(gcmarker, JSID_TO_OBJECT(id));

    shape = shape->previous();
    if (shape && shape->markIfUnmarked(gcmarker->getMarkColor()))
        goto restart;
}

static inline void
ScanBaseShape(GCMarker *gcmarker, RawBaseShape base)
{
    base->assertConsistency();

    if (base->hasGetterObject())
        PushMarkStack(gcmarker, base->getterObject());

    if (base->hasSetterObject())
        PushMarkStack(gcmarker, base->setterObject());

    if (JSObject *parent = base->getObjectParent()) {
        PushMarkStack(gcmarker, parent);
    } else if (GlobalObject *global = base->compartment()->maybeGlobal()) {
        PushMarkStack(gcmarker, global);
    }

    




    if (base->isOwned()) {
        RawUnownedBaseShape unowned = base->baseUnowned();
        JS_ASSERT(base->compartment() == unowned->compartment());
        unowned->markIfUnmarked(gcmarker->getMarkColor());
    }
}

static inline void
ScanLinearString(GCMarker *gcmarker, JSLinearString *str)
{
    JS_COMPARTMENT_ASSERT_STR(gcmarker->runtime, str);
    JS_ASSERT(str->isMarked());

    



    JS_ASSERT(str->JSString::isLinear());
    while (str->hasBase()) {
        str = str->base();
        JS_ASSERT(str->JSString::isLinear());
        JS_COMPARTMENT_ASSERT_STR(gcmarker->runtime, str);
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
        JS_COMPARTMENT_ASSERT_STR(gcmarker->runtime, rope);
        JS_ASSERT(rope->isMarked());
        JSRope *next = NULL;

        JSString *right = rope->rightChild();
        if (right->markIfUnmarked()) {
            if (right->isLinear())
                ScanLinearString(gcmarker, &right->asLinear());
            else
                next = &right->asRope();
        }

        JSString *left = rope->leftChild();
        if (left->markIfUnmarked()) {
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
    JS_COMPARTMENT_ASSERT_STR(gcmarker->runtime, str);

    




    if (str->markIfUnmarked())
        ScanString(gcmarker, str);
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
gc::MarkChildren(JSTracer *trc, RawScript script)
{
    script->markChildren(trc);
}

static void
gc::MarkChildren(JSTracer *trc, RawShape shape)
{
    shape->markChildren(trc);
}

static void
gc::MarkChildren(JSTracer *trc, RawBaseShape base)
{
    base->markChildren(trc);
}









static inline void
MarkCycleCollectorChildren(JSTracer *trc, RawBaseShape base, JSObject **prevParent)
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
gc::MarkCycleCollectorChildren(JSTracer *trc, RawShape shape)
{
    JSObject *prevParent = NULL;
    do {
        MarkCycleCollectorChildren(trc, shape->base(), &prevParent);
        MarkId(trc, &shape->propidRef(), "propid");
        shape = shape->previous();
    } while (shape);
}

static void
ScanTypeObject(GCMarker *gcmarker, types::TypeObject *type)
{
    
    if (!type->singleton) {
        unsigned count = type->getPropertyCount();
        for (unsigned i = 0; i < count; i++) {
            types::Property *prop = type->getProperty(i);
            if (prop && JSID_IS_STRING(prop->id))
                PushMarkStack(gcmarker, JSID_TO_STRING(prop->id));
        }
    }

    if (TaggedProto(type->proto).isObject())
        PushMarkStack(gcmarker, type->proto);

    if (type->singleton && !type->lazy())
        PushMarkStack(gcmarker, type->singleton);

    if (type->newScript) {
        PushMarkStack(gcmarker, type->newScript->fun);
        PushMarkStack(gcmarker, type->newScript->shape.get());
    }

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

    if (TaggedProto(type->proto).isObject())
        MarkObject(trc, &type->proto, "type_proto");

    if (type->singleton && !type->lazy())
        MarkObject(trc, &type->singleton, "type_singleton");

    if (type->newScript) {
        MarkObject(trc, &type->newScript->fun, "type_new_function");
        MarkShape(trc, &type->newScript->shape, "type_new_shape");
    }

    if (type->interpretedFunction)
        MarkObject(trc, &type->interpretedFunction, "type_function");
}

static void
gc::MarkChildren(JSTracer *trc, ion::IonCode *code)
{
#ifdef JS_ION
    code->trace(trc);
#endif
}

template<typename T>
static void
PushArenaTyped(GCMarker *gcmarker, ArenaHeader *aheader)
{
    for (CellIterUnderGC i(aheader); !i.done(); i.next())
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

      case JSTRACE_SCRIPT:
        PushArenaTyped<JSScript>(gcmarker, aheader);
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

      case JSTRACE_IONCODE:
        PushArenaTyped<js::ion::IonCode>(gcmarker, aheader);
        break;
    }
}

struct SlotArrayLayout
{
    union {
        HeapSlot *end;
        HeapSlot::Kind kind;
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
    for (uintptr_t *p = stack.tos; p > stack.stack; ) {
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
        if (obj->getClass() != &ArrayClass)
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
GCMarker::processMarkStackOther(SliceBudget &budget, uintptr_t tag, uintptr_t addr)
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
    } else if (tag == IonCodeTag) {
        MarkChildren(this, reinterpret_cast<ion::IonCode *>(addr));
    } else if (tag == ArenaTag) {
        ArenaHeader *aheader = reinterpret_cast<ArenaHeader *>(addr);
        AllocKind thingKind = aheader->getAllocKind();
        size_t thingSize = Arena::thingSize(thingKind);

        for ( ; aheader; aheader = aheader->next) {
            Arena *arena = aheader->getArena();
            FreeSpan firstSpan(aheader->getFirstFreeSpan());
            const FreeSpan *span = &firstSpan;

            for (uintptr_t thing = arena->thingsStart(thingKind); ; thing += thingSize) {
                JS_ASSERT(thing <= arena->thingsEnd());
                if (thing == span->first) {
                    if (!span->hasNext())
                        break;
                    thing = span->last;
                    span = span->nextSpan();
                } else {
                    JSObject *object = reinterpret_cast<JSObject *>(thing);
                    if (object->hasSingletonType() && object->markIfUnmarked(getMarkColor()))
                        pushObject(object);
                    budget.step();
                }
            }
            if (budget.isOverBudget()) {
                pushArenaList(aheader);
                return;
            }
        }
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
        JS_COMPARTMENT_ASSERT(runtime, obj);
        goto scan_obj;
    }

    processMarkStackOther(budget, tag, addr);
    return;

  scan_value_array:
    JS_ASSERT(vp <= end);
    while (vp != end) {
        const Value &v = *vp++;
        if (v.isString()) {
            JSString *str = v.toString();
            JS_COMPARTMENT_ASSERT_STR(runtime, str);
            JS_ASSERT(str->zone() == runtime->atomsCompartment->zone() ||
                      str->zone() == obj->zone());
            if (str->markIfUnmarked())
                ScanString(this, str);
        } else if (v.isObject()) {
            JSObject *obj2 = &v.toObject();
            JS_COMPARTMENT_ASSERT(runtime, obj2);
            JS_ASSERT(obj->compartment() == obj2->compartment());
            if (obj2->markIfUnmarked(getMarkColor())) {
                pushValueArray(obj, vp, end);
                obj = obj2;
                goto scan_obj;
            }
        }
    }
    return;

  scan_obj:
    {
        JS_COMPARTMENT_ASSERT(runtime, obj);

        budget.step();
        if (budget.isOverBudget()) {
            pushObject(obj);
            return;
        }

        types::TypeObject *type = obj->typeFromGC();
        PushMarkStack(this, type);

        RawShape shape = obj->lastProperty();
        PushMarkStack(this, shape);

        
        Class *clasp = type->clasp;
        if (clasp->trace) {
            JS_ASSERT_IF(runtime->gcMode == JSGC_MODE_INCREMENTAL &&
                         runtime->gcIncrementalEnabled,
                         clasp->flags & JSCLASS_IMPLEMENTS_BARRIERS);
            clasp->trace(this, obj);
        }

        if (!shape->isNative())
            return;

        unsigned nslots = obj->slotSpan();

        if (!obj->hasEmptyElements()) {
            vp = obj->getDenseElements();
            end = vp + obj->getDenseInitializedLength();
            if (!nslots)
                goto scan_value_array;
            pushValueArray(obj, vp, end);
        }

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
    JSRuntime *rt = runtime;

    struct AutoCheckCompartment {
        JSRuntime *runtime;
        AutoCheckCompartment(JSRuntime *rt) : runtime(rt) {
            JS_ASSERT(!rt->gcStrictCompartmentChecking);
            runtime->gcStrictCompartmentChecking = true;
        }
        ~AutoCheckCompartment() { runtime->gcStrictCompartmentChecking = false; }
    } acc(rt);
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
        MarkChildren(trc, static_cast<RawObject>(thing));
        break;

      case JSTRACE_STRING:
        MarkChildren(trc, static_cast<RawString>(thing));
        break;

      case JSTRACE_SCRIPT:
        MarkChildren(trc, static_cast<RawScript>(thing));
        break;

      case JSTRACE_SHAPE:
        MarkChildren(trc, static_cast<RawShape>(thing));
        break;

      case JSTRACE_IONCODE:
        MarkChildren(trc, (js::ion::IonCode *)thing);
        break;

      case JSTRACE_BASE_SHAPE:
        MarkChildren(trc, static_cast<RawBaseShape>(thing));
        break;

      case JSTRACE_TYPE_OBJECT:
        MarkChildren(trc, (types::TypeObject *)thing);
        break;
    }
}

void
js::CallTracer(JSTracer *trc, void *thing, JSGCTraceKind kind)
{
    JS_ASSERT(thing);
    void *tmp = thing;
    MarkKind(trc, &tmp, kind);
    JS_ASSERT(tmp == thing);
}

static void
UnmarkGrayGCThing(void *thing)
{
    static_cast<js::gc::Cell *>(thing)->unmark(js::gc::GRAY);
}

static void
UnmarkGrayChildren(JSTracer *trc, void **thingp, JSGCTraceKind kind);

struct UnmarkGrayTracer : public JSTracer
{
    



    UnmarkGrayTracer(JSRuntime *rt)
      : tracingShape(false), previousShape(NULL)
    {
        JS_TracerInit(this, rt, UnmarkGrayChildren);
        eagerlyTraceWeakMaps = false;
    }

    UnmarkGrayTracer(JSTracer *trc, bool tracingShape)
      : tracingShape(tracingShape), previousShape(NULL)
    {
        JS_TracerInit(this, trc->runtime, UnmarkGrayChildren);
        eagerlyTraceWeakMaps = false;
    }

    
    bool tracingShape;

    
    void *previousShape;
};































static void
UnmarkGrayChildren(JSTracer *trc, void **thingp, JSGCTraceKind kind)
{
    void *thing = *thingp;
    int stackDummy;
    if (!JS_CHECK_STACK_SIZE(js::GetNativeStackLimit(trc->runtime), &stackDummy)) {
        



        trc->runtime->gcGrayBitsValid = false;
        return;
    }

    if (!GCThingIsMarkedGray(thing))
        return;

    UnmarkGrayGCThing(thing);

    






    UnmarkGrayTracer *tracer = static_cast<UnmarkGrayTracer *>(trc);
    UnmarkGrayTracer childTracer(tracer, kind == JSTRACE_SHAPE);

    if (kind != JSTRACE_SHAPE) {
        JS_TraceChildren(&childTracer, thing, kind);
        JS_ASSERT(!childTracer.previousShape);
        return;
    }

    if (tracer->tracingShape) {
        JS_ASSERT(!tracer->previousShape);
        tracer->previousShape = thing;
        return;
    }

    do {
        JS_ASSERT(!GCThingIsMarkedGray(thing));
        JS_TraceChildren(&childTracer, thing, JSTRACE_SHAPE);
        thing = childTracer.previousShape;
        childTracer.previousShape = NULL;
    } while (thing);
}

JS_FRIEND_API(void)
JS::UnmarkGrayGCThingRecursively(void *thing, JSGCTraceKind kind)
{
    JS_ASSERT(kind != JSTRACE_SHAPE);

    if (!GCThingIsMarkedGray(thing))
        return;

    UnmarkGrayGCThing(thing);

    JSRuntime *rt = static_cast<Cell *>(thing)->zone()->rt;
    UnmarkGrayTracer trc(rt);
    JS_TraceChildren(&trc, thing, kind);
}
