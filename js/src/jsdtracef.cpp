


































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

#include "jsdtracef.h"
#include <sys/types.h>

#define TYPEOF(cx,v)    (JSVAL_IS_NULL(v) ? JSTYPE_NULL : JS_TypeOfValue(cx,v))

using namespace js;

static char dempty[] = "<null>";

static char *
jsdtrace_fun_classname(const JSFunction *fun)
{
    return (fun && !FUN_INTERPRETED(fun) && !(fun->flags & JSFUN_TRCINFO) && FUN_CLASP(fun))
           ? (char *)FUN_CLASP(fun)->name
           : dempty;
}

static char *
jsdtrace_filename(JSStackFrame *fp)
{
    return (fp && fp->script && fp->script->filename) ? (char *)fp->script->filename : dempty;
}

static int
jsdtrace_fun_linenumber(JSContext *cx, const JSFunction *fun)
{
    if (fun && FUN_INTERPRETED(fun))
        return (int) JS_GetScriptBaseLineNumber(cx, FUN_SCRIPT(fun));

    return 0;
}

static int
jsdtrace_frame_linenumber(JSContext *cx, JSStackFrame *fp)
{
    if (fp)
        return (int) js_FramePCToLineNumber(cx, fp);

    return 0;
}





















static void *
jsdtrace_jsvaltovoid(JSContext *cx, const js::Value &argval)
{
    if (argval.isNull())
        return (void *)JS_TYPE_STR(JSTYPE_NULL);

    if (argval.isUndefined())
        return (void *)JS_TYPE_STR(JSTYPE_VOID);

    if (argval.isBoolean())
        return (void *)argval.toBoolean();

    if (argval.isString())
        return (void *)js_GetStringBytes(cx, argval.toString());

    if (argval.isNumber()) {
        if (argval.isInt32())
            return (void *)argval.toInt32();
        
        
    }

    return argval.asGCThing();
}

static char *
jsdtrace_fun_name(JSContext *cx, const JSFunction *fun)
{
    if (!fun)
        return dempty;

    JSAtom *atom = fun->atom;
    if (!atom) {
        




        return dempty;
    }

    char *name = (char *)js_GetStringBytes(cx, ATOM_TO_STRING(atom));
    return name ? name : dempty;
}








void
DTrace::enterJSFunImpl(JSContext *cx, JSStackFrame *fp, const JSFunction *fun)
{
    JAVASCRIPT_FUNCTION_ENTRY(jsdtrace_filename(fp), jsdtrace_fun_classname(fun),
                              jsdtrace_fun_name(cx, fun));
}

void
DTrace::handleFunctionInfo(JSContext *cx, JSStackFrame *fp, JSStackFrame *dfp, JSFunction *fun)
{
    JAVASCRIPT_FUNCTION_INFO(jsdtrace_filename(fp), jsdtrace_fun_classname(fun),
                             jsdtrace_fun_name(cx, fun), jsdtrace_fun_linenumber(cx, fun),
                             jsdtrace_filename(dfp), jsdtrace_frame_linenumber(cx, dfp));
}

void
DTrace::handleFunctionArgs(JSContext *cx, JSStackFrame *fp, const JSFunction *fun, jsuint argc,
                           js::Value *argv)
{
    JAVASCRIPT_FUNCTION_ARGS(jsdtrace_filename(fp), jsdtrace_fun_classname(fun),
                             jsdtrace_fun_name(cx, fun), argc, (void *)argv,
                             (argc > 0) ? jsdtrace_jsvaltovoid(cx, argv[0]) : 0,
                             (argc > 1) ? jsdtrace_jsvaltovoid(cx, argv[1]) : 0,
                             (argc > 2) ? jsdtrace_jsvaltovoid(cx, argv[2]) : 0,
                             (argc > 3) ? jsdtrace_jsvaltovoid(cx, argv[3]) : 0,
                             (argc > 4) ? jsdtrace_jsvaltovoid(cx, argv[4]) : 0);
}

void
DTrace::handleFunctionRval(JSContext *cx, JSStackFrame *fp, JSFunction *fun, const js::Value &rval)
{
    JAVASCRIPT_FUNCTION_RVAL(jsdtrace_filename(fp), jsdtrace_fun_classname(fun),
                             jsdtrace_fun_name(cx, fun), jsdtrace_fun_linenumber(cx, fun),
                             NULL, jsdtrace_jsvaltovoid(cx, rval));
}

void
DTrace::handleFunctionReturn(JSContext *cx, JSStackFrame *fp, JSFunction *fun)
{
    JAVASCRIPT_FUNCTION_RETURN(jsdtrace_filename(fp), jsdtrace_fun_classname(fun),
                               jsdtrace_fun_name(cx, fun));
}

void
DTrace::ObjectCreationScope::handleCreationStart()
{
    JAVASCRIPT_OBJECT_CREATE_START(jsdtrace_filename(fp), (char *)clasp->name);
}

void
DTrace::ObjectCreationScope::handleCreationEnd()
{
    JAVASCRIPT_OBJECT_CREATE_DONE(jsdtrace_filename(fp), (char *)clasp->name);
}

void
DTrace::ObjectCreationScope::handleCreationImpl(JSObject *obj)
{
    JAVASCRIPT_OBJECT_CREATE(jsdtrace_filename(cx->fp), (char *)clasp->name, (uintptr_t)obj,
                             jsdtrace_frame_linenumber(cx, cx->fp));
}

void
DTrace::finalizeObjectImpl(JSObject *obj)
{
    Class *clasp = obj->getClass();

    
    JAVASCRIPT_OBJECT_FINALIZE(NULL, (char *)clasp->name, (uintptr_t)obj);
}

void
DTrace::ExecutionScope::startExecution()
{
    JAVASCRIPT_EXECUTE_START(script->filename ? (char *)script->filename : dempty,
                             script->lineno);
}

void
DTrace::ExecutionScope::endExecution()
{
    JAVASCRIPT_EXECUTE_DONE(script->filename ? (char *)script->filename : dempty, script->lineno);
}
