










































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
#include "jsconfig.h"
#include "jsdbgapi.h"
#include "jsexn.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"

#ifdef JS_THREADSAFE
#include "prtypes.h"






static PRUintn threadTPIndex;
static JSBool  tpIndexInited = JS_FALSE;

JSBool
js_InitThreadPrivateIndex(void *ptr)
{
    PRStatus status;

    if (tpIndexInited)
        return JS_TRUE;

    status = PR_NewThreadPrivateIndex(&threadTPIndex, ptr);

    if (status == PR_SUCCESS)
        tpIndexInited = JS_TRUE;
    return status == PR_SUCCESS;
}





void JS_DLL_CALLBACK
js_ThreadDestructorCB(void *ptr)
{
    JSThread *thread = (JSThread *)ptr;

    if (!thread)
        return;
    while (!JS_CLIST_IS_EMPTY(&thread->contextList)) {
        
        JSCList *link = thread->contextList.next;

        JS_REMOVE_AND_INIT_LINK(link);
    }
    GSN_CACHE_CLEAR(&thread->gsnCache);
    free(thread);
}












JSThread *
js_GetCurrentThread(JSRuntime *rt)
{
    JSThread *thread;

    thread = (JSThread *)PR_GetThreadPrivate(threadTPIndex);
    if (!thread) {
        thread = (JSThread *) calloc(1, sizeof(JSThread));
        if (!thread)
            return NULL;

        if (PR_FAILURE == PR_SetThreadPrivate(threadTPIndex, thread)) {
            free(thread);
            return NULL;
        }

        JS_INIT_CLIST(&thread->contextList);
        thread->id = js_CurrentThreadId();

        
#ifdef DEBUG
        memset(thread->gcFreeLists, JS_FREE_PATTERN,
               sizeof(thread->gcFreeLists));
#endif
    }
    return thread;
}






JSBool
js_SetContextThread(JSContext *cx)
{
    JSThread *thread = js_GetCurrentThread(cx->runtime);

    if (!thread) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    



    if (JS_CLIST_IS_EMPTY(&thread->contextList))
        memset(thread->gcFreeLists, 0, sizeof(thread->gcFreeLists));

    cx->thread = thread;
    JS_REMOVE_LINK(&cx->threadLinks);
    JS_APPEND_LINK(&cx->threadLinks, &thread->contextList);
    return JS_TRUE;
}


void
js_ClearContextThread(JSContext *cx)
{
    JS_REMOVE_AND_INIT_LINK(&cx->threadLinks);
#ifdef DEBUG
    if (JS_CLIST_IS_EMPTY(&cx->thread->contextList)) {
        memset(cx->thread->gcFreeLists, JS_FREE_PATTERN,
               sizeof(cx->thread->gcFreeLists));
    }
#endif
    cx->thread = NULL;
}

#endif 

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
    js_OnVersionChange(cx);
}

