









#include "jscntxtinlines.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryReporting.h"

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#ifdef ANDROID
# include <android/log.h>
# include <fstream>
# include <string>
#endif  

#include "jsatom.h"
#include "jscompartment.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsiter.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsprf.h"
#include "jspubtd.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jstypes.h"
#include "jswatchpoint.h"

#include "gc/Marking.h"
#include "jit/Ion.h"
#include "js/CharacterEncoding.h"
#include "js/OldDebugAPI.h"
#include "vm/Debugger.h"
#include "vm/HelperThreads.h"
#include "vm/Shape.h"

#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "vm/Stack-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::DebugOnly;
using mozilla::PodArrayZero;
using mozilla::PodZero;
using mozilla::PointerRangeSize;

bool
js::AutoCycleDetector::init()
{
    ObjectSet &set = cx->cycleDetectorSet;
    hashsetAddPointer = set.lookupForAdd(obj);
    if (!hashsetAddPointer) {
        if (!set.add(hashsetAddPointer, obj))
            return false;
        cyclic = false;
        hashsetGenerationAtInit = set.generation();
    }
    return true;
}

js::AutoCycleDetector::~AutoCycleDetector()
{
    if (!cyclic) {
        if (hashsetGenerationAtInit == cx->cycleDetectorSet.generation())
            cx->cycleDetectorSet.remove(hashsetAddPointer);
        else
            cx->cycleDetectorSet.remove(obj);
    }
}

void
js::TraceCycleDetectionSet(JSTracer *trc, js::ObjectSet &set)
{
    for (js::ObjectSet::Enum e(set); !e.empty(); e.popFront()) {
        JSObject *key = e.front();
        trc->setTracingLocation((void *)&e.front());
        MarkObjectRoot(trc, &key, "cycle detector table entry");
        if (key != e.front())
            e.rekeyFront(key);
    }
}

void
JSCompartment::sweepCallsiteClones()
{
    if (callsiteClones.initialized()) {
        for (CallsiteCloneTable::Enum e(callsiteClones); !e.empty(); e.popFront()) {
            CallsiteCloneKey key = e.front().key();
            if (IsObjectAboutToBeFinalized(&key.original) || IsScriptAboutToBeFinalized(&key.script) ||
                IsObjectAboutToBeFinalized(e.front().value().unsafeGet()))
            {
                e.removeFront();
            } else if (key != e.front().key()) {
                e.rekeyFront(key);
            }
        }
    }
}

JSFunction *
js::ExistingCloneFunctionAtCallsite(const CallsiteCloneTable &table, JSFunction *fun,
                                    JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(fun->nonLazyScript()->shouldCloneAtCallsite());
    JS_ASSERT(!fun->nonLazyScript()->enclosingStaticScope());
    JS_ASSERT(types::UseNewTypeForClone(fun));

    



    JS_ASSERT(fun->isTenured());

    if (!table.initialized())
        return nullptr;

    CallsiteCloneTable::Ptr p = table.readonlyThreadsafeLookup(CallsiteCloneKey(fun, script, script->pcToOffset(pc)));
    if (p)
        return p->value();

    return nullptr;
}

JSFunction *
js::CloneFunctionAtCallsite(JSContext *cx, HandleFunction fun, HandleScript script, jsbytecode *pc)
{
    if (JSFunction *clone = ExistingCloneFunctionAtCallsite(cx->compartment()->callsiteClones, fun, script, pc))
        return clone;

    MOZ_ASSERT(fun->isSelfHostedBuiltin(),
               "only self-hosted builtin functions may be cloned at call sites, and "
               "Function.prototype.caller relies on this");

    RootedObject parent(cx, fun->environment());
    JSFunction *clone = CloneFunctionObject(cx, fun, parent);
    if (!clone)
        return nullptr;

    



    clone->nonLazyScript()->setIsCallsiteClone(fun);

    typedef CallsiteCloneKey Key;
    typedef CallsiteCloneTable Table;

    Table &table = cx->compartment()->callsiteClones;
    if (!table.initialized() && !table.init())
        return nullptr;

    if (!table.putNew(Key(fun, script, script->pcToOffset(pc)), clone))
        return nullptr;

    return clone;
}

