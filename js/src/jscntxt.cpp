











































#include <limits.h> 

#include <new>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef ANDROID
# include <android/log.h>
# include <fstream>
# include <string>
#endif  

#include "jstypes.h"
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
#include "jsgcmark.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsmath.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jspubtd.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"
#include "ion/IonFrames.h"

#ifdef JS_METHODJIT
# include "assembler/assembler/MacroAssembler.h"
#endif
#include "frontend/TokenStream.h"
#include "frontend/ParseMaps.h"
#include "yarr/BumpPointerAllocator.h"

#include "jsatominlines.h"
#include "jscntxtinlines.h"
#include "jscompartment.h"
#include "jsobjinlines.h"

using namespace js;
using namespace js::gc;
void
JSRuntime::sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf, size_t *normal, size_t *temporary,
                               size_t *regexpCode, size_t *stackCommitted)
{
    if (normal)
        *normal = mallocSizeOf(dtoaState);

    if (temporary)
        *temporary = tempLifoAlloc.sizeOfExcludingThis(mallocSizeOf);

    if (regexpCode) {
        size_t method = 0, regexp = 0, unused = 0;
        if (execAlloc_)
            execAlloc_->sizeOfCode(&method, &regexp, &unused);
        JS_ASSERT(method == 0);     
        *regexpCode = regexp + unused;
    }

    if (stackCommitted)
        *stackCommitted = stackSpace.sizeOfCommitted();
}

JS_FRIEND_API(void)
JSRuntime::triggerOperationCallback()
{
    



    JS_ATOMIC_SET(&interrupt, 1);
}

JSC::ExecutableAllocator *
JSRuntime::createExecutableAllocator(JSContext *cx)
{
    JS_ASSERT(!execAlloc_);
    JS_ASSERT(cx->runtime == this);

    execAlloc_ = new_<JSC::ExecutableAllocator>();
    if (!execAlloc_)
        js_ReportOutOfMemory(cx);
    return execAlloc_;
}

WTF::BumpPointerAllocator *
JSRuntime::createBumpPointerAllocator(JSContext *cx)
{
    JS_ASSERT(!bumpAlloc_);
    JS_ASSERT(cx->runtime == this);

    bumpAlloc_ = new_<WTF::BumpPointerAllocator>();
    if (!bumpAlloc_)
        js_ReportOutOfMemory(cx);
    return bumpAlloc_;
}

RegExpCache *
JSRuntime::createRegExpCache(JSContext *cx)
{
    JS_ASSERT(!reCache_);
    JS_ASSERT(cx->runtime == this);

    RegExpCache *newCache = new_<RegExpCache>(this);
    if (!newCache || !newCache->init()) {
        js_ReportOutOfMemory(cx);
        delete_<RegExpCache>(newCache);
        return NULL;
    }

    reCache_ = newCache;
    return reCache_;
}

JSScript *
js_GetCurrentScript(JSContext *cx)
{
    return cx->hasfp() ? cx->fp()->maybeScript() : NULL;
}

JSContext *
js_NewContext(JSRuntime *rt, size_t stackChunkSize)
{
    JS_AbortIfWrongThread(rt);

    




    JSContext *cx = OffTheBooks::new_<JSContext>(rt);
    if (!cx)
        return NULL;

    JS_ASSERT(cx->findVersion() == JSVERSION_DEFAULT);

    if (!cx->busyArrays.init()) {
        Foreground::delete_(cx);
        return NULL;
    }

    



    bool first = JS_CLIST_IS_EMPTY(&rt->contextList);
    JS_APPEND_LINK(&cx->link, &rt->contextList);

    js_InitRandom(cx);

    







    if (first) {
#ifdef JS_THREADSAFE
        JS_BeginRequest(cx);
#endif
        bool ok = rt->staticStrings.init(cx);
        if (ok)
            ok = js_InitCommonAtoms(cx);

#ifdef JS_THREADSAFE
        JS_EndRequest(cx);
#endif
        if (!ok) {
            js_DestroyContext(cx, JSDCM_NEW_FAILED);
            return NULL;
        }
    }

    JSContextCallback cxCallback = rt->cxCallback;
    if (cxCallback && !cxCallback(cx, JSCONTEXT_NEW)) {
        js_DestroyContext(cx, JSDCM_NEW_FAILED);
        return NULL;
    }

    return cx;
}

