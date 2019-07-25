






































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
#ifdef JS_HAS_STATIC_STRINGS
    jsuword delta = reinterpret_cast<jsuword>(ptr) -
                    reinterpret_cast<jsuword>(unitStaticTable);
    if (delta >= UNIT_STATIC_LIMIT * sizeof(JSString))
        return false;

    
    JS_ASSERT(delta % sizeof(JSString) == 0);
    return true;
#else
    return false;
#endif
}

inline bool
JSAtom::isLength2String(const void *ptr)
{
#ifdef JS_HAS_STATIC_STRINGS
    jsuword delta = reinterpret_cast<jsuword>(ptr) -
                    reinterpret_cast<jsuword>(length2StaticTable);
    if (delta >= NUM_SMALL_CHARS * NUM_SMALL_CHARS * sizeof(JSString))
        return false;

    
    JS_ASSERT(delta % sizeof(JSString) == 0);
    return true;
#else
    return false;
#endif
}

inline bool
JSAtom::isHundredString(const void *ptr)
{
#ifdef JS_HAS_STATIC_STRINGS
    jsuword delta = reinterpret_cast<jsuword>(ptr) -
                    reinterpret_cast<jsuword>(hundredStaticTable);
    if (delta >= NUM_HUNDRED_STATICS * sizeof(JSString))
        return false;

    
    JS_ASSERT(delta % sizeof(JSString) == 0);
    return true;
#else
    return false;
#endif
}

inline bool
JSAtom::isStatic(const void *ptr)
{
    return isUnitString(ptr) || isLength2String(ptr) || isHundredString(ptr);
}

