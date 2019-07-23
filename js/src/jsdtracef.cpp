


































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

char *
jsdtrace_funcclass_name(JSFunction *fun)
{
    return (!FUN_INTERPRETED(fun) &&
            !(fun->flags & JSFUN_TRACEABLE) &&
            FUN_CLASP(fun))
           ? (char *)FUN_CLASP(fun)->name
           : dempty;
}

char *
jsdtrace_filename(JSStackFrame *fp)
{
    while (fp && fp->script == NULL)
        fp = fp->down;
    return (fp && fp->script && fp->script->filename)
           ? (char *)fp->script->filename
           : dempty;
}

int
jsdtrace_linenumber(JSContext *cx, JSStackFrame *fp)
{
    while (fp && fp->script == NULL)
        fp = fp->down;
    return (fp && fp->regs)
           ? (int) js_PCToLineNumber(cx, fp->script, fp->regs->pc)
           : -1;
}





















void *
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

char *
jsdtrace_function_name(JSContext *cx, JSStackFrame *fp, JSFunction *fun)
{
    JSAtom *atom;
    JSFrameRegs *regs;
    JSScript *script;
    jsbytecode *pc;
    char *name;

    atom = fun->atom;
    if (!atom) {
        if (fp->fun != fun || !fp->down)
            return dempty;

        regs = fp->down->regs;
        if (!regs)
            return dempty;

        




        pc = regs->pc;
        script = fp->down->script;
        switch ((JSOp) *pc) {
          case JSOP_CALL:
          case JSOP_EVAL:
            JS_ASSERT(fp->argv == regs->sp - (int)GET_ARGC(pc));

            



            break;
          default: ;
        }

        switch ((JSOp) *pc) {
          case JSOP_CALLNAME:
          case JSOP_CALLPROP:
          case JSOP_NAME:
          case JSOP_SETNAME:
          case JSOP_GETPROP:
          case JSOP_SETPROP:
            GET_ATOM_FROM_BYTECODE(script, pc, 0, atom);
            break;

          case JSOP_CALLELEM:
          case JSOP_GETELEM:
          case JSOP_SETELEM:
          case JSOP_CALLGVAR:
          case JSOP_GETGVAR:
          case JSOP_SETGVAR:
          case JSOP_CALLARG:
          case JSOP_CALLLOCAL:
            
            

          default:
            return dempty;
        }
    }

    name = (char *)js_GetStringBytes(cx, ATOM_TO_STRING(atom));
    return name ? name : dempty;
}








void
jsdtrace_function_entry(JSContext *cx, JSStackFrame *fp, JSFunction *fun)
{
    JAVASCRIPT_FUNCTION_ENTRY(
        jsdtrace_filename(fp),
        jsdtrace_funcclass_name(fun),
        jsdtrace_function_name(cx, fp, fun)
    );
}

void
jsdtrace_function_info(JSContext *cx, JSStackFrame *fp, JSStackFrame *dfp,
                       JSFunction *fun)
{
    JAVASCRIPT_FUNCTION_INFO(
        jsdtrace_filename(fp),
        jsdtrace_funcclass_name(fun),
        jsdtrace_function_name(cx, fp, fun),
        fp->script->lineno,
        jsdtrace_filename(dfp),
        jsdtrace_linenumber(cx, dfp)
    );
}

void
jsdtrace_function_args(JSContext *cx, JSStackFrame *fp, JSFunction *fun)
{
    JAVASCRIPT_FUNCTION_ARGS(
        jsdtrace_filename(fp),
        jsdtrace_funcclass_name(fun),
        jsdtrace_function_name(cx, fp, fun),
        fp->argc, (void *)fp->argv,
        (fp->argc > 0) ? jsdtrace_jsvaltovoid(cx, fp->argv[0]) : 0,
        (fp->argc > 1) ? jsdtrace_jsvaltovoid(cx, fp->argv[1]) : 0,
        (fp->argc > 2) ? jsdtrace_jsvaltovoid(cx, fp->argv[2]) : 0,
        (fp->argc > 3) ? jsdtrace_jsvaltovoid(cx, fp->argv[3]) : 0,
        (fp->argc > 4) ? jsdtrace_jsvaltovoid(cx, fp->argv[4]) : 0
    );
}

void
jsdtrace_function_rval(JSContext *cx, JSStackFrame *fp, JSFunction *fun)
{
    JAVASCRIPT_FUNCTION_RVAL(
        jsdtrace_filename(fp),
        jsdtrace_funcclass_name(fun),
        jsdtrace_function_name(cx, fp, fun),
        jsdtrace_linenumber(cx, fp), (void *)fp->rval,
        jsdtrace_jsvaltovoid(cx, fp->rval)
    );
}

void
jsdtrace_function_return(JSContext *cx, JSStackFrame *fp, JSFunction *fun)
{
    JAVASCRIPT_FUNCTION_RETURN(
        jsdtrace_filename(fp),
        jsdtrace_funcclass_name(fun),
        jsdtrace_function_name(cx, fp, fun)
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
        jsdtrace_linenumber(cx, cx->fp)
    );
}

void
jsdtrace_object_finalize(JSObject *obj)
{
    JSClass *clasp;

    clasp = LOCKED_OBJ_GET_CLASS(obj);

    
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
