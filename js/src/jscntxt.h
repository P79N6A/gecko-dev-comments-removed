







































#ifndef jscntxt_h___
#define jscntxt_h___



#include "jsarena.h" 
#include "jsclist.h"
#include "jslong.h"
#include "jsatom.h"
#include "jsconfig.h"
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
    JSScript        *script;
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
        (cache)->script = NULL;                                               \
        if ((cache)->table.ops) {                                             \
            JS_DHashTableFinish(&(cache)->table);                             \
            (cache)->table.ops = NULL;                                        \
        }                                                                     \
        GSN_CACHE_METER(cache, clears);                                       \
    JS_END_MACRO


#define JS_CLEAR_GSN_CACHE(cx)      GSN_CACHE_CLEAR(&JS_GSN_CACHE(cx))
#define JS_METER_GSN_CACHE(cx,cnt)  GSN_CACHE_METER(&JS_GSN_CACHE(cx), cnt)

#ifdef JS_THREADSAFE





struct JSThread {
    
    JSCList             contextList;

    
    jsword              id;

    
    JSGCThing           *gcFreeLists[GC_NUM_FREELISTS];

    



    uint32              gcMallocBytes;

    






    JSGSNCache          gsnCache;
};

#define JS_GSN_CACHE(cx) ((cx)->thread->gsnCache)

extern void JS_DLL_CALLBACK
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




typedef struct JSNativeIteratorState JSNativeIteratorState;

struct JSRuntime {
    
    JSRuntimeState      state;

    
    JSContextCallback   cxCallback;

    
    JSGCArenaList       gcArenaList[GC_NUM_FREELISTS];
    JSDHashTable        gcRootsHash;
    JSDHashTable        *gcLocksHash;
    jsrefcount          gcKeepAtoms;
    uint32              gcBytes;
    uint32              gcLastBytes;
    uint32              gcMaxBytes;
    uint32              gcMaxMallocBytes;
    uint32              gcLevel;
    uint32              gcNumber;
    JSTracer            *gcMarkingTracer;

    





    JSPackedBool        gcPoke;
    JSPackedBool        gcRunning;
#ifdef JS_GC_ZEAL
    uint8               gcZeal;
    uint8               gcPadding;
#else
    uint16              gcPadding;
#endif

    JSGCCallback        gcCallback;
    JSGCThingCallback   gcThingCallback;
    void                *gcThingCallbackClosure;
    uint32              gcMallocBytes;
    JSGCArena           *gcUnscannedArenaStackTop;
#ifdef DEBUG
    size_t              gcUnscannedBagSize;
#endif

    



    JSPtrTable          gcIteratorTable;

#ifdef JS_GCMETER
    JSGCStats           gcStats;
#endif

    



    JSTraceDataOp       gcExtraRootsTraceOp;
    void                *gcExtraRootsData;

    
    JSAtomState         atomState;

    
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

    
    PRLock              *setSlotLock;
    PRCondVar           *setSlotDone;
    JSBool              setSlotBusy;
    JSScope             *setSlotScope;  

    









    PRCondVar           *scopeSharingDone;
    JSScope             *scopeSharingTodo;








#define NO_SCOPE_SHARING_TODO   ((JSScope *) 0xfeedbeef)

    





    PRLock              *debuggerLock;
#endif 
    uint32              debuggerMutations;

    



    JSCheckAccessOp     checkObjectAccess;

    
    JSPrincipalsTranscoder principalsTranscoder;

    
    JSObjectPrincipalsFinder findObjectPrincipals;

    





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

    



    JSNativeIteratorState *nativeIteratorStates;

#ifndef JS_THREADSAFE
    





    JSGSNCache          gsnCache;

#define JS_GSN_CACHE(cx) ((cx)->runtime->gsnCache)
#endif

#ifdef DEBUG
    
