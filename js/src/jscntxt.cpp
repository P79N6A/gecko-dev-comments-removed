










































#include "jsstddef.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsarena.h" 
#include "jsutil.h" 
#include "jsclist.h"
#include "jsprf.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsdbgapi.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jspubtd.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jsstr.h"
#include "jstracer.h"

#ifdef JS_THREADSAFE
#include "prtypes.h"






static PRUintn threadTPIndex;
static JSBool  tpIndexInited = JS_FALSE;

static void
InitOperationLimit(JSContext *cx);

JS_BEGIN_EXTERN_C
JSBool
js_InitThreadPrivateIndex(void (*ptr)(void *))
{
    PRStatus status;

    if (tpIndexInited)
        return JS_TRUE;

    status = PR_NewThreadPrivateIndex(&threadTPIndex, ptr);

    if (status == PR_SUCCESS)
        tpIndexInited = JS_TRUE;
    return status == PR_SUCCESS;
}
JS_END_EXTERN_C

JS_BEGIN_EXTERN_C
JSBool
js_CleanupThreadPrivateData()
{
    if (!tpIndexInited)
        return JS_TRUE;
    return PR_SetThreadPrivate(threadTPIndex, NULL) == PR_SUCCESS;
}
JS_END_EXTERN_C





void
js_ThreadDestructorCB(void *ptr)
{
    JSThread *thread = (JSThread *)ptr;

    if (!thread)
        return;

    



    JS_ASSERT(JS_CLIST_IS_EMPTY(&thread->contextList));
    GSN_CACHE_CLEAR(&thread->gsnCache);
#if defined JS_TRACER
    js_FinishJIT(&thread->traceMonitor);
#endif
    free(thread);
}












JSThread *
js_GetCurrentThread(JSRuntime *rt)
{
    JSThread *thread;

    thread = (JSThread *)PR_GetThreadPrivate(threadTPIndex);
    if (!thread) {
        thread = (JSThread *) malloc(sizeof(JSThread));
        if (!thread)
            return NULL;
#ifdef DEBUG
        memset(thread, JS_FREE_PATTERN, sizeof(JSThread));
#endif
        if (PR_FAILURE == PR_SetThreadPrivate(threadTPIndex, thread)) {
            free(thread);
            return NULL;
        }

        JS_INIT_CLIST(&thread->contextList);
        thread->id = js_CurrentThreadId();
        thread->gcMallocBytes = 0;
#ifdef JS_TRACER
        memset(&thread->traceMonitor, 0, sizeof(thread->traceMonitor));
        js_InitJIT(&thread->traceMonitor);
#endif
        memset(thread->scriptsToGC, 0, sizeof thread->scriptsToGC);

        


    }
    return thread;
}





void
js_InitContextThread(JSContext *cx, JSThread *thread)
{
    JS_ASSERT(CURRENT_THREAD_IS_ME(thread));
    JS_ASSERT(!cx->thread);
    JS_ASSERT(cx->requestDepth == 0);

    



    if (JS_CLIST_IS_EMPTY(&thread->contextList)) {
        memset(&thread->gsnCache, 0, sizeof thread->gsnCache);
        memset(&thread->propertyCache, 0, sizeof thread->propertyCache);
#ifdef DEBUG
        memset(&thread->evalCacheMeter, 0, sizeof thread->evalCacheMeter);
#endif
    }

    JS_APPEND_LINK(&cx->threadLinks, &thread->contextList);
    cx->thread = thread;
}

#endif 







void
js_SyncOptionsToVersion(JSContext* cx)
{
    if (cx->options & JSOPTION_XML)
        cx->version |= JSVERSION_HAS_XML;
    else
        cx->version &= ~JSVERSION_HAS_XML;
    if (cx->options & JSOPTION_ANONFUNFIX)
        cx->version |= JSVERSION_ANONFUNFIX;
    else
        cx->version &= ~JSVERSION_ANONFUNFIX;
}

inline void
js_SyncVersionToOptions(JSContext* cx)
{
    if (cx->version & JSVERSION_HAS_XML)
        cx->options |= JSOPTION_XML;
    else
        cx->options &= ~JSOPTION_XML;
    if (cx->version & JSVERSION_ANONFUNFIX)
        cx->options |= JSOPTION_ANONFUNFIX;
    else
        cx->options &= ~JSOPTION_ANONFUNFIX;
}

void
js_OnVersionChange(JSContext *cx)
{
#ifdef DEBUG
    JSVersion version = JSVERSION_NUMBER(cx);

    JS_ASSERT(version == JSVERSION_DEFAULT || version >= JSVERSION_ECMA_3);
#endif
}

