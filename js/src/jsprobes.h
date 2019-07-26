





#ifndef _JSPROBES_H
#define _JSPROBES_H

#ifdef INCLUDE_MOZILLA_DTRACE
#include "javascript-trace.h"
#endif
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "jsscript.h"

namespace js {

namespace mjit {
struct NativeAddressInfo;
struct JSActiveFrame;
struct JITChunk;
}

namespace Probes {































extern bool ProfilingActive;

extern const char nullName[];
extern const char anonymousName[];






bool callTrackingActive(JSContext *);





bool wantNativeAddressInfo(JSContext *);


bool enterScript(JSContext *, JSScript *, JSFunction *, StackFrame *);


bool exitScript(JSContext *, JSScript *, JSFunction *, AbstractFramePtr);
bool exitScript(JSContext *, JSScript *, JSFunction *, StackFrame *);


bool startExecution(JSScript *script);


bool stopExecution(JSScript *script);




bool createObject(JSContext *cx, JSObject *obj);





bool finalizeObject(JSObject *obj);



enum JITReportGranularity {
    JITREPORT_GRANULARITY_NONE = 0,
    JITREPORT_GRANULARITY_FUNCTION = 1,
    JITREPORT_GRANULARITY_LINE = 2,
    JITREPORT_GRANULARITY_OP = 3
};




JITReportGranularity
JITGranularityRequested(JSContext *cx);

#ifdef JS_METHODJIT



bool
registerMJITCode(JSContext *cx, js::mjit::JITChunk *chunk,
                 mjit::JSActiveFrame *outerFrame,
                 mjit::JSActiveFrame **inlineFrames);




void
discardMJITCode(FreeOp *fop, mjit::JITScript *jscr, mjit::JITChunk *chunk, void* address);




bool
registerICCode(JSContext *cx,
               mjit::JITChunk *chunk, JSScript *script, jsbytecode* pc,
               void *start, size_t size);
#endif 





void
discardExecutableRegion(void *start, size_t size);







void DTraceEnterJSFun(JSContext *cx, JSFunction *fun, JSScript *script);
void DTraceExitJSFun(JSContext *cx, JSFunction *fun, JSScript *script);

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
    return false;
}

inline bool
Probes::wantNativeAddressInfo(JSContext *cx)
{
    return (cx->reportGranularity >= JITREPORT_GRANULARITY_FUNCTION &&
            JITGranularityRequested(cx) >= JITREPORT_GRANULARITY_FUNCTION);
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
                   AbstractFramePtr fp)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_FUNCTION_RETURN_ENABLED())
        DTraceExitJSFun(cx, maybeFun, script);
#endif
#ifdef MOZ_TRACE_JSCALLS
    cx->doFunctionCallback(maybeFun, script, 0);
#endif

    JSRuntime *rt = cx->runtime;
    




    if ((!fp && rt->spsProfiler.enabled()) || (fp && fp.hasPushedSPSFrame()))
        rt->spsProfiler.exit(cx, script, maybeFun);
    return ok;
}

inline bool
Probes::exitScript(JSContext *cx, JSScript *script, JSFunction *maybeFun,
                   StackFrame *fp)
{
    return Probes::exitScript(cx, script, maybeFun, fp ? AbstractFramePtr(fp) : AbstractFramePtr());
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

    return ok;
}
inline bool
Probes::startExecution(JSScript *script)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_START_ENABLED())
        JAVASCRIPT_EXECUTE_START((script->filename() ? (char *)script->filename() : nullName),
                                 script->lineno);
#endif

    return ok;
}

inline bool
Probes::stopExecution(JSScript *script)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_DONE_ENABLED())
        JAVASCRIPT_EXECUTE_DONE((script->filename() ? (char *)script->filename() : nullName),
                                script->lineno);
#endif

    return ok;
}

} 






#endif 
