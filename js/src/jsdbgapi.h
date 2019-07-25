







































#ifndef jsdbgapi_h___
#define jsdbgapi_h___



#include "jsapi.h"
#include "jsopcode.h"
#include "jsprvtd.h"

JS_BEGIN_EXTERN_C












extern JS_PUBLIC_API(JSBool)
JS_GetDebugMode(JSContext *cx);


extern JS_FRIEND_API(JSBool)
js_SetDebugMode(JSContext *cx, JSBool debug);


extern JS_PUBLIC_API(JSBool)
JS_SetDebugMode(JSContext *cx, JSBool debug);






extern jsbytecode *
js_UntrapScriptCode(JSContext *cx, JSScript *script);


extern JS_PUBLIC_API(JSBool)
JS_SetTrap(JSContext *cx, JSScript *script, jsbytecode *pc,
           JSTrapHandler handler, jsval closure);

extern JS_PUBLIC_API(JSOp)
JS_GetTrapOpcode(JSContext *cx, JSScript *script, jsbytecode *pc);

extern JS_PUBLIC_API(void)
JS_ClearTrap(JSContext *cx, JSScript *script, jsbytecode *pc,
             JSTrapHandler *handlerp, jsval *closurep);

extern JS_PUBLIC_API(void)
JS_ClearScriptTraps(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(void)
JS_ClearAllTraps(JSContext *cx);

extern JS_PUBLIC_API(JSTrapStatus)
JS_HandleTrap(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_SetInterrupt(JSRuntime *rt, JSInterruptHook handler, void *closure);

extern JS_PUBLIC_API(JSBool)
JS_ClearInterrupt(JSRuntime *rt, JSInterruptHook *handlerp, void **closurep);



extern JS_PUBLIC_API(JSBool)
JS_SetWatchPoint(JSContext *cx, JSObject *obj, jsid id,
                 JSWatchPointHandler handler, JSObject *closure);

extern JS_PUBLIC_API(JSBool)
JS_ClearWatchPoint(JSContext *cx, JSObject *obj, jsid id,
                   JSWatchPointHandler *handlerp, JSObject **closurep);

extern JS_PUBLIC_API(JSBool)
JS_ClearWatchPointsForObject(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_ClearAllWatchPoints(JSContext *cx);

#ifdef JS_HAS_OBJ_WATCHPOINT




extern void
js_TraceWatchPoints(JSTracer *trc, JSObject *obj);

extern void
js_SweepWatchPoints(JSContext *cx);

#ifdef __cplusplus

extern const js::Shape *
js_FindWatchPoint(JSRuntime *rt, JSObject *obj, jsid id);

extern JSBool
js_watch_set(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
js_watch_set_wrapper(JSContext *cx, uintN argc, js::Value *vp);

extern js::PropertyOp
js_WrapWatchedSetter(JSContext *cx, jsid id, uintN attrs, js::PropertyOp setter);

#endif

#endif 



extern JS_PUBLIC_API(uintN)
JS_PCToLineNumber(JSContext *cx, JSScript *script, jsbytecode *pc);

extern JS_PUBLIC_API(jsbytecode *)
JS_LineNumberToPC(JSContext *cx, JSScript *script, uintN lineno);

extern JS_PUBLIC_API(jsbytecode *)
JS_FirstValidPC(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(jsbytecode *)
JS_EndPC(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(uintN)
JS_GetFunctionArgumentCount(JSContext *cx, JSFunction *fun);

extern JS_PUBLIC_API(JSBool)
JS_FunctionHasLocalNames(JSContext *cx, JSFunction *fun);






extern JS_PUBLIC_API(jsuword *)
JS_GetFunctionLocalNameArray(JSContext *cx, JSFunction *fun, void **markp);

extern JS_PUBLIC_API(JSAtom *)
JS_LocalNameToAtom(jsuword w);

extern JS_PUBLIC_API(JSString *)
JS_AtomKey(JSAtom *atom);

extern JS_PUBLIC_API(void)
JS_ReleaseFunctionLocalNameArray(JSContext *cx, void *mark);

extern JS_PUBLIC_API(JSScript *)
JS_GetFunctionScript(JSContext *cx, JSFunction *fun);

extern JS_PUBLIC_API(JSNative)
JS_GetFunctionNative(JSContext *cx, JSFunction *fun);

extern JS_PUBLIC_API(JSPrincipals *)
JS_GetScriptPrincipals(JSContext *cx, JSScript *script);








extern JS_PUBLIC_API(JSStackFrame *)
JS_FrameIterator(JSContext *cx, JSStackFrame **iteratorp);

extern JS_PUBLIC_API(JSScript *)
JS_GetFrameScript(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(jsbytecode *)
JS_GetFramePC(JSContext *cx, JSStackFrame *fp);




extern JS_PUBLIC_API(JSStackFrame *)
JS_GetScriptedCaller(JSContext *cx, JSStackFrame *fp);





extern JSPrincipals *
js_StackFramePrincipals(JSContext *cx, JSStackFrame *fp);

JSPrincipals *
js_EvalFramePrincipals(JSContext *cx, JSObject *callee, JSStackFrame *caller);

extern JS_PUBLIC_API(void *)
JS_GetFrameAnnotation(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(void)
JS_SetFrameAnnotation(JSContext *cx, JSStackFrame *fp, void *annotation);

extern JS_PUBLIC_API(void *)
JS_GetFramePrincipalArray(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(JSBool)
JS_IsScriptFrame(JSContext *cx, JSStackFrame *fp);


extern JS_PUBLIC_API(JSObject *)
JS_GetFrameObject(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(JSObject *)
JS_GetFrameScopeChain(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(JSObject *)
JS_GetFrameCallObject(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(JSObject *)
JS_GetFrameThis(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(JSFunction *)
JS_GetFrameFunction(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(JSObject *)
JS_GetFrameFunctionObject(JSContext *cx, JSStackFrame *fp);


#define JS_IsContructorFrame JS_IsConstructorFrame
extern JS_PUBLIC_API(JSBool)
JS_IsConstructorFrame(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(JSBool)
JS_IsDebuggerFrame(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(jsval)
JS_GetFrameReturnValue(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(void)
JS_SetFrameReturnValue(JSContext *cx, JSStackFrame *fp, jsval rval);



























extern JS_PUBLIC_API(JSObject *)
JS_GetFrameCalleeObject(JSContext *cx, JSStackFrame *fp);






extern JS_PUBLIC_API(JSBool)
JS_GetValidFrameCalleeObject(JSContext *cx, JSStackFrame *fp, jsval *vp);



extern JS_PUBLIC_API(const char *)
JS_GetScriptFilename(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(uintN)
JS_GetScriptBaseLineNumber(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(uintN)
JS_GetScriptLineExtent(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(JSVersion)
JS_GetScriptVersion(JSContext *cx, JSScript *script);








#define JS_SetNewScriptHook     JS_SetNewScriptHookProc
#define JS_SetDestroyScriptHook JS_SetDestroyScriptHookProc

extern JS_PUBLIC_API(void)
JS_SetNewScriptHook(JSRuntime *rt, JSNewScriptHook hook, void *callerdata);

extern JS_PUBLIC_API(void)
JS_SetDestroyScriptHook(JSRuntime *rt, JSDestroyScriptHook hook,
                        void *callerdata);



extern JS_PUBLIC_API(JSBool)
JS_EvaluateUCInStackFrame(JSContext *cx, JSStackFrame *fp,
                          const jschar *chars, uintN length,
                          const char *filename, uintN lineno,
                          jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_EvaluateInStackFrame(JSContext *cx, JSStackFrame *fp,
                        const char *bytes, uintN length,
                        const char *filename, uintN lineno,
                        jsval *rval);



typedef struct JSPropertyDesc {
    jsval           id;         
    jsval           value;      
    uint8           flags;      
    uint8           spare;      
    uint16          slot;       
    jsval           alias;      
} JSPropertyDesc;

#define JSPD_ENUMERATE  0x01    /* visible to for/in loop */
#define JSPD_READONLY   0x02    /* assignment is error */
#define JSPD_PERMANENT  0x04    /* property cannot be deleted */
#define JSPD_ALIAS      0x08    /* property has an alias id */
#define JSPD_ARGUMENT   0x10    /* argument to function */
#define JSPD_VARIABLE   0x20    /* local variable in function */
#define JSPD_EXCEPTION  0x40    /* exception occurred fetching the property, */
                                
#define JSPD_ERROR      0x80    /* native getter returned JS_FALSE without */
                                

typedef struct JSPropertyDescArray {
    uint32          length;     
    JSPropertyDesc  *array;     
} JSPropertyDescArray;

typedef struct JSScopeProperty JSScopeProperty;

extern JS_PUBLIC_API(JSScopeProperty *)
JS_PropertyIterator(JSObject *obj, JSScopeProperty **iteratorp);

extern JS_PUBLIC_API(JSBool)
JS_GetPropertyDesc(JSContext *cx, JSObject *obj, JSScopeProperty *shape,
                   JSPropertyDesc *pd);

extern JS_PUBLIC_API(JSBool)
JS_GetPropertyDescArray(JSContext *cx, JSObject *obj, JSPropertyDescArray *pda);

extern JS_PUBLIC_API(void)
JS_PutPropertyDescArray(JSContext *cx, JSPropertyDescArray *pda);



extern JS_FRIEND_API(JSBool)
js_GetPropertyByIdWithFakeFrame(JSContext *cx, JSObject *obj, JSObject *scopeobj, jsid id,
                                jsval *vp);

extern JS_FRIEND_API(JSBool)
js_SetPropertyByIdWithFakeFrame(JSContext *cx, JSObject *obj, JSObject *scopeobj, jsid id,
                                jsval *vp);

extern JS_FRIEND_API(JSBool)
js_CallFunctionValueWithFakeFrame(JSContext *cx, JSObject *obj, JSObject *scopeobj, jsval funval,
                                  uintN argc, jsval *argv, jsval *rval);



extern JS_PUBLIC_API(JSBool)
JS_SetDebuggerHandler(JSRuntime *rt, JSDebuggerHandler hook, void *closure);

extern JS_PUBLIC_API(JSBool)
JS_SetSourceHandler(JSRuntime *rt, JSSourceHandler handler, void *closure);

extern JS_PUBLIC_API(JSBool)
JS_SetExecuteHook(JSRuntime *rt, JSInterpreterHook hook, void *closure);

extern JS_PUBLIC_API(JSBool)
JS_SetCallHook(JSRuntime *rt, JSInterpreterHook hook, void *closure);

extern JS_PUBLIC_API(JSBool)
JS_SetThrowHook(JSRuntime *rt, JSThrowHook hook, void *closure);

extern JS_PUBLIC_API(JSBool)
JS_SetDebugErrorHook(JSRuntime *rt, JSDebugErrorHook hook, void *closure);



extern JS_PUBLIC_API(size_t)
JS_GetObjectTotalSize(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(size_t)
JS_GetFunctionTotalSize(JSContext *cx, JSFunction *fun);

extern JS_PUBLIC_API(size_t)
JS_GetScriptTotalSize(JSContext *cx, JSScript *script);






extern JS_PUBLIC_API(uint32)
JS_GetTopScriptFilenameFlags(JSContext *cx, JSStackFrame *fp);





extern JS_PUBLIC_API(uint32)
JS_GetScriptFilenameFlags(JSScript *script);













extern JS_PUBLIC_API(JSBool)
JS_FlagScriptFilenamePrefix(JSRuntime *rt, const char *prefix, uint32 flags);

#define JSFILENAME_NULL         0xffffffff      /* null script filename */
#define JSFILENAME_SYSTEM       0x00000001      /* "system" script, see below */
#define JSFILENAME_PROTECTED    0x00000002      /* scripts need protection */









extern JS_PUBLIC_API(JSBool)
JS_IsSystemObject(JSContext *cx, JSObject *obj);






extern JS_PUBLIC_API(JSBool)
JS_MakeSystemObject(JSContext *cx, JSObject *obj);



extern JS_FRIEND_API(void)
js_RevertVersion(JSContext *cx);

extern JS_PUBLIC_API(const JSDebugHooks *)
JS_GetGlobalDebugHooks(JSRuntime *rt);

extern JS_PUBLIC_API(JSDebugHooks *)
JS_SetContextDebugHooks(JSContext *cx, const JSDebugHooks *hooks);


extern JS_PUBLIC_API(JSDebugHooks *)
JS_ClearContextDebugHooks(JSContext *cx);

#ifdef MOZ_SHARK

extern JS_PUBLIC_API(JSBool)
JS_StartChudRemote();

extern JS_PUBLIC_API(JSBool)
JS_StopChudRemote();

extern JS_PUBLIC_API(JSBool)
JS_ConnectShark();

extern JS_PUBLIC_API(JSBool)
JS_DisconnectShark();

extern JS_FRIEND_API(JSBool)
js_StopShark(JSContext *cx, uintN argc, jsval *vp);

extern JS_FRIEND_API(JSBool)
js_StartShark(JSContext *cx, uintN argc, jsval *vp);

extern JS_FRIEND_API(JSBool)
js_ConnectShark(JSContext *cx, uintN argc, jsval *vp);

extern JS_FRIEND_API(JSBool)
js_DisconnectShark(JSContext *cx, uintN argc, jsval *vp);

#endif 

#ifdef MOZ_CALLGRIND

extern JS_FRIEND_API(JSBool)
js_StopCallgrind(JSContext *cx, uintN argc, jsval *vp);

extern JS_FRIEND_API(JSBool)
js_StartCallgrind(JSContext *cx, uintN argc, jsval *vp);

extern JS_FRIEND_API(JSBool)
js_DumpCallgrind(JSContext *cx, uintN argc, jsval *vp);

#endif 

#ifdef MOZ_VTUNE

extern JS_FRIEND_API(JSBool)
js_StartVtune(JSContext *cx, uintN argc, jsval *vp);

extern JS_FRIEND_API(JSBool)
js_StopVtune(JSContext *cx, uintN argc, jsval *vp);

extern JS_FRIEND_API(JSBool)
js_PauseVtune(JSContext *cx, uintN argc, jsval *vp);

extern JS_FRIEND_API(JSBool)
js_ResumeVtune(JSContext *cx, uintN argc, jsval *vp);

#endif 

#ifdef MOZ_TRACEVIS
extern JS_FRIEND_API(JSBool)
js_InitEthogram(JSContext *cx, uintN argc, jsval *vp);
extern JS_FRIEND_API(JSBool)
js_ShutdownEthogram(JSContext *cx, uintN argc, jsval *vp);
#endif 

JS_END_EXTERN_C

#endif 
