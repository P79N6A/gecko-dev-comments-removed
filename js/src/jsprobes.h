






































#ifndef _JSPROBES_H
#define _JSPROBES_H

#ifdef INCLUDE_MOZILLA_DTRACE
#include "javascript-trace.h"
#endif
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jsscript.h"
#include "jsobj.h"

namespace js {

namespace mjit {
struct NativeAddressInfo;
struct Compiler_ActiveFrame;
}

namespace Probes {































extern bool ProfilingActive;

extern const char nullName[];
extern const char anonymousName[];


JSBool startEngine();


bool createRuntime(JSRuntime *rt);


bool destroyRuntime(JSRuntime *rt);


bool shutdown();






bool callTrackingActive(JSContext *);





bool wantNativeAddressInfo(JSContext *);


bool enterJSFun(JSContext *, JSFunction *, JSScript *, int counter = 1);


bool exitJSFun(JSContext *, JSFunction *, JSScript *, int counter = 0);


bool startExecution(JSContext *cx, JSScript *script);


bool stopExecution(JSContext *cx, JSScript *script);


bool resizeHeap(JSCompartment *compartment, size_t oldSize, size_t newSize);




bool createObject(JSContext *cx, JSObject *obj);


bool resizeObject(JSContext *cx, JSObject *obj, size_t oldSize, size_t newSize);





bool finalizeObject(JSObject *obj);









bool createString(JSContext *cx, JSString *string, size_t length);






bool finalizeString(JSString *string);


bool compileScriptBegin(JSContext *cx, const char *filename, int lineno);


bool compileScriptEnd(JSContext *cx, JSScript *script, const char *filename, int lineno);


bool calloutBegin(JSContext *cx, JSFunction *fun);


bool calloutEnd(JSContext *cx, JSFunction *fun);


bool acquireMemory(JSContext *cx, void *address, size_t nbytes);
bool releaseMemory(JSContext *cx, void *address, size_t nbytes);












bool GCStart(JSCompartment *compartment);
bool GCEnd(JSCompartment *compartment);

bool GCStartMarkPhase(JSCompartment *compartment);
bool GCEndMarkPhase(JSCompartment *compartment);

bool GCStartSweepPhase(JSCompartment *compartment);
bool GCEndSweepPhase(JSCompartment *compartment);









bool CustomMark(JSString *string);
bool CustomMark(const char *string);
bool CustomMark(int marker);



enum JITReportGranularity {
    JITREPORT_GRANULARITY_NONE = 0,
    JITREPORT_GRANULARITY_FUNCTION = 1,
    JITREPORT_GRANULARITY_LINE = 2,
    JITREPORT_GRANULARITY_OP = 3
};






class JITWatcher {
public:
    virtual JITReportGranularity granularityRequested() = 0;

#ifdef JS_METHODJIT
    virtual void registerMJITCode(JSContext *cx, js::mjit::JITScript *jscr,
                                  JSScript *script, JSFunction *fun,
                                  mjit::Compiler_ActiveFrame** inlineFrames,
                                  void *mainCodeAddress, size_t mainCodeSize,
                                  void *stubCodeAddress, size_t stubCodeSize) = 0;

    virtual void discardMJITCode(JSContext *cx, mjit::JITScript *jscr, JSScript *script,
                                 void* address) = 0;

    virtual void registerICCode(JSContext *cx,
                                js::mjit::JITScript *jscr, JSScript *script, jsbytecode* pc,
                                void *start, size_t size) = 0;
#endif

