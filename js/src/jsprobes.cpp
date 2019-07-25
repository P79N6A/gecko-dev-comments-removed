


































#ifdef MOZ_ETW
#include "jswin.h"
#include <evntprov.h>


#include "ETWProvider.h"
#endif

#include "jsapi.h"
#include "jsutil.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jsfun.h"
#include "jsinterp.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jsstr.h"

#include "jsprobes.h"
#include <sys/types.h>

#define TYPEOF(cx,v)    (JSVAL_IS_NULL(v) ? JSTYPE_NULL : JS_TypeOfValue(cx,v))

using namespace js;

const char Probes::nullName[] = "(null)";
const char Probes::anonymousName[] = "(anonymous)";

bool Probes::ProfilingActive = true;

static Vector<Probes::JITWatcher*, 4, SystemAllocPolicy> jitWatchers;

bool
Probes::addJITWatcher(JITWatcher *watcher)
{
    return jitWatchers.append(watcher);
}

bool
Probes::removeJITWatcher(JSRuntime *rt, JITWatcher *watcher)
{
    JITWatcher **place = Find(jitWatchers, watcher);
    if (!place)
        return false;
    if (rt)
        rt->delete_(*place);
    else
        Foreground::delete_(*place);
    jitWatchers.erase(place);
    return true;
}

void
Probes::removeAllJITWatchers(JSRuntime *rt)
{
    if (rt) {
        for (JITWatcher **p = jitWatchers.begin(); p != jitWatchers.end(); ++p)
            rt->delete_(*p);
    } else {
        for (JITWatcher **p = jitWatchers.begin(); p != jitWatchers.end(); ++p)
            Foreground::delete_(*p);
    }
    jitWatchers.clear();
}

Probes::JITReportGranularity
Probes::JITGranularityRequested()
{
    JITReportGranularity want = JITREPORT_GRANULARITY_NONE;
    for (JITWatcher **p = jitWatchers.begin(); p != jitWatchers.end(); ++p) {
        JITReportGranularity request = (*p)->granularityRequested();
        if (request > want)
            want = request;
    }

    return want;
}

#ifdef JS_METHODJIT
void
Probes::registerMJITCode(JSContext *cx, js::mjit::JITScript *jscr,
                         JSScript *script, JSFunction *fun,
                         js::mjit::Compiler_ActiveFrame **inlineFrames,
                         void *mainCodeAddress, size_t mainCodeSize,
                         void *stubCodeAddress, size_t stubCodeSize)
{
    for (JITWatcher **p = jitWatchers.begin(); p != jitWatchers.end(); ++p)
        (*p)->registerMJITCode(cx, jscr, script, fun,
                               inlineFrames,
                               mainCodeAddress, mainCodeSize,
                               stubCodeAddress, stubCodeSize);
}

void
Probes::discardMJITCode(JSContext *cx, mjit::JITScript *jscr, JSScript *script, void* address)
{
    for (JITWatcher **p = jitWatchers.begin(); p != jitWatchers.end(); ++p)
        (*p)->discardMJITCode(cx, jscr, script, address);
}

void
Probes::registerICCode(JSContext *cx,
                       mjit::JITScript *jscr, JSScript *script, jsbytecode* pc,
                       void *start, size_t size)
{
    for (JITWatcher **p = jitWatchers.begin(); p != jitWatchers.end(); ++p)
        (*p)->registerICCode(cx, jscr, script, pc, start, size);
}
#endif


void
Probes::discardExecutableRegion(void *start, size_t size)
{
    for (JITWatcher **p = jitWatchers.begin(); p != jitWatchers.end(); ++p)
        (*p)->discardExecutableRegion(start, size);
}

static JSRuntime *initRuntime;

JSBool
Probes::startEngine()
{
    bool ok = true;

    return ok;
}

bool
Probes::createRuntime(JSRuntime *rt)
{
    bool ok = true;

    static JSCallOnceType once = { 0 };
    initRuntime = rt;
    if (!JS_CallOnce(&once, Probes::startEngine))
        ok = false;

#ifdef MOZ_ETW
    if (!ETWCreateRuntime(rt))
        ok = false;
#endif

    return ok;
}

bool
Probes::destroyRuntime(JSRuntime *rt)
{
    bool ok = true;
#ifdef MOZ_ETW
    if (!ETWDestroyRuntime(rt))
        ok = false;
#endif

    return ok;
}

bool
Probes::shutdown()
{
    bool ok = true;
#ifdef MOZ_ETW
    if (!ETWShutdown())
        ok = false;
#endif

    Probes::removeAllJITWatchers(NULL);

    return ok;
}

#ifdef INCLUDE_MOZILLA_DTRACE
static const char *
ScriptFilename(const JSScript *script)
{
    if (!script)
        return Probes::nullName;
    if (!script->filename)
        return Probes::anonymousName;
    return script->filename;
}