void
js_SetVersion(JSContext *cx, JSVersion version)
{
    cx->version = version;
    js_SyncVersionToOptions(cx);
    js_OnVersionChange(cx);
}

JSContext *
js_NewContext(JSRuntime *rt, size_t stackChunkSize)
{
    JSContext *cx;
    JSBool ok, first;
    JSContextCallback cxCallback;
#ifdef JS_THREADSAFE
    JSThread *thread = js_GetCurrentThread(rt);

    if (!thread)
        return NULL;
#endif

    




    cx = (JSContext *) calloc(1, sizeof *cx);
    if (!cx)
        return NULL;

    cx->runtime = rt;
    js_InitOperationLimit(cx);
    cx->debugHooks = &rt->globalDebugHooks;
#if JS_STACK_GROWTH_DIRECTION > 0
    cx->stackLimit = (jsuword) -1;
#endif
    cx->scriptStackQuota = JS_DEFAULT_SCRIPT_STACK_QUOTA;
#ifdef JS_THREADSAFE
    cx->gcLocalFreeLists = (JSGCFreeListSet *) &js_GCEmptyFreeListSet;
    js_InitContextThread(cx, thread);
#endif
    JS_STATIC_ASSERT(JSVERSION_DEFAULT == 0);
    JS_ASSERT(cx->version == JSVERSION_DEFAULT);
    VOUCH_DOES_NOT_REQUIRE_STACK();
    JS_INIT_ARENA_POOL(&cx->stackPool, "stack", stackChunkSize, sizeof(jsval),
                       &cx->scriptStackQuota);

    JS_INIT_ARENA_POOL(&cx->tempPool, "temp",
                       1024,  
                       sizeof(jsdouble), &cx->scriptStackQuota);

    js_InitRegExpStatics(cx);
    JS_ASSERT(cx->resolveFlags == 0);

    JS_LOCK_GC(rt);
    for (;;) {
        first = (rt->contextList.next == &rt->contextList);
        if (rt->state == JSRTS_UP) {
            JS_ASSERT(!first);

            
            js_WaitForGC(rt);
            break;
        }
        if (rt->state == JSRTS_DOWN) {
            JS_ASSERT(first);
            rt->state = JSRTS_LAUNCHING;
            break;
        }
        JS_WAIT_CONDVAR(rt->stateChange, JS_NO_TIMEOUT);
    }
    JS_APPEND_LINK(&cx->link, &rt->contextList);
    JS_UNLOCK_GC(rt);

    







    if (first) {
#ifdef JS_THREADSAFE
        JS_BeginRequest(cx);
#endif
        ok = js_InitCommonAtoms(cx);

        




        if (ok && !rt->scriptFilenameTable)
            ok = js_InitRuntimeScriptState(rt);
        if (ok)
            ok = js_InitRuntimeNumberState(cx);
        if (ok)
            ok = js_InitRuntimeStringState(cx);
#ifdef JS_THREADSAFE
        JS_EndRequest(cx);
#endif
        if (!ok) {
            js_DestroyContext(cx, JSDCM_NEW_FAILED);
            return NULL;
        }

        JS_LOCK_GC(rt);
        rt->state = JSRTS_UP;
        JS_NOTIFY_ALL_CONDVAR(rt->stateChange);
        JS_UNLOCK_GC(rt);
    }

    cxCallback = rt->cxCallback;
    if (cxCallback && !cxCallback(cx, JSCONTEXT_NEW)) {
        js_DestroyContext(cx, JSDCM_NEW_FAILED);
        return NULL;
    }

    return cx;
}

#if defined DEBUG && defined XP_UNIX
# include <stdio.h>

class AutoFile {
public:
    AutoFile() : mFp(NULL) {}

    ~AutoFile() {
        if (mFp)
            fclose(mFp);
    }

    FILE *open(const char *fname, const char *mode) {
        return mFp = fopen(fname, mode);
    }
    operator FILE *() {
        return mFp;
    }

private:
    FILE *mFp;
};

static void
DumpEvalCacheMeter(JSContext *cx)
{
    struct {
        const char *name;
        ptrdiff_t  offset;
    } table[] = {
#define frob(x) { #x, offsetof(JSEvalCacheMeter, x) }
        EVAL_CACHE_METER_LIST(frob)
#undef frob
    };
    JSEvalCacheMeter *ecm = &JS_CACHE_LOCUS(cx)->evalCacheMeter;

    static AutoFile fp;
    if (!fp) {
        fp.open("/tmp/evalcache.stats", "w");
        if (!fp)
            return;
    }

    fprintf(fp, "eval cache meter (%p):\n",
#ifdef JS_THREADSAFE
            (void *) cx->thread
#else
            (void *) cx->runtime
#endif
            );
    for (uintN i = 0; i < JS_ARRAY_LENGTH(table); ++i) {
        fprintf(fp, "%-8.8s  %llu\n",
                table[i].name, *(uint64 *)((uint8 *)ecm + table[i].offset));
    }
    fprintf(fp, "hit ratio %g%%\n", ecm->hit * 100. / ecm->probe);
    fprintf(fp, "avg steps %g\n", double(ecm->step) / ecm->probe);
    fflush(fp);
}
# define DUMP_EVAL_CACHE_METER(cx) DumpEvalCacheMeter(cx)
#else
# define DUMP_EVAL_CACHE_METER(cx) ((void) 0)
#endif