    virtual void discardExecutableRegion(void *start, size_t size) = 0;
};





bool
addJITWatcher(JITWatcher *watcher);





bool
removeJITWatcher(JSRuntime *rt, JITWatcher *watcher);




void
removeAllJITWatchers(JSRuntime *rt);




JITReportGranularity
JITGranularityRequested();

#ifdef JS_METHODJIT



void
registerMJITCode(JSContext *cx, js::mjit::JITScript *jscr,
                 JSScript *script, JSFunction *fun,
                 mjit::Compiler_ActiveFrame** inlineFrames,
                 void *mainCodeAddress, size_t mainCodeSize,
                 void *stubCodeAddress, size_t stubCodeSize);




void
discardMJITCode(JSContext *cx, mjit::JITScript *jscr, JSScript *script, void* address);




void
registerICCode(JSContext *cx,
               mjit::JITScript *jscr, JSScript *script, jsbytecode* pc,
               void *start, size_t size);
#endif 





void
discardExecutableRegion(void *start, size_t size);







void DTraceEnterJSFun(JSContext *cx, JSFunction *fun, JSScript *script);
void DTraceExitJSFun(JSContext *cx, JSFunction *fun, JSScript *script);




#ifdef MOZ_ETW

bool ETWCreateRuntime(JSRuntime *rt);
bool ETWDestroyRuntime(JSRuntime *rt);
bool ETWShutdown();
bool ETWCallTrackingActive(JSContext *cx);
bool ETWEnterJSFun(JSContext *cx, JSFunction *fun, JSScript *script, int counter);
bool ETWExitJSFun(JSContext *cx, JSFunction *fun, JSScript *script, int counter);
bool ETWCreateObject(JSContext *cx, JSObject *obj);
bool ETWFinalizeObject(JSObject *obj);
bool ETWResizeObject(JSContext *cx, JSObject *obj, size_t oldSize, size_t newSize);
bool ETWCreateString(JSContext *cx, JSString *string, size_t length);
bool ETWFinalizeString(JSString *string);
bool ETWCompileScriptBegin(const char *filename, int lineno);
bool ETWCompileScriptEnd(const char *filename, int lineno);
bool ETWCalloutBegin(JSContext *cx, JSFunction *fun);
bool ETWCalloutEnd(JSContext *cx, JSFunction *fun);
bool ETWAcquireMemory(JSContext *cx, void *address, size_t nbytes);
bool ETWReleaseMemory(JSContext *cx, void *address, size_t nbytes);
bool ETWGCStart(JSCompartment *compartment);
bool ETWGCEnd(JSCompartment *compartment);
bool ETWGCStartMarkPhase(JSCompartment *compartment);
bool ETWGCEndMarkPhase(JSCompartment *compartment);
bool ETWGCStartSweepPhase(JSCompartment *compartment);
bool ETWGCEndSweepPhase(JSCompartment *compartment);
bool ETWCustomMark(JSString *string);
bool ETWCustomMark(const char *string);
bool ETWCustomMark(int marker);
bool ETWStartExecution(JSContext *cx, JSScript *script);
bool ETWStopExecution(JSContext *cx, JSScript *script);
bool ETWResizeHeap(JSCompartment *compartment, size_t oldSize, size_t newSize);
#endif

} 






inline bool
Probes::callTrackingActive(JSContext *cx)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_FUNCTION_ENTRY_ENABLED() || JAVASCRIPT_FUNCTION_RETURN_ENABLED())
        return true;
#endif
#ifdef MOZ_TRACE_JSCALLS
    if (cx->functionCallback)
        return true;
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && ETWCallTrackingActive(cx))
        return true;
#endif
    return false;
}

inline bool
Probes::wantNativeAddressInfo(JSContext *cx)
{
    return (cx->reportGranularity >= JITREPORT_GRANULARITY_FUNCTION &&
            JITGranularityRequested() >= JITREPORT_GRANULARITY_FUNCTION);
}

inline bool
Probes::enterJSFun(JSContext *cx, JSFunction *fun, JSScript *script, int counter)
{
    bool ok = true;
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_FUNCTION_ENTRY_ENABLED())
        DTraceEnterJSFun(cx, fun, script);
#endif
#ifdef MOZ_TRACE_JSCALLS
    cx->doFunctionCallback(fun, script, counter);
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWEnterJSFun(cx, fun, script, counter))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::exitJSFun(JSContext *cx, JSFunction *fun, JSScript *script, int counter)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_FUNCTION_RETURN_ENABLED())
        DTraceExitJSFun(cx, fun, script);
