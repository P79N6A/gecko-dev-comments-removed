


































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
#include "jsobjinlines.h"

#ifdef __APPLE__
#include "sharkctl.h"
#endif

#include "jsprobes.h"
#include <sys/types.h>

#define TYPEOF(cx,v)    (JSVAL_IS_NULL(v) ? JSTYPE_NULL : JS_TypeOfValue(cx,v))

using namespace js;

const char Probes::nullName[] = "(null)";

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





















#if 0
static void *
jsprobes_jsvaltovoid(JSContext *cx, const js::Value &argval)
{
    if (argval.isNull())
        return (void *)JS_TYPE_STR(JSTYPE_NULL);

    if (argval.isUndefined())
        return (void *)JS_TYPE_STR(JSTYPE_VOID);

    if (argval.isBoolean())
        return (void *)argval.toBoolean();

    if (argval.isNumber()) {
        if (argval.isInt32())
            return (void *)argval.toInt32();
        
        
    }

    return argval.toGCThing();
}
#endif

const char *
Probes::FunctionName(JSContext *cx, const JSFunction *fun, JSAutoByteString *bytes)
{
    if (!fun)
        return nullName;

    JSAtom *atom = fun->atom;
    if (!atom) {
        




        return nullName;
    }

    return bytes->encode(cx, atom) ? bytes->ptr() : nullName;
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
#ifdef MOZ_SHARK
    if (Shark::Start())
        return true;
#endif
    return false;
}

void
Probes::stopProfiling()
{
#ifdef MOZ_SHARK
    Shark::Stop();
#endif
}