static const char *
FunctionName(JSContext *cx, const JSFunction *fun, JSAutoByteString* bytes)
{
    if (!fun)
        return Probes::nullName;
    JSAtom *atom = const_cast<JSAtom*>(fun->atom);
    if (!atom)
        return Probes::anonymousName;
    return bytes->encode(cx, atom) ? bytes->ptr() : Probes::nullName;
}

static const char *
FunctionClassname(const JSFunction *fun)
{
    if (!fun || fun->isInterpreted())
        return Probes::nullName;
    if (fun->getConstructorClass())
        return fun->getConstructorClass()->name;
    return Probes::nullName;
}








void
Probes::DTraceEnterJSFun(JSContext *cx, JSFunction *fun, JSScript *script)
{
    JSAutoByteString funNameBytes;
    JAVASCRIPT_FUNCTION_ENTRY(ScriptFilename(script), FunctionClassname(fun),
                              FunctionName(cx, fun, &funNameBytes));
}

void
Probes::DTraceExitJSFun(JSContext *cx, JSFunction *fun, JSScript *script)
{
    JSAutoByteString funNameBytes;
    JAVASCRIPT_FUNCTION_RETURN(ScriptFilename(script), FunctionClassname(fun),
                               FunctionName(cx, fun, &funNameBytes));
}
#endif

#ifdef MOZ_ETW
static void
current_location(JSContext *cx, int* lineno, char const **filename)
{
    JSScript *script = js_GetCurrentScript(cx);
    if (! script) {
        *lineno = -1;
        *filename = "(uninitialized)";
        return;
    }
    *lineno = js_PCToLineNumber(cx, script, js_GetCurrentBytecodePC(cx));
    *filename = ScriptFilename(script);
}







bool
Probes::ETWCallTrackingActive(JSContext *cx)
{
    return MCGEN_ENABLE_CHECK(MozillaSpiderMonkey_Context, EvtFunctionEntry);
}

bool
Probes::ETWCreateRuntime(JSRuntime *rt)
{
    static bool registered = false;
    if (!registered) {
        EventRegisterMozillaSpiderMonkey();
        registered = true;
    }
    return true;
}

bool
Probes::ETWDestroyRuntime(JSRuntime *rt)
{
    return true;
}

bool
Probes::ETWShutdown()
{
    EventUnregisterMozillaSpiderMonkey();
    return true;
}

bool
Probes::ETWEnterJSFun(JSContext *cx, JSFunction *fun, JSScript *script, int counter)
{
    int lineno = script ? script->lineno : -1;
    JSAutoByteString bytes;
    return (EventWriteEvtFunctionEntry(ScriptFilename(script), lineno,
                                       ObjectClassname((JSObject *)fun),
                                       FunctionName(cx, fun, &bytes)) == ERROR_SUCCESS);
}

bool
Probes::ETWExitJSFun(JSContext *cx, JSFunction *fun, JSScript *script, int counter)
{
    int lineno = script ? script->lineno : -1;
    JSAutoByteString bytes;
    return (EventWriteEvtFunctionExit(ScriptFilename(script), lineno,
                                      ObjectClassname((JSObject *)fun),
                                      FunctionName(cx, fun, &bytes)) == ERROR_SUCCESS);
}

bool
Probes::ETWCreateObject(JSContext *cx, JSObject *obj)
{
    int lineno;
    const char * script_filename;
    current_location(cx, &lineno, &script_filename);

    return EventWriteEvtObjectCreate(script_filename, lineno,
                                     ObjectClassname(obj), reinterpret_cast<JSUint64>(obj),
                                     obj ? obj->slotsAndStructSize() : 0) == ERROR_SUCCESS;
}

bool
Probes::ETWFinalizeObject(JSObject *obj)
{
    return EventWriteEvtObjectFinalize(ObjectClassname(obj),
                                       reinterpret_cast<JSUint64>(obj)) == ERROR_SUCCESS;
}

bool
Probes::ETWResizeObject(JSContext *cx, JSObject *obj, size_t oldSize, size_t newSize)
{
    int lineno;
    const char *script_filename;
    current_location(cx, &lineno, &script_filename);

    return EventWriteEvtObjectResize(script_filename, lineno,
                                     ObjectClassname(obj), reinterpret_cast<JSUint64>(obj),
                                     oldSize, newSize) == ERROR_SUCCESS;
}

bool
Probes::ETWCreateString(JSContext *cx, JSString *string, size_t length)
{
    int lineno;
    const char *script_filename;
    current_location(cx, &lineno, &script_filename);

    return EventWriteEvtStringCreate(script_filename, lineno,
                                     reinterpret_cast<JSUint64>(string), length) == ERROR_SUCCESS;
}