    jsrefcount          inlineCalls;
    jsrefcount          nativeCalls;
    jsrefcount          nonInlineCalls;
    jsrefcount          constructs;

    
    jsrefcount          claimAttempts;
    jsrefcount          claimedScopes;
    jsrefcount          deadContexts;
    jsrefcount          deadlocksAvoided;
    jsrefcount          liveScopes;
    jsrefcount          sharedScopes;
    jsrefcount          totalScopes;
    jsrefcount          badUndependStrings;
    jsrefcount          liveScopeProps;
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
    double              lengthSum;
    double              lengthSquaredSum;
    double              strdepLengthSum;
    double              strdepLengthSquaredSum;
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



















#define JSTVU_SINGLE        (-1)
#define JSTVU_TRACE         (-2)
#define JSTVU_SPROP         (-3)
#define JSTVU_WEAK_ROOTS    (-4)
#define JSTVU_PARSE_CONTEXT (-5)





















JS_STATIC_ASSERT(sizeof(JSTempValueUnion) == sizeof(jsval));
JS_STATIC_ASSERT(sizeof(JSTempValueUnion) == sizeof(JSObject *));

#define JS_PUSH_TEMP_ROOT_COMMON(cx,tvr)                                      \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((cx)->tempValueRooters != (tvr));                           \
        (tvr)->down = (cx)->tempValueRooters;                                 \
        (cx)->tempValueRooters = (tvr);                                       \
    JS_END_MACRO

#define JS_PUSH_SINGLE_TEMP_ROOT(cx,val,tvr)                                  \
    JS_BEGIN_MACRO                                                            \
        (tvr)->count = JSTVU_SINGLE;                                          \
        (tvr)->u.value = val;                                                 \
        JS_PUSH_TEMP_ROOT_COMMON(cx, tvr);                                    \
    JS_END_MACRO

#define JS_PUSH_TEMP_ROOT(cx,cnt,arr,tvr)                                     \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((ptrdiff_t)(cnt) >= 0);                                     \
        (tvr)->count = (ptrdiff_t)(cnt);                                      \
        (tvr)->u.array = (arr);                                               \
        JS_PUSH_TEMP_ROOT_COMMON(cx, tvr);                                    \
    JS_END_MACRO

#define JS_PUSH_TEMP_ROOT_TRACE(cx,trace_,tvr)                                \
    JS_BEGIN_MACRO                                                            \
        (tvr)->count = JSTVU_TRACE;                                           \
        (tvr)->u.trace = (trace_);                                            \
        JS_PUSH_TEMP_ROOT_COMMON(cx, tvr);                                    \
    JS_END_MACRO

#define JS_PUSH_TEMP_ROOT_OBJECT(cx,obj,tvr)                                  \
    JS_BEGIN_MACRO                                                            \
        (tvr)->count = JSTVU_SINGLE;                                          \
        (tvr)->u.object = (obj);                                              \
        JS_PUSH_TEMP_ROOT_COMMON(cx, tvr);                                    \
    JS_END_MACRO

#define JS_PUSH_TEMP_ROOT_STRING(cx,str,tvr)                                  \
    JS_BEGIN_MACRO                                                            \
        (tvr)->count = JSTVU_SINGLE;                                          \
        (tvr)->u.string = (str);                                              \
        JS_PUSH_TEMP_ROOT_COMMON(cx, tvr);                                    \
    JS_END_MACRO

#define JS_PUSH_TEMP_ROOT_GCTHING(cx,thing,tvr)                               \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(JSVAL_IS_OBJECT((jsval)thing));                             \
        (tvr)->count = JSTVU_SINGLE;                                          \
        (tvr)->u.gcthing = (thing);                                           \
        JS_PUSH_TEMP_ROOT_COMMON(cx, tvr);                                    \
    JS_END_MACRO

#define JS_POP_TEMP_ROOT(cx,tvr)                                              \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((cx)->tempValueRooters == (tvr));                           \
        (cx)->tempValueRooters = (tvr)->down;                                 \
    JS_END_MACRO

#define JS_TEMP_ROOT_EVAL(cx,cnt,val,expr)                                    \
    JS_BEGIN_MACRO                                                            \
        JSTempValueRooter tvr;                                                \
        JS_PUSH_TEMP_ROOT(cx, cnt, val, &tvr);                                \
        (expr);                                                               \
        JS_POP_TEMP_ROOT(cx, &tvr);                                           \
    JS_END_MACRO

#define JS_PUSH_TEMP_ROOT_SPROP(cx,sprop_,tvr)                                \
    JS_BEGIN_MACRO                                                            \
        (tvr)->count = JSTVU_SPROP;                                           \
        (tvr)->u.sprop = (sprop_);                                            \
        JS_PUSH_TEMP_ROOT_COMMON(cx, tvr);                                    \
    JS_END_MACRO

#define JS_PUSH_TEMP_ROOT_WEAK_COPY(cx,weakRoots_,tvr)                        \
    JS_BEGIN_MACRO                                                            \
        (tvr)->count = JSTVU_WEAK_ROOTS;                                      \
        (tvr)->u.weakRoots = (weakRoots_);                                    \
        JS_PUSH_TEMP_ROOT_COMMON(cx, tvr);                                    \
    JS_END_MACRO

#define JS_PUSH_TEMP_ROOT_PARSE_CONTEXT(cx,pc,tvr)                            \
    JS_BEGIN_MACRO                                                            \
        (tvr)->count = JSTVU_PARSE_CONTEXT;                                   \
        (tvr)->u.parseContext = (pc);                                         \
        JS_PUSH_TEMP_ROOT_COMMON(cx, tvr);                                    \
    JS_END_MACRO

struct JSContext {
    
