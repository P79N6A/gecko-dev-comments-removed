






































#ifndef jsgc_h___
#define jsgc_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsdhash.h"
#include "jsbit.h"
#include "jsutil.h"
#include "jstask.h"
#include "jsvector.h"
#include "jsversion.h"

#define JSTRACE_XML         2




#define JSTRACE_LIMIT       3

const uintN JS_EXTERNAL_STRING_LIMIT = 8;





extern intN
js_GetExternalStringGCType(JSString *str);

extern JS_FRIEND_API(uint32)
js_GetGCThingTraceKind(void *thing);





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

extern bool
js_IsAboutToBeFinalized(void *thing);





#define IS_GC_MARKING_TRACER(trc) ((trc)->callback == NULL)

#if JS_HAS_XML_SUPPORT
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) < JSTRACE_LIMIT)
#else
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) <= JSTRACE_STRING)
#endif

extern void
js_TraceStackFrame(JSTracer *trc, JSStackFrame *fp);

extern JS_REQUIRES_STACK void
js_TraceRuntime(JSTracer *trc);

extern JS_REQUIRES_STACK JS_FRIEND_API(void)
js_TraceContext(JSTracer *trc, JSContext *acx);




#ifndef JS_THREADSAFE
# define js_TriggerGC(cx, gcLocked)    js_TriggerGC (cx)
#endif

extern void
js_TriggerGC(JSContext *cx, JSBool gcLocked);




typedef enum JSGCInvocationKind {
    
    GC_NORMAL           = 0,

    



    GC_LAST_CONTEXT     = 1,

    


    GC_LOCK_HELD        = 0x10
} JSGCInvocationKind;

extern void
js_GC(JSContext *cx, JSGCInvocationKind gckind);









extern bool
js_SetProtoOrParentCheckingForCycles(JSContext *cx, JSObject *obj,
                                     uint32 slot, JSObject *pobj);

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
    return unsigned(FINALIZE_STRING) <= thingKind &&
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

static inline JSString *
js_NewGCExternalString(JSContext *cx, uintN type)
{
    JS_ASSERT(type < JS_EXTERNAL_STRING_LIMIT);
    type += FINALIZE_EXTERNAL_STRING0;
    return (JSString *) js_NewFinalizableGCThing(cx, type);
}

static inline JSFunction*
js_NewGCFunction(JSContext *cx)
{
    return (JSFunction *) js_NewFinalizableGCThing(cx, FINALIZE_FUNCTION);
}

#if JS_HAS_XML_SUPPORT
static inline JSXML *
js_NewGCXML(JSContext *cx)
{
    return (JSXML *) js_NewFinalizableGCThing(cx, FINALIZE_XML);
}
#endif

struct JSGCArena;
struct JSGCChunkInfo;

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

struct JSWeakRoots {
    
    void              *finalizableNewborns[FINALIZE_LIMIT];

    
    JSAtom            *lastAtom;

    
    void              *lastInternalResult;

    void mark(JSTracer *trc);
};

#define JS_CLEAR_WEAK_ROOTS(wr) (memset((wr), 0, sizeof(JSWeakRoots)))

#ifdef JS_THREADSAFE

namespace js {











class BackgroundSweepTask : public JSBackgroundTask {
    static const size_t FREE_ARRAY_SIZE = size_t(1) << 16;
    static const size_t FREE_ARRAY_LENGTH = FREE_ARRAY_SIZE / sizeof(void *);

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

  public:
    BackgroundSweepTask()
        : freeCursor(NULL), freeCursorEnd(NULL) { }

    void freeLater(void* ptr) {
        if (freeCursor != freeCursorEnd)
            *freeCursor++ = ptr;
        else
            replenishAndFreeLater(ptr);
    }

    virtual void run();
};

}
#endif

extern void
js_FinalizeStringRT(JSRuntime *rt, JSString *str);

#if defined JS_GCMETER
const bool JS_WANT_GC_METER_PRINT = true;
#elif defined DEBUG
# define JS_GCMETER 1
const bool JS_WANT_GC_METER_PRINT = false;
#endif