JSContext *
js::NewContext(JSRuntime *rt, size_t stackChunkSize)
{
    JS_AbortIfWrongThread(rt);

    JSContext *cx = js_new<JSContext>(rt);
    if (!cx)
        return nullptr;

    if (!cx->cycleDetectorSet.init()) {
        js_delete(cx);
        return nullptr;
    }

    



    rt->contextList.insertBack(cx);

    






    if (!rt->haveCreatedContext) {
        JS_BeginRequest(cx);
        bool ok = rt->initializeAtoms(cx);
        if (ok)
            ok = rt->initSelfHosting(cx);

        if (ok && !rt->parentRuntime)
            ok = rt->transformToPermanentAtoms();

        JS_EndRequest(cx);

        if (!ok) {
            DestroyContext(cx, DCM_NEW_FAILED);
            return nullptr;
        }

        rt->haveCreatedContext = true;
    }

    JSContextCallback cxCallback = rt->cxCallback;
    if (cxCallback && !cxCallback(cx, JSCONTEXT_NEW, rt->cxCallbackData)) {
        DestroyContext(cx, DCM_NEW_FAILED);
        return nullptr;
    }

    return cx;
}

void
js::DestroyContext(JSContext *cx, DestroyContextMode mode)
{
    JSRuntime *rt = cx->runtime();
    JS_AbortIfWrongThread(rt);

    if (cx->outstandingRequests != 0)
        MOZ_CRASH();

    cx->checkNoGCRooters();

    if (mode != DCM_NEW_FAILED) {
        if (JSContextCallback cxCallback = rt->cxCallback) {
            



            JS_ALWAYS_TRUE(cxCallback(cx, JSCONTEXT_DESTROY,
                                      rt->cxCallbackData));
        }
    }

    cx->remove();
    bool last = !rt->hasContexts();
    if (last) {
        



        for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next())
            c->types.print(cx, false);
    }
    if (mode == DCM_FORCE_GC) {
        JS_ASSERT(!rt->isHeapBusy());
        JS::PrepareForFullGC(rt);
        rt->gc.gc(GC_NORMAL, JS::gcreason::DESTROY_CONTEXT);
    }
    js_delete_poison(cx);
}

void
ContextFriendFields::checkNoGCRooters() {
#if defined(JSGC_USE_EXACT_ROOTING) && defined(DEBUG)
    for (int i = 0; i < THING_ROOT_LIMIT; ++i)
        JS_ASSERT(thingGCRooters[i] == nullptr);
#endif
}

bool
AutoResolving::alreadyStartedSlow() const
{
    JS_ASSERT(link);
    AutoResolving *cursor = link;
    do {
        JS_ASSERT(this != cursor);
        if (object.get() == cursor->object && id.get() == cursor->id && kind == cursor->kind)
            return true;
    } while (!!(cursor = cursor->link));
    return false;
}

static void
ReportError(JSContext *cx, const char *message, JSErrorReport *reportp,
            JSErrorCallback callback, void *userRef)
{
    





    JS_ASSERT(reportp);
    if ((!callback || callback == js_GetErrorMessage) &&
        reportp->errorNumber == JSMSG_UNCAUGHT_EXCEPTION)
    {
        reportp->flags |= JSREPORT_EXCEPTION;
    }

    


    if (!JS_IsRunning(cx) || !js_ErrorToException(cx, message, reportp, callback, userRef)) {
        if (message)
            CallErrorReporter(cx, message, reportp);
    }
}





static void
PopulateReportBlame(JSContext *cx, JSErrorReport *report)
{
    



    NonBuiltinFrameIter iter(cx);
    if (iter.done())
        return;

    report->filename = iter.scriptFilename();
    report->lineno = iter.computeLine(&report->column);
    report->originPrincipals = iter.originPrincipals();
}