bool
Probes::ETWFinalizeString(JSString *string)
{
    return EventWriteEvtStringFinalize(reinterpret_cast<JSUint64>(string), string->length()) == ERROR_SUCCESS;
}

bool
Probes::ETWCompileScriptBegin(const char *filename, int lineno)
{
    return EventWriteEvtScriptCompileBegin(filename, lineno) == ERROR_SUCCESS;
}

bool
Probes::ETWCompileScriptEnd(const char *filename, int lineno)
{
    return EventWriteEvtScriptCompileEnd(filename, lineno) == ERROR_SUCCESS;
}

bool
Probes::ETWCalloutBegin(JSContext *cx, JSFunction *fun)
{
    const char *script_filename;
    int lineno;
    JSAutoByteString bytes;
    current_location(cx, &lineno, &script_filename);

    return EventWriteEvtCalloutBegin(script_filename,
                                     lineno,
                                     ObjectClassname((JSObject *)fun),
                                     FunctionName(cx, fun, &bytes)) == ERROR_SUCCESS;
}

bool
Probes::ETWCalloutEnd(JSContext *cx, JSFunction *fun)
{
        const char *script_filename;
        int lineno;
        JSAutoByteString bytes;
        current_location(cx, &lineno, &script_filename);

        return EventWriteEvtCalloutEnd(script_filename,
                                       lineno,
                                       ObjectClassname((JSObject *)fun),
                                       FunctionName(cx, fun, &bytes)) == ERROR_SUCCESS;
}

bool
Probes::ETWAcquireMemory(JSContext *cx, void *address, size_t nbytes)
{
    return EventWriteEvtMemoryAcquire(reinterpret_cast<JSUint64>(cx->compartment),
                                      reinterpret_cast<JSUint64>(address),
                                      nbytes) == ERROR_SUCCESS;
}

bool
Probes::ETWReleaseMemory(JSContext *cx, void *address, size_t nbytes)
{
    return EventWriteEvtMemoryRelease(reinterpret_cast<JSUint64>(cx->compartment),
                                      reinterpret_cast<JSUint64>(address),
                                      nbytes) == ERROR_SUCCESS;
}

bool
Probes::ETWGCStart(JSCompartment *compartment)
{
    return EventWriteEvtGCStart(reinterpret_cast<JSUint64>(compartment)) == ERROR_SUCCESS;
}

bool
Probes::ETWGCEnd(JSCompartment *compartment)
{
    return EventWriteEvtGCEnd(reinterpret_cast<JSUint64>(compartment)) == ERROR_SUCCESS;
}

bool
Probes::ETWGCStartMarkPhase(JSCompartment *compartment)
{
    return EventWriteEvtGCStartMarkPhase(reinterpret_cast<JSUint64>(compartment)) == ERROR_SUCCESS;
}

bool
Probes::ETWGCEndMarkPhase(JSCompartment *compartment)
{
    return EventWriteEvtGCEndMarkPhase(reinterpret_cast<JSUint64>(compartment)) == ERROR_SUCCESS;
}

bool
Probes::ETWGCStartSweepPhase(JSCompartment *compartment)
{
    return EventWriteEvtGCStartSweepPhase(reinterpret_cast<JSUint64>(compartment)) == ERROR_SUCCESS;
}

bool
Probes::ETWGCEndSweepPhase(JSCompartment *compartment)
{
    return EventWriteEvtGCEndSweepPhase(reinterpret_cast<JSUint64>(compartment)) == ERROR_SUCCESS;
}

bool
Probes::ETWCustomMark(JSString *string)
{
    const jschar *chars = string->getCharsZ(NULL);
    return !chars || EventWriteEvtCustomString(chars) == ERROR_SUCCESS;
}

bool
Probes::ETWCustomMark(const char *string)
{
    return EventWriteEvtCustomANSIString(string) == ERROR_SUCCESS;
}

bool
Probes::ETWCustomMark(int marker)
{
    return EventWriteEvtCustomInt(marker) == ERROR_SUCCESS;
}

bool
Probes::ETWStartExecution(JSContext *cx, JSScript *script)
{
    int lineno = script ? script->lineno : -1;
    return EventWriteEvtExecuteStart(ScriptFilename(script), lineno) == ERROR_SUCCESS;
}

bool
Probes::ETWStopExecution(JSContext *cx, JSScript *script)
{
    int lineno = script ? script->lineno : -1;
    return EventWriteEvtExecuteDone(ScriptFilename(script), lineno) == ERROR_SUCCESS;
}

bool
Probes::ETWResizeHeap(JSCompartment *compartment, size_t oldSize, size_t newSize)
{
    return EventWriteEvtHeapResize(reinterpret_cast<JSUint64>(compartment),
                                   oldSize, newSize) == ERROR_SUCCESS;
}

#endif
