


































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

static char dempty[] = "<null>";

static char *
jsdtrace_fun_classname(JSFunction *fun)
{
    return (fun &&
            !FUN_INTERPRETED(fun) &&
            !(fun->flags & JSFUN_TRCINFO) &&
            FUN_CLASP(fun))
           ? (char *)FUN_CLASP(fun)->name
           : dempty;
}

static char *
jsdtrace_filename(JSStackFrame *fp)
{
    return (fp && fp->script && fp->script->filename)
           ? (char *)fp->script->filename
           : dempty;
}

static int
jsdtrace_fun_linenumber(JSContext *cx, JSFunction *fun)
{
    if (fun && FUN_INTERPRETED(fun))
        return (int) JS_GetScriptBaseLineNumber(cx, FUN_SCRIPT(fun));

    return 0;
}

int
jsdtrace_frame_linenumber(JSContext *cx, JSStackFrame *fp)
{
    if (fp && fp->regs)
        return (int) js_FramePCToLineNumber(cx, fp);

    return 0;
}





















static void *
jsdtrace_jsvaltovoid(JSContext *cx, jsval argval)
{
    JSType type = TYPEOF(cx, argval);

    switch (type) {
      case JSTYPE_NULL:
      case JSTYPE_VOID:
        return (void *)JS_TYPE_STR(type);

      case JSTYPE_BOOLEAN:
        return (void *)JSVAL_TO_BOOLEAN(argval);

      case JSTYPE_STRING:
        return (void *)js_GetStringBytes(cx, JSVAL_TO_STRING(argval));

      case JSTYPE_NUMBER:
        if (JSVAL_IS_INT(argval))
            return (void *)JSVAL_TO_INT(argval);
        return JSVAL_TO_DOUBLE(argval);

      default:
        return JSVAL_TO_GCTHING(argval);
    }
    
}

static char *
jsdtrace_fun_name(JSContext *cx, JSFunction *fun)
{
    JSAtom *atom;
    char *name;

    if (!fun)
        return dempty;

    atom = fun->atom;
    if (!atom) {
        




        return dempty;
    }

    name = (char *)js_GetStringBytes(cx, ATOM_TO_STRING(atom));
    return name ? name : dempty;
}








void
jsdtrace_function_entry(JSContext *cx, JSStackFrame *fp, JSFunction *fun)
{
    JAVASCRIPT_FUNCTION_ENTRY(
        jsdtrace_filename(fp),
        jsdtrace_fun_classname(fun),
        jsdtrace_fun_name(cx, fun)
    );
}

void
jsdtrace_function_info(JSContext *cx, JSStackFrame *fp, JSStackFrame *dfp,
                       JSFunction *fun)
{
    JAVASCRIPT_FUNCTION_INFO(
        jsdtrace_filename(fp),
        jsdtrace_fun_classname(fun),
        jsdtrace_fun_name(cx, fun),
        jsdtrace_fun_linenumber(cx, fun),
        jsdtrace_filename(dfp),
        jsdtrace_frame_linenumber(cx, dfp)
    );
}

void
jsdtrace_function_args(JSContext *cx, JSStackFrame *fp, JSFunction *fun, jsuint argc, jsval *argv)
{
    JAVASCRIPT_FUNCTION_ARGS(
        jsdtrace_filename(fp),
        jsdtrace_fun_classname(fun),
        jsdtrace_fun_name(cx, fun),
        argc, (void *)argv,
        (argc > 0) ? jsdtrace_jsvaltovoid(cx, argv[0]) : 0,
        (argc > 1) ? jsdtrace_jsvaltovoid(cx, argv[1]) : 0,
        (argc > 2) ? jsdtrace_jsvaltovoid(cx, argv[2]) : 0,
        (argc > 3) ? jsdtrace_jsvaltovoid(cx, argv[3]) : 0,
        (argc > 4) ? jsdtrace_jsvaltovoid(cx, argv[4]) : 0
    );
}

void
jsdtrace_function_rval(JSContext *cx, JSStackFrame *fp, JSFunction *fun, jsval *rval)
{
    JAVASCRIPT_FUNCTION_RVAL(
        jsdtrace_filename(fp),
        jsdtrace_fun_classname(fun),
        jsdtrace_fun_name(cx, fun),
        jsdtrace_fun_linenumber(cx, fun),
        (void *)rval,
        jsdtrace_jsvaltovoid(cx, *rval)
    );
}

void
jsdtrace_function_return(JSContext *cx, JSStackFrame *fp, JSFunction *fun)
{
    JAVASCRIPT_FUNCTION_RETURN(
        jsdtrace_filename(fp),
        jsdtrace_fun_classname(fun),
        jsdtrace_fun_name(cx, fun)
    );
}

void
jsdtrace_object_create_start(JSStackFrame *fp, JSClass *clasp)
{
    JAVASCRIPT_OBJECT_CREATE_START(jsdtrace_filename(fp), (char *)clasp->name);
}

void
jsdtrace_object_create_done(JSStackFrame *fp, JSClass *clasp)
{
    JAVASCRIPT_OBJECT_CREATE_DONE(jsdtrace_filename(fp), (char *)clasp->name);
}

void
jsdtrace_object_create(JSContext *cx, JSClass *clasp, JSObject *obj)
{
    JAVASCRIPT_OBJECT_CREATE(
        jsdtrace_filename(cx->fp),
        (char *)clasp->name,
        (uintptr_t)obj,
        jsdtrace_frame_linenumber(cx, cx->fp)
    );
}

void
jsdtrace_object_finalize(JSObject *obj)
{
    JSClass *clasp;

    clasp = obj->getClass();

    
    JAVASCRIPT_OBJECT_FINALIZE(NULL, (char *)clasp->name, (uintptr_t)obj);
}

void
jsdtrace_execute_start(JSScript *script)
{
    JAVASCRIPT_EXECUTE_START(
        script->filename ? (char *)script->filename : dempty,
        script->lineno
    );
}

void
jsdtrace_execute_done(JSScript *script)
{
    JAVASCRIPT_EXECUTE_DONE(
        script->filename ? (char *)script->filename : dempty,
        script->lineno
    );
}