void
js_ReportOutOfMemory(ThreadSafeContext *cxArg)
{
#ifdef JS_MORE_DETERMINISTIC
    




    fprintf(stderr, "js_ReportOutOfMemory called\n");
#endif

    if (cxArg->isForkJoinContext()) {
        cxArg->asForkJoinContext()->setPendingAbortFatal(ParallelBailoutOutOfMemory);
        return;
    }

    if (!cxArg->isJSContext())
        return;

    JSContext *cx = cxArg->asJSContext();
    cx->runtime()->hadOutOfMemory = true;

    
    if (JS::OutOfMemoryCallback oomCallback = cx->runtime()->oomCallback) {
        AutoSuppressGC suppressGC(cx);
        oomCallback(cx, cx->runtime()->oomCallbackData);
    }

    if (JS_IsRunning(cx)) {
        cx->setPendingException(StringValue(cx->names().outOfMemory));
        return;
    }

    
    const JSErrorFormatString *efs = js_GetErrorMessage(nullptr, JSMSG_OUT_OF_MEMORY);
    const char *msg = efs ? efs->format : "Out of memory";

    
    JSErrorReport report;
    PodZero(&report);
    report.flags = JSREPORT_ERROR;
    report.errorNumber = JSMSG_OUT_OF_MEMORY;
    PopulateReportBlame(cx, &report);

    
    if (JSErrorReporter onError = cx->errorReporter) {
        AutoSuppressGC suppressGC(cx);
        onError(cx, msg, &report);
    }

    









    JS_ASSERT(!cx->isExceptionPending());
}

JS_FRIEND_API(void)
js_ReportOverRecursed(JSContext *maybecx)
{
#ifdef JS_MORE_DETERMINISTIC
    







    fprintf(stderr, "js_ReportOverRecursed called\n");
#endif
    if (maybecx)
        JS_ReportErrorNumber(maybecx, js_GetErrorMessage, nullptr, JSMSG_OVER_RECURSED);
}

void
js_ReportOverRecursed(ThreadSafeContext *cx)
{
    if (cx->isJSContext())
        js_ReportOverRecursed(cx->asJSContext());
    else if (cx->isExclusiveContext())
        cx->asExclusiveContext()->addPendingOverRecursed();
}

void
js_ReportAllocationOverflow(ThreadSafeContext *cxArg)
{
    if (!cxArg)
        return;

    if (cxArg->isForkJoinContext()) {
        cxArg->asForkJoinContext()->setPendingAbortFatal(ParallelBailoutOutOfMemory);
        return;
    }

    if (!cxArg->isJSContext())
        return;
    JSContext *cx = cxArg->asJSContext();

    AutoSuppressGC suppressGC(cx);
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_ALLOC_OVERFLOW);
}







static bool
checkReportFlags(JSContext *cx, unsigned *flags)
{
    if (JSREPORT_IS_STRICT_MODE_ERROR(*flags)) {
        




        JSScript *script = cx->currentScript();
        if (script && script->strict())
            *flags &= ~JSREPORT_WARNING;
        else if (cx->compartment()->options().extraWarnings(cx))
            *flags |= JSREPORT_WARNING;
        else
            return true;
    } else if (JSREPORT_IS_STRICT(*flags)) {
        
        if (!cx->compartment()->options().extraWarnings(cx))
            return true;
    }

    
    if (JSREPORT_IS_WARNING(*flags) && cx->runtime()->options().werror())
        *flags &= ~JSREPORT_WARNING;

    return false;
}

bool
js_ReportErrorVA(JSContext *cx, unsigned flags, const char *format, va_list ap)
{
    char *message;
    char16_t *ucmessage;
    size_t messagelen;
    JSErrorReport report;
    bool warning;

    if (checkReportFlags(cx, &flags))
        return true;

    message = JS_vsmprintf(format, ap);
    if (!message)
        return false;
    messagelen = strlen(message);

    PodZero(&report);
    report.flags = flags;
    report.errorNumber = JSMSG_USER_DEFINED_ERROR;
    report.ucmessage = ucmessage = InflateString(cx, message, &messagelen);
    PopulateReportBlame(cx, &report);

    warning = JSREPORT_IS_WARNING(report.flags);

    ReportError(cx, message, &report, nullptr, nullptr);
    js_free(message);
    js_free(ucmessage);
    return warning;
}


void
js::ReportUsageError(JSContext *cx, HandleObject callee, const char *msg)
{
    const char *usageStr = "usage";
    PropertyName *usageAtom = Atomize(cx, usageStr, strlen(usageStr))->asPropertyName();
    RootedId id(cx, NameToId(usageAtom));
    DebugOnly<Shape *> shape = static_cast<Shape *>(callee->nativeLookup(cx, id));
    JS_ASSERT(!shape->configurable());
    JS_ASSERT(!shape->writable());
    JS_ASSERT(shape->hasDefaultGetter());

    RootedValue usage(cx);
    if (!JS_LookupProperty(cx, callee, "usage", &usage))
        return;

    if (usage.isUndefined()) {
        JS_ReportError(cx, "%s", msg);
    } else {
        JSString *str = usage.toString();
        JS::Anchor<JSString *> a_str(str);

        if (!str->ensureFlat(cx))
            return;
        AutoStableStringChars chars(cx);
        if (!chars.initTwoByte(cx, str))
            return;

        JS_ReportError(cx, "%s. Usage: %hs", msg, chars.twoByteRange().start().get());
    }
}

