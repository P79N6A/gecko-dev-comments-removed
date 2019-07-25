






































#ifndef jsgc_h___
#define jsgc_h___



#include <setjmp.h>

#include "jstypes.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsdhash.h"
#include "jsbit.h"
#include "jsgcchunk.h"
#include "jsutil.h"
#include "jsvector.h"
#include "jsversion.h"
#include "jsobj.h"
#include "jsfun.h"
#include "jsgcstats.h"

#define JSTRACE_XML         2




#define JSTRACE_LIMIT       3




const size_t GC_ARENA_ALLOCATION_TRIGGER = 25 * js::GC_CHUNK_SIZE;






const float GC_HEAP_GROWTH_FACTOR = 1.5;

const uintN JS_EXTERNAL_STRING_LIMIT = 8;





extern intN
js_GetExternalStringGCType(JSString *str);

extern JS_FRIEND_API(uint32)
js_GetGCThingTraceKind(void *thing);

extern size_t
ThingsPerArena(size_t thingSize);





JSRuntime *
js_GetGCThingRuntime(void *thing);

#if 1





#define GC_POKE(cx, oldval) ((cx)->runtime->gcPoke = JS_TRUE)
#else
#define GC_POKE(cx, oldval) ((cx)->runtime->gcPoke = JSVAL_IS_GCTHING(oldval))
#endif

extern JSBool
js_InitGC(JSRuntime *rt, uint32 maxbytes);

extern void
js_FinishGC(JSRuntime *rt);

extern intN
js_ChangeExternalStringFinalizer(JSStringFinalizeOp oldop,
                                 JSStringFinalizeOp newop);

extern JSBool
js_AddRoot(JSContext *cx, js::Value *vp, const char *name);

extern JSBool
js_AddGCThingRoot(JSContext *cx, void **rp, const char *name);

#ifdef DEBUG
extern void
js_DumpNamedRoots(JSRuntime *rt,
                  void (*dump)(const char *name, void *rp, JSGCRootType type, void *data),
                  void *data);
#endif

extern uint32
js_MapGCRoots(JSRuntime *rt, JSGCRootMapFun map, void *data);


typedef struct JSPtrTable {
    size_t      count;
    void        **array;
} JSPtrTable;

extern JSBool
js_RegisterCloseableIterator(JSContext *cx, JSObject *obj);

#ifdef JS_TRACER
extern JSBool
js_ReserveObjects(JSContext *cx, size_t nobjects);
#endif

extern JSBool
js_LockGCThingRT(JSRuntime *rt, void *thing);

extern void
js_UnlockGCThingRT(JSRuntime *rt, void *thing);

extern JS_FRIEND_API(bool)
js_IsAboutToBeFinalized(void *thing);

extern JS_FRIEND_API(bool)
js_GCThingIsMarked(void *thing, uint32 color);





#define IS_GC_MARKING_TRACER(trc) ((trc)->callback == NULL)

#if JS_HAS_XML_SUPPORT
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) < JSTRACE_LIMIT)
#else
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) <= JSTRACE_STRING)
#endif

extern void
js_TraceStackFrame(JSTracer *trc, JSStackFrame *fp);

namespace js {

extern JS_REQUIRES_STACK void
MarkRuntime(JSTracer *trc);

extern void
TraceRuntime(JSTracer *trc);

extern JS_REQUIRES_STACK JS_FRIEND_API(void)
MarkContext(JSTracer *trc, JSContext *acx);


extern void
TriggerGC(JSRuntime *rt);

} 




typedef enum JSGCInvocationKind {
    
    GC_NORMAL           = 0,

    



    GC_LAST_CONTEXT     = 1,

    


    GC_LOCK_HELD        = 0x10
} JSGCInvocationKind;

extern void
js_GC(JSContext *cx, JSGCInvocationKind gckind);

#ifdef JS_THREADSAFE






extern void
js_WaitForGC(JSRuntime *rt);

#else 

# define js_WaitForGC(rt)    ((void) 0)

#endif





enum JSFinalizeGCThingKind {
    FINALIZE_OBJECT,
    FINALIZE_FUNCTION,
#if JS_HAS_XML_SUPPORT
    FINALIZE_XML,
#endif
    FINALIZE_SHORT_STRING,
    FINALIZE_STRING,
    FINALIZE_EXTERNAL_STRING0,
    FINALIZE_EXTERNAL_STRING1,
    FINALIZE_EXTERNAL_STRING2,
    FINALIZE_EXTERNAL_STRING3,
    FINALIZE_EXTERNAL_STRING4,
    FINALIZE_EXTERNAL_STRING5,
    FINALIZE_EXTERNAL_STRING6,
    FINALIZE_EXTERNAL_STRING7,
    FINALIZE_EXTERNAL_STRING_LAST = FINALIZE_EXTERNAL_STRING7,
    FINALIZE_LIMIT
};