void
js_DestroyContext(JSContext *cx, JSDestroyContextMode mode)
{
    JSRuntime *rt = cx->runtime;
    JS_AbortIfWrongThread(rt);

    JS_ASSERT(!cx->enumerators);

#ifdef JS_THREADSAFE
    JS_ASSERT(cx->outstandingRequests == 0);
#endif

    if (mode != JSDCM_NEW_FAILED) {
        if (JSContextCallback cxCallback = rt->cxCallback) {
            



            DebugOnly<JSBool> callbackStatus = cxCallback(cx, JSCONTEXT_DESTROY);
            JS_ASSERT(callbackStatus);
        }
    }

    JS_LOCK_GC(rt);
    JS_REMOVE_LINK(&cx->link);
    bool last = !rt->hasContexts();
    if (last || mode == JSDCM_FORCE_GC || mode == JSDCM_MAYBE_GC) {
        JS_ASSERT(!rt->gcRunning);

#ifdef JS_THREADSAFE
        rt->gcHelperThread.waitBackgroundSweepEnd();
#endif
        JS_UNLOCK_GC(rt);

        if (last) {
            



            {
                AutoLockGC lock(rt);
                for (CompartmentsIter c(rt); !c.done(); c.next())
                    c->types.print(cx, false);
            }

            
            js_FinishCommonAtoms(cx);

            
            for (CompartmentsIter c(rt); !c.done(); c.next())
                c->clearTraps(cx);
            JS_ClearAllWatchPoints(cx);

            js_GC(cx, NULL, GC_NORMAL, gcreason::LAST_CONTEXT);

        } else if (mode == JSDCM_FORCE_GC) {
            js_GC(cx, NULL, GC_NORMAL, gcreason::DESTROY_CONTEXT);
        } else if (mode == JSDCM_MAYBE_GC) {
            JS_MaybeGC(cx);
        }
        JS_LOCK_GC(rt);
    }
#ifdef JS_THREADSAFE
    rt->gcHelperThread.waitBackgroundSweepEnd();
#endif
    JS_UNLOCK_GC(rt);
    Foreground::delete_(cx);
}

JSContext *
js_ContextIterator(JSRuntime *rt, JSBool unlocked, JSContext **iterp)
{
    JSContext *cx = *iterp;

    Maybe<AutoLockGC> lockIf;
    if (unlocked)
        lockIf.construct(rt);
    cx = JSContext::fromLinkField(cx ? cx->link.next : rt->contextList.next);
    if (&cx->link == &rt->contextList)
        cx = NULL;
    *iterp = cx;
    return cx;
}

namespace js {

bool
AutoResolving::alreadyStartedSlow() const
{
    JS_ASSERT(link);
    AutoResolving *cursor = link;
    do {
        JS_ASSERT(this != cursor);
        if (object == cursor->object && id == cursor->id && kind == cursor->kind)
            return true;
    } while (!!(cursor = cursor->link));
    return false;
}

} 

