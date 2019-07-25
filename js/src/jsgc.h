






































#ifndef jsgc_h___
#define jsgc_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsdhash.h"
#include "jsbit.h"
#include "jsutil.h"
#include "jstask.h"
#include "jsversion.h"

#define JSTRACE_DOUBLE      2
#define JSTRACE_XML         3




#define JSTRACE_LIMIT       4

const uintN JS_EXTERNAL_STRING_LIMIT = 8;





extern intN
js_GetExternalStringGCType(JSString *str);

extern JS_FRIEND_API(uint32)
js_GetGCThingTraceKind(void *thing);





JSRuntime*
js_GetGCStringRuntime(JSString *str);

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
js_AddRootRT(JSRuntime *rt, js::Value *vp, const char *name);

extern JSBool
js_AddGCThingRoot(JSContext *cx, void **rp, const char *name);

extern JSBool
js_AddGCThingRootRT(JSRuntime *rt, void **rp, const char *name);

extern JSBool
js_RemoveRoot(JSRuntime *rt, void *rp);

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





extern jsdouble *
js_NewWeaklyRootedDoubleAtom(JSContext *cx, jsdouble d);

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
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) <= JSTRACE_DOUBLE)
#endif

extern void
js_TraceStackFrame(JSTracer *trc, JSStackFrame *fp);

extern JS_REQUIRES_STACK void
js_TraceRuntime(JSTracer *trc, JSBool allAtoms);

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

    




    GC_LOCK_HELD        = 0x10,
    GC_KEEP_ATOMS       = GC_LOCK_HELD,

    



    GC_SET_SLOT_REQUEST = GC_LOCK_HELD | 1,

    



    GC_LAST_DITCH       = GC_LOCK_HELD | 2
} JSGCInvocationKind;

extern void
js_GC(JSContext *cx, JSGCInvocationKind gckind);





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

struct JSGCDoubleArenaList {
    JSGCArena       *head;          
    JSGCArena       *cursor;        
};

struct JSGCFreeLists {
    JSGCThing       *doubles;
    JSGCThing       *finalizables[FINALIZE_LIMIT];

    void purge();
    void moveTo(JSGCFreeLists * another);

#ifdef DEBUG
    bool isEmpty() const {
        if (doubles)
            return false;
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
    jsdouble          *newbornDouble;

    
    JSAtom            *lastAtom;

    
    void              *lastInternalResult;

    void mark(JSTracer *trc);
};

#define JS_CLEAR_WEAK_ROOTS(wr) (memset((wr), 0, sizeof(JSWeakRoots)))

#ifdef JS_THREADSAFE
class JSFreePointerListTask : public JSBackgroundTask {
    void *head;
  public:
    JSFreePointerListTask() : head(NULL) {}

    void add(void* ptr) {
        *(void**)ptr = head;
        head = ptr;
    }

    void run() {
        void *ptr = head;
        while (ptr) {
            void *next = *(void **)ptr;
            js_free(ptr);
            ptr = next;
        }
    }
};
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
    uint32  maxlevel;   
    uint32  poke;       
    uint32  afree;      
    uint32  stackseg;   
    uint32  segslots;   
    uint32  nclose;     
    uint32  maxnclose;  
    uint32  closelater; 
    uint32  maxcloselater; 

    JSGCArenaStats  arenaStats[FINALIZE_LIMIT];
    JSGCArenaStats  doubleArenaStats;
};

extern JS_FRIEND_API(void)
js_DumpGCStats(JSRuntime *rt, FILE *fp);

#endif 





extern void
js_MarkTraps(JSTracer *trc);

namespace js {

void
CallGCMarker(JSTracer *trc, void *thing, uint32 kind);

static inline void
CallGCMarkerForGCThing(JSTracer *trc, const js::Value &v)
{
    CallGCMarker(trc, v.asGCThing(), v.traceKind());
}

void
CallGCMarkerForGCThing(JSTracer *trc, void *thing);

static inline void
CallGCMarkerForGCThing(JSTracer *trc, void *thing, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    CallGCMarkerForGCThing(trc, thing);
}

static inline void
CallGCMarkerIfGCThing(JSTracer *trc, const js::Value &v)
{
    if (v.isGCThing())
        CallGCMarkerForGCThing(trc, v);
}

static inline void
CallGCMarkerIfGCThing(JSTracer *trc, const js::Value &v, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    if (v.isGCThing())
        CallGCMarkerForGCThing(trc, v);
}

static inline void
CallGCMarkerIfGCThing(JSTracer *trc, jsid id)
{
    if (JSID_IS_GCTHING(id))
        CallGCMarker(trc, JSID_TO_GCTHING(id), JSID_TRACE_KIND(id));
}

static inline void
CallGCMarkerIfGCThing(JSTracer *trc, jsid id, const char *name)
{
    JS_SET_TRACING_NAME(trc, name);
    if (JSID_IS_GCTHING(id))
        CallGCMarker(trc, JSID_TO_GCTHING(id), JSID_TRACE_KIND(id));
}

void
TraceObjectVector(JSTracer *trc, JSObject **vec, uint32 len);

inline void
TraceValues(JSTracer *trc, Value *beg, Value *end, const char *name)
{
    for (Value *vp = beg; vp < end; ++vp) {
        if (vp->isGCThing()) {
            JS_SET_TRACING_INDEX(trc, name, vp - beg);
            CallGCMarkerForGCThing(trc, *vp);
        }
    }
}

inline void
TraceValues(JSTracer *trc, size_t len, Value *vec, const char *name)
{
    for (Value *vp = vec, *end = vp + len; vp < end; vp++) {
        if (vp->isGCThing()) {
            JS_SET_TRACING_INDEX(trc, name, vp - vec);
            CallGCMarkerForGCThing(trc, *vp);
        }
    }
}

inline void
TraceIds(JSTracer *trc, size_t len, jsid *vec, const char *name)
{
    for (jsid *idp = vec, *end = idp + len; idp < end; idp++) {
        if (JSID_IS_GCTHING(*idp)) {
            JS_SET_TRACING_INDEX(trc, name, idp - vec);
            CallGCMarker(trc, JSID_TO_GCTHING(*idp), JSID_TRACE_KIND(*idp));
        }
    }
}

}

#endif