static inline bool
IsFinalizableStringKind(unsigned thingKind)
{
    return unsigned(FINALIZE_SHORT_STRING) <= thingKind &&
           thingKind <= unsigned(FINALIZE_EXTERNAL_STRING_LAST);
}







extern void *
js_NewFinalizableGCThing(JSContext *cx, unsigned thingKind);

static inline JSObject *
js_NewGCObject(JSContext *cx)
{
    return (JSObject *) js_NewFinalizableGCThing(cx, FINALIZE_OBJECT);
}

static inline JSString *
js_NewGCString(JSContext *cx)
{
    return (JSString *) js_NewFinalizableGCThing(cx, FINALIZE_STRING);
}

struct JSShortString;

static inline JSShortString *
js_NewGCShortString(JSContext *cx)
{
    return (JSShortString *) js_NewFinalizableGCThing(cx, FINALIZE_SHORT_STRING);
}

static inline JSString *
js_NewGCExternalString(JSContext *cx, uintN type)
{
    JS_ASSERT(type < JS_EXTERNAL_STRING_LIMIT);
    type += FINALIZE_EXTERNAL_STRING0;
    return (JSString *) js_NewFinalizableGCThing(cx, type);
}

static inline JSFunction *
js_NewGCFunction(JSContext *cx)
{
    JSFunction* obj = (JSFunction *)js_NewFinalizableGCThing(cx, FINALIZE_FUNCTION);

#ifdef DEBUG
    if (obj) {
        memset((uint8 *) obj + sizeof(JSObject), JS_FREE_PATTERN,
               sizeof(JSFunction) - sizeof(JSObject));
    }
#endif

    return obj;
}

#if JS_HAS_XML_SUPPORT
static inline JSXML *
js_NewGCXML(JSContext *cx)
{
    return (JSXML *) js_NewFinalizableGCThing(cx, FINALIZE_XML);
}
#endif

struct JSGCArena;

struct JSGCArenaList {
    JSGCArena       *head;          
    JSGCArena       *cursor;        
    uint32          thingKind;      
    uint32          thingSize;      

};

struct JSGCFreeLists {
    JSGCThing       *finalizables[FINALIZE_LIMIT];

    void purge();
    void moveTo(JSGCFreeLists * another);

#ifdef DEBUG
    bool isEmpty() const {
        for (size_t i = 0; i != JS_ARRAY_LENGTH(finalizables); ++i) {
            if (finalizables[i])
                return false;
        }
        return true;
    }
#endif
};

extern void
js_DestroyScriptsToGC(JSContext *cx, JSThreadData *data);

namespace js {

#ifdef JS_THREADSAFE











class GCHelperThread {
    static const size_t FREE_ARRAY_SIZE = size_t(1) << 16;
    static const size_t FREE_ARRAY_LENGTH = FREE_ARRAY_SIZE / sizeof(void *);

    PRThread*         thread;
    PRCondVar*        wakeup;
    PRCondVar*        sweepingDone;
    bool              shutdown;
    bool              sweeping;

    Vector<void **, 16, js::SystemAllocPolicy> freeVector;
    void            **freeCursor;
    void            **freeCursorEnd;

    JS_FRIEND_API(void)
    replenishAndFreeLater(void *ptr);

    static void freeElementsAndArray(void **array, void **end) {
        JS_ASSERT(array <= end);
        for (void **p = array; p != end; ++p)
            js_free(*p);
        js_free(array);
    }

    static void threadMain(void* arg);

    void threadLoop(JSRuntime *rt);
    void doSweep();

  public:
    GCHelperThread()
      : thread(NULL),
        wakeup(NULL),
        sweepingDone(NULL),
        shutdown(false),
        sweeping(false),
        freeCursor(NULL),
        freeCursorEnd(NULL) { }
    
    bool init(JSRuntime *rt);
    void finish(JSRuntime *rt);
    
    
    void startBackgroundSweep(JSRuntime *rt);
    
    
    void waitBackgroundSweepEnd(JSRuntime *rt);
    
    void freeLater(void *ptr) {
        JS_ASSERT(!sweeping);
        if (freeCursor != freeCursorEnd)
            *freeCursor++ = ptr;
        else
            replenishAndFreeLater(ptr);
    }
};

#endif 


struct GCChunkInfo;

struct GCChunkHasher {
    typedef jsuword Lookup;

    



    static HashNumber hash(jsuword chunk) {
        JS_ASSERT(!(chunk & GC_CHUNK_MASK));
        return HashNumber(chunk >> GC_CHUNK_SHIFT);
    }

