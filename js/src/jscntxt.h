







































#ifndef jscntxt_h___
#define jscntxt_h___



#include "jsarena.h" 
#include "jsclist.h"
#include "jslong.h"
#include "jsatom.h"
#include "jsversion.h"
#include "jsdhash.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsobj.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsregexp.h"
#include "jsutil.h"

JS_BEGIN_EXTERN_C







typedef struct JSGSNCache {
    jsbytecode      *code;
    JSDHashTable    table;
#ifdef JS_GSNMETER
    uint32          hits;
    uint32          misses;
    uint32          fills;
    uint32          clears;
# define GSN_CACHE_METER(cache,cnt) (++(cache)->cnt)
#else
# define GSN_CACHE_METER(cache,cnt)
#endif
} JSGSNCache;

#define GSN_CACHE_CLEAR(cache)                                                \
    JS_BEGIN_MACRO                                                            \
        (cache)->code = NULL;                                                 \
        if ((cache)->table.ops) {                                             \
            JS_DHashTableFinish(&(cache)->table);                             \
            (cache)->table.ops = NULL;                                        \
        }                                                                     \
        GSN_CACHE_METER(cache, clears);                                       \
    JS_END_MACRO


#define JS_CLEAR_GSN_CACHE(cx)      GSN_CACHE_CLEAR(&JS_GSN_CACHE(cx))
#define JS_METER_GSN_CACHE(cx,cnt)  GSN_CACHE_METER(&JS_GSN_CACHE(cx), cnt)

#ifdef __cplusplus
namespace nanojit {
    class Fragment;
    class Fragmento;
}
class TraceRecorder;
extern "C++" { template<typename T> class Queue; }
typedef Queue<uint16> SlotList;
class TypeMap;

# define CLS(T)  T*
#else
# define CLS(T)  void*
#endif






typedef struct JSTraceMonitor {
    





    JSBool                  onTrace;
    CLS(nanojit::Fragmento) fragmento;
    CLS(TraceRecorder)      recorder;
    uint32                  globalShape;
    CLS(SlotList)           globalSlots;
    CLS(TypeMap)            globalTypeMap;
    jsval                   *recoveryDoublePool;
    jsval                   *recoveryDoublePoolPtr;
} JSTraceMonitor;

#ifdef JS_TRACER
# define JS_ON_TRACE(cx)   (JS_TRACE_MONITOR(cx).onTrace)
#else
# define JS_ON_TRACE(cx)   JS_FALSE
#endif

#ifdef JS_THREADSAFE





struct JSThread {
    
    JSCList             contextList;

    
    jsword              id;

    



    uint32              gcMallocBytes;

    






    JSGSNCache          gsnCache;

    
    JSPropertyCache     propertyCache;

    
    JSTraceMonitor      traceMonitor;

    
    JSScript            *scriptsToGC;
};

#define JS_GSN_CACHE(cx)        ((cx)->thread->gsnCache)
#define JS_PROPERTY_CACHE(cx)   ((cx)->thread->propertyCache)
#define JS_TRACE_MONITOR(cx)    ((cx)->thread->traceMonitor)
#define JS_SCRIPTS_TO_GC(cx)    ((cx)->thread->scriptsToGC)

extern void
js_ThreadDestructorCB(void *ptr);

extern JSBool
js_SetContextThread(JSContext *cx);

extern void
js_ClearContextThread(JSContext *cx);

extern JSThread *
js_GetCurrentThread(JSRuntime *rt);

#endif 

typedef enum JSDestroyContextMode {
    JSDCM_NO_GC,
    JSDCM_MAYBE_GC,
    JSDCM_FORCE_GC,
    JSDCM_NEW_FAILED
} JSDestroyContextMode;

typedef enum JSRuntimeState {
    JSRTS_DOWN,
    JSRTS_LAUNCHING,
    JSRTS_UP,
    JSRTS_LANDING
} JSRuntimeState;

typedef struct JSPropertyTreeEntry {
    JSDHashEntryHdr     hdr;
    JSScopeProperty     *child;
} JSPropertyTreeEntry;

typedef struct JSSetSlotRequest JSSetSlotRequest;

