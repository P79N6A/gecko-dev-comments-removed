





#ifndef vm_Probes_inl_h
#define vm_Probes_inl_h

#include "vm/Probes.h"

#include "jscntxt.h"

namespace js {






inline bool
probes::CallTrackingActive(JSContext *cx)
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
probes::WantNativeAddressInfo(JSContext *cx)
{
    return cx->reportGranularity >= JITREPORT_GRANULARITY_FUNCTION &&
           JITGranularityRequested(cx) >= JITREPORT_GRANULARITY_FUNCTION;
}

inline bool
probes::EnterScript(JSContext *cx, JSScript *script, JSFunction *maybeFun,
                    InterpreterFrame *fp)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_FUNCTION_ENTRY_ENABLED())
        DTraceEnterJSFun(cx, maybeFun, script);
#endif
#ifdef MOZ_TRACE_JSCALLS
    cx->doFunctionCallback(maybeFun, script, 1);
#endif

    JSRuntime *rt = cx->runtime();
    if (rt->spsProfiler.enabled()) {
        if (!rt->spsProfiler.enter(script, maybeFun))
            return false;
        JS_ASSERT_IF(!fp->isGeneratorFrame(), !fp->hasPushedSPSFrame());
        fp->setPushedSPSFrame();
    }

    return true;
}

inline void
probes::ExitScript(JSContext *cx, JSScript *script, JSFunction *maybeFun, bool popSPSFrame)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_FUNCTION_RETURN_ENABLED())
        DTraceExitJSFun(cx, maybeFun, script);
#endif
#ifdef MOZ_TRACE_JSCALLS
    cx->doFunctionCallback(maybeFun, script, 0);
#endif

    if (popSPSFrame)
        cx->runtime()->spsProfiler.exit(script, maybeFun);
}

inline bool
probes::StartExecution(JSScript *script)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_START_ENABLED())
        JAVASCRIPT_EXECUTE_START((script->filename() ? (char *)script->filename() : nullName),
                                 script->lineno());
#endif

    return ok;
}

inline bool
probes::StopExecution(JSScript *script)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_DONE_ENABLED())
        JAVASCRIPT_EXECUTE_DONE((script->filename() ? (char *)script->filename() : nullName),
                                script->lineno());
#endif

    return ok;
}

} 

#endif 