void
js_DestroyContext(JSContext *cx, JSDestroyContextMode mode)
{
    JSRuntime *rt;
    JSContextCallback cxCallback;
    JSBool last;
    JSArgumentFormatMap *map;
    JSLocalRootStack *lrs;
    JSLocalRootChunk *lrc;

#ifdef JS_THREADSAFE
    JS_ASSERT(CURRENT_THREAD_IS_ME(cx->thread));
#endif
    rt = cx->runtime;

    if (mode != JSDCM_NEW_FAILED) {
        cxCallback = rt->cxCallback;
        if (cxCallback) {
            



#ifdef DEBUG
            JSBool callbackStatus =
#endif
            cxCallback(cx, JSCONTEXT_DESTROY);
            JS_ASSERT(callbackStatus);
        }
    }

    JS_LOCK_GC(rt);
    JS_ASSERT(rt->state == JSRTS_UP || rt->state == JSRTS_LAUNCHING);
#ifdef JS_THREADSAFE
    



    if (cx->requestDepth == 0)
        js_WaitForGC(rt);
    js_RevokeGCLocalFreeLists(cx);
#endif
    JS_REMOVE_LINK(&cx->link);
    last = (rt->contextList.next == &rt->contextList);
    if (last)
        rt->state = JSRTS_LANDING;
    JS_UNLOCK_GC(rt);

    if (last) {
#ifdef JS_THREADSAFE
        









        if (cx->requestDepth == 0)
            JS_BeginRequest(cx);
#endif

        
        js_FinishRuntimeNumberState(cx);
        js_FinishRuntimeStringState(cx);

        
        js_FinishCommonAtoms(cx);

        
        JS_ClearAllTraps(cx);
        JS_ClearAllWatchPoints(cx);
    }

    
    JS_ClearRegExpRoots(cx);

#ifdef JS_THREADSAFE
    






    while (cx->requestDepth != 0)
        JS_EndRequest(cx);
#endif

    if (last) {
        js_GC(cx, GC_LAST_CONTEXT);
        DUMP_EVAL_CACHE_METER(cx);

        



        if (rt->scriptFilenameTable && rt->scriptFilenameTable->nentries == 0)
            js_FinishRuntimeScriptState(rt);

        
        JS_LOCK_GC(rt);
        rt->state = JSRTS_DOWN;
        JS_NOTIFY_ALL_CONDVAR(rt->stateChange);
        JS_UNLOCK_GC(rt);
    } else {
        if (mode == JSDCM_FORCE_GC)
            js_GC(cx, GC_NORMAL);
        else if (mode == JSDCM_MAYBE_GC)
            JS_MaybeGC(cx);
    }

    
    js_FreeRegExpStatics(cx);
    VOUCH_DOES_NOT_REQUIRE_STACK();
    JS_FinishArenaPool(&cx->stackPool);
    JS_FinishArenaPool(&cx->tempPool);

    if (cx->lastMessage)
        free(cx->lastMessage);

    
    map = cx->argumentFormatMap;
    while (map) {
        JSArgumentFormatMap *temp = map;
        map = map->next;
        JS_free(cx, temp);
    }

    
    if (cx->resolvingTable) {
        JS_DHashTableDestroy(cx->resolvingTable);
        cx->resolvingTable = NULL;
    }

    lrs = cx->localRootStack;
    if (lrs) {
        while ((lrc = lrs->topChunk) != &lrs->firstChunk) {
            lrs->topChunk = lrc->down;
            JS_free(cx, lrc);
        }
        JS_free(cx, lrs);
    }

#ifdef JS_THREADSAFE
    





    JS_REMOVE_LINK(&cx->threadLinks);
#endif

    
    free(cx);
}

JSBool
js_ValidContextPointer(JSRuntime *rt, JSContext *cx)
{
    JSCList *cl;

    for (cl = rt->contextList.next; cl != &rt->contextList; cl = cl->next) {
        if (cl == &cx->link)
            return JS_TRUE;
    }
    JS_RUNTIME_METER(rt, deadContexts);
    return JS_FALSE;
}