static void
ReportError(JSContext *cx, const char *message, JSErrorReport *reportp,
            JSErrorCallback callback, void *userRef)
{
    





    JS_ASSERT(reportp);
    if ((!callback || callback == js_GetErrorMessage) &&
        reportp->errorNumber == JSMSG_UNCAUGHT_EXCEPTION)
        reportp->flags |= JSREPORT_EXCEPTION;

    







    if (!JS_IsRunning(cx) ||
        !js_ErrorToException(cx, message, reportp, callback, userRef)) {
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
    



    for (FrameRegsIter iter(cx); !iter.done(); ++iter) {
        if (iter.fp()->isScriptFrame()) {
            report->filename = iter.fp()->script()->filename;
            report->lineno = js_PCToLineNumber(cx, iter.fp()->script(), iter.pc());
            report->originPrincipals = iter.fp()->script()->originPrincipals;
            break;
        }
    }
}








void
js_ReportOutOfMemory(JSContext *cx)
{
    cx->runtime->hadOutOfMemory = true;

    JSErrorReport report;
    JSErrorReporter onError = cx->errorReporter;

    
    const JSErrorFormatString *efs =
        js_GetLocalizedErrorMessage(cx, NULL, NULL, JSMSG_OUT_OF_MEMORY);
    const char *msg = efs ? efs->format : "Out of memory";

    
    PodZero(&report);
    report.flags = JSREPORT_ERROR;
    report.errorNumber = JSMSG_OUT_OF_MEMORY;
    PopulateReportBlame(cx, &report);

    





    cx->clearPendingException();
    if (onError) {
        JSDebugErrorHook hook = cx->debugHooks->debugErrorHook;
        if (hook &&
            !hook(cx, msg, &report, cx->debugHooks->debugErrorHookData)) {
            onError = NULL;
        }
    }

    if (onError) {
        AutoAtomicIncrement incr(&cx->runtime->inOOMReport);
        onError(cx, msg, &report);
    }
}

JS_FRIEND_API(void)
js_ReportOverRecursed(JSContext *maybecx)
{
#ifdef JS_MORE_DETERMINISTIC
    







    fprintf(stderr, "js_ReportOverRecursed called\n");
#endif
    if (maybecx)
        JS_ReportErrorNumber(maybecx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
}

void
js_ReportAllocationOverflow(JSContext *maybecx)
{
    if (maybecx)
        JS_ReportErrorNumber(maybecx, js_GetErrorMessage, NULL, JSMSG_ALLOC_OVERFLOW);
}







static bool
checkReportFlags(JSContext *cx, uintN *flags)
{
    if (JSREPORT_IS_STRICT_MODE_ERROR(*flags)) {
        




        JSScript *script = cx->stack.currentScript();
        if (script && script->strictModeCode)
            *flags &= ~JSREPORT_WARNING;
        else if (cx->hasStrictOption())
            *flags |= JSREPORT_WARNING;
        else
            return true;
    } else if (JSREPORT_IS_STRICT(*flags)) {
        
        if (!cx->hasStrictOption())
            return true;
    }

    
    if (JSREPORT_IS_WARNING(*flags) && cx->hasWErrorOption())
        *flags &= ~JSREPORT_WARNING;

    return false;
}

JSBool
js_ReportErrorVA(JSContext *cx, uintN flags, const char *format, va_list ap)
{
    char *message;
    jschar *ucmessage;
    size_t messagelen;
    JSErrorReport report;
    JSBool warning;

    if (checkReportFlags(cx, &flags))
        return JS_TRUE;

    message = JS_vsmprintf(format, ap);
    if (!message)
        return JS_FALSE;
    messagelen = strlen(message);

    PodZero(&report);
    report.flags = flags;
    report.errorNumber = JSMSG_USER_DEFINED_ERROR;
    report.ucmessage = ucmessage = InflateString(cx, message, &messagelen);
    PopulateReportBlame(cx, &report);

    warning = JSREPORT_IS_WARNING(report.flags);

    ReportError(cx, message, &report, NULL, NULL);
    Foreground::free_(message);
    Foreground::free_(ucmessage);
    return warning;
}












JSBool
js_ExpandErrorArguments(JSContext *cx, JSErrorCallback callback,
                        void *userRef, const uintN errorNumber,
                        char **messagep, JSErrorReport *reportp,
                        bool charArgs, va_list ap)
{
    const JSErrorFormatString *efs;
    int i;
    int argCount;

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
                cx->malloc_(sizeof(jschar *) * (argCount + 1));
            if (!reportp->messageArgs)
                return JS_FALSE;
            reportp->messageArgs[argCount] = NULL;
            for (i = 0; i < argCount; i++) {
                if (charArgs) {
                    char *charArg = va_arg(ap, char *);
                    size_t charArgLength = strlen(charArg);
                    reportp->messageArgs[i] = InflateString(cx, charArg, &charArgLength);
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

                buffer = fmt = InflateString(cx, efs->format, &len);
                if (!buffer)
                    goto error;
                expandedLength = len
                                 - (3 * argCount)       
                                 + totalArgsLength;

                



                reportp->ucmessage = out = (jschar *)
                    cx->malloc_((expandedLength + 1) * sizeof(jschar));
                if (!out) {
                    cx->free_(buffer);
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
                cx->free_(buffer);
                *messagep = DeflateString(cx, reportp->ucmessage,
                                          size_t(out - reportp->ucmessage));
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
                reportp->ucmessage = InflateString(cx, *messagep, &len);
                if (!reportp->ucmessage)
                    goto error;
            }
        }
    }
    if (*messagep == NULL) {
        
        const char *defaultErrorMessage
            = "No error message available for error number %d";
        size_t nbytes = strlen(defaultErrorMessage) + 16;
        *messagep = (char *)cx->malloc_(nbytes);
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
                cx->free_((void *)reportp->messageArgs[i++]);
        }
        cx->free_((void *)reportp->messageArgs);
        reportp->messageArgs = NULL;
    }
    if (reportp->ucmessage) {
        cx->free_((void *)reportp->ucmessage);
        reportp->ucmessage = NULL;
    }
    if (*messagep) {
        cx->free_((void *)*messagep);
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

    if (checkReportFlags(cx, &flags))
        return JS_TRUE;
    warning = JSREPORT_IS_WARNING(flags);

    PodZero(&report);
    report.flags = flags;
    report.errorNumber = errorNumber;
    PopulateReportBlame(cx, &report);

    if (!js_ExpandErrorArguments(cx, callback, userRef, errorNumber,
                                 &message, &report, !!charArgs, ap)) {
        return JS_FALSE;
    }

    ReportError(cx, message, &report, callback, userRef);

    if (message)
        cx->free_(message);
    if (report.messageArgs) {
        



        if (charArgs) {
            int i = 0;
            while (report.messageArgs[i])
                cx->free_((void *)report.messageArgs[i++]);
        }
        cx->free_((void *)report.messageArgs);
    }
    if (report.ucmessage)
        cx->free_((void *)report.ucmessage);

    return warning;
}

JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *reportp)
{
    JSErrorReporter onError;

    if (!message)
        return;

    if (cx->lastMessage)
        Foreground::free_(cx->lastMessage);
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
js_ReportIsNullOrUndefined(JSContext *cx, intN spindex, const Value &v,
                           JSString *fallback)
{
    char *bytes;
    JSBool ok;

    bytes = DecompileValueGenerator(cx, spindex, v, fallback);
    if (!bytes)
        return JS_FALSE;

    if (strcmp(bytes, js_undefined_str) == 0 ||
        strcmp(bytes, js_null_str) == 0) {
        ok = JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                          js_GetErrorMessage, NULL,
                                          JSMSG_NO_PROPERTIES, bytes,
                                          NULL, NULL);
    } else if (v.isUndefined()) {
        ok = JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                          js_GetErrorMessage, NULL,
                                          JSMSG_UNEXPECTED_TYPE, bytes,
                                          js_undefined_str, NULL);
    } else {
        JS_ASSERT(v.isNull());
        ok = JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                          js_GetErrorMessage, NULL,
                                          JSMSG_UNEXPECTED_TYPE, bytes,
                                          js_null_str, NULL);
    }

    cx->free_(bytes);
    return ok;
}

