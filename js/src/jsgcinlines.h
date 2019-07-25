






































#ifndef jsgcinlines_h___
#define jsgcinlines_h___

#include "jsgc.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsscope.h"
#include "jsxml.h"

#include "jslock.h"
#include "jstl.h"

namespace js {

struct Shape;

namespace gc {

inline JSGCTraceKind
GetGCThingTraceKind(const void *thing)
{
    JS_ASSERT(thing);
    const Cell *cell = reinterpret_cast<const Cell *>(thing);
    return MapAllocToTraceKind(cell->getAllocKind());
}


const size_t SLOTS_TO_THING_KIND_LIMIT = 17;


static inline AllocKind
GetGCObjectKind(size_t numSlots)
{
    extern AllocKind slotsToThingKind[];

    if (numSlots >= SLOTS_TO_THING_KIND_LIMIT)
        return FINALIZE_OBJECT16;
    return slotsToThingKind[numSlots];
}

static inline AllocKind
GetGCObjectKind(Class *clasp)
{
    if (clasp == &FunctionClass)
        return FINALIZE_FUNCTION;
    uint32 nslots = JSCLASS_RESERVED_SLOTS(clasp);
    if (clasp->flags & JSCLASS_HAS_PRIVATE)
        nslots++;
    return GetGCObjectKind(nslots);
}


static inline AllocKind
GetGCArrayKind(size_t numSlots)
{
    extern AllocKind slotsToThingKind[];

    





    JS_STATIC_ASSERT(sizeof(ObjectElements) == 2 * sizeof(Value));
    if (numSlots + 2 >= SLOTS_TO_THING_KIND_LIMIT)
        return FINALIZE_OBJECT2;
    return slotsToThingKind[numSlots + 2];
}

static inline AllocKind
GetGCObjectFixedSlotsKind(size_t numFixedSlots)
{
    extern AllocKind slotsToThingKind[];

    JS_ASSERT(numFixedSlots < SLOTS_TO_THING_KIND_LIMIT);
    return slotsToThingKind[numFixedSlots];
}

static inline bool
IsBackgroundAllocKind(AllocKind kind)
{
    JS_ASSERT(kind <= FINALIZE_FUNCTION);
    return kind % 2 == 1;
}

static inline AllocKind
GetBackgroundAllocKind(AllocKind kind)
{
    JS_ASSERT(!IsBackgroundAllocKind(kind));
    return (AllocKind) (kind + 1);
}





static inline bool
TryIncrementAllocKind(AllocKind *kindp)
{
    size_t next = size_t(*kindp) + 2;
    if (next > size_t(FINALIZE_OBJECT_LAST))
        return false;
    *kindp = AllocKind(next);
    return true;
}


static inline size_t
GetGCKindSlots(AllocKind thingKind)
{
    
    switch (thingKind) {
      case FINALIZE_OBJECT0:
      case FINALIZE_OBJECT0_BACKGROUND:
        return 0;
      case FINALIZE_OBJECT2:
      case FINALIZE_OBJECT2_BACKGROUND:
      case FINALIZE_FUNCTION:
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
ForEachArenaAndCell(JSCompartment *compartment, AllocKind thingKind,
                    ArenaOp arenaOp, CellOp cellOp)
{
    size_t thingSize = Arena::thingSize(thingKind);
    ArenaHeader *aheader = compartment->arenas.getFirstArena(thingKind);

    for (; aheader; aheader = aheader->next) {
        Arena *arena = aheader->getArena();
        arenaOp(arena);
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
                Cell *t = reinterpret_cast<Cell *>(thing);
                cellOp(t);
            }
        }
    }
}

class CellIterImpl
{
    size_t firstThingOffset;
    size_t thingSize;
    ArenaHeader *aheader;
    FreeSpan firstSpan;
    const FreeSpan *span;
    uintptr_t thing;
    Cell *cell;

  protected:
    CellIterImpl() {
    }

    void init(JSCompartment *comp, AllocKind kind) {
        JS_ASSERT(comp->arenas.isSynchronizedFreeList(kind));
        firstThingOffset = Arena::firstThingOffset(kind);
        thingSize = Arena::thingSize(kind);
        aheader = comp->arenas.getFirstArena(kind);
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
            thing = aheader->arenaAddress() | firstThingOffset;
            aheader = aheader->next;
        }
        cell = reinterpret_cast<Cell *>(thing);
        thing += thingSize;
    }
};

class CellIterUnderGC : public CellIterImpl {

  public:
    CellIterUnderGC(JSCompartment *comp, AllocKind kind) {
        JS_ASSERT(comp->rt->gcRunning);
        init(comp, kind);
    }
};






class CellIter: public CellIterImpl
{
    ArenaLists *lists;
    AllocKind kind;
#ifdef DEBUG
    size_t *counter;
#endif
  public:
    CellIter(JSContext *cx, JSCompartment *comp, AllocKind kind)
      : lists(&comp->arenas),
        kind(kind) {
#ifdef JS_THREADSAFE
        JS_ASSERT(comp->arenas.doneBackgroundFinalize(kind));
#endif
        if (lists->isSynchronizedFreeList(kind)) {
            lists = NULL;
        } else {
            JS_ASSERT(!comp->rt->gcRunning);
            lists->copyFreeListToArena(kind);
        }
#ifdef DEBUG
        counter = &JS_THREAD_DATA(cx)->noGCOrAllocationCheck;
        ++*counter;
#endif
        init(comp, kind);
    }

    ~CellIter() {
#ifdef DEBUG
        JS_ASSERT(*counter > 0);
        --*counter;
#endif
        if (lists)
            lists->clearFreeListInArena(kind);
    }
};



inline void EmptyArenaOp(Arena *arena) {}
inline void EmptyCellOp(Cell *t) {}

} 
} 








template <typename T>
inline T *
NewGCThing(JSContext *cx, js::gc::AllocKind kind, size_t thingSize)
{
    JS_ASSERT(thingSize == js::gc::Arena::thingSize(kind));
#ifdef JS_THREADSAFE
    JS_ASSERT_IF((cx->compartment == cx->runtime->atomsCompartment),
                 kind == js::gc::FINALIZE_STRING || kind == js::gc::FINALIZE_SHORT_STRING);
#endif
    JS_ASSERT(!cx->runtime->gcRunning);
    JS_ASSERT(!JS_THREAD_DATA(cx)->noGCOrAllocationCheck);

#ifdef JS_GC_ZEAL
    if (cx->runtime->needZealousGC())
        js::gc::RunDebugGC(cx);
#endif

    void *t = cx->compartment->arenas.allocateFromFreeList(kind, thingSize);
    return static_cast<T *>(t ? t : js::gc::ArenaLists::refillFreeList(cx, kind));
}

inline JSObject *
js_NewGCObject(JSContext *cx, js::gc::AllocKind kind)
{
    JS_ASSERT(kind >= js::gc::FINALIZE_OBJECT0 && kind <= js::gc::FINALIZE_FUNCTION);
    JSObject *obj = NewGCThing<JSObject>(cx, kind, js::gc::Arena::thingSize(kind));
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

inline js::BaseShape *
js_NewGCBaseShape(JSContext *cx)
{
    return NewGCThing<js::BaseShape>(cx, js::gc::FINALIZE_BASE_SHAPE, sizeof(js::BaseShape));
}

#if JS_HAS_XML_SUPPORT
extern JSXML *
js_NewGCXML(JSContext *cx);
#endif

#endif 