    JSCList             links;

    
    uint32              operationCounter;

#if JS_HAS_XML_SUPPORT
    





    uint8               xmlSettingFlags;
    uint8               padding;
#else
    uint16              padding;
#endif

    
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

    
    JSBranchCallback    branchCallback;
    JSErrorReporter     errorReporter;

    
    uintN               interpLevel;

    
    void                *data;

    
    JSStackFrame        *dormantFrameChain; 
#ifdef JS_THREADSAFE
    JSThread            *thread;
    jsrefcount          requestDepth;
    JSScope             *scopeToShare;      
    JSScope             *lockedSealedScope; 

    JSCList             threadLinks;        

#define CX_FROM_THREAD_LINKS(tl) \
    ((JSContext *)((char *)(tl) - offsetof(JSContext, threadLinks)))
#endif

    
    JSStackHeader       *stackHeaders;

    
    JSLocalRootStack    *localRootStack;

    
    JSTempValueRooter   *tempValueRooters;

    
    JSDebugHooks        *debugHooks;
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

  private:
    static void *operator new(size_t);
    static void operator delete(void *, size_t);

    JSContext *mContext;
    JSTempValueRooter mTvr;
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

#define JS_HAS_NATIVE_BRANCH_CALLBACK_OPTION(cx)                              \
    JS_HAS_OPTION(cx, JSOPTION_NATIVE_BRANCH_CALLBACK)






extern JSBool
js_InitThreadPrivateIndex(void (JS_DLL_CALLBACK *ptr)(void *));





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

extern const JSErrorFormatString *
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





extern JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *report);

extern void
js_ReportIsNotDefined(JSContext *cx, const char *name);






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


#define JSOW_JUMP                   1
#define JSOW_ALLOCATION             100
#define JSOW_LOOKUP_PROPERTY        5
#define JSOW_GET_PROPERTY           10
#define JSOW_SET_PROPERTY           20
#define JSOW_NEW_PROPERTY           200
#define JSOW_DELETE_PROPERTY        30

#define JSOW_BRANCH_CALLBACK        JS_BIT(12)






JS_STATIC_ASSERT((JSOW_BRANCH_CALLBACK & (JSOW_BRANCH_CALLBACK - 1)) == 0);





#define JS_COUNT_OPERATION(cx, weight)                                        \
    ((void)(JS_ASSERT((weight) > 0),                                          \
            JS_ASSERT((weight) <= JSOW_BRANCH_CALLBACK),                      \
            (cx)->operationCounter = (((cx)->operationCounter + (weight)) |   \
                                      (~(JSOW_BRANCH_CALLBACK - 1) &          \
                                       (cx)->operationCounter))))






#define JS_CHECK_OPERATION_LIMIT(cx, weight)                                  \
    (JS_COUNT_OPERATION(cx, weight),                                          \
     ((cx)->operationCounter < JSOW_BRANCH_CALLBACK ||                        \
     js_ResetOperationCounter(cx)))





extern JSBool
js_ResetOperationCounter(JSContext *cx);

JS_END_EXTERN_C

#endif 
