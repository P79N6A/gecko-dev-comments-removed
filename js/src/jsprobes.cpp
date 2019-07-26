






#include "jsapi.h"
#include "jsutil.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jsfun.h"
#include "jsinterp.h"
#include "jsobj.h"
#include "jsprobes.h"
#include "jsscript.h"
#include "jsstr.h"

#include "methodjit/Compiler.h"

#include "jsobjinlines.h"

#define TYPEOF(cx,v)    (JSVAL_IS_NULL(v) ? JSTYPE_NULL : JS_TypeOfValue(cx,v))

using namespace js;

const char Probes::nullName[] = "(null)";
const char Probes::anonymousName[] = "(anonymous)";

bool Probes::ProfilingActive = true;

Probes::JITReportGranularity
Probes::JITGranularityRequested(JSContext *cx)
{
    if (cx->runtime->spsProfiler.enabled())
        return JITREPORT_GRANULARITY_LINE;
    return JITREPORT_GRANULARITY_NONE;
}

#ifdef JS_METHODJIT

bool
Probes::registerMJITCode(JSContext *cx, js::mjit::JITChunk *chunk,
                         js::mjit::JSActiveFrame *outerFrame,
                         js::mjit::JSActiveFrame **inlineFrames)
{
    if (cx->runtime->spsProfiler.enabled() &&
        !cx->runtime->spsProfiler.registerMJITCode(chunk, outerFrame, inlineFrames))
    {
        return false;
    }

    return true;
}

void
Probes::discardMJITCode(FreeOp *fop, mjit::JITScript *jscr, mjit::JITChunk *chunk, void* address)
{
    if (fop->runtime()->spsProfiler.enabled())
        fop->runtime()->spsProfiler.discardMJITCode(jscr, chunk, address);
}

bool
Probes::registerICCode(JSContext *cx,
                       mjit::JITChunk *chunk, RawScript script, jsbytecode* pc,
                       void *start, size_t size)
{
    if (cx->runtime->spsProfiler.enabled() &&
        !cx->runtime->spsProfiler.registerICCode(chunk, script, pc, start, size))
    {
        return false;
    }
    return true;
}
#endif


void
Probes::discardExecutableRegion(void *start, size_t size)
{
    



}

#ifdef INCLUDE_MOZILLA_DTRACE
static const char *
ScriptFilename(const RawScript script)
{
    if (!script)
        return Probes::nullName;
    if (!script->filename())
        return Probes::anonymousName;
    return script->filename();
}

static const char *
FunctionName(JSContext *cx, RawFunction fun, JSAutoByteString* bytes)
{
    if (!fun)
        return Probes::nullName;
    if (!fun->displayAtom())
        return Probes::anonymousName;
    return bytes->encodeLatin1(cx, fun->displayAtom()) ? bytes->ptr() : Probes::nullName;
}








void
Probes::DTraceEnterJSFun(JSContext *cx, RawFunction fun, RawScript script)
{
    JSAutoByteString funNameBytes;
    JAVASCRIPT_FUNCTION_ENTRY(ScriptFilename(script), Probes::nullName,
                              FunctionName(cx, fun, &funNameBytes));
}

void
Probes::DTraceExitJSFun(JSContext *cx, RawFunction fun, RawScript script)
{
    JSAutoByteString funNameBytes;
    JAVASCRIPT_FUNCTION_RETURN(ScriptFilename(script), Probes::nullName,
                               FunctionName(cx, fun, &funNameBytes));
}
#endif
