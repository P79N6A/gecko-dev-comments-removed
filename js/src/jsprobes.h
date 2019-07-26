






#ifndef _JSPROBES_H
#define _JSPROBES_H

#ifdef INCLUDE_MOZILLA_DTRACE
#include "javascript-trace.h"
#endif
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jsscript.h"
#include "jsobj.h"

#ifdef JS_METHODJIT
#include "methodjit/MethodJIT.h"
#endif

#include "vm/ObjectImpl-inl.h"

namespace js {

namespace mjit {
struct NativeAddressInfo;
struct JSActiveFrame;
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


bool enterScript(JSContext *, JSScript *, JSFunction *, StackFrame *);


bool exitScript(JSContext *, JSScript *, JSFunction *, StackFrame *);


bool startExecution(JSContext *cx, JSScript *script);


bool stopExecution(JSContext *cx, JSScript *script);


bool resizeHeap(JSCompartment *compartment, size_t oldSize, size_t newSize);




bool createObject(JSContext *cx, JSObject *obj);


bool objectResizeActive();


bool resizeObject(JSContext *cx, JSObject *obj, size_t oldSize, size_t newSize);





bool finalizeObject(JSObject *obj);









bool createString(JSContext *cx, JSString *string, size_t length);






bool finalizeString(JSString *string);


bool compileScriptBegin(const char *filename, int lineno);


bool compileScriptEnd(const char *filename, int lineno);


bool calloutBegin(JSContext *cx, JSFunction *fun);


bool calloutEnd(JSContext *cx, JSFunction *fun);


bool acquireMemory(JSContext *cx, void *address, size_t nbytes);
bool releaseMemory(JSContext *cx, void *address, size_t nbytes);












bool GCStart();
bool GCEnd();

bool GCStartMarkPhase();
bool GCEndMarkPhase();

bool GCStartSweepPhase();
bool GCEndSweepPhase();









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
    struct NativeRegion {
        mjit::JSActiveFrame *frame;
        JSScript *script;
        size_t inlinedOffset;
        jsbytecode *pc;
        jsbytecode *endpc;
        uintptr_t mainOffset;
        uintptr_t stubOffset;
        bool enter;
    };

    typedef Vector<NativeRegion, 0, RuntimeAllocPolicy> RegionVector;

    virtual JITReportGranularity granularityRequested() = 0;

#ifdef JS_METHODJIT
    static bool CollectNativeRegions(RegionVector &regions,
                                     JSRuntime *rt,
                                     mjit::JITChunk *jit,
                                     mjit::JSActiveFrame *outerFrame,
                                     mjit::JSActiveFrame **inlineFrames);

    virtual void registerMJITCode(JSContext *cx, js::mjit::JITChunk *chunk,
                                  mjit::JSActiveFrame *outerFrame,
                                  mjit::JSActiveFrame **inlineFrames,
                                  void *mainCodeAddress, size_t mainCodeSize,
                                  void *stubCodeAddress, size_t stubCodeSize) = 0;

    virtual void discardMJITCode(FreeOp *fop, mjit::JITScript *jscr, mjit::JITChunk *chunk,
                                 void* address) = 0;

    virtual void registerICCode(JSContext *cx,
                                js::mjit::JITChunk *chunk, JSScript *script, jsbytecode* pc,
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
registerMJITCode(JSContext *cx, js::mjit::JITChunk *chunk,
                 mjit::JSActiveFrame *outerFrame,
                 mjit::JSActiveFrame **inlineFrames,
                 void *mainCodeAddress, size_t mainCodeSize,
                 void *stubCodeAddress, size_t stubCodeSize);




void
discardMJITCode(FreeOp *fop, mjit::JITScript *jscr, mjit::JITChunk *chunk, void* address);




void
registerICCode(JSContext *cx,
               mjit::JITChunk *chunk, JSScript *script, jsbytecode* pc,
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
bool ETWGCStart();
bool ETWGCEnd();
bool ETWGCStartMarkPhase();
bool ETWGCEndMarkPhase();
bool ETWGCStartSweepPhase();
bool ETWGCEndSweepPhase();
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
Probes::enterScript(JSContext *cx, JSScript *script, JSFunction *maybeFun,
                    StackFrame *fp)
{
    bool ok = true;
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_FUNCTION_ENTRY_ENABLED())
        DTraceEnterJSFun(cx, maybeFun, script);
#endif
#ifdef MOZ_TRACE_JSCALLS
    cx->doFunctionCallback(maybeFun, script, 1);
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWEnterJSFun(cx, maybeFun, script, 1))
        ok = false;
#endif

    JSRuntime *rt = cx->runtime;
    if (rt->spsProfiler.enabled()) {
        rt->spsProfiler.enter(cx, script, maybeFun);
        JS_ASSERT_IF(!fp->isGeneratorFrame(), !fp->hasPushedSPSFrame());
        fp->setPushedSPSFrame();
    }

    return ok;
}

inline bool
Probes::exitScript(JSContext *cx, JSScript *script, JSFunction *maybeFun,
                   StackFrame *fp)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_FUNCTION_RETURN_ENABLED())
        DTraceExitJSFun(cx, maybeFun, script);
#endif
#ifdef MOZ_TRACE_JSCALLS
    cx->doFunctionCallback(maybeFun, script, 0);
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWExitJSFun(cx, maybeFun, script, 0))
        ok = false;
#endif

    JSRuntime *rt = cx->runtime;
    




    if ((fp == NULL && rt->spsProfiler.enabled()) ||
        (fp != NULL && fp->hasPushedSPSFrame()))
    {
        rt->spsProfiler.exit(cx, script, maybeFun);
    }
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
Probes::objectResizeActive()
{
#ifdef MOZ_ETW
    if (ProfilingActive)
        return true;
#endif

    return false;
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
Probes::compileScriptBegin(const char *filename, int lineno)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCompileScriptBegin(filename, lineno))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::compileScriptEnd(const char *filename, int lineno)
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
Probes::GCStart()
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCStart())
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCEnd()
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCEnd())
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCStartMarkPhase()
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCStartMarkPhase())
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCEndMarkPhase()
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCEndMarkPhase())
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCStartSweepPhase()
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCStartSweepPhase())
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCEndSweepPhase()
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCEndSweepPhase())
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

} 






#endif 