namespace js {

struct Shape;

namespace gc {

inline JSGCTraceKind
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
GetGCObjectKind(size_t numSlots, bool isArray = false)
{
    extern FinalizeKind slotsToThingKind[];

    if (numSlots >= SLOTS_TO_THING_KIND_LIMIT) {
        





        return isArray ? FINALIZE_OBJECT0 : FINALIZE_OBJECT16;
    }
    return slotsToThingKind[numSlots];
}

static inline bool
IsBackgroundFinalizeKind(FinalizeKind kind)
{
    JS_ASSERT(kind <= FINALIZE_OBJECT_LAST);
    return kind % 2 == 1;
}

static inline FinalizeKind
GetBackgroundFinalizeKind(FinalizeKind kind)
{
    JS_ASSERT(!IsBackgroundFinalizeKind(kind));
    return (FinalizeKind) (kind + 1);
}

static inline bool
CanBumpFinalizeKind(FinalizeKind kind)
{
    JS_ASSERT(kind <= FINALIZE_OBJECT_LAST);
    return (kind + 2) <= FINALIZE_OBJECT_LAST;
}


static inline FinalizeKind
BumpFinalizeKind(FinalizeKind kind)
{
    JS_ASSERT(CanBumpFinalizeKind(kind));
    return (FinalizeKind) (kind + 2);
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





template <class ArenaOp, class CellOp>
void
ForEachArenaAndCell(JSCompartment *compartment, FinalizeKind thingKind,
                    ArenaOp arenaOp, CellOp cellOp)
{
    size_t thingSize = GCThingSizeMap[thingKind];
    ArenaHeader *aheader = compartment->arenas[thingKind].getHead();

    for (; aheader; aheader = aheader->next) {
        Arena *arena = aheader->getArena();
        arenaOp(arena);
        FreeSpan firstSpan(aheader->getFirstFreeSpan());
        const FreeSpan *span = &firstSpan;

        for (uintptr_t thing = arena->thingsStart(thingSize); ; thing += thingSize) {
            JS_ASSERT(thing <= arena->thingsEnd());
            if (thing == span->first) {
                if (!span->hasNext())
                    break;
                thing = span->last;
                span = span->nextSpan();
            } else {
                Cell *t = reinterpret_cast<Cell *>(thing);
                cellOp(t);
            }
        }
    }
}

class CellIterImpl
{
    size_t thingSize;
    ArenaHeader *aheader;
    FreeSpan firstSpan;
    const FreeSpan *span;
    uintptr_t thing;
    Cell *cell;

  protected:
    CellIterImpl() {
    }

    void init(JSCompartment *comp, FinalizeKind thingKind) {
        thingSize = GCThingSizeMap[thingKind];
        aheader = comp->arenas[thingKind].getHead();
        firstSpan.initAsEmpty();
        span = &firstSpan;
        thing = span->first;
        next();
    }

  public:
    bool done() const {
        return !cell;
    }

    template<typename T> T *get() const {
        JS_ASSERT(!done());
        return static_cast<T *>(cell);
    }

    Cell *getCell() const {
        JS_ASSERT(!done());
        return cell;
    }

    void next() {
        for (;;) {
            if (thing != span->first)
                break;
            if (JS_LIKELY(span->hasNext())) {
                thing = span->last + thingSize;
                span = span->nextSpan();
                break;
            }
            if (!aheader) {
                cell = NULL;
                return;
            }
            firstSpan = aheader->getFirstFreeSpan();
            span = &firstSpan;
            thing = aheader->getArena()->thingsStart(thingSize);
            aheader = aheader->next;
        }
        cell = reinterpret_cast<Cell *>(thing);
        thing += thingSize;
    }
};

class CellIterUnderGC : public CellIterImpl {

  public:
    CellIterUnderGC(JSCompartment *comp, FinalizeKind thingKind) {
        JS_ASSERT(comp->rt->gcRunning);
        JS_ASSERT(comp->freeLists.lists[thingKind].isEmpty());
        init(comp, thingKind);
    }
};






class CellIter: public CellIterImpl
{
    FreeLists *lists;
    FinalizeKind thingKind;
#ifdef DEBUG
    size_t *counter;
#endif
  public:
    CellIter(JSContext *cx, JSCompartment *comp, FinalizeKind thingKind)
      : lists(&comp->freeLists),
        thingKind(thingKind) {
#ifdef JS_THREADSAFE
        JS_ASSERT(comp->arenas[thingKind].doneBackgroundFinalize());
#endif
        if (lists->isSynchronizedWithArena(thingKind)) {
            lists = NULL;
        } else {
            JS_ASSERT(!comp->rt->gcRunning);
            lists->copyToArena(thingKind);
        }
#ifdef DEBUG
        counter = &JS_THREAD_DATA(cx)->noGCOrAllocationCheck;
        ++*counter;
#endif
        init(comp, thingKind);
    }

    ~CellIter() {
#ifdef DEBUG
        JS_ASSERT(*counter > 0);
        --*counter;
#endif
        if (lists)
            lists->clearInArena(thingKind);
    }
};



inline void EmptyArenaOp(Arena *arena) {}
inline void EmptyCellOp(Cell *t) {}

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
    JS_ASSERT(!JS_THREAD_DATA(cx)->noGCOrAllocationCheck);

#ifdef JS_GC_ZEAL
    if (cx->runtime->needZealousGC())
        js::gc::RunDebugGC(cx);
#endif

    void *t = cx->compartment->freeLists.getNext(thingKind, thingSize);
    return static_cast<T *>(t ? t : js::gc::RefillFinalizableFreeList(cx, thingKind));
}

inline JSObject *
js_NewGCObject(JSContext *cx, js::gc::FinalizeKind kind)
{
    JS_ASSERT(kind >= js::gc::FINALIZE_OBJECT0 && kind <= js::gc::FINALIZE_OBJECT_LAST);
    JSObject *obj = NewGCThing<JSObject>(cx, kind, js::gc::GCThingSizeMap[kind]);
    if (obj)
        obj->earlyInit(js::gc::GetGCKindSlots(kind));
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

inline JSScript *
js_NewGCScript(JSContext *cx)
{
    return NewGCThing<JSScript>(cx, js::gc::FINALIZE_SCRIPT, sizeof(JSScript));
}

inline js::Shape *
js_NewGCShape(JSContext *cx)
{
    return NewGCThing<js::Shape>(cx, js::gc::FINALIZE_SHAPE, sizeof(js::Shape));
}

#if JS_HAS_XML_SUPPORT
extern JSXML *
js_NewGCXML(JSContext *cx);
#endif

#endif 