#endif
#ifdef MOZ_TRACE_JSCALLS
    if (counter > 0)
        counter = -counter;
    cx->doFunctionCallback(fun, script, counter);
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWExitJSFun(cx, fun, script, counter))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::resizeHeap(JSCompartment *compartment, size_t oldSize, size_t newSize)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWResizeHeap(compartment, oldSize, newSize))
        ok = false;
#endif

    return ok;
}

#ifdef INCLUDE_MOZILLA_DTRACE
static const char *ObjectClassname(JSObject *obj) {
    if (!obj)
        return "(null object)";
    Class *clasp = obj->getClass();
    if (!clasp)
        return "(null)";
    const char *class_name = clasp->name;
    if (!class_name)
        return "(null class name)";
    return class_name;
}
#endif

inline bool
Probes::createObject(JSContext *cx, JSObject *obj)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_CREATE_ENABLED())
        JAVASCRIPT_OBJECT_CREATE(ObjectClassname(obj), (uintptr_t)obj);
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCreateObject(cx, obj))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::finalizeObject(JSObject *obj)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_FINALIZE_ENABLED()) {
        Class *clasp = obj->getClass();

        
        JAVASCRIPT_OBJECT_FINALIZE(NULL, (char *)clasp->name, (uintptr_t)obj);
    }
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWFinalizeObject(obj))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::resizeObject(JSContext *cx, JSObject *obj, size_t oldSize, size_t newSize)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWResizeObject(cx, obj, oldSize, newSize))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::createString(JSContext *cx, JSString *string, size_t length)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCreateString(cx, string, length))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::finalizeString(JSString *string)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWFinalizeString(string))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::compileScriptBegin(JSContext *cx, const char *filename, int lineno)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCompileScriptBegin(filename, lineno))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::compileScriptEnd(JSContext *cx, JSScript *script, const char *filename, int lineno)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCompileScriptEnd(filename, lineno))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::calloutBegin(JSContext *cx, JSFunction *fun)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCalloutBegin(cx, fun))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::calloutEnd(JSContext *cx, JSFunction *fun)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCalloutEnd(cx, fun))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::acquireMemory(JSContext *cx, void *address, size_t nbytes)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWAcquireMemory(cx, address, nbytes))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::releaseMemory(JSContext *cx, void *address, size_t nbytes)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWReleaseMemory(cx, address, nbytes))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCStart(JSCompartment *compartment)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCStart(compartment))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCEnd(JSCompartment *compartment)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCEnd(compartment))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCStartMarkPhase(JSCompartment *compartment)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCStartMarkPhase(compartment))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCEndMarkPhase(JSCompartment *compartment)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCEndMarkPhase(compartment))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCStartSweepPhase(JSCompartment *compartment)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCStartSweepPhase(compartment))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCEndSweepPhase(JSCompartment *compartment)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCEndSweepPhase(compartment))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::CustomMark(JSString *string)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCustomMark(string))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::CustomMark(const char *string)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCustomMark(string))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::CustomMark(int marker)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCustomMark(marker))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::startExecution(JSContext *cx, JSScript *script)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_START_ENABLED())
        JAVASCRIPT_EXECUTE_START((script->filename ? (char *)script->filename : nullName),
                                 script->lineno);
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWStartExecution(cx, script))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::stopExecution(JSContext *cx, JSScript *script)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_DONE_ENABLED())
        JAVASCRIPT_EXECUTE_DONE((script->filename ? (char *)script->filename : nullName),
                                script->lineno);
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWStopExecution(cx, script))
        ok = false;
#endif

    return ok;
}

struct AutoFunctionCallProbe {
    JSContext * const cx;
    JSFunction *fun;
    JSScript *script;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    AutoFunctionCallProbe(JSContext *cx, JSFunction *fun, JSScript *script
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx), fun(fun), script(script)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        Probes::enterJSFun(cx, fun, script);
    }

    ~AutoFunctionCallProbe() {
        Probes::exitJSFun(cx, fun, script);
    }
};

} 






#endif 