void
js_ReportMissingArg(JSContext *cx, const Value &v, uintN arg)
{
    char argbuf[11];
    char *bytes;
    JSAtom *atom;

    JS_snprintf(argbuf, sizeof argbuf, "%u", arg);
    bytes = NULL;
    if (IsFunctionObject(v)) {
        atom = v.toObject().toFunction()->atom;
        bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK,
                                        v, atom);
        if (!bytes)
            return;
    }
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_MISSING_FUN_ARG, argbuf,
                         bytes ? bytes : "");
    cx->free_(bytes);
}

JSBool
js_ReportValueErrorFlags(JSContext *cx, uintN flags, const uintN errorNumber,
                         intN spindex, const Value &v, JSString *fallback,
                         const char *arg1, const char *arg2)
{
    char *bytes;
    JSBool ok;

    JS_ASSERT(js_ErrorFormatString[errorNumber].argCount >= 1);
    JS_ASSERT(js_ErrorFormatString[errorNumber].argCount <= 3);
    bytes = DecompileValueGenerator(cx, spindex, v, fallback);
    if (!bytes)
        return JS_FALSE;

    ok = JS_ReportErrorFlagsAndNumber(cx, flags, js_GetErrorMessage,
                                      NULL, errorNumber, bytes, arg1, arg2);
    cx->free_(bytes);
    return ok;
}

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
js_InvokeOperationCallback(JSContext *cx)
{
    JS_ASSERT_REQUEST_DEPTH(cx);

    JSRuntime *rt = cx->runtime;
    JS_ASSERT(rt->interrupt != 0);

    




    JS_ATOMIC_SET(&rt->interrupt, 0);

    if (rt->gcIsNeeded)
        js_GC(cx, rt->gcTriggerCompartment, GC_NORMAL, rt->gcTriggerReason);

#ifdef JS_THREADSAFE
    









    JS_YieldRequest(cx);
#endif

    JSOperationCallback cb = cx->operationCallback;

    





    return !cb || cb(cx);
}

