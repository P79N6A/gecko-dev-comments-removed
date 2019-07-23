


































#ifdef INCLUDE_MOZILLA_DTRACE
#include "javascript-trace.h"
#endif
#include "jspubtd.h"
#include "jsprvtd.h"

#ifndef _JSDTRACEF_H
#define _JSDTRACEF_H

JS_BEGIN_EXTERN_C

extern void
jsdtrace_function_entry(JSContext *cx, JSStackFrame *fp, const JSFunction *fun);

extern void
jsdtrace_function_info(JSContext *cx, JSStackFrame *fp, JSStackFrame *dfp,
                       const JSFunction *fun);

extern void
jsdtrace_function_args(JSContext *cx, JSStackFrame *fp, const JSFunction *fun,
                       jsuint argc, jsval *argv);

extern void
jsdtrace_function_rval(JSContext *cx, JSStackFrame *fp, const JSFunction *fun,
                       jsval rval);

extern void
jsdtrace_function_return(JSContext *cx, JSStackFrame *fp, const JSFunction *fun);

extern void
jsdtrace_object_create_start(JSStackFrame *fp, const JSClass *clasp);

extern void
jsdtrace_object_create_done(JSStackFrame *fp, const JSClass *clasp);

extern void
jsdtrace_object_create(JSContext *cx, const JSClass *clasp, const JSObject *obj);

extern void
jsdtrace_object_finalize(const JSObject *obj);

extern void
jsdtrace_execute_start(const JSScript *script);

extern void
jsdtrace_execute_done(const JSScript *script);

JS_END_EXTERN_C

namespace js {

class DTrace {
  public:
    



    static void enterJSFun(JSContext *cx, JSStackFrame *fp, const JSFunction *fun,
                           JSStackFrame *dfp, jsuint argc, jsval *argv, jsval *lval = NULL);
    static void exitJSFun(JSContext *cx, JSStackFrame *fp, const JSFunction *fun,
                          jsval rval, jsval *lval = NULL);

    static void finalizeObject(const JSObject *obj);

    class ExecutionScope {
        const JSScript *script;
        void startExecution();
        void endExecution();
      public:
        explicit ExecutionScope(const JSScript *script) : script(script) { startExecution(); }
        ~ExecutionScope() { endExecution(); }
    };

};

inline void
DTrace::enterJSFun(JSContext *cx, JSStackFrame *fp, const JSFunction *fun,
                   JSStackFrame *dfp, jsuint argc, jsval *argv, jsval *lval)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (!lval || VALUE_IS_FUNCTION(cx, *lval)) {
        if (JAVASCRIPT_FUNCTION_ENTRY_ENABLED())
            jsdtrace_function_entry(cx, fp, fun);
        if (JAVASCRIPT_FUNCTION_INFO_ENABLED())
            jsdtrace_function_info(cx, fp, dfp, fun);
        if (JAVASCRIPT_FUNCTION_ARGS_ENABLED())
            jsdtrace_function_args(cx, fp, fun, argc, argv);
    }
#endif
}

inline void
DTrace::exitJSFun(JSContext *cx, JSStackFrame *fp, const JSFunction *fun,
                  jsval rval, jsval *lval)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (!lval || VALUE_IS_FUNCTION(cx, *lval)) {
        if (JAVASCRIPT_FUNCTION_RVAL_ENABLED())
            jsdtrace_function_rval(cx, fp, fun, rval);
        if (JAVASCRIPT_FUNCTION_RETURN_ENABLED())
            jsdtrace_function_return(cx, fp, fun);
    }
#endif
}

inline void
DTrace::finalizeObject(const JSObject *obj)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_FINALIZE_ENABLED())
        jsdtrace_object_finalize(obj);
#endif
}

inline void
DTrace::ExecutionScope::startExecution()
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_START_ENABLED())
        jsdtrace_execute_start(script);
#endif
}

inline void
DTrace::ExecutionScope::endExecution()
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_DONE_ENABLED())
        jsdtrace_execute_done(script);
#endif
}

} 
    
#endif 