bool
js::PrintError(JSContext *cx, FILE *file, const char *message, JSErrorReport *report,
               bool reportWarnings)
{
    if (!report) {
        fprintf(file, "%s\n", message);
        fflush(file);
        return false;
    }

    
    if (JSREPORT_IS_WARNING(report->flags) && !reportWarnings)
        return false;

    char *prefix = nullptr;
    if (report->filename)
        prefix = JS_smprintf("%s:", report->filename);
    if (report->lineno) {
        char *tmp = prefix;
        prefix = JS_smprintf("%s%u:%u ", tmp ? tmp : "", report->lineno, report->column);
        JS_free(cx, tmp);
    }
    if (JSREPORT_IS_WARNING(report->flags)) {
        char *tmp = prefix;
        prefix = JS_smprintf("%s%swarning: ",
                             tmp ? tmp : "",
                             JSREPORT_IS_STRICT(report->flags) ? "strict " : "");
        JS_free(cx, tmp);
    }

    
    const char *ctmp;
    while ((ctmp = strchr(message, '\n')) != 0) {
        ctmp++;
        if (prefix)
            fputs(prefix, file);
        fwrite(message, 1, ctmp - message, file);
        message = ctmp;
    }

    
    if (prefix)
        fputs(prefix, file);
    fputs(message, file);

    if (report->linebuf) {
        
        int n = strlen(report->linebuf);
        fprintf(file, ":\n%s%s%s%s",
                prefix,
                report->linebuf,
                (n > 0 && report->linebuf[n-1] == '\n') ? "" : "\n",
                prefix);
        n = report->tokenptr - report->linebuf;
        for (int i = 0, j = 0; i < n; i++) {
            if (report->linebuf[i] == '\t') {
                for (int k = (j + 8) & ~7; j < k; j++) {
                    fputc('.', file);
                }
                continue;
            }
            fputc('.', file);
            j++;
        }
        fputc('^', file);
    }
    fputc('\n', file);
    fflush(file);
    JS_free(cx, prefix);
    return true;
}












