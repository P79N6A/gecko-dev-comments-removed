






































#ifndef jsgc_h___
#define jsgc_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsdhash.h"
#include "jsutil.h"

JS_BEGIN_EXTERN_C


#define GCX_OBJECT              0               /* JSObject */
#define GCX_STRING              1               /* JSString */
#define GCX_DOUBLE              2               /* jsdouble */
#define GCX_MUTABLE_STRING      3               /* JSString that's mutable --
                                                   single-threaded only! */
#define GCX_PRIVATE             4               
#define GCX_NAMESPACE           5               /* JSXMLNamespace */
#define GCX_QNAME               6               /* JSXMLQName */
#define GCX_XML                 7               /* JSXML */
#define GCX_EXTERNAL_STRING     8               /* JSString w/ external chars */

#define GCX_NTYPES_LOG2         4               /* type index bits */
#define GCX_NTYPES              JS_BIT(GCX_NTYPES_LOG2)


#define GCF_TYPEMASK    JS_BITMASK(GCX_NTYPES_LOG2)
#define GCF_MARK        JS_BIT(GCX_NTYPES_LOG2)
#define GCF_FINAL       JS_BIT(GCX_NTYPES_LOG2 + 1)
#define GCF_SYSTEM      JS_BIT(GCX_NTYPES_LOG2 + 2)
#define GCF_LOCKSHIFT   (GCX_NTYPES_LOG2 + 3)   /* lock bit shift */
#define GCF_LOCK        JS_BIT(GCF_LOCKSHIFT)   /* lock request bit in API */


#define GCF_MUTABLE     2

#if (GCX_STRING | GCF_MUTABLE) != GCX_MUTABLE_STRING
# error "mutable string type index botch!"
#endif

extern uint8 *
js_GetGCThingFlags(void *thing);





JSRuntime*
js_GetGCStringRuntime(JSString *str);

#if 1





#define GC_POKE(cx, oldval) ((cx)->runtime->gcPoke = JS_TRUE)
#else
#define GC_POKE(cx, oldval) ((cx)->runtime->gcPoke = JSVAL_IS_GCTHING(oldval))
#endif

extern intN
js_ChangeExternalStringFinalizer(JSStringFinalizeOp oldop,
                                 JSStringFinalizeOp newop);

extern JSBool
js_InitGC(JSRuntime *rt, uint32 maxbytes);

extern void
js_FinishGC(JSRuntime *rt);

extern JSBool
js_AddRoot(JSContext *cx, void *rp, const char *name);

extern JSBool
js_AddRootRT(JSRuntime *rt, void *rp, const char *name);

extern JSBool
js_RemoveRoot(JSRuntime *rt, void *rp);

#ifdef DEBUG
extern void
js_DumpNamedRoots(JSRuntime *rt,
                  void (*dump)(const char *name, void *rp, void *data),
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

#if JS_HAS_GENERATORS




typedef struct JSGCCloseState {
    



    JSGenerator         *reachableList;

    



    JSGenerator         *todoQueue;

#ifndef JS_THREADSAFE
    



    JSBool              runningCloseHook;
#endif
} JSGCCloseState;

extern void
js_RegisterGenerator(JSContext *cx, JSGenerator *gen);

extern JSBool
js_RunCloseHooks(JSContext *cx);

#endif




struct JSGCThing {
    JSGCThing   *next;
    uint8       *flagp;
};

#define GC_NBYTES_MAX           (10 * sizeof(JSGCThing))
#define GC_NUM_FREELISTS        (GC_NBYTES_MAX / sizeof(JSGCThing))
#define GC_FREELIST_NBYTES(i)   (((i) + 1) * sizeof(JSGCThing))
#define GC_FREELIST_INDEX(n)    (((n) / sizeof(JSGCThing)) - 1)

extern void *
js_NewGCThing(JSContext *cx, uintN flags, size_t nbytes);

extern JSBool
js_LockGCThing(JSContext *cx, void *thing);

extern JSBool
js_LockGCThingRT(JSRuntime *rt, void *thing);

extern JSBool
js_UnlockGCThingRT(JSRuntime *rt, void *thing);

extern JSBool
js_IsAboutToBeFinalized(JSContext *cx, void *thing);





#define IS_GC_MARKING_TRACER(trc) ((trc)->callback == NULL)

#ifdef DEBUG

extern JS_FRIEND_API(JSTracer *)
js_NewGCHeapDumper(JSContext *cx, void *thingToFind, FILE *fp,
                   size_t maxRecursionDepth, void *thingToIgnore);

extern JS_FRIEND_API(JSBool)
js_FreeGCHeapDumper(JSTracer *trc);

#endif

JS_STATIC_ASSERT(JSTRACE_STRING == 2);

#define JSTRACE_FUNCTION    3
#define JSTRACE_ATOM        4
#define JSTRACE_NAMESPACE   5
#define JSTRACE_QNAME       6
#define JSTRACE_XML         7

#if JS_HAS_XML_SUPPORT
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) <= JSTRACE_XML)
#else
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) <= JSTRACE_ATOM)
#endif

