






































#ifndef jsgcinlines_h___
#define jsgcinlines_h___

#include "jsgc.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsscope.h"
#include "jsxml.h"

#include "jslock.h"
#include "jstl.h"

inline bool
JSAtom::isUnitString(const void *ptr)
{
    jsuword delta = reinterpret_cast<jsuword>(ptr) -
                    reinterpret_cast<jsuword>(unitStaticTable);
    if (delta >= UNIT_STATIC_LIMIT * sizeof(JSString))
        return false;

    
    JS_ASSERT(delta % sizeof(JSString) == 0);
    return true;
}

inline bool
JSAtom::isLength2String(const void *ptr)
{
    jsuword delta = reinterpret_cast<jsuword>(ptr) -
                    reinterpret_cast<jsuword>(length2StaticTable);
    if (delta >= NUM_SMALL_CHARS * NUM_SMALL_CHARS * sizeof(JSString))
        return false;

    
    JS_ASSERT(delta % sizeof(JSString) == 0);
    return true;
}

inline bool
JSAtom::isHundredString(const void *ptr)
{
    jsuword delta = reinterpret_cast<jsuword>(ptr) -
                    reinterpret_cast<jsuword>(hundredStaticTable);
    if (delta >= NUM_HUNDRED_STATICS * sizeof(JSString))
        return false;

    
    JS_ASSERT(delta % sizeof(JSString) == 0);
    return true;
}

inline bool
JSAtom::isStatic(const void *ptr)
{
    return isUnitString(ptr) || isLength2String(ptr) || isHundredString(ptr);
}

namespace js {

struct Shape;

namespace gc {

inline uint32
GetGCThingTraceKind(const void *thing)
{
    JS_ASSERT(thing);
    if (JSAtom::isStatic(thing))
        return JSTRACE_STRING;
    const Cell *cell = reinterpret_cast<const Cell *>(thing);
    return GetFinalizableTraceKind(cell->arenaHeader()->getThingKind());
}


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
      case FINALIZE_OBJECT0_BACKGROUND:
        return 0;
      case FINALIZE_OBJECT2:
      case FINALIZE_OBJECT2_BACKGROUND:
        return 2;
      case FINALIZE_OBJECT4:
      case FINALIZE_OBJECT4_BACKGROUND:
        return 4;
      case FINALIZE_OBJECT8:
      case FINALIZE_OBJECT8_BACKGROUND:
        return 8;
      case FINALIZE_OBJECT12:
      case FINALIZE_OBJECT12_BACKGROUND:
        return 12;
      case FINALIZE_OBJECT16:
      case FINALIZE_OBJECT16_BACKGROUND:
        return 16;
      default:
        JS_NOT_REACHED("Bad object finalize kind");
        return 0;
    }
}

static inline void
GCPoke(JSContext *cx, Value oldval)
{
    




#if 1
    cx->runtime->gcPoke = JS_TRUE;
#else
    cx->runtime->gcPoke = oldval.isGCThing();
#endif

#ifdef JS_GC_ZEAL
    
    if (cx->runtime->gcZeal())
        cx->runtime->gcNextScheduled = 1;
#endif
}

} 
} 








template <typename T>
inline T *
NewGCThing(JSContext *cx, unsigned thingKind, size_t thingSize)
{
    JS_ASSERT(thingKind < js::gc::FINALIZE_LIMIT);
    JS_ASSERT(thingSize == js::gc::GCThingSizeMap[thingKind]);
#ifdef JS_THREADSAFE
    JS_ASSERT_IF((cx->compartment == cx->runtime->atomsCompartment),
                 (thingKind == js::gc::FINALIZE_STRING) ||
                 (thingKind == js::gc::FINALIZE_SHORT_STRING));
#endif
    JS_ASSERT(!cx->runtime->gcRunning);

#ifdef JS_GC_ZEAL
    if (cx->runtime->needZealousGC())
        js::gc::RunDebugGC(cx);
#endif

    js::gc::Cell *cell = cx->compartment->freeLists.getNext(thingKind, thingSize);
    return static_cast<T *>(cell ? cell : js::gc::RefillFinalizableFreeList(cx, thingKind));
}

inline JSObject *
js_NewGCObject(JSContext *cx, js::gc::FinalizeKind kind)
{
    JS_ASSERT(kind >= js::gc::FINALIZE_OBJECT0 && kind <= js::gc::FINALIZE_OBJECT_LAST);
    JSObject *obj = NewGCThing<JSObject>(cx, kind, js::gc::GCThingSizeMap[kind]);
    if (obj) {
        obj->capacity = js::gc::GetGCKindSlots(kind);
        obj->lastProp = NULL; 
    }
    return obj;
}

inline JSString *
js_NewGCString(JSContext *cx)
{
    return NewGCThing<JSString>(cx, js::gc::FINALIZE_STRING, sizeof(JSString));
}

inline JSShortString *
js_NewGCShortString(JSContext *cx)
{
    return NewGCThing<JSShortString>(cx, js::gc::FINALIZE_SHORT_STRING, sizeof(JSShortString));
}

inline JSExternalString *
js_NewGCExternalString(JSContext *cx)
{
    return NewGCThing<JSExternalString>(cx, js::gc::FINALIZE_EXTERNAL_STRING,
                                        sizeof(JSExternalString));
}

inline JSFunction*
js_NewGCFunction(JSContext *cx)
{
    JSFunction *fun = NewGCThing<JSFunction>(cx, js::gc::FINALIZE_FUNCTION, sizeof(JSFunction));
    if (fun) {
        fun->capacity = JSObject::FUN_CLASS_RESERVED_SLOTS;
        fun->lastProp = NULL; 
    }
    return fun;
}

inline js::Shape *
js_NewGCShape(JSContext *cx)
{
    return NewGCThing<js::Shape>(cx, js::gc::FINALIZE_SHAPE, sizeof(js::Shape));
}

#if JS_HAS_XML_SUPPORT
inline JSXML *
js_NewGCXML(JSContext *cx)
{
    return NewGCThing<JSXML>(cx, js::gc::FINALIZE_XML, sizeof(JSXML));
}
#endif


#endif 