JSBool
js_HandleExecutionInterrupt(JSContext *cx)
{
    JSBool result = JS_TRUE;
    if (cx->runtime->interrupt)
        result = js_InvokeOperationCallback(cx) && result;
    return result;
}

StackFrame *
js_GetScriptedCaller(JSContext *cx, StackFrame *fp)
{
    if (!fp)
        fp = js_GetTopStackFrame(cx, FRAME_EXPAND_ALL);
    while (fp && fp->isDummyFrame())
        fp = fp->prev();
    JS_ASSERT_IF(fp, fp->isScriptFrame());
    return fp;
}

jsbytecode*
js_GetCurrentBytecodePC(JSContext* cx)
{
    return cx->hasfp() ? cx->regs().pc : NULL;
}

void
DSTOffsetCache::purge()
{
    




    offsetMilliseconds = 0;
    rangeStartSeconds = rangeEndSeconds = INT64_MIN;
    oldOffsetMilliseconds = 0;
    oldRangeStartSeconds = oldRangeEndSeconds = INT64_MIN;

    sanityCheck();
}







DSTOffsetCache::DSTOffsetCache()
{
    purge();
}

JSContext::JSContext(JSRuntime *rt)
  : ContextFriendFields(rt),
    defaultVersion(JSVERSION_DEFAULT),
    hasVersionOverride(false),
    throwing(false),
    exception(UndefinedValue()),
    runOptions(0),
    reportGranularity(JS_DEFAULT_JITREPORT_GRANULARITY),
    localeCallbacks(NULL),
    resolvingList(NULL),
    generatingError(false),
    compartment(NULL),
    stack(thisDuringConstruction()),  
    parseMapPool_(NULL),
    globalObject(NULL),
    argumentFormatMap(NULL),
    lastMessage(NULL),
    errorReporter(NULL),
    operationCallback(NULL),
    data(NULL),
    data2(NULL),
#ifdef JS_THREADSAFE
    outstandingRequests(0),