extern void
js_CallGCThingTracer(JSTracer *trc, void *thing);

extern void
js_TraceStackFrame(JSTracer *trc, JSStackFrame *fp);

extern JS_FRIEND_API(void)
js_TraceRuntime(JSTracer *trc);

extern JS_FRIEND_API(void)
js_TraceContext(JSTracer *trc, JSContext *acx);




typedef enum JSGCInvocationKind {
    
    GC_NORMAL,

    



    GC_LAST_CONTEXT,

    



    GC_LAST_DITCH
} JSGCInvocationKind;

extern void
js_GC(JSContext *cx, JSGCInvocationKind gckind);


extern void
js_UpdateMallocCounter(JSContext *cx, size_t nbytes);

#ifdef DEBUG_notme
#define JS_GCMETER 1
#endif

#ifdef JS_GCMETER

typedef struct JSGCStats {
#ifdef JS_THREADSAFE
    uint32  localalloc; 
#endif
    uint32  alloc;      
    uint32  retry;      
    uint32  retryhalt;  
    uint32  fail;       
    uint32  finalfail;  
    uint32  lockborn;   
    uint32  lock;       
    uint32  unlock;     
    uint32  depth;      
    uint32  maxdepth;   
    uint32  cdepth;     
    uint32  maxcdepth;  
    uint32  unscanned;  

#ifdef DEBUG
    uint32  maxunscanned;       
#endif
    uint32  maxlevel;   
    uint32  poke;       
    uint32  nopoke;     
    uint32  afree;      
    uint32  stackseg;   
    uint32  segslots;   
    uint32  nclose;     
    uint32  maxnclose;  
    uint32  closelater; 
    uint32  maxcloselater; 
} JSGCStats;

extern JS_FRIEND_API(void)
js_DumpGCStats(JSRuntime *rt, FILE *fp);

#endif 

typedef struct JSGCArena JSGCArena;
typedef struct JSGCArenaList JSGCArenaList;

#ifdef JS_GCMETER
typedef struct JSGCArenaStats JSGCArenaStats;

struct JSGCArenaStats {
    uint32  narenas;        
    uint32  maxarenas;      
    uint32  nthings;        
    uint32  maxthings;      
    uint32  totalnew;       
    uint32  freelen;        
    uint32  recycle;        
    uint32  totalarenas;    

    uint32  totalfreelen;   

};
#endif

struct JSGCArenaList {
    JSGCArena   *last;          
    uint16      lastLimit;      

    uint16      thingSize;      
    JSGCThing   *freeList;      
#ifdef JS_GCMETER
    JSGCArenaStats stats;
#endif
};

typedef struct JSWeakRoots {
    
    JSGCThing           *newborn[GCX_NTYPES];

    
    JSAtom              *lastAtom;

    
    jsval               lastInternalResult;
} JSWeakRoots;

JS_STATIC_ASSERT(JSVAL_NULL == 0);
#define JS_CLEAR_WEAK_ROOTS(wr) (memset((wr), 0, sizeof(JSWeakRoots)))


#ifdef DEBUG_notme
#define TOO_MUCH_GC 1
#endif

#ifdef WAY_TOO_MUCH_GC
#define TOO_MUCH_GC 1
#endif

JS_END_EXTERN_C

#endif 