JSContext *
js_ContextIterator(JSRuntime *rt, JSBool unlocked, JSContext **iterp)
{
    JSContext *cx = *iterp;

    if (unlocked)
        JS_LOCK_GC(rt);
    cx = js_ContextFromLinkField(cx ? cx->link.next : rt->contextList.next);
    if (&cx->link == &rt->contextList)
        cx = NULL;
    *iterp = cx;
    if (unlocked)
        JS_UNLOCK_GC(rt);
    return cx;
}

static JSDHashNumber
resolving_HashKey(JSDHashTable *table, const void *ptr)
{
    const JSResolvingKey *key = (const JSResolvingKey *)ptr;

    return ((JSDHashNumber)JS_PTR_TO_UINT32(key->obj) >> JSVAL_TAGBITS) ^ key->id;
}

JS_PUBLIC_API(JSBool)
resolving_MatchEntry(JSDHashTable *table,
                     const JSDHashEntryHdr *hdr,
                     const void *ptr)
{
    const JSResolvingEntry *entry = (const JSResolvingEntry *)hdr;
    const JSResolvingKey *key = (const JSResolvingKey *)ptr;

    return entry->key.obj == key->obj && entry->key.id == key->id;
}

static const JSDHashTableOps resolving_dhash_ops = {
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    resolving_HashKey,
    resolving_MatchEntry,
    JS_DHashMoveEntryStub,
    JS_DHashClearEntryStub,
    JS_DHashFinalizeStub,
    NULL
};

JSBool
js_StartResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                  JSResolvingEntry **entryp)
{
    JSDHashTable *table;
    JSResolvingEntry *entry;

    table = cx->resolvingTable;
    if (!table) {
        table = JS_NewDHashTable(&resolving_dhash_ops, NULL,
                                 sizeof(JSResolvingEntry),
                                 JS_DHASH_MIN_SIZE);
        if (!table)
            goto outofmem;
        cx->resolvingTable = table;
    }

    entry = (JSResolvingEntry *)
            JS_DHashTableOperate(table, key, JS_DHASH_ADD);
    if (!entry)
        goto outofmem;

    if (entry->flags & flag) {
        
        entry = NULL;
    } else {
        
        if (!entry->key.obj)
            entry->key = *key;
        entry->flags |= flag;
    }
    *entryp = entry;
    return JS_TRUE;

outofmem:
    JS_ReportOutOfMemory(cx);
    return JS_FALSE;
}

void
js_StopResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                 JSResolvingEntry *entry, uint32 generation)
{
    JSDHashTable *table;

    




    table = cx->resolvingTable;
    if (!entry || table->generation != generation) {
        entry = (JSResolvingEntry *)
                JS_DHashTableOperate(table, key, JS_DHASH_LOOKUP);
    }
    JS_ASSERT(JS_DHASH_ENTRY_IS_BUSY(&entry->hdr));
    entry->flags &= ~flag;
    if (entry->flags)
        return;

    





    if (table->removedCount < JS_DHASH_TABLE_SIZE(table) >> 2)
        JS_DHashTableRawRemove(table, &entry->hdr);
    else
        JS_DHashTableOperate(table, key, JS_DHASH_REMOVE);
}

JSBool
js_EnterLocalRootScope(JSContext *cx)
{
    JSLocalRootStack *lrs;
    int mark;

    lrs = cx->localRootStack;
    if (!lrs) {
        lrs = (JSLocalRootStack *) JS_malloc(cx, sizeof *lrs);
        if (!lrs)
            return JS_FALSE;
        lrs->scopeMark = JSLRS_NULL_MARK;
        lrs->rootCount = 0;
        lrs->topChunk = &lrs->firstChunk;
        lrs->firstChunk.down = NULL;
        cx->localRootStack = lrs;
    }

    
    mark = js_PushLocalRoot(cx, lrs, INT_TO_JSVAL(lrs->scopeMark));
    if (mark < 0)
        return JS_FALSE;
    lrs->scopeMark = (uint32) mark;
    return JS_TRUE;
}