bool
js_ExpandErrorArguments(ExclusiveContext *cx, JSErrorCallback callback,
                        void *userRef, const unsigned errorNumber,
                        char **messagep, JSErrorReport *reportp,
                        ErrorArgumentsType argumentsType, va_list ap)
{
    const JSErrorFormatString *efs;
    int i;
    int argCount;
    bool messageArgsPassed = !!reportp->messageArgs;

    *messagep = nullptr;

    if (!callback)
        callback = js_GetErrorMessage;

    {
        AutoSuppressGC suppressGC(cx);
        efs = callback(userRef, errorNumber);
    }

    if (efs) {
        reportp->exnType = efs->exnType;

        size_t totalArgsLength = 0;
        size_t argLengths[10]; 
        argCount = efs->argCount;
        JS_ASSERT(argCount <= 10);
        if (argCount > 0) {
            





            if (messageArgsPassed) {
                JS_ASSERT(!reportp->messageArgs[argCount]);
            } else {
                reportp->messageArgs = cx->pod_malloc<const char16_t*>(argCount + 1);
                if (!reportp->messageArgs)
                    return false;
                
                reportp->messageArgs[argCount] = nullptr;
            }
            for (i = 0; i < argCount; i++) {
                if (messageArgsPassed) {
                    
                } else if (argumentsType == ArgumentsAreASCII) {
                    char *charArg = va_arg(ap, char *);
                    size_t charArgLength = strlen(charArg);
                    reportp->messageArgs[i] = InflateString(cx, charArg, &charArgLength);
                    if (!reportp->messageArgs[i])
                        goto error;
                } else {
                    reportp->messageArgs[i] = va_arg(ap, char16_t *);
                }
                argLengths[i] = js_strlen(reportp->messageArgs[i]);
                totalArgsLength += argLengths[i];
            }
        }
        



        if (argCount > 0) {
            if (efs->format) {
                char16_t *buffer, *fmt, *out;
                int expandedArgs = 0;
                size_t expandedLength;
                size_t len = strlen(efs->format);

                buffer = fmt = InflateString(cx, efs->format, &len);
                if (!buffer)
                    goto error;
                expandedLength = len
                                 - (3 * argCount)       
                                 + totalArgsLength;

                



                reportp->ucmessage = out = cx->pod_malloc<char16_t>(expandedLength + 1);
                if (!out) {
                    js_free(buffer);
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
                js_free(buffer);
                size_t msgLen = PointerRangeSize(static_cast<const char16_t *>(reportp->ucmessage),
                                                 static_cast<const char16_t *>(out));
                mozilla::Range<const char16_t> ucmsg(reportp->ucmessage, msgLen);
                *messagep = JS::LossyTwoByteCharsToNewLatin1CharsZ(cx, ucmsg).c_str();
                if (!*messagep)
                    goto error;
            }
        } else {
            
            JS_ASSERT(!reportp->messageArgs);
            



            if (efs->format) {
                size_t len;
                *messagep = DuplicateString(cx, efs->format).release();
                if (!*messagep)
                    goto error;
                len = strlen(*messagep);
                reportp->ucmessage = InflateString(cx, *messagep, &len);
                if (!reportp->ucmessage)
                    goto error;
            }
        }
    }
    if (*messagep == nullptr) {
        
        const char *defaultErrorMessage
            = "No error message available for error number %d";
        size_t nbytes = strlen(defaultErrorMessage) + 16;
        *messagep = cx->pod_malloc<char>(nbytes);
        if (!*messagep)
            goto error;
        JS_snprintf(*messagep, nbytes, defaultErrorMessage, errorNumber);
    }
    return true;

error:
    if (!messageArgsPassed && reportp->messageArgs) {
        
        if (argumentsType == ArgumentsAreASCII) {
            i = 0;
            while (reportp->messageArgs[i])
                js_free((void *)reportp->messageArgs[i++]);
        }
        js_free((void *)reportp->messageArgs);
        reportp->messageArgs = nullptr;
    }
    if (reportp->ucmessage) {
        js_free((void *)reportp->ucmessage);
        reportp->ucmessage = nullptr;
    }
    if (*messagep) {
        js_free((void *)*messagep);
        *messagep = nullptr;
    }
    return false;
}

bool
js_ReportErrorNumberVA(JSContext *cx, unsigned flags, JSErrorCallback callback,
                       void *userRef, const unsigned errorNumber,
                       ErrorArgumentsType argumentsType, va_list ap)
{
    JSErrorReport report;
    char *message;
    bool warning;

    if (checkReportFlags(cx, &flags))
        return true;
    warning = JSREPORT_IS_WARNING(flags);

    PodZero(&report);
    report.flags = flags;
    report.errorNumber = errorNumber;
    PopulateReportBlame(cx, &report);

    if (!js_ExpandErrorArguments(cx, callback, userRef, errorNumber,
                                 &message, &report, argumentsType, ap)) {
        return false;
    }

    ReportError(cx, message, &report, callback, userRef);

    js_free(message);
    if (report.messageArgs) {
        



        if (argumentsType == ArgumentsAreASCII) {
            int i = 0;
            while (report.messageArgs[i])
                js_free((void *)report.messageArgs[i++]);
        }
        js_free((void *)report.messageArgs);
    }
    js_free((void *)report.ucmessage);

    return warning;
}

bool
js_ReportErrorNumberUCArray(JSContext *cx, unsigned flags, JSErrorCallback callback,
                            void *userRef, const unsigned errorNumber,
                            const char16_t **args)
{
    if (checkReportFlags(cx, &flags))
        return true;
    bool warning = JSREPORT_IS_WARNING(flags);

    JSErrorReport report;
    PodZero(&report);
    report.flags = flags;
    report.errorNumber = errorNumber;
    PopulateReportBlame(cx, &report);
    report.messageArgs = args;

    char *message;
    va_list dummy;
    if (!js_ExpandErrorArguments(cx, callback, userRef, errorNumber,
                                 &message, &report, ArgumentsAreUnicode, dummy)) {
        return false;
    }

    ReportError(cx, message, &report, callback, userRef);

    js_free(message);
    js_free((void *)report.ucmessage);

    return warning;
}

void
js::CallErrorReporter(JSContext *cx, const char *message, JSErrorReport *reportp)
{
    JS_ASSERT(message);
    JS_ASSERT(reportp);

    if (JSErrorReporter onError = cx->errorReporter)
        onError(cx, message, reportp);
}

void
js_ReportIsNotDefined(JSContext *cx, const char *name)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_DEFINED, name);
}

