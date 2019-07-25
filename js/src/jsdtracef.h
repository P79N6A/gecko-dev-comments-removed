


































#ifdef INCLUDE_MOZILLA_DTRACE
#include "javascript-trace.h"
#endif
#include "jspubtd.h"
#include "jsprvtd.h"

#ifndef _JSDTRACEF_H
#define _JSDTRACEF_H

namespace js {

class DTrace {
    static void enterJSFunImpl(JSContext *cx, JSStackFrame *fp, const JSFunction *fun);
    static void handleFunctionInfo(JSContext *cx, JSStackFrame *fp, JSStackFrame *dfp,
                                   JSFunction *fun);
    static void handleFunctionArgs(JSContext *cx, JSStackFrame *fp, const JSFunction *fun,
                                   jsuint argc, jsval *argv);
    static void handleFunctionRval(JSContext *cx, JSStackFrame *fp, JSFunction *fun, jsval rval);
    static void handleFunctionReturn(JSContext *cx, JSStackFrame *fp, JSFunction *fun);
    static void finalizeObjectImpl(JSObject *obj);
  public:
    



    static void enterJSFun(JSContext *cx, JSStackFrame *fp, JSFunction *fun,
                           JSStackFrame *dfp, jsuint argc, jsval *argv, jsval *lval = NULL);
    static void exitJSFun(JSContext *cx, JSStackFrame *fp, JSFunction *fun, jsval rval,
                          jsval *lval = NULL);

    static void finalizeObject(JSObject *obj);

    class ExecutionScope {
        const JSScript *script;
        void startExecution();
        void endExecution();
      public:
        explicit ExecutionScope(JSScript *script);
        ~ExecutionScope();
    };

    class ObjectCreationScope {
        JSContext       * const cx;
        JSStackFrame    * const fp;
        JSClass         * const clasp;
        void handleCreationStart();
        void handleCreationImpl(JSObject *obj);
        void handleCreationEnd();
      public:
        ObjectCreationScope(JSContext *cx, JSStackFrame *fp, JSClass *clasp);
        void handleCreation(JSObject *obj);
        ~ObjectCreationScope();
    };

};

inline void
DTrace::enterJSFun(JSContext *cx, JSStackFrame *fp, JSFunction *fun, JSStackFrame *dfp,
                   jsuint argc, jsval *argv, jsval *lval)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (!lval || VALUE_IS_FUNCTION(cx, *lval)) {
        if (JAVASCRIPT_FUNCTION_ENTRY_ENABLED())
            enterJSFunImpl(cx, fp, fun);
        if (JAVASCRIPT_FUNCTION_INFO_ENABLED())
            handleFunctionInfo(cx, fp, dfp, fun);
        if (JAVASCRIPT_FUNCTION_ARGS_ENABLED())
            handleFunctionArgs(cx, fp, fun, argc, argv);
    }
#endif
}

inline void
DTrace::exitJSFun(JSContext *cx, JSStackFrame *fp, JSFunction *fun, jsval rval, jsval *lval)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (!lval || VALUE_IS_FUNCTION(cx, *lval)) {
        if (JAVASCRIPT_FUNCTION_RVAL_ENABLED())
            handleFunctionRval(cx, fp, fun, rval);
        if (JAVASCRIPT_FUNCTION_RETURN_ENABLED())
            handleFunctionReturn(cx, fp, fun);
    }
#endif
}

inline void
DTrace::finalizeObject(JSObject *obj)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_FINALIZE_ENABLED())
        finalizeObjectImpl(obj);
#endif
}



inline
DTrace::ExecutionScope::ExecutionScope(JSScript *script)
  : script(script)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_START_ENABLED())
        startExecution();
#endif
}

inline
DTrace::ExecutionScope::~ExecutionScope()
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_DONE_ENABLED())
        endExecution();
#endif
}



inline
DTrace::ObjectCreationScope::ObjectCreationScope(JSContext *cx, JSStackFrame *fp, JSClass *clasp)
  : cx(cx), fp(fp), clasp(clasp)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_CREATE_START_ENABLED())
        handleCreationStart();
#endif
}

inline void
DTrace::ObjectCreationScope::handleCreation(JSObject *obj)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_CREATE_ENABLED())
        handleCreationImpl(obj);
#endif
}

inline
DTrace::ObjectCreationScope::~ObjectCreationScope()
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_CREATE_DONE_ENABLED())
        handleCreationEnd();
#endif
}

} 
    
#endif 