struct JSSetSlotRequest {
    JSObject            *obj;           
    JSObject            *pobj;          
    uint16              slot;           
    uint16              errnum;         
    JSSetSlotRequest    *next;          
};

struct JSRuntime {
    
    JSRuntimeState      state;

    
    JSContextCallback   cxCallback;

    
    JSGCChunkInfo       *gcChunkList;
    JSGCArenaList       gcArenaList[GC_NUM_FREELISTS];
    JSGCDoubleArenaList gcDoubleArenaList;
    JSGCFreeListSet     *gcFreeListsPool;
    JSDHashTable        gcRootsHash;
    JSDHashTable        *gcLocksHash;
    jsrefcount          gcKeepAtoms;
    uint32              gcBytes;
    uint32              gcLastBytes;
    uint32              gcMaxBytes;
    uint32              gcMaxMallocBytes;
    uint32              gcEmptyArenaPoolLifespan;
    uint32              gcLevel;
    uint32              gcNumber;
    JSTracer            *gcMarkingTracer;

    





    JSPackedBool        gcPoke;
    JSPackedBool        gcRunning;
    uint16              gcPadding;
#ifdef JS_GC_ZEAL
    jsrefcount          gcZeal;
#endif

    JSGCCallback        gcCallback;
    uint32              gcMallocBytes;
    JSGCArenaInfo       *gcUntracedArenaStackTop;
#ifdef DEBUG
    size_t              gcTraceLaterCount;
#endif

    



    JSPtrTable          gcIteratorTable;

    



    JSTraceDataOp       gcExtraRootsTraceOp;
    void                *gcExtraRootsData;

    





    JSSetSlotRequest    *setSlotRequests;

    
    JSBool              rngInitialized;
    int64               rngMultiplier;
    int64               rngAddend;
    int64               rngMask;
    int64               rngSeed;
    jsdouble            rngDscale;

    
    jsdouble            *jsNaN;
    jsdouble            *jsNegativeInfinity;
    jsdouble            *jsPositiveInfinity;

#ifdef JS_THREADSAFE
    JSLock              *deflatedStringCacheLock;
#endif
    JSHashTable         *deflatedStringCache;
#ifdef DEBUG
    uint32              deflatedStringCacheBytes;
#endif

    



    JSString            *emptyString;
    JSString            **unitStrings;

    
    JSCList             contextList;

    
    JSDebugHooks        globalDebugHooks;

    
    JSCList             trapList;
    JSCList             watchPointList;

    
    void                *data;

#ifdef JS_THREADSAFE
    
    PRLock              *gcLock;
    PRCondVar           *gcDone;
    PRCondVar           *requestDone;
    uint32              requestCount;
    JSThread            *gcThread;

    
    PRLock              *rtLock;
#ifdef DEBUG
    jsword              rtLockOwner;
#endif

    
    PRCondVar           *stateChange;

    









    PRCondVar           *titleSharingDone;
    JSTitle             *titleSharingTodo;








#define NO_TITLE_SHARING_TODO   ((JSTitle *) 0xfeedbeef)

    





    PRLock              *debuggerLock;
#endif 
    uint32              debuggerMutations;

    



    JSSecurityCallbacks *securityCallbacks;

    





    JSDHashTable        propertyTreeHash;
    JSScopeProperty     *propertyFreeList;
    JSArenaPool         propertyArenaPool;
    int32               propertyRemovals;

    
    struct JSHashTable  *scriptFilenameTable;
    JSCList             scriptFilenamePrefixes;
#ifdef JS_THREADSAFE
    PRLock              *scriptFilenameTableLock;
#endif

    
    const char          *thousandsSeparator;
    const char          *decimalSeparator;
    const char          *numGrouping;

    






    JSObject            *anynameObject;
    JSObject            *functionNamespaceObject;

    



    JSNativeEnumerator  *nativeEnumerators;

#ifndef JS_THREADSAFE
    