#ifdef JS_GCMETER

struct JSGCArenaStats {
    uint32  alloc;          
    uint32  localalloc;     
    uint32  retry;          
    uint32  fail;           
    uint32  nthings;        
    uint32  maxthings;      
    double  totalthings;    
    uint32  narenas;        
    uint32  newarenas;      
    uint32  livearenas;     
    uint32  maxarenas;      
    uint32  totalarenas;    

};

struct JSGCStats {
    uint32  finalfail;  
    uint32  lockborn;   
    uint32  lock;       
    uint32  unlock;     
    uint32  depth;      
    uint32  maxdepth;   
    uint32  cdepth;     
    uint32  maxcdepth;  
    uint32  unmarked;   

#ifdef DEBUG
    uint32  maxunmarked;

#endif
    uint32  poke;           
    uint32  afree;          
    uint32  stackseg;       
    uint32  segslots;       
    uint32  nclose;         
    uint32  maxnclose;      
    uint32  closelater;     
    uint32  maxcloselater;  
    uint32  nallarenas;     
    uint32  maxnallarenas;  
    uint32  nchunks;        
    uint32  maxnchunks;     

    JSGCArenaStats  arenaStats[FINALIZE_LIMIT];
};

extern JS_FRIEND_API(void)
js_DumpGCStats(JSRuntime *rt, FILE *fp);

#endif 





extern void
js_MarkTraps(JSTracer *trc);

namespace js {


void
MarkRaw(JSTracer *trc, void *thing, uint32 kind);

static inline void
Mark(JSTracer *trc, void *thing, uint32 kind, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkRaw(trc, thing, kind);
}

static inline void
MarkString(JSTracer *trc, JSString *str, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkRaw(trc, str, JSTRACE_STRING);
}

static inline void
MarkStringRange(JSTracer *trc, size_t len, JSString **vec, const char *name)
{
    for (uint32 i = 0; i < len; i++) {
        if (JSString *str = vec[i])
            MarkString(trc, str, name);
    }
}

static inline void
MarkAtomRange(JSTracer *trc, size_t len, JSAtom **vec, const char *name)
{
    MarkStringRange(trc, len, reinterpret_cast<JSString **>(vec), name);
}

static inline void
MarkObject(JSTracer *trc, JSObject *obj, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkRaw(trc, obj, JSTRACE_OBJECT);
}

static inline void
MarkObjectRange(JSTracer *trc, size_t len, JSObject **vec, const char *name)
{
    for (uint32 i = 0; i < len; i++) {
        if (JSObject *obj = vec[i])
            MarkObject(trc, obj, name);
    }
}


static inline void
MarkValueRaw(JSTracer *trc, const js::Value &v)
{
    if (v.isMarkable())
        return MarkRaw(trc, v.asGCThing(), v.gcKind());
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
MarkId(JSTracer *trc, jsid id, const char *name)
{
    if (JSID_IS_STRING(id))
        Mark(trc, JSID_TO_STRING(id), JSTRACE_STRING, name);
    else if (JS_UNLIKELY(JSID_IS_OBJECT(id)))
        Mark(trc, JSID_TO_OBJECT(id), JSTRACE_OBJECT, name);
}

static inline void
MarkIdRange(JSTracer *trc, jsid *beg, jsid *end, const char *name)
{
    for (jsid *idp = beg; idp != end; ++idp)
        MarkId(trc, *idp, name);
}

static inline void
MarkIdRange(JSTracer *trc, size_t len, jsid *vec, const char *name)
{
    MarkIdRange(trc, vec, vec + len, name);
}


void
MarkGCThingRaw(JSTracer *trc, void *thing);

static inline void
MarkGCThing(JSTracer *trc, void *thing, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    MarkGCThingRaw(trc, thing);
}

static inline void
MarkGCThing(JSTracer *trc, void *thing, const char *name, size_t index)
{
    JS_SET_TRACING_INDEX(trc, name, index);
    MarkGCThingRaw(trc, thing);
}

JSCompartment *
NewCompartment(JSContext *cx);

void
SweepCompartments(JSContext *cx);

} 

#endif