    static bool match(jsuword k, jsuword l) {
        JS_ASSERT(!(k & GC_CHUNK_MASK));
        JS_ASSERT(!(l & GC_CHUNK_MASK));
        return k == l;
    }
};

typedef HashSet<jsuword, GCChunkHasher, SystemAllocPolicy> GCChunkSet;
typedef Vector<GCChunkInfo *, 32, SystemAllocPolicy> GCChunkInfoVector;

struct ConservativeGCThreadData {

    



    jsuword             *nativeStackTop;

    union {
        jmp_buf         jmpbuf;
        jsuword         words[JS_HOWMANY(sizeof(jmp_buf), sizeof(jsuword))];
    } registerSnapshot;

    




    unsigned requestThreshold;

    JS_NEVER_INLINE void recordStackTop();

#ifdef JS_THREADSAFE
    void updateForRequestEnd(unsigned suspendCount) {
        if (suspendCount)
            recordStackTop();
        else
            nativeStackTop = NULL;
    }
#endif

    bool hasStackToScan() const {
        return !!nativeStackTop;
    }
};

struct GCMarker : public JSTracer {
  private:
    
    uint32 color;

    
    JSGCArena           *unmarkedArenaStackTop;
#ifdef DEBUG
    size_t              markLaterCount;
#endif

  public:
#if defined(JS_DUMP_CONSERVATIVE_GC_ROOTS) || defined(JS_GCMETER)
    ConservativeGCStats conservativeStats;
#endif

#ifdef JS_DUMP_CONSERVATIVE_GC_ROOTS
    struct ConservativeRoot { void *thing; uint32 traceKind; };
    Vector<ConservativeRoot, 0, SystemAllocPolicy> conservativeRoots;
    const char *conservativeDumpFileName;

    void dumpConservativeRoots();
#endif

    js::Vector<JSObject *, 0, js::SystemAllocPolicy> arraysToSlowify;

  public:
    explicit GCMarker(JSContext *cx);
    ~GCMarker();

    uint32 getMarkColor() const {
        return color;
    }

    void setMarkColor(uint32 newColor) {
        



        markDelayedChildren();
        color = newColor;
    }

    void delayMarkingChildren(void *thing);

    JS_FRIEND_API(void) markDelayedChildren();

    void slowifyArrays();
};

} 

extern void
js_FinalizeStringRT(JSRuntime *rt, JSString *str);





extern void
js_MarkTraps(JSTracer *trc);

namespace js {








extern bool
SetProtoCheckingForCycles(JSContext *cx, JSObject *obj, JSObject *proto);


void
Mark(JSTracer *trc, void *thing, uint32 kind);

static inline void
Mark(JSTracer *trc, void *thing, uint32 kind, const char *name)
{
    JS_ASSERT(thing);
    JS_SET_TRACING_NAME(trc, name);
    Mark(trc, thing, kind);
}

static inline void
MarkString(JSTracer *trc, JSString *str)
{
    JS_ASSERT(str);
    Mark(trc, str, JSTRACE_STRING);
}

static inline void
MarkString(JSTracer *trc, JSString *str, const char *name)
{
    JS_ASSERT(str);
    JS_SET_TRACING_NAME(trc, name);
    Mark(trc, str, JSTRACE_STRING);
}

static inline void
MarkAtomRange(JSTracer *trc, size_t len, JSAtom **vec, const char *name)
{
    for (uint32 i = 0; i < len; i++) {
        if (JSAtom *atom = vec[i]) {
            JS_SET_TRACING_INDEX(trc, name, i);
            Mark(trc, ATOM_TO_STRING(atom), JSTRACE_STRING);
        }
    }
}

static inline void
MarkObject(JSTracer *trc, JSObject &obj, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    Mark(trc, &obj, JSTRACE_OBJECT);
}

static inline void
MarkObjectRange(JSTracer *trc, size_t len, JSObject **vec, const char *name)
{
    for (uint32 i = 0; i < len; i++) {
        if (JSObject *obj = vec[i]) {
            JS_SET_TRACING_INDEX(trc, name, i);
            Mark(trc, obj, JSTRACE_OBJECT);
        }
    }
}


static inline void
MarkValueRaw(JSTracer *trc, const js::Value &v)
{
    if (v.isMarkable())
        return Mark(trc, v.asGCThing(), v.gcKind());
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

void
MarkStackRangeConservatively(JSTracer *trc, Value *begin, Value *end);

static inline void
MarkId(JSTracer *trc, jsid id)
{
    if (JSID_IS_STRING(id))
        Mark(trc, JSID_TO_STRING(id), JSTRACE_STRING);
    else if (JS_UNLIKELY(JSID_IS_OBJECT(id)))
        Mark(trc, JSID_TO_OBJECT(id), JSTRACE_OBJECT);
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


void
MarkGCThing(JSTracer *trc, void *thing);

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

JSCompartment *
NewCompartment(JSContext *cx, JSPrincipals *principals);

} 

#endif