    JSGSNCache          gsnCache;

    
    JSPropertyCache     propertyCache;

    
    JSTraceMonitor      traceMonitor;

    
    JSScript            *scriptsToGC;

#define JS_GSN_CACHE(cx)        ((cx)->runtime->gsnCache)
#define JS_PROPERTY_CACHE(cx)   ((cx)->runtime->propertyCache)
#define JS_TRACE_MONITOR(cx)    ((cx)->runtime->traceMonitor)
#define JS_SCRIPTS_TO_GC(cx)    ((cx)->runtime->scriptsToGC)
#endif

    











    uint32              shapeGen;

    
    JSAtomState         atomState;

    




#define NATIVE_ENUM_CACHE_LOG2  8
#define NATIVE_ENUM_CACHE_MASK  JS_BITMASK(NATIVE_ENUM_CACHE_LOG2)
#define NATIVE_ENUM_CACHE_SIZE  JS_BIT(NATIVE_ENUM_CACHE_LOG2)

#define NATIVE_ENUM_CACHE_HASH(shape)                                         \
    ((((shape) >> NATIVE_ENUM_CACHE_LOG2) ^ (shape)) & NATIVE_ENUM_CACHE_MASK)

    jsuword             nativeEnumCache[NATIVE_ENUM_CACHE_SIZE];

    






    JSBool              anyArrayProtoHasElement;

    




#ifdef JS_DUMP_ENUM_CACHE_STATS
    int32               nativeEnumProbes;
    int32               nativeEnumMisses;
# define ENUM_CACHE_METER(name)     JS_ATOMIC_INCREMENT(&cx->runtime->name)
#else
# define ENUM_CACHE_METER(name)     ((void) 0)
#endif

#ifdef JS_DUMP_LOOP_STATS
    
    JSBasicStats        loopStats;
#endif

#if defined DEBUG || defined JS_DUMP_PROPTREE_STATS
    
    jsrefcount          inlineCalls;
    jsrefcount          nativeCalls;
    jsrefcount          nonInlineCalls;
    jsrefcount          constructs;

    
    jsrefcount          claimAttempts;
    jsrefcount          claimedTitles;
    jsrefcount          deadContexts;
    jsrefcount          deadlocksAvoided;
    jsrefcount          liveScopes;
    jsrefcount          sharedTitles;
    jsrefcount          totalScopes;
    jsrefcount          liveScopeProps;
    jsrefcount          liveScopePropsPreSweep;
    jsrefcount          totalScopeProps;
    jsrefcount          livePropTreeNodes;
    jsrefcount          duplicatePropTreeNodes;
    jsrefcount          totalPropTreeNodes;
    jsrefcount          propTreeKidsChunks;
    jsrefcount          middleDeleteFixups;

    
    jsrefcount          liveStrings;
    jsrefcount          totalStrings;
    jsrefcount          liveDependentStrings;
    jsrefcount          totalDependentStrings;
    jsrefcount          badUndependStrings;
    double              lengthSum;
    double              lengthSquaredSum;
    double              strdepLengthSum;
    double              strdepLengthSquaredSum;
#endif 

#ifdef JS_SCOPE_DEPTH_METER
    



    JSBasicStats        protoLookupDepthStats;
    JSBasicStats        scopeSearchDepthStats;

    



    JSBasicStats        hostenvScopeDepthStats;
    JSBasicStats        lexicalScopeDepthStats;
#endif

#ifdef JS_GCMETER
    JSGCStats           gcStats;
#endif
};

#ifdef DEBUG
# define JS_RUNTIME_METER(rt, which)    JS_ATOMIC_INCREMENT(&(rt)->which)
# define JS_RUNTIME_UNMETER(rt, which)  JS_ATOMIC_DECREMENT(&(rt)->which)
#else
# define JS_RUNTIME_METER(rt, which)
# define JS_RUNTIME_UNMETER(rt, which)
#endif

#define JS_KEEP_ATOMS(rt)   JS_ATOMIC_INCREMENT(&(rt)->gcKeepAtoms);
#define JS_UNKEEP_ATOMS(rt) JS_ATOMIC_DECREMENT(&(rt)->gcKeepAtoms);

#ifdef JS_ARGUMENT_FORMATTER_DEFINED