bool
js_ReportIsNullOrUndefined(JSContext *cx, int spindex, HandleValue v,
                           HandleString fallback)
{
    char *bytes;
    bool ok;

    bytes = DecompileValueGenerator(cx, spindex, v, fallback);
    if (!bytes)
        return false;

    if (strcmp(bytes, js_undefined_str) == 0 ||
        strcmp(bytes, js_null_str) == 0) {
        ok = JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                          js_GetErrorMessage, nullptr,
                                          JSMSG_NO_PROPERTIES, bytes,
                                          nullptr, nullptr);
    } else if (v.isUndefined()) {
        ok = JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                          js_GetErrorMessage, nullptr,
                                          JSMSG_UNEXPECTED_TYPE, bytes,
                                          js_undefined_str, nullptr);
    } else {
        JS_ASSERT(v.isNull());
        ok = JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                          js_GetErrorMessage, nullptr,
                                          JSMSG_UNEXPECTED_TYPE, bytes,
                                          js_null_str, nullptr);
    }

    js_free(bytes);
    return ok;
}

void
js_ReportMissingArg(JSContext *cx, HandleValue v, unsigned arg)
{
    char argbuf[11];
    char *bytes;
    RootedAtom atom(cx);

    JS_snprintf(argbuf, sizeof argbuf, "%u", arg);
    bytes = nullptr;
    if (IsFunctionObject(v)) {
        atom = v.toObject().as<JSFunction>().atom();
        bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK,
                                        v, atom);
        if (!bytes)
            return;
    }
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                         JSMSG_MISSING_FUN_ARG, argbuf,
                         bytes ? bytes : "");
    js_free(bytes);
}

bool
js_ReportValueErrorFlags(JSContext *cx, unsigned flags, const unsigned errorNumber,
                         int spindex, HandleValue v, HandleString fallback,
                         const char *arg1, const char *arg2)
{
    char *bytes;
    bool ok;

    JS_ASSERT(js_ErrorFormatString[errorNumber].argCount >= 1);
    JS_ASSERT(js_ErrorFormatString[errorNumber].argCount <= 3);
    bytes = DecompileValueGenerator(cx, spindex, v, fallback);
    if (!bytes)
        return false;

    ok = JS_ReportErrorFlagsAndNumber(cx, flags, js_GetErrorMessage,
                                      nullptr, errorNumber, bytes, arg1, arg2);
    js_free(bytes);
    return ok;
}

const JSErrorFormatString js_ErrorFormatString[JSErr_Limit] = {
#define MSG_DEF(name, count, exception, format) \
    { format, count, exception } ,
#include "js.msg"
#undef MSG_DEF
};

JS_FRIEND_API(const JSErrorFormatString *)
js_GetErrorMessage(void *userRef, const unsigned errorNumber)
{
    if (errorNumber > 0 && errorNumber < JSErr_Limit)
        return &js_ErrorFormatString[errorNumber];
    return nullptr;
}

