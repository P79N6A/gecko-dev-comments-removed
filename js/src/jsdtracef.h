


































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
                                   jsuint argc, js::Value *argv);
    static void handleFunctionRval(JSContext *cx, JSStackFrame *fp, JSFunction *fun,
                                   const js::Value &rval);
    static void handleFunctionReturn(JSContext *cx, JSStackFrame *fp, JSFunction *fun);
    static void finalizeObjectImpl(JSObject *obj);
  public:
    



    static void enterJSFun(JSContext *cx, JSStackFrame *fp, JSFunction *fun,
                           JSStackFrame *dfp, jsuint argc, js::Value *argv,
                           js::Value *lval = NULL);
    static void exitJSFun(JSContext *cx, JSStackFrame *fp, JSFunction *fun,
                          const js::Value &rval,
                          js::Value *lval = NULL);

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
        js::Class       * const clasp;
        void handleCreationStart();
        void handleCreationImpl(JSObject *obj);
        void handleCreationEnd();
      public:
        ObjectCreationScope(JSContext *cx, JSStackFrame *fp, js::Class *clasp);
        void handleCreation(JSObject *obj);
        ~ObjectCreationScope();
    };

};

inline void
DTrace::enterJSFun(JSContext *cx, JSStackFrame *fp, JSFunction *fun, JSStackFrame *dfp,
                   jsuint argc, js::Value *argv, js::Value *lval)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (!lval || IsFunctionObject(*lval)) {
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
DTrace::exitJSFun(JSContext *cx, JSStackFrame *fp, JSFunction *fun,
                  const js::Value &rval, js::Value *lval)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (!lval || IsFunctionObject(*lval)) {
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
DTrace::ObjectCreationScope::ObjectCreationScope(JSContext *cx, JSStackFrame *fp, js::Class *clasp)
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
