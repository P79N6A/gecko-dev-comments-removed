


































#include "javascript-trace.h"
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

#endif 
