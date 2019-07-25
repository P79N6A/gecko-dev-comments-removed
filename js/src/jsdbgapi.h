






#ifndef jsdbgapi_h___
#define jsdbgapi_h___



#include "jsapi.h"
#include "jsprvtd.h"

JS_BEGIN_EXTERN_C

extern JS_PUBLIC_API(JSCrossCompartmentCall *)
JS_EnterCrossCompartmentCallScript(JSContext *cx, JSScript *target);

extern JS_PUBLIC_API(JSCrossCompartmentCall *)
JS_EnterCrossCompartmentCallStackFrame(JSContext *cx, JSStackFrame *target);

#ifdef __cplusplus
JS_END_EXTERN_C

namespace JS {

class JS_PUBLIC_API(AutoEnterScriptCompartment)
{
  protected:
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

class JS_PUBLIC_API(AutoEnterFrameCompartment) : public AutoEnterScriptCompartment
{
  public:
    bool enter(JSContext *cx, JSStackFrame *target);
};

} 

#ifdef DEBUG
JS_FRIEND_API(void) js_DumpValue(const js::Value &val);
JS_FRIEND_API(void) js_DumpId(jsid id);
JS_FRIEND_API(void) js_DumpStackFrame(JSContext *cx, js::StackFrame *start = NULL);
#endif
JS_FRIEND_API(void) js_DumpBacktrace(JSContext *cx);

JS_BEGIN_EXTERN_C
#endif

extern JS_PUBLIC_API(JSString *)
JS_DecompileScript(JSContext *cx, JSScript *script, const char *name, unsigned indent);





extern JS_PUBLIC_API(void)
JS_SetRuntimeDebugMode(JSRuntime *rt, JSBool debug);












extern JS_PUBLIC_API(JSBool)
JS_GetDebugMode(JSContext *cx);





JS_FRIEND_API(JSBool)
JS_SetDebugModeForAllCompartments(JSContext *cx, JSBool debug);






JS_FRIEND_API(JSBool)
JS_SetDebugModeForCompartment(JSContext *cx, JSCompartment *comp, JSBool debug);




JS_FRIEND_API(JSBool)
JS_SetDebugMode(JSContext *cx, JSBool debug);


extern JS_PUBLIC_API(JSBool)
JS_SetSingleStepMode(JSContext *cx, JSScript *script, JSBool singleStep);


extern JS_PUBLIC_API(JSBool)
JS_SetTrap(JSContext *cx, JSScript *script, jsbytecode *pc,
           JSTrapHandler handler, jsval closure);

extern JS_PUBLIC_API(void)
JS_ClearTrap(JSContext *cx, JSScript *script, jsbytecode *pc,
             JSTrapHandler *handlerp, jsval *closurep);

extern JS_PUBLIC_API(void)
JS_ClearScriptTraps(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(void)
JS_ClearAllTrapsForCompartment(JSContext *cx);

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



extern JS_PUBLIC_API(unsigned)
JS_PCToLineNumber(JSContext *cx, JSScript *script, jsbytecode *pc);

extern JS_PUBLIC_API(jsbytecode *)
JS_LineNumberToPC(JSContext *cx, JSScript *script, unsigned lineno);

extern JS_PUBLIC_API(jsbytecode *)
JS_EndPC(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(JSBool)
JS_GetLinePCs(JSContext *cx, JSScript *script,
              unsigned startLine, unsigned maxLines,
              unsigned* count, unsigned** lines, jsbytecode*** pcs);

extern JS_PUBLIC_API(unsigned)
JS_GetFunctionArgumentCount(JSContext *cx, JSFunction *fun);

extern JS_PUBLIC_API(JSBool)
JS_FunctionHasLocalNames(JSContext *cx, JSFunction *fun);






extern JS_PUBLIC_API(uintptr_t *)
JS_GetFunctionLocalNameArray(JSContext *cx, JSFunction *fun, void **markp);

extern JS_PUBLIC_API(JSAtom *)
JS_LocalNameToAtom(uintptr_t w);

extern JS_PUBLIC_API(JSString *)
JS_AtomKey(JSAtom *atom);

extern JS_PUBLIC_API(void)
JS_ReleaseFunctionLocalNameArray(JSContext *cx, void *mark);

extern JS_PUBLIC_API(JSScript *)
JS_GetFunctionScript(JSContext *cx, JSFunction *fun);

extern JS_PUBLIC_API(JSNative)
JS_GetFunctionNative(JSContext *cx, JSFunction *fun);

extern JS_PUBLIC_API(JSPrincipals *)
JS_GetScriptPrincipals(JSScript *script);

extern JS_PUBLIC_API(JSPrincipals *)
JS_GetScriptOriginPrincipals(JSScript *script);








extern JS_PUBLIC_API(JSStackFrame *)
JS_FrameIterator(JSContext *cx, JSStackFrame **iteratorp);

extern JS_PUBLIC_API(JSScript *)
JS_GetFrameScript(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(jsbytecode *)
JS_GetFramePC(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(void *)
JS_GetFrameAnnotation(JSContext *cx, JSStackFrame *fp);

extern JS_PUBLIC_API(void)
JS_SetFrameAnnotation(JSContext *cx, JSStackFrame *fp, void *annotation);

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

JS_PUBLIC_API(JSFunction *)
JS_GetScriptFunction(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(JSObject *)
JS_GetParentOrScopeChain(JSContext *cx, JSObject *obj);


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









extern JS_PUBLIC_API(const char *)
JS_GetDebugClassName(JSObject *obj);



extern JS_PUBLIC_API(const char *)
JS_GetScriptFilename(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(const jschar *)
JS_GetScriptSourceMap(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(unsigned)
JS_GetScriptBaseLineNumber(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(unsigned)
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
                          const jschar *chars, unsigned length,
                          const char *filename, unsigned lineno,
                          jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_EvaluateInStackFrame(JSContext *cx, JSStackFrame *fp,
                        const char *bytes, unsigned length,
                        const char *filename, unsigned lineno,
                        jsval *rval);



typedef struct JSPropertyDesc {
    jsval           id;         
    jsval           value;      
    uint8_t         flags;      
    uint8_t         spare;      
    jsval           alias;      
} JSPropertyDesc;

#define JSPD_ENUMERATE  0x01    /* visible to for/in loop */
#define JSPD_READONLY   0x02    /* assignment is error */
#define JSPD_PERMANENT  0x04    /* property cannot be deleted */
#define JSPD_ALIAS      0x08    /* property has an alias id */
#define JSPD_EXCEPTION  0x40    /* exception occurred fetching the property, */
                                
#define JSPD_ERROR      0x80    /* native getter returned JS_FALSE without */
                                

typedef struct JSPropertyDescArray {
    uint32_t        length;     
    JSPropertyDesc  *array;     
} JSPropertyDescArray;

typedef struct JSScopeProperty JSScopeProperty;

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



extern JS_FRIEND_API(void)
js_RevertVersion(JSContext *cx);

extern JS_PUBLIC_API(const JSDebugHooks *)
JS_GetGlobalDebugHooks(JSRuntime *rt);












extern JS_PUBLIC_API(JSBool)
JS_StartProfiling(const char *profileName);





extern JS_PUBLIC_API(JSBool)
JS_StopProfiling(const char *profileName);





extern JS_PUBLIC_API(JSBool)
JS_DumpProfile(const char *outfile, const char *profileName);






extern JS_PUBLIC_API(JSBool)
JS_PauseProfilers(const char *profileName);




extern JS_PUBLIC_API(JSBool)
JS_ResumeProfilers(const char *profileName);




extern JS_PUBLIC_API(JSBool)
JS_DefineProfilingFunctions(JSContext *cx, JSObject *obj);


extern JS_PUBLIC_API(JSBool)
JS_DefineDebuggerObject(JSContext *cx, JSObject *obj);






JS_PUBLIC_API(const char *)
JS_UnsafeGetLastProfilingError();

#ifdef MOZ_CALLGRIND

extern JS_FRIEND_API(JSBool)
js_StopCallgrind();

extern JS_FRIEND_API(JSBool)
js_StartCallgrind();

extern JS_FRIEND_API(JSBool)
js_DumpCallgrind(const char *outfile);

#endif 

#ifdef MOZ_VTUNE

extern JS_FRIEND_API(bool)
js_StartVtune(const char *profileName);

extern JS_FRIEND_API(bool)
js_StopVtune();

extern JS_FRIEND_API(bool)
js_PauseVtune();

extern JS_FRIEND_API(bool)
js_ResumeVtune();

#endif 

#ifdef __linux__

extern JS_FRIEND_API(JSBool)
js_StartPerf();

extern JS_FRIEND_API(JSBool)
js_StopPerf();

#endif 

extern JS_PUBLIC_API(void)
JS_DumpBytecode(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(void)
JS_DumpCompartmentBytecode(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_DumpPCCounts(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(void)
JS_DumpCompartmentPCCounts(JSContext *cx);

extern JS_PUBLIC_API(JSObject *)
JS_UnwrapObject(JSObject *obj);

extern JS_PUBLIC_API(JSObject *)
JS_UnwrapObjectAndInnerize(JSObject *obj);


extern JS_FRIEND_API(JSBool)
js_CallContextDebugHandler(JSContext *cx);

JS_END_EXTERN_C

#endif 