void
js_LeaveLocalRootScopeWithResult(JSContext *cx, jsval rval)
{
    JSLocalRootStack *lrs;
    uint32 mark, m, n;
    JSLocalRootChunk *lrc;

    
    lrs = cx->localRootStack;
    JS_ASSERT(lrs && lrs->rootCount != 0);
    if (!lrs || lrs->rootCount == 0)
        return;

    mark = lrs->scopeMark;
    JS_ASSERT(mark != JSLRS_NULL_MARK);
    if (mark == JSLRS_NULL_MARK)
        return;

    
    m = mark >> JSLRS_CHUNK_SHIFT;
    n = (lrs->rootCount - 1) >> JSLRS_CHUNK_SHIFT;
    while (n > m) {
        lrc = lrs->topChunk;
        JS_ASSERT(lrc != &lrs->firstChunk);
        lrs->topChunk = lrc->down;
        JS_free(cx, lrc);
        --n;
    }

    





    lrc = lrs->topChunk;
    m = mark & JSLRS_CHUNK_MASK;
    lrs->scopeMark = (uint32) JSVAL_TO_INT(lrc->roots[m]);
    if (JSVAL_IS_GCTHING(rval) && !JSVAL_IS_NULL(rval)) {
        if (mark == 0) {
            cx->weakRoots.lastInternalResult = rval;
        } else {
            





            lrc->roots[m++] = rval;
            ++mark;
        }
    }
    lrs->rootCount = (uint32) mark;

    








    if (mark == 0) {
        cx->localRootStack = NULL;
        JS_free(cx, lrs);
    } else if (m == 0) {
        lrs->topChunk = lrc->down;
        JS_free(cx, lrc);
    }
}

void
js_ForgetLocalRoot(JSContext *cx, jsval v)
{
    JSLocalRootStack *lrs;
    uint32 i, j, m, n, mark;
    JSLocalRootChunk *lrc, *lrc2;
    jsval top;

    lrs = cx->localRootStack;
    JS_ASSERT(lrs && lrs->rootCount);
    if (!lrs || lrs->rootCount == 0)
        return;

    
    n = lrs->rootCount - 1;
    m = n & JSLRS_CHUNK_MASK;
    lrc = lrs->topChunk;
    top = lrc->roots[m];

    
    mark = lrs->scopeMark;
    JS_ASSERT(mark < n);
    if (mark >= n)
        return;

    
    if (top != v) {
        
        i = n;
        j = m;
        lrc2 = lrc;
        while (--i > mark) {
            if (j == 0)
                lrc2 = lrc2->down;
            j = i & JSLRS_CHUNK_MASK;
            if (lrc2->roots[j] == v)
                break;
        }

        
        JS_ASSERT(i != mark);
        if (i == mark)
            return;

        
        lrc2->roots[j] = top;
    }

    
    lrc->roots[m] = JSVAL_NULL;
    lrs->rootCount = n;
    if (m == 0) {
        JS_ASSERT(n != 0);
        JS_ASSERT(lrc != &lrs->firstChunk);
        lrs->topChunk = lrc->down;
        JS_free(cx, lrc);
    }
}

int
js_PushLocalRoot(JSContext *cx, JSLocalRootStack *lrs, jsval v)
{
    uint32 n, m;
    JSLocalRootChunk *lrc;

    n = lrs->rootCount;
    m = n & JSLRS_CHUNK_MASK;
    if (n == 0 || m != 0) {
        



        if ((uint32)(n + 1) == 0) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_TOO_MANY_LOCAL_ROOTS);
            return -1;
        }
        lrc = lrs->topChunk;
        JS_ASSERT(n != 0 || lrc == &lrs->firstChunk);
    } else {
        



        lrc = (JSLocalRootChunk *) JS_malloc(cx, sizeof *lrc);
        if (!lrc)
            return -1;
        lrc->down = lrs->topChunk;
        lrs->topChunk = lrc;
    }
    lrs->rootCount = n + 1;
    lrc->roots[m] = v;
    return (int) n;
}

void
js_TraceLocalRoots(JSTracer *trc, JSLocalRootStack *lrs)
{
    uint32 n, m, mark;
    JSLocalRootChunk *lrc;
    jsval v;

    n = lrs->rootCount;
    if (n == 0)
        return;

    mark = lrs->scopeMark;
    lrc = lrs->topChunk;
    do {
        while (--n > mark) {
            m = n & JSLRS_CHUNK_MASK;
            v = lrc->roots[m];
            JS_ASSERT(JSVAL_IS_GCTHING(v) && v != JSVAL_NULL);
            JS_SET_TRACING_INDEX(trc, "local_root", n);
            js_CallValueTracerIfGCThing(trc, v);
            if (m == 0)
                lrc = lrc->down;
        }
        m = n & JSLRS_CHUNK_MASK;
        mark = JSVAL_TO_INT(lrc->roots[m]);
        if (m == 0)
            lrc = lrc->down;
    } while (n != 0);
    JS_ASSERT(!lrc);
}

static void
ReportError(JSContext *cx, const char *message, JSErrorReport *reportp)
{
    





    JS_ASSERT(reportp);
    if (reportp->errorNumber == JSMSG_UNCAUGHT_EXCEPTION)
        reportp->flags |= JSREPORT_EXCEPTION;

    







    if (!JS_IsRunning(cx) || !js_ErrorToException(cx, message, reportp)) {
        js_ReportErrorAgain(cx, message, reportp);
    } else if (cx->debugHooks->debugErrorHook && cx->errorReporter) {
        JSDebugErrorHook hook = cx->debugHooks->debugErrorHook;
        
        if (hook)
            hook(cx, message, reportp, cx->debugHooks->debugErrorHookData);
    }
}