struct JSArgumentFormatMap {
    const char          *format;
    size_t              length;
    JSArgumentFormatter formatter;
    JSArgumentFormatMap *next;
};
#endif

struct JSStackHeader {
    uintN               nslots;
    JSStackHeader       *down;
};

#define JS_STACK_SEGMENT(sh)    ((jsval *)(sh) + 2)







typedef struct JSResolvingKey {
    JSObject            *obj;
    jsid                id;
} JSResolvingKey;

typedef struct JSResolvingEntry {
    JSDHashEntryHdr     hdr;
    JSResolvingKey      key;
    uint32              flags;
} JSResolvingEntry;

#define JSRESFLAG_LOOKUP        0x1     /* resolving id from lookup */
#define JSRESFLAG_WATCH         0x2     /* resolving id from watch */

typedef struct JSLocalRootChunk JSLocalRootChunk;

#define JSLRS_CHUNK_SHIFT       8
#define JSLRS_CHUNK_SIZE        JS_BIT(JSLRS_CHUNK_SHIFT)
#define JSLRS_CHUNK_MASK        JS_BITMASK(JSLRS_CHUNK_SHIFT)

struct JSLocalRootChunk {
    jsval               roots[JSLRS_CHUNK_SIZE];
    JSLocalRootChunk    *down;
};

typedef struct JSLocalRootStack {
    uint32              scopeMark;
    uint32              rootCount;
    JSLocalRootChunk    *topChunk;
    JSLocalRootChunk    firstChunk;
} JSLocalRootStack;

#define JSLRS_NULL_MARK ((uint32) -1)
















#define JSTVU_SINGLE        (-1)    /* u.value or u.<gcthing> is single jsval
                                       or GC-thing */
#define JSTVU_TRACE         (-2)    

#define JSTVU_SPROP         (-3)    /* u.sprop roots property tree node */
#define JSTVU_WEAK_ROOTS    (-4)    /* u.weakRoots points to saved weak roots */
#define JSTVU_PARSE_CONTEXT (-5)    /* u.parseContext roots JSParseContext* */
#define JSTVU_SCRIPT        (-6)    /* u.script roots JSScript* */













JS_STATIC_ASSERT(sizeof(JSTempValueUnion) == sizeof(jsval));
JS_STATIC_ASSERT(sizeof(JSTempValueUnion) == sizeof(void *));

#define JS_PUSH_TEMP_ROOT_COMMON(cx,x,tvr,cnt,kind)                           \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((cx)->tempValueRooters != (tvr));                           \
        (tvr)->count = (cnt);                                                 \
        (tvr)->u.kind = (x);                                                  \
        (tvr)->down = (cx)->tempValueRooters;                                 \
        (cx)->tempValueRooters = (tvr);                                       \
    JS_END_MACRO

#define JS_POP_TEMP_ROOT(cx,tvr)                                              \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((cx)->tempValueRooters == (tvr));                           \
        (cx)->tempValueRooters = (tvr)->down;                                 \
    JS_END_MACRO

#define JS_PUSH_TEMP_ROOT(cx,cnt,arr,tvr)                                     \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((int)(cnt) >= 0);                                           \
        JS_PUSH_TEMP_ROOT_COMMON(cx, arr, tvr, (ptrdiff_t) (cnt), array);     \
    JS_END_MACRO

#define JS_PUSH_SINGLE_TEMP_ROOT(cx,val,tvr)                                  \
    JS_PUSH_TEMP_ROOT_COMMON(cx, val, tvr, JSTVU_SINGLE, value)

#define JS_PUSH_TEMP_ROOT_OBJECT(cx,obj,tvr)                                  \
    JS_PUSH_TEMP_ROOT_COMMON(cx, obj, tvr, JSTVU_SINGLE, object)

#define JS_PUSH_TEMP_ROOT_STRING(cx,str,tvr)                                  \
    JS_PUSH_TEMP_ROOT_COMMON(cx, str, tvr, JSTVU_SINGLE, string)

#define JS_PUSH_TEMP_ROOT_XML(cx,xml_,tvr)                                    \
    JS_PUSH_TEMP_ROOT_COMMON(cx, xml_, tvr, JSTVU_SINGLE, xml)

