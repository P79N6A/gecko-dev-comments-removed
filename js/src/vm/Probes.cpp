





#include "vm/Probes-inl.h"

#include "jscntxt.h"

#ifdef INCLUDE_MOZILLA_DTRACE
#include "jsscriptinlines.h" 
#endif

#define TYPEOF(cx,v)    (JSVAL_IS_NULL(v) ? JSTYPE_NULL : JS_TypeOfValue(cx,v))

using namespace js;

const char Probes::nullName[] = "(null)";
const char Probes::anonymousName[] = "(anonymous)";

bool Probes::ProfilingActive = true;

Probes::JITReportGranularity
Probes::JITGranularityRequested(JSContext *cx)
{
    if (cx->runtime()->spsProfiler.enabled())
        return JITREPORT_GRANULARITY_LINE;
    return JITREPORT_GRANULARITY_NONE;
}


void
Probes::discardExecutableRegion(void *start, size_t size)
{
    



}

#ifdef INCLUDE_MOZILLA_DTRACE
static const char *
ScriptFilename(const JSScript *script)
{
    if (!script)
        return Probes::nullName;
    if (!script->filename())
        return Probes::anonymousName;
    return script->filename();
}

static const char *
FunctionName(JSContext *cx, JSFunction *fun, JSAutoByteString* bytes)
{
    if (!fun)
        return Probes::nullName;
    if (!fun->displayAtom())
        return Probes::anonymousName;
    return bytes->encodeLatin1(cx, fun->displayAtom()) ? bytes->ptr() : Probes::nullName;
}








void
Probes::DTraceEnterJSFun(JSContext *cx, JSFunction *fun, JSScript *script)
{
    JSAutoByteString funNameBytes;
    JAVASCRIPT_FUNCTION_ENTRY(ScriptFilename(script), Probes::nullName,
                              FunctionName(cx, fun, &funNameBytes));
}

void
Probes::DTraceExitJSFun(JSContext *cx, JSFunction *fun, JSScript *script)
{
    JSAutoByteString funNameBytes;
    JAVASCRIPT_FUNCTION_RETURN(ScriptFilename(script), Probes::nullName,
                               FunctionName(cx, fun, &funNameBytes));
}
#endif
