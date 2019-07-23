


































#ifdef INCLUDE_MOZILLA_DTRACE
#include "javascript-trace.h"
#endif
#include "jspubtd.h"
#include "jsprvtd.h"

#ifndef _JSDTRACEF_H
#define _JSDTRACEF_H

JS_BEGIN_EXTERN_C

extern void
jsdtrace_function_entry(JSContext *cx, JSStackFrame *fp, JSFunction *fun);

extern void
jsdtrace_function_info(JSContext *cx, JSStackFrame *fp, JSStackFrame *dfp,
                       JSFunction *fun);

extern void
jsdtrace_function_args(JSContext *cx, JSStackFrame *fp, JSFunction *fun, jsuint argc, jsval *argv);

extern void
jsdtrace_function_rval(JSContext *cx, JSStackFrame *fp, JSFunction *fun, jsval *rval);

extern void
jsdtrace_function_return(JSContext *cx, JSStackFrame *fp, JSFunction *fun);

extern void
jsdtrace_object_create_start(JSStackFrame *fp, JSClass *clasp);

extern void
jsdtrace_object_create_done(JSStackFrame *fp, JSClass *clasp);

extern void
jsdtrace_object_create(JSContext *cx, JSClass *clasp, JSObject *obj);

extern void
jsdtrace_object_finalize(JSObject *obj);

extern void
jsdtrace_execute_start(JSScript *script);

extern void
jsdtrace_execute_done(JSScript *script);

JS_END_EXTERN_C

namespace js {

class DTrace {
  public:
    



    static void enterJSFun(const JSContext *cx, const JSStackFrame *fp, const JSFunction *fun,
                           const JSStackFrame *dfp, jsuint argc, const jsval *argv,
                           const jsval *lval = NULL);
    static void exitJSFun(const JSContext *cx, const JSStackFrame *fp, const JSFunction *fun,
                          const jsval *rval, const jsval *lval = NULL);

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
DTrace::enterJSFun(const JSContext *cx, const JSStackFrame *fp, const JSFunction *fun,
                   const JSStackFrame *dfp, jsuint argc, const jsval *argv,
                   const jsval *lval)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (!lval || VALUE_IS_FUNCTION(cx, lval)) {
        if (JAVASCRIPT_FUNCTION_ENTRY_ENABLED())
            jsdtrace_function_entry(cx, &frame, fun);
        if (JAVASCRIPT_FUNCTION_INFO_ENABLED())
            jsdtrace_function_info(cx, &frame, frame.down, fun);
        if (JAVASCRIPT_FUNCTION_ARGS_ENABLED())
            jsdtrace_function_args(cx, &frame, fun, frame.argc, frame.argv);
    }
#endif
}

inline void
DTrace::exitJSFun(const JSContext *cx, const JSStackFrame *fp, const JSFunction *fun,
                  const jsval *rval, const jsval *lval)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (!lval || VALUE_IS_FUNCTION(cx, lval)) {
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
    if (JAVASCRIPT_EXECUTE_END_ENABLED())
        jsdtrace_execute_done(script);
#endif
}

} 
    
#endif 
