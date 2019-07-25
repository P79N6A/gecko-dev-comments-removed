







































#ifndef jsdbgapi_h___
#define jsdbgapi_h___



#include "jsapi.h"
#include "jsopcode.h"
#include "jsprvtd.h"

JS_BEGIN_EXTERN_C

extern JS_PUBLIC_API(JSCrossCompartmentCall *)
JS_EnterCrossCompartmentCallScript(JSContext *cx, JSScript *target);

#ifdef __cplusplus
JS_END_EXTERN_C

namespace JS {

class JS_PUBLIC_API(AutoEnterScriptCompartment)
{
    JSCrossCompartmentCall *call;

  public:
    AutoEnterScriptCompartment() : call(NULL) {}

    bool enter(JSContext *cx, JSScript *target);

    bool entered() const { return call != NULL; }

    ~AutoEnterScriptCompartment() {
        if (call && call != reinterpret_cast<JSCrossCompartmentCall*>(1))
            JS_LeaveCrossCompartmentCall(call);
    }
};

} 

JS_BEGIN_EXTERN_C
#endif

extern JS_PUBLIC_API(JSScript *)
JS_GetScriptFromObject(JSObject *scriptObject);

extern JS_PUBLIC_API(JSString *)
JS_DecompileScript(JSContext *cx, JSScript *script, const char *name, uintN indent);





extern JS_PUBLIC_API(void)
JS_SetRuntimeDebugMode(JSRuntime *rt, JSBool debug);












extern JS_PUBLIC_API(JSBool)
JS_GetDebugMode(JSContext *cx);






JS_FRIEND_API(JSBool)
JS_SetDebugModeForCompartment(JSContext *cx, JSCompartment *comp, JSBool debug);




JS_FRIEND_API(JSBool)
JS_SetDebugMode(JSContext *cx, JSBool debug);


extern JS_FRIEND_API(JSBool)
js_SetSingleStepMode(JSContext *cx, JSScript *script, JSBool singleStep);


extern JS_PUBLIC_API(JSBool)
JS_SetSingleStepMode(JSContext *cx, JSScript *script, JSBool singleStep);






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




extern JSBool
js_TraceWatchPoints(JSTracer *trc);

extern void
js_SweepWatchPoints(JSContext *cx);

#ifdef __cplusplus

extern JSBool
js_watch_set(JSContext *cx, JSObject *obj, jsid id, JSBool strict, js::Value *vp);

namespace js {

bool
IsWatchedProperty(JSContext *cx, const Shape *shape);

}

#endif

#endif 



extern JS_PUBLIC_API(uintN)
JS_PCToLineNumber(JSContext *cx, JSScript *script, jsbytecode *pc);

extern JS_PUBLIC_API(jsbytecode *)
JS_LineNumberToPC(JSContext *cx, JSScript *script, uintN lineno);

extern JS_PUBLIC_API(jsbytecode *)
JS_EndPC(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(JSBool)
JS_GetLinePCs(JSContext *cx, JSScript *script,
              uintN startLine, uintN maxLines,
              uintN* count, uintN** lines, jsbytecode*** pcs);

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

extern JS_PUBLIC_API(JSBool)
JS_GetFrameThis(JSContext *cx, JSStackFrame *fp, jsval *thisv);

extern JS_PUBLIC_API(JSFunction *)
JS_GetFrameFunction(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(JSObject *)
JS_GetFrameFunctionObject(JSContext *cx, JSStackFrame *fp);


#define JS_IsContructorFrame JS_IsConstructorFrame
extern JS_PUBLIC_API(JSBool)
JS_IsConstructorFrame(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(JSBool)
JS_IsDebuggerFrame(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(JSBool)
JS_IsGlobalFrame(JSContext *cx, JSStackFrame *fp);

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

extern JS_PUBLIC_API(JSBool)
JS_StartProfiling();

extern JS_PUBLIC_API(void)
JS_StopProfiling();

extern JS_PUBLIC_API(JSBool)
JS_DefineProfilingFunctions(JSContext *cx, JSObject *obj);

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

#ifdef MOZ_TRACE_JSCALLS
typedef void (*JSFunctionCallback)(const JSFunction *fun,
                                   const JSScript *scr,
                                   const JSContext *cx,
                                   int entering);










extern JS_PUBLIC_API(void)
JS_SetFunctionCallback(JSContext *cx, JSFunctionCallback fcb);

extern JS_PUBLIC_API(JSFunctionCallback)
JS_GetFunctionCallback(JSContext *cx);
#endif 

JS_END_EXTERN_C

#endif 
