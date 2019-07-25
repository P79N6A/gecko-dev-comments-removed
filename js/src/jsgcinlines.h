






































#ifndef jsgcinlines_h___
#define jsgcinlines_h___

#include "jsgc.h"
#include "jscntxt.h"
#include "jscompartment.h"

#include "jslock.h"
#include "jstl.h"

#ifdef JS_GCMETER
# define METER(x)               ((void) (x))
# define METER_IF(condition, x) ((void) ((condition) && (x)))
#else
# define METER(x)               ((void) 0)
# define METER_IF(condition, x) ((void) 0)
#endif

namespace js {
namespace gc {


const size_t SLOTS_TO_THING_KIND_LIMIT = 17;


static inline FinalizeKind
GetGCObjectKind(size_t numSlots)
{
    extern FinalizeKind slotsToThingKind[];

    if (numSlots >= SLOTS_TO_THING_KIND_LIMIT)
        return FINALIZE_OBJECT0;
    return slotsToThingKind[numSlots];
}


static inline size_t
GetGCKindSlots(FinalizeKind thingKind)
{
    
    switch (thingKind) {
      case FINALIZE_OBJECT0:
        return 0;
      case FINALIZE_OBJECT2:
        return 2;
      case FINALIZE_OBJECT4:
        return 4;
      case FINALIZE_OBJECT8:
        return 8;
      case FINALIZE_OBJECT12:
        return 12;
      case FINALIZE_OBJECT16:
        return 16;
      default:
        JS_NOT_REACHED("Bad object finalize kind");
        return 0;
    }
}

} 
} 








template <typename T>
JS_ALWAYS_INLINE T *
NewFinalizableGCThing(JSContext *cx, unsigned thingKind)
{
    JS_ASSERT(thingKind < js::gc::FINALIZE_LIMIT);
#ifdef JS_THREADSAFE
    JS_ASSERT_IF((cx->compartment == cx->runtime->atomsCompartment),
                 (thingKind == js::gc::FINALIZE_STRING) ||
                 (thingKind == js::gc::FINALIZE_SHORT_STRING));
#endif

    METER(cx->compartment->compartmentStats[thingKind].alloc++);
    do {
        js::gc::FreeCell *cell = cx->compartment->freeLists.getNext(thingKind);
        if (cell) {
            CheckGCFreeListLink(cell);
            return (T *)cell;
        }
        if (!RefillFinalizableFreeList(cx, thingKind))
            return NULL;
    } while (true);
}

#undef METER
#undef METER_IF

inline JSObject *
js_NewGCObject(JSContext *cx, js::gc::FinalizeKind kind)
{
    JS_ASSERT(kind >= js::gc::FINALIZE_OBJECT0 && kind <= js::gc::FINALIZE_OBJECT_LAST);
    JSObject *obj = NewFinalizableGCThing<JSObject>(cx, kind);
    if (obj)
        obj->capacity = js::gc::GetGCKindSlots(kind);
    return obj;
}

inline JSString *
js_NewGCString(JSContext *cx)
{
    return NewFinalizableGCThing<JSString>(cx, js::gc::FINALIZE_STRING);    
}

inline JSShortString *
js_NewGCShortString(JSContext *cx)
{
    return NewFinalizableGCThing<JSShortString>(cx, js::gc::FINALIZE_SHORT_STRING);
}

inline JSExternalString *
js_NewGCExternalString(JSContext *cx, uintN type)
{
    JS_ASSERT(type < JSExternalString::TYPE_LIMIT);
    JSExternalString *str = NewFinalizableGCThing<JSExternalString>(cx, js::gc::FINALIZE_EXTERNAL_STRING);
    return str;
}

inline JSFunction*
js_NewGCFunction(JSContext *cx)
{
    JSFunction *fun = NewFinalizableGCThing<JSFunction>(cx, js::gc::FINALIZE_FUNCTION);
    if (fun)
        fun->capacity = JSObject::FUN_CLASS_RESERVED_SLOTS;
    return fun;
}

#if JS_HAS_XML_SUPPORT
inline JSXML *
js_NewGCXML(JSContext *cx)
{
    return NewFinalizableGCThing<JSXML>(cx, js::gc::FINALIZE_XML);
}
#endif