bool
js::InvokeInterruptCallback(JSContext *cx)
{
    JS_ASSERT(cx->runtime()->requestDepth >= 1);

    JSRuntime *rt = cx->runtime();
    JS_ASSERT(rt->interrupt);

    
    
    
    rt->interrupt = false;

    
    
    rt->resetJitStackLimit();

    cx->gcIfNeeded();

    rt->interruptPar = false;

    
    
    jit::AttachFinishedCompilations(cx);

    
    
    
    JSInterruptCallback cb = cx->runtime()->interruptCallback;
    if (!cb)
        return true;

    if (cb(cx)) {
        
        
        if (cx->compartment()->debugMode()) {
            ScriptFrameIter iter(cx);
            if (iter.script()->stepModeEnabled()) {
                RootedValue rval(cx);
                switch (Debugger::onSingleStep(cx, &rval)) {
                  case JSTRAP_ERROR:
                    return false;
                  case JSTRAP_CONTINUE:
                    return true;
                  case JSTRAP_RETURN:
                    
                    Debugger::propagateForcedReturn(cx, iter.abstractFramePtr(), rval);
                    return false;
                  case JSTRAP_THROW:
                    cx->setPendingException(rval);
                    return false;
                  default:;
                }
            }
        }

        return true;
    }

    
    
    JSString *stack = ComputeStackString(cx);
    JSFlatString *flat = stack ? stack->ensureFlat(cx) : nullptr;

    const char16_t *chars;
    AutoStableStringChars stableChars(cx);
    if (flat && stableChars.initTwoByte(cx, flat))
        chars = stableChars.twoByteRange().start().get();
    else
        chars = MOZ_UTF16("(stack not available)");
    JS_ReportErrorFlagsAndNumberUC(cx, JSREPORT_WARNING, js_GetErrorMessage, nullptr,
                                   JSMSG_TERMINATED, chars);

    return false;
}

bool
js::HandleExecutionInterrupt(JSContext *cx)
{
    if (cx->runtime()->interrupt)
        return InvokeInterruptCallback(cx);
    return true;
}

ThreadSafeContext::ThreadSafeContext(JSRuntime *rt, PerThreadData *pt, ContextKind kind)
  : ContextFriendFields(rt),
    contextKind_(kind),
    perThreadData(pt),
    allocator_(nullptr)
{
}

bool
ThreadSafeContext::isForkJoinContext() const
{
    return contextKind_ == Context_ForkJoin;
}

ForkJoinContext *
ThreadSafeContext::asForkJoinContext()
{
    JS_ASSERT(isForkJoinContext());
    return reinterpret_cast<ForkJoinContext *>(this);
}

void
ThreadSafeContext::recoverFromOutOfMemory()
{
    
    if (JSContext *maybecx = maybeJSContext()) {
        if (maybecx->isExceptionPending()) {
            MOZ_ASSERT(maybecx->isThrowingOutOfMemory());
            maybecx->clearPendingException();
        } else {
            MOZ_ASSERT(maybecx->runtime()->hadOutOfMemory);
        }
    }
}

JSContext::JSContext(JSRuntime *rt)
  : ExclusiveContext(rt, &rt->mainThread, Context_JS),
    throwing(false),
    unwrappedException_(UndefinedValue()),
    options_(),
    propagatingForcedReturn_(false),
    reportGranularity(JS_DEFAULT_JITREPORT_GRANULARITY),
    resolvingList(nullptr),
    generatingError(false),
    savedFrameChains_(),
    cycleDetectorSet(MOZ_THIS_IN_INITIALIZER_LIST()),
    errorReporter(nullptr),
    data(nullptr),
    data2(nullptr),
    outstandingRequests(0),
    iterValue(MagicValue(JS_NO_ITER_VALUE)),
    jitIsBroken(false),
#ifdef MOZ_TRACE_JSCALLS
    functionCallback(nullptr),
#endif
    innermostGenerator_(nullptr)
{
    JS_ASSERT(static_cast<ContextFriendFields*>(this) ==
              ContextFriendFields::get(this));
}

JSContext::~JSContext()
{
    
    JS_ASSERT(!resolvingList);
}

bool
JSContext::getPendingException(MutableHandleValue rval)
{
    JS_ASSERT(throwing);
    rval.set(unwrappedException_);
    if (IsAtomsCompartment(compartment()))
        return true;
    clearPendingException();
    if (!compartment()->wrap(this, rval))
        return false;
    assertSameCompartment(this, rval);
    setPendingException(rval);
    return true;
}

bool
JSContext::isThrowingOutOfMemory()
{
    return throwing && unwrappedException_ == StringValue(names().outOfMemory);
}

void
JSContext::enterGenerator(JSGenerator *gen)
{
    JS_ASSERT(!gen->prevGenerator);
    gen->prevGenerator = innermostGenerator_;
    innermostGenerator_ = gen;
}

void
JSContext::leaveGenerator(JSGenerator *gen)
{
    JS_ASSERT(innermostGenerator_ == gen);
    innermostGenerator_ = innermostGenerator_->prevGenerator;
    gen->prevGenerator = nullptr;
}