#endif
    autoGCRooters(NULL),
    debugHooks(&rt->globalDebugHooks),
    securityCallbacks(NULL),
    resolveFlags(0),
    rngSeed(0),
    iterValue(MagicValue(JS_NO_ITER_VALUE)),
#ifdef JS_METHODJIT
    methodJitEnabled(false),
#endif
    inferenceEnabled(false),
#ifdef MOZ_TRACE_JSCALLS
    functionCallback(NULL),
#endif
    enumerators(NULL),
#ifdef JS_THREADSAFE
    gcBackgroundFree(NULL),
#endif
    activeCompilations(0)
#ifdef DEBUG
    , stackIterAssertionEnabled(true)
#endif
{
    PodZero(&sharpObjectMap);
    PodZero(&link);
#ifdef JS_THREADSAFE
    PodZero(&threadLinks);
#endif
#ifdef JSGC_ROOT_ANALYSIS
    PodArrayZero(thingGCRooters);
#ifdef DEBUG
    checkGCRooters = NULL;
#endif
#endif
}

JSContext::~JSContext()
{
    
    if (parseMapPool_)
        Foreground::delete_<ParseMapPool>(parseMapPool_);

    if (lastMessage)
        Foreground::free_(lastMessage);

    
    JSArgumentFormatMap *map = argumentFormatMap;
    while (map) {
        JSArgumentFormatMap *temp = map;
        map = map->next;
        Foreground::free_(temp);
    }

    JS_ASSERT(!resolvingList);
}

void
JSContext::resetCompartment()
{
    JSObject *scopeobj;
    if (stack.hasfp()) {
        scopeobj = &fp()->scopeChain();
    } else {
        scopeobj = globalObject;
        if (!scopeobj)
            goto error;

        



        OBJ_TO_INNER_OBJECT(this, scopeobj);
        if (!scopeobj)
            goto error;
    }

    compartment = scopeobj->compartment();
    inferenceEnabled = compartment->types.inferenceEnabled;

    if (isExceptionPending())
        wrapPendingException();
    updateJITEnabled();
    return;

error:

    



    compartment = NULL;
}






void
JSContext::wrapPendingException()
{
    Value v = getPendingException();
    clearPendingException();
    if (compartment->wrap(this, &v))
        setPendingException(v);
}

JSGenerator *
JSContext::generatorFor(StackFrame *fp) const
{
    JS_ASSERT(stack.containsSlow(fp));
    JS_ASSERT(fp->isGeneratorFrame());
    JS_ASSERT(!fp->isFloatingGenerator());
    JS_ASSERT(!genStack.empty());

    if (JS_LIKELY(fp == genStack.back()->liveFrame()))
        return genStack.back();

    
    for (size_t i = 0; i < genStack.length(); ++i) {
        if (genStack[i]->liveFrame() == fp)
            return genStack[i];
    }
    JS_NOT_REACHED("no matching generator");
    return NULL;
}

bool
JSContext::runningWithTrustedPrincipals() const
{
    return !compartment || compartment->principals == runtime->trustedPrincipals();
}

void
JSRuntime::updateMallocCounter(JSContext *cx, size_t nbytes)
{
    
    ptrdiff_t oldCount = gcMallocBytes;
    ptrdiff_t newCount = oldCount - ptrdiff_t(nbytes);
    gcMallocBytes = newCount;
    if (JS_UNLIKELY(newCount <= 0 && oldCount > 0))
        onTooMuchMalloc();
    else if (cx && cx->compartment)
        cx->compartment->updateMallocCounter(nbytes);
}

JS_FRIEND_API(void)
JSRuntime::onTooMuchMalloc()
{
    TriggerGC(this, gcreason::TOO_MUCH_MALLOC);
}

