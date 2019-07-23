






































#ifndef jsgc_h___
#define jsgc_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsdhash.h"
#include "jsbit.h"
#include "jsutil.h"

JS_BEGIN_EXTERN_C

JS_STATIC_ASSERT(JSTRACE_STRING == 2);

#define JSTRACE_FUNCTION    3
#define JSTRACE_NAMESPACE   4
#define JSTRACE_QNAME       5
#define JSTRACE_XML         6




#define JSTRACE_LIMIT       7





#define GCX_OBJECT              JSTRACE_OBJECT      /* JSObject */
#define GCX_DOUBLE              JSTRACE_DOUBLE      /* jsdouble */
#define GCX_STRING              JSTRACE_STRING      /* JSString */
#define GCX_FUNCTION            JSTRACE_FUNCTION    /* JSFunction */
#define GCX_NAMESPACE           JSTRACE_NAMESPACE   /* JSXMLNamespace */
#define GCX_QNAME               JSTRACE_QNAME       /* JSXMLQName */
#define GCX_XML                 JSTRACE_XML         /* JSXML */
#define GCX_EXTERNAL_STRING     JSTRACE_LIMIT       /* JSString with external
                                                       chars */



#define GCX_NTYPES              (GCX_EXTERNAL_STRING + 8)




#define GCX_LIMIT_LOG2         4           /* type index bits */
#define GCX_LIMIT              JS_BIT(GCX_LIMIT_LOG2)

JS_STATIC_ASSERT(GCX_NTYPES <= GCX_LIMIT);


#define GCF_TYPEMASK    JS_BITMASK(GCX_LIMIT_LOG2)
#define GCF_MARK        JS_BIT(GCX_LIMIT_LOG2)
#define GCF_FINAL       JS_BIT(GCX_LIMIT_LOG2 + 1)
#define GCF_LOCKSHIFT   (GCX_LIMIT_LOG2 + 2)   /* lock bit shift */
#define GCF_LOCK        JS_BIT(GCF_LOCKSHIFT)   /* lock request bit in API */





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











#define GC_WRITE_BARRIER(cx,scope,oldval,newval)                              \
    JS_BEGIN_MACRO                                                            \
        if (SCOPE_IS_BRANDED(scope) &&                                        \
            (oldval) != (newval) &&                                           \
            (VALUE_IS_FUNCTION(cx,oldval) || VALUE_IS_FUNCTION(cx,newval))) { \
            SCOPE_MAKE_UNIQUE_SHAPE(cx, scope);                               \
        }                                                                     \
        GC_POKE(cx, oldval);                                                  \
    JS_END_MACRO

extern JSBool
js_InitGC(JSRuntime *rt, uint32 maxbytes);

extern void
js_FinishGC(JSRuntime *rt);

extern intN
js_ChangeExternalStringFinalizer(JSStringFinalizeOp oldop,
                                 JSStringFinalizeOp newop);

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
js_NewDoubleInRootedValue(JSContext *cx, jsdouble d, jsval *vp);





extern jsdouble *
js_NewWeaklyRootedDouble(JSContext *cx, jsdouble d);

extern JSBool
js_LockGCThingRT(JSRuntime *rt, void *thing);

extern JSBool
js_UnlockGCThingRT(JSRuntime *rt, void *thing);

extern JSBool
js_IsAboutToBeFinalized(JSContext *cx, void *thing);





#define IS_GC_MARKING_TRACER(trc) ((trc)->callback == NULL)

#if JS_HAS_XML_SUPPORT
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) <= JSTRACE_XML)
#else
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) <= JSTRACE_FUNCTION)
#endif





JS_STATIC_ASSERT(JSTRACE_FUNCTION + 1 == JSTRACE_NAMESPACE);






extern void
js_CallValueTracerIfGCThing(JSTracer *trc, jsval v);

extern void
js_TraceStackFrame(JSTracer *trc, JSStackFrame *fp);

extern void
js_TraceRuntime(JSTracer *trc, JSBool allAtoms);

extern JS_FRIEND_API(void)
js_TraceContext(JSTracer *trc, JSContext *acx);




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


extern void
js_UpdateMallocCounter(JSContext *cx, size_t nbytes);

typedef struct JSGCArenaInfo JSGCArenaInfo;
typedef struct JSGCArenaList JSGCArenaList;
typedef struct JSGCChunkInfo JSGCChunkInfo;

struct JSGCArenaList {
    JSGCArenaInfo   *last;          
    uint16          lastCount;      

    uint16          thingSize;      

    JSGCThing       *freeList;      
};

typedef union JSGCDoubleCell JSGCDoubleCell;

union JSGCDoubleCell {
    double          number;
    JSGCDoubleCell  *link;
};

JS_STATIC_ASSERT(sizeof(JSGCDoubleCell) == sizeof(double));

typedef struct JSGCDoubleArenaList {
    JSGCArenaInfo   *first;             
    uint8           *nextDoubleFlags;   

} JSGCDoubleArenaList;

struct JSWeakRoots {
    
    void            *newborn[GCX_NTYPES];

    
    jsval           lastAtom;

    
    jsval           lastInternalResult;
};

JS_STATIC_ASSERT(JSVAL_NULL == 0);
#define JS_CLEAR_WEAK_ROOTS(wr) (memset((wr), 0, sizeof(JSWeakRoots)))

#ifdef DEBUG_notme
#define JS_GCMETER 1
#endif

#ifdef JS_GCMETER

typedef struct JSGCArenaStats {
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

} JSGCArenaStats;

typedef struct JSGCStats {
    uint32  finalfail;  
    uint32  lockborn;   
    uint32  lock;       
    uint32  unlock;     
    uint32  depth;      
    uint32  maxdepth;   
    uint32  cdepth;     
    uint32  maxcdepth;  
    uint32  untraced;   

#ifdef DEBUG
    uint32  maxuntraced;

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

    JSGCArenaStats  arenaStats[GC_NUM_FREELISTS];
    JSGCArenaStats  doubleArenaStats;
} JSGCStats;

extern JS_FRIEND_API(void)
js_DumpGCStats(JSRuntime *rt, FILE *fp);

#endif 

JS_END_EXTERN_C

#endif 