JSContext *
js_NewContext(JSRuntime *rt, size_t stackChunkSize)
{
    JSContext *cx;
    JSBool ok, first;
    JSContextCallback cxCallback;

    cx = (JSContext *) malloc(sizeof *cx);
    if (!cx)
        return NULL;
    memset(cx, 0, sizeof *cx);

    cx->runtime = rt;
#if JS_STACK_GROWTH_DIRECTION > 0
    cx->stackLimit = (jsuword)-1;
#endif
#ifdef JS_THREADSAFE
    JS_INIT_CLIST(&cx->threadLinks);
    js_SetContextThread(cx);
#endif

    JS_LOCK_GC(rt);
    for (;;) {
        first = (rt->contextList.next == &rt->contextList);
        if (rt->state == JSRTS_UP) {
            JS_ASSERT(!first);
            break;
        }
        if (rt->state == JSRTS_DOWN) {
            JS_ASSERT(first);
            rt->state = JSRTS_LAUNCHING;
            break;
        }
        JS_WAIT_CONDVAR(rt->stateChange, JS_NO_TIMEOUT);
    }
    JS_APPEND_LINK(&cx->links, &rt->contextList);
    JS_UNLOCK_GC(rt);

    






    cx->version = JSVERSION_DEFAULT;
    JS_InitArenaPool(&cx->stackPool, "stack", stackChunkSize, sizeof(jsval));
    JS_InitArenaPool(&cx->tempPool, "temp", 1024, sizeof(jsdouble));

    if (!js_InitRegExpStatics(cx, &cx->regExpStatics)) {
        js_DestroyContext(cx, JSDCM_NEW_FAILED);
        return NULL;
    }

    







    if (first) {
#ifdef JS_THREADSAFE
        JS_BeginRequest(cx);
#endif
        






        ok = (rt->atomState.liveAtoms == 0)
             ? js_InitAtomState(cx, &rt->atomState)
             : js_InitPinnedAtoms(cx, &rt->atomState);
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

void
js_DestroyContext(JSContext *cx, JSDestroyContextMode mode)
{
    JSRuntime *rt;
    JSContextCallback cxCallback;
    JSBool last;
    JSArgumentFormatMap *map;
    JSLocalRootStack *lrs;
    JSLocalRootChunk *lrc;

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
    JS_REMOVE_LINK(&cx->links);
    last = (rt->contextList.next == &rt->contextList);
    if (last)
        rt->state = JSRTS_LANDING;
    JS_UNLOCK_GC(rt);

    if (last) {
#ifdef JS_THREADSAFE
        









        if (cx->requestDepth == 0)
            JS_BeginRequest(cx);
#endif

        
        js_UnpinPinnedAtoms(&rt->atomState);

        
        js_FinishRuntimeNumberState(cx);
        js_FinishRuntimeStringState(cx);

        
        JS_ClearAllTraps(cx);
        JS_ClearAllWatchPoints(cx);
    }

    





    js_FreeRegExpStatics(cx, &cx->regExpStatics);

#ifdef JS_THREADSAFE
    










    while (cx->requestDepth != 0)
        JS_EndRequest(cx);
#endif

    if (last) {
        js_GC(cx, GC_LAST_CONTEXT);

        
        if (rt->atomState.liveAtoms == 0)
            js_FreeAtomState(cx, &rt->atomState);

        
        if (rt->scriptFilenameTable && rt->scriptFilenameTable->nentries == 0)
            js_FinishRuntimeScriptState(rt);

        



        js_FinishDeflatedStringCache(rt);

        
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
    js_ClearContextThread(cx);
#endif

    
    free(cx);
}

JSBool
js_ValidContextPointer(JSRuntime *rt, JSContext *cx)
{
    JSCList *cl;

    for (cl = rt->contextList.next; cl != &rt->contextList; cl = cl->next) {
        if (cl == &cx->links)
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
    if (!cx)
        cx = (JSContext *)&rt->contextList;
    cx = (JSContext *)cx->links.next;
    if (&cx->links == &rt->contextList)
        cx = NULL;
    *iterp = cx;
    if (unlocked)
        JS_UNLOCK_GC(rt);
    return cx;
}

JS_STATIC_DLL_CALLBACK(JSDHashNumber)
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
js_MarkLocalRoots(JSContext *cx, JSLocalRootStack *lrs)
{
    uint32 n, m, mark;
    JSLocalRootChunk *lrc;

    n = lrs->rootCount;
    if (n == 0)
        return;

    mark = lrs->scopeMark;
    lrc = lrs->topChunk;
    do {
        while (--n > mark) {
#ifdef GC_MARK_DEBUG
            char name[22];
            JS_snprintf(name, sizeof name, "<local root %u>", n);
#endif
            m = n & JSLRS_CHUNK_MASK;
            JS_ASSERT(JSVAL_IS_GCTHING(lrc->roots[m]));
            GC_MARK(cx, JSVAL_TO_GCTHING(lrc->roots[m]), name);
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

    







    if (!js_ErrorToException(cx, message, reportp)) {
        js_ReportErrorAgain(cx, message, reportp);
    } else if (cx->runtime->debugErrorHook && cx->errorReporter) {
        JSDebugErrorHook hook = cx->runtime->debugErrorHook;
        
        if (hook)
            hook(cx, message, reportp, cx->runtime->debugErrorHookData);
    }
}








void
js_ReportOutOfMemory(JSContext *cx)
{
    JSStackFrame *fp;
    JSErrorReport report;
    JSErrorReporter onError = cx->errorReporter;

    
    const JSErrorFormatString *efs =
        js_GetLocalizedErrorMessage(cx, NULL, NULL, JSMSG_OUT_OF_MEMORY);
    const char *msg = efs ? efs->format : "Out of memory";

    
    memset(&report, 0, sizeof (struct JSErrorReport));
    report.flags = JSREPORT_ERROR;
    report.errorNumber = JSMSG_OUT_OF_MEMORY;

    



    for (fp = cx->fp; fp; fp = fp->down) {
        if (fp->script && fp->pc) {
            report.filename = fp->script->filename;
            report.lineno = js_PCToLineNumber(cx, fp->script, fp->pc);
            break;
        }
    }

    



    if (onError) {
        JSDebugErrorHook hook = cx->runtime->debugErrorHook;
        if (hook &&
            !hook(cx, msg, &report, cx->runtime->debugErrorHookData)) {
            onError = NULL;
        }
    }

    if (onError)
        onError(cx, msg, &report);
}

JSBool
js_ReportErrorVA(JSContext *cx, uintN flags, const char *format, va_list ap)
{
    char *message;
    jschar *ucmessage;
    size_t messagelen;
    JSStackFrame *fp;
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

    
    for (fp = cx->fp; fp; fp = fp->down) {
        if (fp->script && fp->pc) {
            report.filename = fp->script->filename;
            report.lineno = js_PCToLineNumber(cx, fp->script, fp->pc);
            break;
        }
    }

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
    JSStackFrame *fp;
    JSErrorReport report;
    char *message;
    JSBool warning;

    if ((flags & JSREPORT_STRICT) && !JS_HAS_STRICT_OPTION(cx))
        return JS_TRUE;

    memset(&report, 0, sizeof (struct JSErrorReport));
    report.flags = flags;
    report.errorNumber = errorNumber;

    



    for (fp = cx->fp; fp; fp = fp->down) {
        if (fp->script && fp->pc) {
            report.filename = fp->script->filename;
            report.lineno = js_PCToLineNumber(cx, fp->script, fp->pc);
            break;
        }
    }

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
        JSDebugErrorHook hook = cx->runtime->debugErrorHook;
        if (hook &&
            !hook(cx, cx->lastMessage, reportp,
                  cx->runtime->debugErrorHookData)) {
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

const JSErrorFormatString *
js_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber)
{
    if ((errorNumber > 0) && (errorNumber < JSErr_Limit))
        return &js_ErrorFormatString[errorNumber];
    return NULL;
}

JSBool
js_ResetOperationCounter(JSContext *cx)
{
    JS_ASSERT(cx->operationCounter & JSOW_BRANCH_CALLBACK);

    cx->operationCounter = 0;
    return !cx->branchCallback || cx->branchCallback(cx, NULL);
}
