





#include "vm/Probes-inl.h"

#include "jscntxt.h"

#ifdef INCLUDE_MOZILLA_DTRACE
#include "jsscriptinlines.h"
#endif

#define TYPEOF(cx,v)    (v.isNull() ? JSTYPE_NULL : JS_TypeOfValue(cx,v))

using namespace js;

const char probes::nullName[] = "(null)";
const char probes::anonymousName[] = "(anonymous)";

bool probes::ProfilingActive = true;

#ifdef INCLUDE_MOZILLA_DTRACE
static const char*
ScriptFilename(const JSScript* script)
{
    if (!script)
        return probes::nullName;
    if (!script->filename())
        return probes::anonymousName;
    return script->filename();
}

static const char*
FunctionName(JSContext* cx, JSFunction* fun, JSAutoByteString* bytes)
{
    if (!fun)
        return probes::nullName;
    if (!fun->displayAtom())
        return probes::anonymousName;
    return bytes->encodeLatin1(cx, fun->displayAtom()) ? bytes->ptr() : probes::nullName;
}








void
probes::DTraceEnterJSFun(JSContext* cx, JSFunction* fun, JSScript* script)
{
    JSAutoByteString funNameBytes;
    JAVASCRIPT_FUNCTION_ENTRY(ScriptFilename(script), probes::nullName,
                              FunctionName(cx, fun, &funNameBytes));
}

void
probes::DTraceExitJSFun(JSContext* cx, JSFunction* fun, JSScript* script)
{
    JSAutoByteString funNameBytes;
    JAVASCRIPT_FUNCTION_RETURN(ScriptFilename(script), probes::nullName,
                               FunctionName(cx, fun, &funNameBytes));
}
#endif