#define JS_PUSH_TEMP_ROOT_TRACE(cx,trace_,tvr)                                \
    JS_PUSH_TEMP_ROOT_COMMON(cx, trace_, tvr, JSTVU_TRACE, trace)

#define JS_PUSH_TEMP_ROOT_SPROP(cx,sprop_,tvr)                                \
    JS_PUSH_TEMP_ROOT_COMMON(cx, sprop_, tvr, JSTVU_SPROP, sprop)

#define JS_PUSH_TEMP_ROOT_WEAK_COPY(cx,weakRoots_,tvr)                        \
    JS_PUSH_TEMP_ROOT_COMMON(cx, weakRoots_, tvr, JSTVU_WEAK_ROOTS, weakRoots)

#define JS_PUSH_TEMP_ROOT_PARSE_CONTEXT(cx,pc,tvr)                            \
    JS_PUSH_TEMP_ROOT_COMMON(cx, pc, tvr, JSTVU_PARSE_CONTEXT, parseContext)

#define JS_PUSH_TEMP_ROOT_SCRIPT(cx,script_,tvr)                              \
    JS_PUSH_TEMP_ROOT_COMMON(cx, script_, tvr, JSTVU_SCRIPT, script)


#define JSRESOLVE_INFER         0xffff  /* infer bits from current bytecode */

struct JSContext {
    
    JSCList             links;

    



    int32               operationCount;

#if JS_HAS_XML_SUPPORT
    





    uint8               xmlSettingFlags;
    uint8               padding;
#else
    uint16              padding;
#endif

    


#define JS_DISPLAY_SIZE 16

    JSStackFrame        *display[JS_DISPLAY_SIZE];

    
    uint16              version;

    
    uint32              options;            

    
    JSLocaleCallbacks   *localeCallbacks;

    





    JSDHashTable        *resolvingTable;

#if JS_HAS_LVALUE_RETURN
    





    jsval               rval2;
    JSPackedBool        rval2set;
#endif

    




    JSPackedBool        generatingError;

    
    JSPackedBool        insideGCMarkCallback;

    
    JSPackedBool        throwing;           
    jsval               exception;          

    
    jsuword             stackLimit;

    
    size_t              scriptStackQuota;

    
    JSRuntime           *runtime;

    
    JSArenaPool         stackPool;
    JSStackFrame        *fp;

    
    JSArenaPool         tempPool;

    
    JSObject            *globalObject;

    
    JSWeakRoots         weakRoots;

    
    JSRegExpStatics     regExpStatics;

    
    JSSharpObjectMap    sharpObjectMap;

    
    JSArgumentFormatMap *argumentFormatMap;

    
    char                *lastMessage;
#ifdef DEBUG
    void                *tracefp;
#endif

    
    JSErrorReporter     errorReporter;

    




    uint32              operationCallbackIsSet :    1;
    uint32              operationLimit         :    31;
    JSOperationCallback operationCallback;

    
    uintN               interpLevel;

    
    void                *data;
    void                *data2;

    
    JSStackFrame        *dormantFrameChain; 
#ifdef JS_THREADSAFE
    JSThread            *thread;
    jsrefcount          requestDepth;
    
    jsrefcount          outstandingRequests;
    JSTitle             *titleToShare;      
    JSTitle             *lockedSealedTitle; 

    JSCList             threadLinks;        

#define CX_FROM_THREAD_LINKS(tl) \
    ((JSContext *)((char *)(tl) - offsetof(JSContext, threadLinks)))
#endif

    
    JSStackHeader       *stackHeaders;

    
    JSLocalRootStack    *localRootStack;

    
    JSTempValueRooter   *tempValueRooters;

#ifdef JS_THREADSAFE
    JSGCFreeListSet     *gcLocalFreeLists;
#endif

    
    JSGCDoubleCell      *doubleFreeList;

    
    JSDebugHooks        *debugHooks;

    
    JSSecurityCallbacks *securityCallbacks;

    
    JSArenaPool         regexpPool;

    
    uintN               resolveFlags;
};