namespace js {
namespace gc {

static JS_ALWAYS_INLINE void
TypedMarker(JSTracer *trc, JSXML *thing);

static JS_ALWAYS_INLINE void
TypedMarker(JSTracer *trc, JSObject *thing);

static JS_ALWAYS_INLINE void
TypedMarker(JSTracer *trc, JSFunction *thing);

static JS_ALWAYS_INLINE void
TypedMarker(JSTracer *trc, JSShortString *thing);

static JS_ALWAYS_INLINE void
TypedMarker(JSTracer *trc, JSString *thing);

template<typename T>
static JS_ALWAYS_INLINE void
Mark(JSTracer *trc, T *thing)
{
    JS_ASSERT(thing);
    JS_ASSERT(JS_IS_VALID_TRACE_KIND(GetGCThingTraceKind(thing)));
    JS_ASSERT(trc->debugPrinter || trc->debugPrintArg);

    
    JS_ASSERT_IF(trc->context->runtime->gcCurrentCompartment, IS_GC_MARKING_TRACER(trc));

    JSRuntime *rt = trc->context->runtime;
    
    if (rt->gcCurrentCompartment && thing->asCell()->compartment() != rt->gcCurrentCompartment)
        goto out;

    if (!IS_GC_MARKING_TRACER(trc)) {
        uint32 kind = GetGCThingTraceKind(thing);
        trc->callback(trc, thing, kind);
        goto out;
    }

    TypedMarker(trc, thing);

  out:
#ifdef DEBUG
    trc->debugPrinter = NULL;
    trc->debugPrintArg = NULL;
#endif
    return;     
}

static inline void
MarkString(JSTracer *trc, JSString *str)
{
    JS_ASSERT(str);
    if (JSString::isStatic(str))
        return;
    JS_ASSERT(GetArena<JSString>((Cell *)str)->assureThingIsAligned((JSString *)str));
    Mark(trc, str);
}

static inline void
MarkString(JSTracer *trc, JSString *str, const char *name)
{
    JS_ASSERT(str);
    JS_SET_TRACING_NAME(trc, name);
    MarkString(trc, str);
}

static inline void
MarkObject(JSTracer *trc, JSObject &obj, const char *name)
{
    JS_ASSERT(trc);
    JS_ASSERT(&obj);
    JS_SET_TRACING_NAME(trc, name);
    JS_ASSERT(GetArena<JSObject>((Cell *)&obj)->assureThingIsAligned(&obj) ||
              GetArena<JSObject_Slots2>((Cell *)&obj)->assureThingIsAligned(&obj) ||
              GetArena<JSObject_Slots4>((Cell *)&obj)->assureThingIsAligned(&obj) ||
              GetArena<JSObject_Slots8>((Cell *)&obj)->assureThingIsAligned(&obj) ||
              GetArena<JSObject_Slots12>((Cell *)&obj)->assureThingIsAligned(&obj) ||
              GetArena<JSObject_Slots16>((Cell *)&obj)->assureThingIsAligned(&obj) ||
              GetArena<JSFunction>((Cell *)&obj)->assureThingIsAligned(&obj));
    Mark(trc, &obj);
}

static inline void
MarkChildren(JSTracer *trc, JSObject *obj)
{
    



    if (obj->type && !obj->type->marked)
        obj->type->trace(trc);

    
    if (!obj->map)
        return;

    
    if (!obj->isDenseArray() && obj->newType && !obj->newType->marked)
        obj->newType->trace(trc);
    if (JSObject *parent = obj->getParent())
        MarkObject(trc, *parent, "parent");

    Class *clasp = obj->getClass();
    if (clasp->trace)
        clasp->trace(trc, obj);

    if (obj->isNative())
        js_TraceObject(trc, obj);
}

static inline void
MarkChildren(JSTracer *trc, JSString *str)
{
    if (str->isDependent())
        MarkString(trc, str->dependentBase(), "base");
    else if (str->isRope()) {
        MarkString(trc, str->ropeLeft(), "left child");
        MarkString(trc, str->ropeRight(), "right child");
    }
}

#ifdef JS_HAS_XML_SUPPORT
static inline void
MarkChildren(JSTracer *trc, JSXML *xml)
{
    js_TraceXML(trc, xml);
}
#endif

static inline bool
RecursionTooDeep(GCMarker *gcmarker) {
#ifdef JS_GC_ASSUME_LOW_C_STACK
    return true;
#else
    int stackDummy;
    return !JS_CHECK_STACK_SIZE(gcmarker->stackLimit, &stackDummy);
#endif
}

static JS_ALWAYS_INLINE void
TypedMarker(JSTracer *trc, JSXML *thing)
{
    if (!reinterpret_cast<Cell *>(thing)->markIfUnmarked(reinterpret_cast<GCMarker *>(trc)->getMarkColor()))
        return;
    GCMarker *gcmarker = static_cast<GCMarker *>(trc);
    if (RecursionTooDeep(gcmarker)) {
        gcmarker->delayMarkingChildren(thing);
    } else {
        MarkChildren(trc, thing);
    }
}

static JS_ALWAYS_INLINE void
TypedMarker(JSTracer *trc, JSObject *thing)
{
    JS_ASSERT(thing);
    JS_ASSERT(JSTRACE_OBJECT == GetFinalizableTraceKind(thing->asCell()->arena()->header()->thingKind));

    GCMarker *gcmarker = static_cast<GCMarker *>(trc);
    if (!thing->markIfUnmarked(gcmarker->getMarkColor()))
        return;
    
    if (RecursionTooDeep(gcmarker)) {
        gcmarker->delayMarkingChildren(thing);
    } else {
        MarkChildren(trc, thing);
    }
}

static JS_ALWAYS_INLINE void
TypedMarker(JSTracer *trc, JSFunction *thing)
{
    JS_ASSERT(thing);
    JS_ASSERT(JSTRACE_OBJECT == GetFinalizableTraceKind(thing->asCell()->arena()->header()->thingKind));

    GCMarker *gcmarker = static_cast<GCMarker *>(trc);
    if (!thing->markIfUnmarked(gcmarker->getMarkColor()))
        return;

    if (RecursionTooDeep(gcmarker)) {
        gcmarker->delayMarkingChildren(thing);
    } else {
        MarkChildren(trc, static_cast<JSObject *>(thing));
    }
}

static JS_ALWAYS_INLINE void
TypedMarker(JSTracer *trc, JSShortString *thing)
{
    




    (void) thing->asCell()->markIfUnmarked();
}

}  

namespace detail {

static JS_ALWAYS_INLINE JSString *
Tag(JSString *str)
{
    JS_ASSERT(!(size_t(str) & 1));
    return (JSString *)(size_t(str) | 1);
}

static JS_ALWAYS_INLINE bool
Tagged(JSString *str)
{
    return (size_t(str) & 1) != 0;
}

static JS_ALWAYS_INLINE JSString *
Untag(JSString *str)
{
    JS_ASSERT((size_t(str) & 1) == 1);
    return (JSString *)(size_t(str) & ~size_t(1));
}

static JS_ALWAYS_INLINE void
NonRopeTypedMarker(JSRuntime *rt, JSString *str)
{
    
    JS_ASSERT(!str->isRope());

    if (rt->gcCurrentCompartment) {
        for (;;) {
            if (JSString::isStatic(str))
                break;

            



            if (str->asCell()->compartment() != rt->gcCurrentCompartment) {
                JS_ASSERT(str->asCell()->compartment() == rt->atomsCompartment);
                break;
            }
            if (!str->asCell()->markIfUnmarked())
                break;
            if (!str->isDependent())
                break;
            str = str->dependentBase();
        }
    } else {
        while (!JSString::isStatic(str) &&
               str->asCell()->markIfUnmarked() &&
               str->isDependent()) {
            str = str->dependentBase();
        }
    }
}

}  

namespace gc {

static JS_ALWAYS_INLINE void
TypedMarker(JSTracer *trc, JSString *str)
{
    using namespace detail;
    JSRuntime *rt = trc->context->runtime;
    JS_ASSERT(!JSString::isStatic(str));
#ifdef DEBUG
    JSCompartment *strComp = str->asCell()->compartment();
#endif
    if (!str->isRope()) {
        NonRopeTypedMarker(rt, str);
        return;
    }

    





    JSString *parent = NULL;
    first_visit_node: {
        JS_ASSERT(strComp == str->asCell()->compartment() || str->asCell()->compartment() == rt->atomsCompartment);
        JS_ASSERT(!JSString::isStatic(str));
        if (!str->asCell()->markIfUnmarked())
            goto finish_node;
        JSString *left = str->ropeLeft();
        if (left->isRope()) {
            JS_ASSERT(!Tagged(str->u.left) && !Tagged(str->s.right));
            str->u.left = Tag(parent);
            parent = str;
            str = left;
            goto first_visit_node;
        }
        JS_ASSERT_IF(!JSString::isStatic(left), 
                     strComp == left->asCell()->compartment()
                     || left->asCell()->compartment() == rt->atomsCompartment);
        NonRopeTypedMarker(rt, left);
    }
    visit_right_child: {
        JSString *right = str->ropeRight();
        if (right->isRope()) {
            JS_ASSERT(!Tagged(str->u.left) && !Tagged(str->s.right));
            str->s.right = Tag(parent);
            parent = str;
            str = right;
            goto first_visit_node;
        }
        JS_ASSERT_IF(!JSString::isStatic(right), 
                     strComp == right->asCell()->compartment()
                     || right->asCell()->compartment() == rt->atomsCompartment);
        NonRopeTypedMarker(rt, right);
    }
    finish_node: {
        if (!parent)
            return;
        if (Tagged(parent->u.left)) {
            JS_ASSERT(!Tagged(parent->s.right));
            JSString *nextParent = Untag(parent->u.left);
            parent->u.left = str;
            str = parent;
            parent = nextParent;
            goto visit_right_child;
        }
        JS_ASSERT(Tagged(parent->s.right));
        JSString *nextParent = Untag(parent->s.right);
        parent->s.right = str;
        str = parent;
        parent = nextParent;
        goto finish_node;
    }
}

static inline void
MarkAtomRange(JSTracer *trc, size_t len, JSAtom **vec, const char *name)
{
    for (uint32 i = 0; i < len; i++) {
        if (JSAtom *atom = vec[i]) {
            JS_SET_TRACING_INDEX(trc, name, i);
            JSString *str = ATOM_TO_STRING(atom);
            if (!JSString::isStatic(str))
                Mark(trc, str);
        }
    }
}

static inline void
MarkObjectRange(JSTracer *trc, size_t len, JSObject **vec, const char *name)
{
    for (uint32 i = 0; i < len; i++) {
        if (JSObject *obj = vec[i]) {
            JS_SET_TRACING_INDEX(trc, name, i);
            Mark(trc, obj);
        }
    }
}

static inline void
MarkId(JSTracer *trc, jsid id)
{
    if (JSID_IS_STRING(id)) {
        JSString *str = JSID_TO_STRING(id);
        if (!JSString::isStatic(str))
            Mark(trc, str);
    }
    else if (JS_UNLIKELY(JSID_IS_OBJECT(id)))
        Mark(trc, JSID_TO_OBJECT(id));
}

static inline void
MarkId(JSTracer *trc, jsid id, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkId(trc, id);
}

static inline void
MarkIdRange(JSTracer *trc, jsid *beg, jsid *end, const char *name)
{
    for (jsid *idp = beg; idp != end; ++idp) {
        JS_SET_TRACING_INDEX(trc, name, (idp - beg));
        MarkId(trc, *idp);
    }
}

static inline void
MarkIdRange(JSTracer *trc, size_t len, jsid *vec, const char *name)
{
    MarkIdRange(trc, vec, vec + len, name);
}

static inline void
MarkKind(JSTracer *trc, void *thing, uint32 kind)
{
    JS_ASSERT(thing);
    JS_ASSERT(kind == GetGCThingTraceKind(thing));
    switch (kind) {
        case JSTRACE_OBJECT:
            Mark(trc, reinterpret_cast<JSObject *>(thing));
            break;
        case JSTRACE_STRING:
            MarkString(trc, reinterpret_cast<JSString *>(thing));
            break;
#if JS_HAS_XML_SUPPORT
        case JSTRACE_XML:
            Mark(trc, reinterpret_cast<JSXML *>(thing));
            break;
#endif
        default:
            JS_ASSERT(false);
    }
}


static inline void
MarkValueRaw(JSTracer *trc, const js::Value &v)
{
    if (v.isMarkable()) {
        JS_ASSERT(v.toGCThing());
        return MarkKind(trc, v.toGCThing(), v.gcKind());
    }
}

static inline void
MarkValue(JSTracer *trc, const js::Value &v, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkValueRaw(trc, v);
}

static inline void
MarkValueRange(JSTracer *trc, Value *beg, Value *end, const char *name)
{
    for (Value *vp = beg; vp < end; ++vp) {
        JS_SET_TRACING_INDEX(trc, name, vp - beg);
        MarkValueRaw(trc, *vp);
    }
}

static inline void
MarkValueRange(JSTracer *trc, size_t len, Value *vec, const char *name)
{
    MarkValueRange(trc, vec, vec + len, name);
}

static inline void
MarkShapeRange(JSTracer *trc, const Shape **beg, const Shape **end, const char *name)
{
    for (const Shape **sp = beg; sp < end; ++sp) {
        JS_SET_TRACING_INDEX(trc, name, sp - beg);
        (*sp)->trace(trc);
    }
}

static inline void
MarkShapeRange(JSTracer *trc, size_t len, const Shape **vec, const char *name)
{
    MarkShapeRange(trc, vec, vec + len, name);
}


static inline void
MarkGCThing(JSTracer *trc, void *thing, uint32 kind)
{
    if (!thing)
        return;

    MarkKind(trc, thing, kind);
}

static inline void
MarkGCThing(JSTracer *trc, void *thing)
{
    if (!thing)
        return;
    MarkKind(trc, thing, GetGCThingTraceKind(thing));
}

static inline void
MarkGCThing(JSTracer *trc, void *thing, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkGCThing(trc, thing);
}

static inline void
MarkGCThing(JSTracer *trc, void *thing, const char *name, size_t index)
{
    JS_SET_TRACING_INDEX(trc, name, index);
    MarkGCThing(trc, thing);
}

static inline void
Mark(JSTracer *trc, void *thing, uint32 kind, const char *name)
{
    JS_ASSERT(thing);
    JS_SET_TRACING_NAME(trc, name);
    MarkKind(trc, thing, kind);
}

}}

#endif 