JS_FRIEND_API(void *)
JSRuntime::onOutOfMemory(void *p, size_t nbytes, JSContext *cx)
{
    



    ShrinkGCBuffers(this);
#ifdef JS_THREADSAFE
    {
        AutoLockGC lock(this);
        gcHelperThread.waitBackgroundSweepOrAllocEnd();
    }
#endif
    if (!p)
        p = OffTheBooks::malloc_(nbytes);
    else if (p == reinterpret_cast<void *>(1))
        p = OffTheBooks::calloc_(nbytes);
    else
      p = OffTheBooks::realloc_(p, nbytes);
    if (p)
        return p;
    if (cx)
        js_ReportOutOfMemory(cx);
    return NULL;
}

void
JSRuntime::purge(JSContext *cx)
{
    tempLifoAlloc.freeUnused();
    gsnCache.purge();

    
    propertyCache.purge(cx);

    delete_<RegExpCache>(reCache_);
    reCache_ = NULL;
}

void
JSContext::purge()
{
    if (!activeCompilations) {
        Foreground::delete_<ParseMapPool>(parseMapPool_);
        parseMapPool_ = NULL;
    }
}

#if defined(JS_METHODJIT)
static bool
ComputeIsJITBroken()
{
#if !defined(ANDROID) || defined(GONK)
    return false;
#else  
    if (getenv("JS_IGNORE_JIT_BROKENNESS")) {
        return false;
    }

    std::string line;

    
    std::ifstream osrelease("/proc/sys/kernel/osrelease");
    std::getline(osrelease, line);
    __android_log_print(ANDROID_LOG_INFO, "Gecko", "Detected osrelease `%s'",
                        line.c_str());

    if (line.npos == line.find("2.6.29")) {
        
        __android_log_print(ANDROID_LOG_INFO, "Gecko", "JITs are not broken");
        return false;
    }

    
    line = "";
    bool broken = false;
    std::ifstream cpuinfo("/proc/cpuinfo");
    do {
        if (0 == line.find("Hardware")) {
            const char* blacklist[] = {
                "SCH-I400",     
                "SGH-T959",     
                "SGH-I897",     
                "SCH-I500",     
                "SPH-D700",     
                "GT-I9000",     
                NULL
            };
            for (const char** hw = &blacklist[0]; *hw; ++hw) {
                if (line.npos != line.find(*hw)) {
                    __android_log_print(ANDROID_LOG_INFO, "Gecko",
                                        "Blacklisted device `%s'", *hw);
                    broken = true;
                    break;
                }
            }
            break;
        }
        std::getline(cpuinfo, line);
    } while(!cpuinfo.fail() && !cpuinfo.eof());

    __android_log_print(ANDROID_LOG_INFO, "Gecko", "JITs are %sbroken",
                        broken ? "" : "not ");

    return broken;
#endif  
}

static bool
IsJITBrokenHere()
{
    static bool computedIsBroken = false;
    static bool isBroken = false;
    if (!computedIsBroken) {
        isBroken = ComputeIsJITBroken();
        computedIsBroken = true;
    }
    return isBroken;
}
#endif

void
JSContext::updateJITEnabled()
{
#ifdef JS_METHODJIT
    methodJitEnabled = (runOptions & JSOPTION_METHODJIT) && !IsJITBrokenHere();
#endif
}

size_t
JSContext::sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const
{
    




    return mallocSizeOf(this) + busyArrays.sizeOfExcludingThis(mallocSizeOf);
}

namespace JS {

#if defined JS_THREADSAFE && defined DEBUG

AutoCheckRequestDepth::AutoCheckRequestDepth(JSContext *cx)
    : cx(cx)
{
    JS_ASSERT(cx->runtime->requestDepth || cx->runtime->gcRunning);
    JS_ASSERT(cx->runtime->onOwnerThread());
    cx->runtime->checkRequestDepth++;
}

AutoCheckRequestDepth::~AutoCheckRequestDepth()
{
    JS_ASSERT(cx->runtime->checkRequestDepth != 0);
    cx->runtime->checkRequestDepth--;
}

#endif

} 