#ifdef JS_THREADSAFE
# define JS_THREAD_ID(cx)       ((cx)->thread ? (cx)->thread->id : 0)
#endif

#ifdef __cplusplus

class JSAutoTempValueRooter
{
  public:
    JSAutoTempValueRooter(JSContext *cx, size_t len, jsval *vec)
        : mContext(cx) {
        JS_PUSH_TEMP_ROOT(mContext, len, vec, &mTvr);
    }
    JSAutoTempValueRooter(JSContext *cx, jsval v)
        : mContext(cx) {
        JS_PUSH_SINGLE_TEMP_ROOT(mContext, v, &mTvr);
    }

    ~JSAutoTempValueRooter() {
        JS_POP_TEMP_ROOT(mContext, &mTvr);
    }

  protected:
    JSContext *mContext;

  private:
#ifndef AIX
    static void *operator new(size_t);
    static void operator delete(void *, size_t);
#endif

    JSTempValueRooter mTvr;
};

class JSAutoResolveFlags
{
  public:
    JSAutoResolveFlags(JSContext *cx, uintN flags)
        : mContext(cx), mSaved(cx->resolveFlags) {
        cx->resolveFlags = flags;
    }

    ~JSAutoResolveFlags() { mContext->resolveFlags = mSaved; }

  private:
    JSContext *mContext;
    uintN mSaved;
};
#endif




























#define JS_HAS_OPTION(cx,option)        (((cx)->options & (option)) != 0)
#define JS_HAS_STRICT_OPTION(cx)        JS_HAS_OPTION(cx, JSOPTION_STRICT)
#define JS_HAS_WERROR_OPTION(cx)        JS_HAS_OPTION(cx, JSOPTION_WERROR)
#define JS_HAS_COMPILE_N_GO_OPTION(cx)  JS_HAS_OPTION(cx, JSOPTION_COMPILE_N_GO)
#define JS_HAS_ATLINE_OPTION(cx)        JS_HAS_OPTION(cx, JSOPTION_ATLINE)

#define JSVERSION_MASK                  0x0FFF  /* see JSVersion in jspubtd.h */
#define JSVERSION_HAS_XML               0x1000  /* flag induced by XML option */

#define JSVERSION_NUMBER(cx)            ((JSVersion)((cx)->version &          \
                                                     JSVERSION_MASK))
#define JS_HAS_XML_OPTION(cx)           ((cx)->version & JSVERSION_HAS_XML || \
                                         JSVERSION_NUMBER(cx) >= JSVERSION_1_6)






extern JSBool
js_InitThreadPrivateIndex(void (*ptr)(void *));





extern void
js_OnVersionChange(JSContext *cx);





extern void
js_SetVersion(JSContext *cx, JSVersion version);





extern JSContext *
js_NewContext(JSRuntime *rt, size_t stackChunkSize);

extern void
js_DestroyContext(JSContext *cx, JSDestroyContextMode mode);





extern JSBool
js_ValidContextPointer(JSRuntime *rt, JSContext *cx);





extern JSContext *
js_ContextIterator(JSRuntime *rt, JSBool unlocked, JSContext **iterp);




extern JSBool
js_StartResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                  JSResolvingEntry **entryp);

extern void
js_StopResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                 JSResolvingEntry *entry, uint32 generation);









extern JSBool
js_EnterLocalRootScope(JSContext *cx);

#define js_LeaveLocalRootScope(cx) \
    js_LeaveLocalRootScopeWithResult(cx, JSVAL_NULL)

extern void
js_LeaveLocalRootScopeWithResult(JSContext *cx, jsval rval);

extern void
js_ForgetLocalRoot(JSContext *cx, jsval v);

extern int
js_PushLocalRoot(JSContext *cx, JSLocalRootStack *lrs, jsval v);

extern void
js_TraceLocalRoots(JSTracer *trc, JSLocalRootStack *lrs);





typedef enum JSErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "js.msg"
#undef MSG_DEF
    JSErr_Limit
} JSErrNum;

extern JS_FRIEND_API(const JSErrorFormatString *)
js_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber);

#ifdef va_start
extern JSBool
js_ReportErrorVA(JSContext *cx, uintN flags, const char *format, va_list ap);

