


































#include "jsapi.h"
#include "jsutil.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jsfun.h"
#include "jsinterp.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jsstr.h"

#ifdef __APPLE__
#include "sharkctl.h"
#endif

#include "jsprobes.h"
#include <sys/types.h>

#define TYPEOF(cx,v)    (JSVAL_IS_NULL(v) ? JSTYPE_NULL : JS_TypeOfValue(cx,v))

using namespace js;

const char Probes::nullName[] = "(null)";
const char Probes::anonymousName[] = "(anonymous)";

bool Probes::ProfilingActive = true;

bool
Probes::controlProfilers(JSContext *cx, bool toState)
{
    JSBool ok = JS_TRUE;
#if defined(MOZ_CALLGRIND) || defined(MOZ_VTUNE)
    jsval dummy;
#endif

    if (! ProfilingActive && toState) {
#if defined(MOZ_SHARK) && defined(__APPLE__)
        if (!Shark::Start())
            ok = JS_FALSE;
#endif
#ifdef MOZ_CALLGRIND
        if (! js_StartCallgrind(cx, 0, &dummy))
            ok = JS_FALSE;
#endif
#ifdef MOZ_VTUNE
        if (! js_ResumeVtune(cx, 0, &dummy))
            ok = JS_FALSE;
#endif
    } else if (ProfilingActive && ! toState) {
#if defined(MOZ_SHARK) && defined(__APPLE__)
        Shark::Stop();
#endif
#ifdef MOZ_CALLGRIND
        if (! js_StopCallgrind(cx, 0, &dummy))
            ok = JS_FALSE;
#endif
#ifdef MOZ_VTUNE
        if (! js_PauseVtune(cx, 0, &dummy))
            ok = JS_FALSE;
#endif
    }

    ProfilingActive = toState;

    return ok;
}

void
Probes::current_location(JSContext *cx, int* lineno, char const **filename)
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

const char *
Probes::FunctionClassname(const JSFunction *fun)
{
    return (fun && !FUN_INTERPRETED(fun) && !(fun->flags & JSFUN_TRCINFO) && FUN_CLASP(fun))
           ? (char *)FUN_CLASP(fun)->name
           : nullName;
}

const char *
Probes::ScriptFilename(JSScript *script)
{
    return (script && script->filename) ? (char *)script->filename : nullName;
}

int
Probes::FunctionLineNumber(JSContext *cx, const JSFunction *fun)
{
    if (fun && FUN_INTERPRETED(fun))
        return (int) JS_GetScriptBaseLineNumber(cx, FUN_SCRIPT(fun));

    return 0;
}

#ifdef INCLUDE_MOZILLA_DTRACE







void
Probes::enterJSFunImpl(JSContext *cx, JSFunction *fun, JSScript *script)
{
    JSAutoByteString funNameBytes;
    JAVASCRIPT_FUNCTION_ENTRY(ScriptFilename(script), FunctionClassname(fun),
                              FunctionName(cx, fun, &funNameBytes));
}

void
Probes::handleFunctionReturn(JSContext *cx, JSFunction *fun, JSScript *script)
{
    JSAutoByteString funNameBytes;
    JAVASCRIPT_FUNCTION_RETURN(ScriptFilename(script), FunctionClassname(fun),
                               FunctionName(cx, fun, &funNameBytes));
}

#endif

bool
Probes::startProfiling()
{
#if defined(MOZ_SHARK) && defined(__APPLE__)
    if (Shark::Start())
        return true;
#endif
    return false;
}

void
Probes::stopProfiling()
{
#if defined(MOZ_SHARK) && defined(__APPLE__)
    Shark::Stop();
#endif
}