bool
JSContext::saveFrameChain()
{
    if (!savedFrameChains_.append(SavedFrameChain(compartment(), enterCompartmentDepth_)))
        return false;

    if (Activation *act = mainThread().activation())
        act->saveFrameChain();

    setCompartment(nullptr);
    enterCompartmentDepth_ = 0;

    return true;
}

void
JSContext::restoreFrameChain()
{
    JS_ASSERT(enterCompartmentDepth_ == 0); 
                                            
    SavedFrameChain sfc = savedFrameChains_.popCopy();
    setCompartment(sfc.compartment);
    enterCompartmentDepth_ = sfc.enterCompartmentCount;

    if (Activation *act = mainThread().activation())
        act->restoreFrameChain();
}

bool
JSContext::currentlyRunning() const
{
    for (ActivationIterator iter(runtime()); !iter.done(); ++iter) {
        if (iter->cx() == this) {
            if (iter->hasSavedFrameChain())
                return false;
            return true;
        }
    }

    return false;
}

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
            static const char* const blacklist[] = {
                "SCH-I400",     
                "SGH-T959",     
                "SGH-I897",     
                "SCH-I500",     
                "SPH-D700",     
                "GT-I9000",     
                nullptr
            };
            for (const char* const* hw = &blacklist[0]; *hw; ++hw) {
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

void
JSContext::updateJITEnabled()
{
    jitIsBroken = IsJITBrokenHere();
}

size_t
JSContext::sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
    




    return mallocSizeOf(this) + cycleDetectorSet.sizeOfExcludingThis(mallocSizeOf);
}

void
JSContext::mark(JSTracer *trc)
{
    

    
    if (isExceptionPending())
        MarkValueRoot(trc, &unwrappedException_, "unwrapped exception");

    TraceCycleDetectionSet(trc, cycleDetectorSet);

    MarkValueRoot(trc, &iterValue, "iterValue");
}

void *
ThreadSafeContext::stackLimitAddressForJitCode(StackKind kind)
{
#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    return runtime_->mainThread.addressOfSimulatorStackLimit();
#endif
    return stackLimitAddress(kind);
}

JSVersion
JSContext::findVersion() const
{
    if (JSScript *script = currentScript(nullptr, ALLOW_CROSS_COMPARTMENT))
        return script->getVersion();

    if (compartment() && compartment()->options().version() != JSVERSION_UNKNOWN)
        return compartment()->options().version();

    return runtime()->defaultVersion();
}

#ifdef DEBUG

JS::AutoCheckRequestDepth::AutoCheckRequestDepth(JSContext *cx)
    : cx(cx)
{
    JS_ASSERT(cx->runtime()->requestDepth || cx->runtime()->isHeapBusy());
    JS_ASSERT(CurrentThreadCanAccessRuntime(cx->runtime()));
    cx->runtime()->checkRequestDepth++;
}

JS::AutoCheckRequestDepth::AutoCheckRequestDepth(ContextFriendFields *cxArg)
    : cx(static_cast<ThreadSafeContext *>(cxArg)->maybeJSContext())
{
    if (cx) {
        JS_ASSERT(cx->runtime()->requestDepth || cx->runtime()->isHeapBusy());
        JS_ASSERT(CurrentThreadCanAccessRuntime(cx->runtime()));
        cx->runtime()->checkRequestDepth++;
    }
}

JS::AutoCheckRequestDepth::~AutoCheckRequestDepth()
{
    if (cx) {
        JS_ASSERT(cx->runtime()->checkRequestDepth != 0);
        cx->runtime()->checkRequestDepth--;
    }
}

#endif

#ifdef JS_CRASH_DIAGNOSTICS
void CompartmentChecker::check(InterpreterFrame *fp)
{
    if (fp)
        check(fp->scopeChain());
}

void CompartmentChecker::check(AbstractFramePtr frame)
{
    if (frame)
        check(frame.scopeChain());
}
#endif

void
js::CrashAtUnhandlableOOM(const char *reason)
{
    char msgbuf[1024];
    JS_snprintf(msgbuf, sizeof(msgbuf), "[unhandlable oom] %s", reason);
    MOZ_ReportAssertionFailure(msgbuf, __FILE__, __LINE__);
    MOZ_CRASH();
}