static void
PopulateReportBlame(JSContext *cx, JSErrorReport *report)
{
    JSStackFrame *fp;

    



    for (fp = js_GetTopStackFrame(cx); fp; fp = fp->down) {
        if (fp->regs) {
            report->filename = fp->script->filename;
            report->lineno = js_FramePCToLineNumber(cx, fp);
            break;
        }
    }
}








void
js_ReportOutOfMemory(JSContext *cx)
{
    JSErrorReport report;
    JSErrorReporter onError = cx->errorReporter;

    
    const JSErrorFormatString *efs =
        js_GetLocalizedErrorMessage(cx, NULL, NULL, JSMSG_OUT_OF_MEMORY);
    const char *msg = efs ? efs->format : "Out of memory";

    
    memset(&report, 0, sizeof (struct JSErrorReport));
    report.flags = JSREPORT_ERROR;
    report.errorNumber = JSMSG_OUT_OF_MEMORY;
    PopulateReportBlame(cx, &report);

    





    cx->throwing = JS_FALSE;
    if (onError) {
        JSDebugErrorHook hook = cx->debugHooks->debugErrorHook;
        if (hook &&
            !hook(cx, msg, &report, cx->debugHooks->debugErrorHookData)) {
            onError = NULL;
        }
    }

    if (onError)
        onError(cx, msg, &report);
}

void
js_ReportOutOfScriptQuota(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_SCRIPT_STACK_QUOTA);
}

void
js_ReportOverRecursed(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
}

void
js_ReportAllocationOverflow(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_ALLOC_OVERFLOW);
}

JSBool
js_ReportErrorVA(JSContext *cx, uintN flags, const char *format, va_list ap)
{
    char *message;
    jschar *ucmessage;
    size_t messagelen;
    JSErrorReport report;
    JSBool warning;

    if ((flags & JSREPORT_STRICT) && !JS_HAS_STRICT_OPTION(cx))
        return JS_TRUE;

    message = JS_vsmprintf(format, ap);
    if (!message)
        return JS_FALSE;
    messagelen = strlen(message);

    memset(&report, 0, sizeof (struct JSErrorReport));
    report.flags = flags;
    report.errorNumber = JSMSG_USER_DEFINED_ERROR;
    report.ucmessage = ucmessage = js_InflateString(cx, message, &messagelen);
    PopulateReportBlame(cx, &report);

    warning = JSREPORT_IS_WARNING(report.flags);
    if (warning && JS_HAS_WERROR_OPTION(cx)) {
        report.flags &= ~JSREPORT_WARNING;
        warning = JS_FALSE;
    }

    ReportError(cx, message, &report);
    free(message);
    JS_free(cx, ucmessage);
    return warning;
}