extern JSBool
js_ReportErrorNumberVA(JSContext *cx, uintN flags, JSErrorCallback callback,
                       void *userRef, const uintN errorNumber,
                       JSBool charArgs, va_list ap);

extern JSBool
js_ExpandErrorArguments(JSContext *cx, JSErrorCallback callback,
                        void *userRef, const uintN errorNumber,
                        char **message, JSErrorReport *reportp,
                        JSBool *warningp, JSBool charArgs, va_list ap);
#endif

extern void
js_ReportOutOfMemory(JSContext *cx);




extern void
js_ReportOutOfScriptQuota(JSContext *cx);

extern void
js_ReportOverRecursed(JSContext *cx);

extern void
js_ReportAllocationOverflow(JSContext *cx);

#define JS_CHECK_RECURSION(cx, onerror)                                       \
    JS_BEGIN_MACRO                                                            \
        int stackDummy_;                                                      \
                                                                              \
        if (!JS_CHECK_STACK_SIZE(cx, stackDummy_)) {                          \
            js_ReportOverRecursed(cx);                                        \
            onerror;                                                          \
        }                                                                     \
    JS_END_MACRO





extern JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *report);

extern void
js_ReportIsNotDefined(JSContext *cx, const char *name);




extern JSBool
js_ReportIsNullOrUndefined(JSContext *cx, intN spindex, jsval v,
                           JSString *fallback);

extern void
js_ReportMissingArg(JSContext *cx, jsval *vp, uintN arg);






extern JSBool
js_ReportValueErrorFlags(JSContext *cx, uintN flags, const uintN errorNumber,
                         intN spindex, jsval v, JSString *fallback,
                         const char *arg1, const char *arg2);

#define js_ReportValueError(cx,errorNumber,spindex,v,fallback)                \
    ((void)js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,          \
                                    spindex, v, fallback, NULL, NULL))

#define js_ReportValueError2(cx,errorNumber,spindex,v,fallback,arg1)          \
    ((void)js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,          \
                                    spindex, v, fallback, arg1, NULL))

#define js_ReportValueError3(cx,errorNumber,spindex,v,fallback,arg1,arg2)     \
    ((void)js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,          \
                                    spindex, v, fallback, arg1, arg2))

extern JSErrorFormatString js_ErrorFormatString[JSErr_Limit];








#if JS_STACK_GROWTH_DIRECTION > 0
# define JS_CHECK_STACK_SIZE(cx, lval)  ((jsuword)&(lval) < (cx)->stackLimit)
#else
# define JS_CHECK_STACK_SIZE(cx, lval)  ((jsuword)&(lval) > (cx)->stackLimit)
#endif










#define JS_CHECK_OPERATION_LIMIT(cx, weight)                                  \
    (JS_CHECK_OPERATION_WEIGHT(weight),                                       \
     (((cx)->operationCount -= (weight)) > 0 || js_ResetOperationCount(cx)))







#define JS_COUNT_OPERATION(cx, weight)                                        \
    ((void)(JS_CHECK_OPERATION_WEIGHT(weight),                                \
            (cx)->operationCount = ((cx)->operationCount > 0)                 \
                                   ? (cx)->operationCount - (weight)          \
                                   : 0))





#define JS_CHECK_OPERATION_WEIGHT(weight)                                     \
    (JS_ASSERT((uint32) (weight) > 0),                                        \
     JS_ASSERT((uint32) (weight) < JS_BIT(30)))


#define JSOW_JUMP                   1
#define JSOW_ALLOCATION             100
#define JSOW_LOOKUP_PROPERTY        5
#define JSOW_GET_PROPERTY           10
#define JSOW_SET_PROPERTY           20
#define JSOW_NEW_PROPERTY           200
#define JSOW_DELETE_PROPERTY        30
#define JSOW_ENTER_SHARP            JS_OPERATION_WEIGHT_BASE
#define JSOW_SCRIPT_JUMP            JS_OPERATION_WEIGHT_BASE





extern JSBool
js_ResetOperationCount(JSContext *cx);

JS_END_EXTERN_C

#endif 