JSBool
js_ExpandErrorArguments(JSContext *cx, JSErrorCallback callback,
                        void *userRef, const uintN errorNumber,
                        char **messagep, JSErrorReport *reportp,
                        JSBool *warningp, JSBool charArgs, va_list ap)
{
    const JSErrorFormatString *efs;
    int i;
    int argCount;

    *warningp = JSREPORT_IS_WARNING(reportp->flags);
    if (*warningp && JS_HAS_WERROR_OPTION(cx)) {
        reportp->flags &= ~JSREPORT_WARNING;
        *warningp = JS_FALSE;
    }

    *messagep = NULL;

    
    if (!callback || callback == js_GetErrorMessage)
        efs = js_GetLocalizedErrorMessage(cx, userRef, NULL, errorNumber);
    else
        efs = callback(userRef, NULL, errorNumber);
    if (efs) {
        size_t totalArgsLength = 0;
        size_t argLengths[10]; 
        argCount = efs->argCount;
        JS_ASSERT(argCount <= 10);
        if (argCount > 0) {
            





            reportp->messageArgs = (const jschar **)
                JS_malloc(cx, sizeof(jschar *) * (argCount + 1));
            if (!reportp->messageArgs)
                return JS_FALSE;
            reportp->messageArgs[argCount] = NULL;
            for (i = 0; i < argCount; i++) {
                if (charArgs) {
                    char *charArg = va_arg(ap, char *);
                    size_t charArgLength = strlen(charArg);
                    reportp->messageArgs[i]
                        = js_InflateString(cx, charArg, &charArgLength);
                    if (!reportp->messageArgs[i])
                        goto error;
                } else {
                    reportp->messageArgs[i] = va_arg(ap, jschar *);
                }
                argLengths[i] = js_strlen(reportp->messageArgs[i]);
                totalArgsLength += argLengths[i];
            }
            
            reportp->messageArgs[i] = NULL;
        }
        



        if (argCount > 0) {
            if (efs->format) {
                jschar *buffer, *fmt, *out;
                int expandedArgs = 0;
                size_t expandedLength;
                size_t len = strlen(efs->format);

                buffer = fmt = js_InflateString (cx, efs->format, &len);
                if (!buffer)
                    goto error;
                expandedLength = len
                                 - (3 * argCount)       
                                 + totalArgsLength;

                



                reportp->ucmessage = out = (jschar *)
                    JS_malloc(cx, (expandedLength + 1) * sizeof(jschar));
                if (!out) {
                    JS_free (cx, buffer);
                    goto error;
                }
                while (*fmt) {
                    if (*fmt == '{') {
                        if (isdigit(fmt[1])) {
                            int d = JS7_UNDEC(fmt[1]);
                            JS_ASSERT(d < argCount);
                            js_strncpy(out, reportp->messageArgs[d],
                                       argLengths[d]);
                            out += argLengths[d];
                            fmt += 3;
                            expandedArgs++;
                            continue;
                        }
                    }
                    *out++ = *fmt++;
                }
                JS_ASSERT(expandedArgs == argCount);
                *out = 0;
                JS_free (cx, buffer);
                *messagep =
                    js_DeflateString(cx, reportp->ucmessage,
                                     (size_t)(out - reportp->ucmessage));
                if (!*messagep)
                    goto error;
            }
        } else {
            



            if (efs->format) {
                size_t len;
                *messagep = JS_strdup(cx, efs->format);
                if (!*messagep)
                    goto error;
                len = strlen(*messagep);
                reportp->ucmessage = js_InflateString(cx, *messagep, &len);
                if (!reportp->ucmessage)
                    goto error;
            }
        }
    }
    if (*messagep == NULL) {
        
        const char *defaultErrorMessage
            = "No error message available for error number %d";
        size_t nbytes = strlen(defaultErrorMessage) + 16;
        *messagep = (char *)JS_malloc(cx, nbytes);
        if (!*messagep)
            goto error;
        JS_snprintf(*messagep, nbytes, defaultErrorMessage, errorNumber);
    }
    return JS_TRUE;

error:
    if (reportp->messageArgs) {
        
        if (charArgs) {
            i = 0;
            while (reportp->messageArgs[i])
                JS_free(cx, (void *)reportp->messageArgs[i++]);
        }
        JS_free(cx, (void *)reportp->messageArgs);
        reportp->messageArgs = NULL;
    }
    if (reportp->ucmessage) {
        JS_free(cx, (void *)reportp->ucmessage);
        reportp->ucmessage = NULL;
    }
    if (*messagep) {
        JS_free(cx, (void *)*messagep);
        *messagep = NULL;
    }
    return JS_FALSE;
}

JSBool
js_ReportErrorNumberVA(JSContext *cx, uintN flags, JSErrorCallback callback,
                       void *userRef, const uintN errorNumber,
                       JSBool charArgs, va_list ap)
{
    JSErrorReport report;
    char *message;
    JSBool warning;

    if ((flags & JSREPORT_STRICT) && !JS_HAS_STRICT_OPTION(cx))
        return JS_TRUE;

    memset(&report, 0, sizeof (struct JSErrorReport));
    report.flags = flags;
    report.errorNumber = errorNumber;
    PopulateReportBlame(cx, &report);

    if (!js_ExpandErrorArguments(cx, callback, userRef, errorNumber,
                                 &message, &report, &warning, charArgs, ap)) {
        return JS_FALSE;
    }

    ReportError(cx, message, &report);

    if (message)
        JS_free(cx, message);
    if (report.messageArgs) {
        



        if (charArgs) {
            int i = 0;
            while (report.messageArgs[i])
                JS_free(cx, (void *)report.messageArgs[i++]);
        }
        JS_free(cx, (void *)report.messageArgs);
    }
    if (report.ucmessage)
        JS_free(cx, (void *)report.ucmessage);

    return warning;
}

JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *reportp)
{
    JSErrorReporter onError;

    if (!message)
        return;

    if (cx->lastMessage)
        free(cx->lastMessage);
    cx->lastMessage = JS_strdup(cx, message);
    if (!cx->lastMessage)
        return;
    onError = cx->errorReporter;

    



    if (onError) {
        JSDebugErrorHook hook = cx->debugHooks->debugErrorHook;
        if (hook &&
            !hook(cx, cx->lastMessage, reportp,
                  cx->debugHooks->debugErrorHookData)) {
            onError = NULL;
        }
    }
    if (onError)
        onError(cx, cx->lastMessage, reportp);
}

void
js_ReportIsNotDefined(JSContext *cx, const char *name)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_DEFINED, name);
}

JSBool
js_ReportIsNullOrUndefined(JSContext *cx, intN spindex, jsval v,
                           JSString *fallback)
{
    char *bytes;
    JSBool ok;

    bytes = js_DecompileValueGenerator(cx, spindex, v, fallback);
    if (!bytes)
        return JS_FALSE;

    if (strcmp(bytes, js_undefined_str) == 0 ||
        strcmp(bytes, js_null_str) == 0) {
        ok = JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                          js_GetErrorMessage, NULL,
                                          JSMSG_NO_PROPERTIES, bytes,
                                          NULL, NULL);
    } else if (JSVAL_IS_VOID(v)) {
        ok = JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                          js_GetErrorMessage, NULL,
                                          JSMSG_NULL_OR_UNDEFINED, bytes,
                                          js_undefined_str, NULL);
    } else {
        JS_ASSERT(JSVAL_IS_NULL(v));
        ok = JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                          js_GetErrorMessage, NULL,
                                          JSMSG_NULL_OR_UNDEFINED, bytes,
                                          js_null_str, NULL);
    }

    JS_free(cx, bytes);
    return ok;
}

void
js_ReportMissingArg(JSContext *cx, jsval *vp, uintN arg)
{
    char argbuf[11];
    char *bytes;
    JSAtom *atom;

    JS_snprintf(argbuf, sizeof argbuf, "%u", arg);
    bytes = NULL;
    if (VALUE_IS_FUNCTION(cx, *vp)) {
        atom = GET_FUNCTION_PRIVATE(cx, JSVAL_TO_OBJECT(*vp))->atom;
        bytes = js_DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, *vp,
                                           ATOM_TO_STRING(atom));
        if (!bytes)
            return;
    }
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_MISSING_FUN_ARG, argbuf,
                         bytes ? bytes : "");
    JS_free(cx, bytes);
}

JSBool
js_ReportValueErrorFlags(JSContext *cx, uintN flags, const uintN errorNumber,
                         intN spindex, jsval v, JSString *fallback,
                         const char *arg1, const char *arg2)
{
    char *bytes;
    JSBool ok;

    JS_ASSERT(js_ErrorFormatString[errorNumber].argCount >= 1);
    JS_ASSERT(js_ErrorFormatString[errorNumber].argCount <= 3);
    bytes = js_DecompileValueGenerator(cx, spindex, v, fallback);
    if (!bytes)
        return JS_FALSE;

    ok = JS_ReportErrorFlagsAndNumber(cx, flags, js_GetErrorMessage,
                                      NULL, errorNumber, bytes, arg1, arg2);
    JS_free(cx, bytes);
    return ok;
}

#if defined DEBUG && defined XP_UNIX

void js_traceon(JSContext *cx)  { cx->tracefp = stderr; }
void js_traceoff(JSContext *cx) { cx->tracefp = NULL; }
#endif

JSErrorFormatString js_ErrorFormatString[JSErr_Limit] = {
#define MSG_DEF(name, number, count, exception, format) \
    { format, count, exception } ,
#include "js.msg"
#undef MSG_DEF
};

JS_FRIEND_API(const JSErrorFormatString *)
js_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber)
{
    if ((errorNumber > 0) && (errorNumber < JSErr_Limit))
        return &js_ErrorFormatString[errorNumber];
    return NULL;
}

JSBool
js_ResetOperationCount(JSContext *cx)
{
    JSScript *script;
    JSStackFrame *fp;

    JS_ASSERT(cx->operationCount <= 0);
    JS_ASSERT(cx->operationLimit > 0);

    cx->operationCount = (int32) cx->operationLimit;
    JSOperationCallback cb = cx->operationCallback;
    if (cb) {
        if (!cx->branchCallbackWasSet)
            return cb(cx);

        




        fp = js_GetTopStackFrame(cx);
        script = fp ? fp->script : NULL;
        if (script || JS_HAS_OPTION(cx, JSOPTION_NATIVE_BRANCH_CALLBACK))
            return ((JSBranchCallback) cb)(cx, script);
    }
    return JS_TRUE;
}

#ifndef JS_TRACER

extern JS_FORCES_STACK JSStackFrame *
js_GetTopStackFrame(JSContext *cx)
{
    return cx->fp;
}
#endif

JSStackFrame *
js_GetScriptedCaller(JSContext *cx, JSStackFrame *fp)
{
    if (!fp)
        fp = js_GetTopStackFrame(cx);
    while (fp) {
        if (fp->script)
            return fp;
        fp = fp->down;
    }
    return NULL;
}
