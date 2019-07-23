








































#ifndef jsdebug_h___
#define jsdebug_h___




#ifdef __cplusplus
extern "C"
{
#endif
#include "jstypes.h"
#ifdef __cplusplus
}
#endif

JS_BEGIN_EXTERN_C
#include "jsapi.h"
#include "jsdbgapi.h"
#ifdef LIVEWIRE
#include "lwdbgapi.h"
#endif
JS_END_EXTERN_C

JS_BEGIN_EXTERN_C







#ifdef EXPORT_JSD_API
#define JSD_PUBLIC_API(t)    JS_EXPORT_API(t)
#define JSD_PUBLIC_DATA(t)   JS_EXPORT_DATA(t)
#else
#define JSD_PUBLIC_API(t)    JS_IMPORT_API(t)
#define JSD_PUBLIC_DATA(t)   JS_IMPORT_DATA(t)
#endif

#define JSD_FRIEND_API(t)    JSD_PUBLIC_API(t)
#define JSD_FRIEND_DATA(t)   JSD_PUBLIC_DATA(t)




typedef struct JSDContext        JSDContext;
typedef struct JSDScript         JSDScript;
typedef struct JSDSourceText     JSDSourceText;
typedef struct JSDThreadState    JSDThreadState;
typedef struct JSDStackFrameInfo JSDStackFrameInfo;
typedef struct JSDValue          JSDValue;
typedef struct JSDProperty       JSDProperty;
typedef struct JSDObject         JSDObject;















typedef void
(* JSD_SetContextProc)(JSDContext* jsdc, void* user);


typedef struct
{
    uintN              size;       
    JSD_SetContextProc setContext;
} JSD_UserCallbacks;







extern JSD_PUBLIC_API(void)
JSD_SetUserCallbacks(JSRuntime*         jsrt,
                     JSD_UserCallbacks* callbacks,
                     void*              user);






extern JSD_PUBLIC_API(JSDContext*)
JSD_DebuggerOn(void);




extern JSD_PUBLIC_API(JSDContext*)
JSD_DebuggerOnForUser(JSRuntime*         jsrt,
                      JSD_UserCallbacks* callbacks,
                      void*              user);




extern JSD_PUBLIC_API(void)
JSD_DebuggerOff(JSDContext* jsdc);




extern JSD_PUBLIC_API(uintN)
JSD_GetMajorVersion(void);




extern JSD_PUBLIC_API(uintN)
JSD_GetMinorVersion(void);




extern JSD_PUBLIC_API(JSContext*)
JSD_GetDefaultJSContext(JSDContext* jsdc);




extern JSD_PUBLIC_API(void *)
JSD_SetContextPrivate(JSDContext *jsdc, void *data);




extern JSD_PUBLIC_API(void *)
JSD_GetContextPrivate(JSDContext *jsdc);




extern JSD_PUBLIC_API(void)
JSD_ClearAllProfileData(JSDContext* jsdc);






#define JSD_INCLUDE_NATIVE_FRAMES 0x01





#define JSD_PROFILE_WHEN_SET      0x02





#define JSD_DEBUG_WHEN_SET        0x04



#define JSD_COLLECT_PROFILE_DATA  0x08




#define JSD_HIDE_DISABLED_FRAMES  0x10
















#define JSD_MASK_TOP_FRAME_ONLY   0x20




#define JSD_DISABLE_OBJECT_TRACE  0x40

extern JSD_PUBLIC_API(void)
JSD_SetContextFlags (JSDContext* jsdc, uint32 flags);

extern JSD_PUBLIC_API(uint32)
JSD_GetContextFlags (JSDContext* jsdc);     







extern JSD_PUBLIC_API(void)
JSD_JSContextInUse(JSDContext* jsdc, JSContext* context);





extern JSD_PUBLIC_API(JSDContext*)
JSD_JSDContextForJSContext(JSContext* context);













extern JSD_PUBLIC_API(void)
JSD_LockScriptSubsystem(JSDContext* jsdc);




extern JSD_PUBLIC_API(void)
JSD_UnlockScriptSubsystem(JSDContext* jsdc);
















extern JSD_PUBLIC_API(JSDScript*)
JSD_IterateScripts(JSDContext* jsdc, JSDScript **iterp);




extern JSD_PUBLIC_API(uintN)
JSD_GetScriptCallCount(JSDContext* jsdc, JSDScript *script);




extern JSD_PUBLIC_API(uintN)
JSD_GetScriptMaxRecurseDepth(JSDContext* jsdc, JSDScript *script);




extern JSD_PUBLIC_API(jsdouble)
JSD_GetScriptMinExecutionTime(JSDContext* jsdc, JSDScript *script);




extern JSD_PUBLIC_API(jsdouble)
JSD_GetScriptMaxExecutionTime(JSDContext* jsdc, JSDScript *script);




extern JSD_PUBLIC_API(jsdouble)
JSD_GetScriptTotalExecutionTime(JSDContext* jsdc, JSDScript *script);





extern JSD_PUBLIC_API(jsdouble)
JSD_GetScriptMinOwnExecutionTime(JSDContext* jsdc, JSDScript *script);





extern JSD_PUBLIC_API(jsdouble)
JSD_GetScriptMaxOwnExecutionTime(JSDContext* jsdc, JSDScript *script);





extern JSD_PUBLIC_API(jsdouble)
JSD_GetScriptTotalOwnExecutionTime(JSDContext* jsdc, JSDScript *script);




extern JSD_PUBLIC_API(void)
JSD_ClearScriptProfileData(JSDContext* jsdc, JSDScript *script);




extern JSD_PUBLIC_API(JSScript*)
JSD_GetJSScript(JSDContext* jsdc, JSDScript *script);




extern JSD_PUBLIC_API(JSFunction*)
JSD_GetJSFunction(JSDContext* jsdc, JSDScript *script);





#define JSD_SCRIPT_PROFILE_BIT 0x01




#define JSD_SCRIPT_DEBUG_BIT   0x02

extern JSD_PUBLIC_API(uint32)
JSD_GetScriptFlags(JSDContext *jsdc, JSDScript* jsdscript);

extern JSD_PUBLIC_API(void)
JSD_SetScriptFlags(JSDContext *jsdc, JSDScript* jsdscript, uint32 flags);




extern JSD_PUBLIC_API(void *)
JSD_SetScriptPrivate(JSDScript* jsdscript, void *data);




extern JSD_PUBLIC_API(void *)
JSD_GetScriptPrivate(JSDScript* jsdscript);




extern JSD_PUBLIC_API(JSBool)
JSD_IsActiveScript(JSDContext* jsdc, JSDScript *jsdscript);




extern JSD_PUBLIC_API(const char*)
JSD_GetScriptFilename(JSDContext* jsdc, JSDScript *jsdscript);




extern JSD_PUBLIC_API(const char*)
JSD_GetScriptFunctionName(JSDContext* jsdc, JSDScript *jsdscript);






extern JSD_PUBLIC_API(uintN)
JSD_GetScriptBaseLineNumber(JSDContext* jsdc, JSDScript *jsdscript);




extern JSD_PUBLIC_API(uintN)
JSD_GetScriptLineExtent(JSDContext* jsdc, JSDScript *jsdscript);







typedef void
(* JSD_ScriptHookProc)(JSDContext* jsdc,
                       JSDScript*  jsdscript,
                       JSBool      creating,
                       void*       callerdata);






extern JSD_PUBLIC_API(JSBool)
JSD_SetScriptHook(JSDContext* jsdc, JSD_ScriptHookProc hook, void* callerdata);




extern JSD_PUBLIC_API(JSBool)
JSD_GetScriptHook(JSDContext* jsdc, JSD_ScriptHookProc* hook, void** callerdata);










extern JSD_PUBLIC_API(jsuword)
JSD_GetClosestPC(JSDContext* jsdc, JSDScript* jsdscript, uintN line);






extern JSD_PUBLIC_API(uintN)
JSD_GetClosestLine(JSDContext* jsdc, JSDScript* jsdscript, jsuword pc);










extern JSD_PUBLIC_API(void)
JSD_ScriptCreated(JSDContext* jsdc,
                  JSContext   *cx,
                  const char  *filename,    
                  uintN       lineno,       
                  JSScript    *script,
                  JSFunction  *fun);




extern JSD_PUBLIC_API(void)
JSD_ScriptDestroyed(JSDContext* jsdc,
                    JSContext   *cx,
                    JSScript    *script);




















typedef enum
{
    JSD_SOURCE_INITED       = 0, 
    JSD_SOURCE_PARTIAL      = 1, 
    JSD_SOURCE_COMPLETED    = 2, 
    JSD_SOURCE_ABORTED      = 3, 
    JSD_SOURCE_FAILED       = 4, 
    JSD_SOURCE_CLEARED      = 5  
} JSDSourceStatus;











extern JSD_PUBLIC_API(void)
JSD_LockSourceTextSubsystem(JSDContext* jsdc);




extern JSD_PUBLIC_API(void)
JSD_UnlockSourceTextSubsystem(JSDContext* jsdc);






extern JSD_PUBLIC_API(JSDSourceText*)
JSD_IterateSources(JSDContext* jsdc, JSDSourceText **iterp);






extern JSD_PUBLIC_API(JSDSourceText*)
JSD_FindSourceForURL(JSDContext* jsdc, const char* url);




extern JSD_PUBLIC_API(const char*)
JSD_GetSourceURL(JSDContext* jsdc, JSDSourceText* jsdsrc);








extern JSD_PUBLIC_API(JSBool)
JSD_GetSourceText(JSDContext* jsdc, JSDSourceText* jsdsrc,
                  const char** ppBuf, intN* pLen);







extern JSD_PUBLIC_API(void)
JSD_ClearSourceText(JSDContext* jsdc, JSDSourceText* jsdsrc);




extern JSD_PUBLIC_API(JSDSourceStatus)
JSD_GetSourceStatus(JSDContext* jsdc, JSDSourceText* jsdsrc);







extern JSD_PUBLIC_API(JSBool)
JSD_IsSourceDirty(JSDContext* jsdc, JSDSourceText* jsdsrc);




extern JSD_PUBLIC_API(void)
JSD_SetSourceDirty(JSDContext* jsdc, JSDSourceText* jsdsrc, JSBool dirty);








extern JSD_PUBLIC_API(uintN)
JSD_GetSourceAlterCount(JSDContext* jsdc, JSDSourceText* jsdsrc);






extern JSD_PUBLIC_API(uintN)
JSD_IncrementSourceAlterCount(JSDContext* jsdc, JSDSourceText* jsdsrc);





extern JSD_PUBLIC_API(void)
JSD_DestroyAllSources( JSDContext* jsdc );








extern JSD_PUBLIC_API(JSDSourceText*)
JSD_NewSourceText(JSDContext* jsdc, const char* url);







extern JSD_PUBLIC_API(JSDSourceText*)
JSD_AppendSourceText(JSDContext*     jsdc,
                     JSDSourceText*  jsdsrc,
                     const char*     text,       
                     size_t          length,
                     JSDSourceStatus status);








extern JSD_PUBLIC_API(JSDSourceText*)
JSD_AppendUCSourceText(JSDContext*     jsdc,
                       JSDSourceText*  jsdsrc,
                       const jschar*   text,       
                       size_t          length,
                       JSDSourceStatus status);















extern JSD_PUBLIC_API(JSBool)
JSD_AddFullSourceText(JSDContext* jsdc,
                      const char* text,       
                      size_t      length,
                      const char* url);





#define JSD_HOOK_INTERRUPTED            0
#define JSD_HOOK_BREAKPOINT             1
#define JSD_HOOK_DEBUG_REQUESTED        2
#define JSD_HOOK_DEBUGGER_KEYWORD       3
#define JSD_HOOK_THROW                  4


#define JSD_HOOK_RETURN_HOOK_ERROR      0
#define JSD_HOOK_RETURN_CONTINUE        1
#define JSD_HOOK_RETURN_ABORT           2
#define JSD_HOOK_RETURN_RET_WITH_VAL    3
#define JSD_HOOK_RETURN_THROW_WITH_VAL  4
#define JSD_HOOK_RETURN_CONTINUE_THROW  5




typedef uintN
(* JSD_ExecutionHookProc)(JSDContext*     jsdc,
                          JSDThreadState* jsdthreadstate,
                          uintN           type,
                          void*           callerdata,
                          jsval*          rval);


#define JSD_HOOK_TOPLEVEL_START  0   /* about to evaluate top level script */
#define JSD_HOOK_TOPLEVEL_END    1   /* done evaluting top level script    */
#define JSD_HOOK_FUNCTION_CALL   2   /* about to call a function           */
#define JSD_HOOK_FUNCTION_RETURN 3   /* done calling function              */







typedef JSBool
(* JSD_CallHookProc)(JSDContext*     jsdc,
                     JSDThreadState* jsdthreadstate,
                     uintN           type,
                     void*           callerdata);





extern JSD_PUBLIC_API(JSBool)
JSD_SetExecutionHook(JSDContext*           jsdc,
                     JSDScript*            jsdscript,
                     jsuword               pc,
                     JSD_ExecutionHookProc hook,
                     void*                 callerdata);




extern JSD_PUBLIC_API(JSBool)
JSD_ClearExecutionHook(JSDContext*          jsdc,
                       JSDScript*           jsdscript,
                       jsuword              pc);




extern JSD_PUBLIC_API(JSBool)
JSD_ClearAllExecutionHooksForScript(JSDContext* jsdc, JSDScript* jsdscript);





extern JSD_PUBLIC_API(JSBool)
JSD_ClearAllExecutionHooks(JSDContext* jsdc);








extern JSD_PUBLIC_API(JSBool)
JSD_SetInterruptHook(JSDContext*           jsdc,
                     JSD_ExecutionHookProc hook,
                     void*                 callerdata);




extern JSD_PUBLIC_API(JSBool)
JSD_ClearInterruptHook(JSDContext* jsdc);





extern JSD_PUBLIC_API(JSBool)
JSD_SetDebugBreakHook(JSDContext*           jsdc,
                      JSD_ExecutionHookProc hook,
                      void*                 callerdata);




extern JSD_PUBLIC_API(JSBool)
JSD_ClearDebugBreakHook(JSDContext* jsdc);





extern JSD_PUBLIC_API(JSBool)
JSD_SetDebuggerHook(JSDContext*           jsdc,
                    JSD_ExecutionHookProc hook,
                    void*                 callerdata);




extern JSD_PUBLIC_API(JSBool)
JSD_ClearDebuggerHook(JSDContext* jsdc);





extern JSD_PUBLIC_API(JSBool)
JSD_SetThrowHook(JSDContext*           jsdc,
                 JSD_ExecutionHookProc hook,
                 void*                 callerdata);



extern JSD_PUBLIC_API(JSBool)
JSD_ClearThrowHook(JSDContext* jsdc);




extern JSD_PUBLIC_API(JSBool)
JSD_SetTopLevelHook(JSDContext*      jsdc,
                    JSD_CallHookProc hook,
                    void*            callerdata);



extern JSD_PUBLIC_API(JSBool)
JSD_ClearTopLevelHook(JSDContext* jsdc);




extern JSD_PUBLIC_API(JSBool)
JSD_SetFunctionHook(JSDContext*      jsdc,
                    JSD_CallHookProc hook,
                    void*            callerdata);



extern JSD_PUBLIC_API(JSBool)
JSD_ClearFunctionHook(JSDContext* jsdc);







extern JSD_PUBLIC_API(uintN)
JSD_GetCountOfStackFrames(JSDContext* jsdc, JSDThreadState* jsdthreadstate);




extern JSD_PUBLIC_API(JSDStackFrameInfo*)
JSD_GetStackFrame(JSDContext* jsdc, JSDThreadState* jsdthreadstate);




extern JSD_PUBLIC_API(JSContext*)
JSD_GetJSContext(JSDContext* jsdc, JSDThreadState* jsdthreadstate);




extern JSD_PUBLIC_API(JSDStackFrameInfo*)
JSD_GetCallingStackFrame(JSDContext* jsdc,
                         JSDThreadState* jsdthreadstate,
                         JSDStackFrameInfo* jsdframe);




extern JSD_PUBLIC_API(JSDScript*)
JSD_GetScriptForStackFrame(JSDContext* jsdc,
                           JSDThreadState* jsdthreadstate,
                           JSDStackFrameInfo* jsdframe);




extern JSD_PUBLIC_API(jsuword)
JSD_GetPCForStackFrame(JSDContext* jsdc,
                       JSDThreadState* jsdthreadstate,
                       JSDStackFrameInfo* jsdframe);





extern JSD_PUBLIC_API(JSDValue*)
JSD_GetCallObjectForStackFrame(JSDContext* jsdc,
                               JSDThreadState* jsdthreadstate,
                               JSDStackFrameInfo* jsdframe);






extern JSD_PUBLIC_API(JSDValue*)
JSD_GetScopeChainForStackFrame(JSDContext* jsdc,
                               JSDThreadState* jsdthreadstate,
                               JSDStackFrameInfo* jsdframe);





extern JSD_PUBLIC_API(JSDValue*)
JSD_GetThisForStackFrame(JSDContext* jsdc,
                         JSDThreadState* jsdthreadstate,
                         JSDStackFrameInfo* jsdframe);





extern JSD_PUBLIC_API(const char*)
JSD_GetNameForStackFrame(JSDContext* jsdc,
                         JSDThreadState* jsdthreadstate,
                         JSDStackFrameInfo* jsdframe);




extern JSD_PUBLIC_API(JSBool)
JSD_IsStackFrameNative(JSDContext* jsdc,
                       JSDThreadState* jsdthreadstate,
                       JSDStackFrameInfo* jsdframe);





extern JSD_PUBLIC_API(JSBool)
JSD_IsStackFrameDebugger(JSDContext* jsdc,
                         JSDThreadState* jsdthreadstate,
                         JSDStackFrameInfo* jsdframe);




extern JSD_PUBLIC_API(JSBool)
JSD_IsStackFrameConstructing(JSDContext* jsdc,
                             JSDThreadState* jsdthreadstate,
                             JSDStackFrameInfo* jsdframe);






extern JSD_PUBLIC_API(JSBool)
JSD_EvaluateUCScriptInStackFrame(JSDContext* jsdc,
                                 JSDThreadState* jsdthreadstate,
                                 JSDStackFrameInfo* jsdframe,
                                 const jschar *bytes, uintN length,
                                 const char *filename, uintN lineno,
                                 jsval *rval);




extern JSD_PUBLIC_API(JSBool)
JSD_AttemptUCScriptInStackFrame(JSDContext* jsdc,
                                JSDThreadState* jsdthreadstate,
                                JSDStackFrameInfo* jsdframe,
                                const jschar *bytes, uintN length,
                                const char *filename, uintN lineno,
                                jsval *rval);


extern JSD_PUBLIC_API(JSBool)
JSD_EvaluateScriptInStackFrame(JSDContext* jsdc,
                               JSDThreadState* jsdthreadstate,
                               JSDStackFrameInfo* jsdframe,
                               const char *bytes, uintN length,
                               const char *filename, uintN lineno, jsval *rval);




extern JSD_PUBLIC_API(JSBool)
JSD_AttemptScriptInStackFrame(JSDContext* jsdc,
                              JSDThreadState* jsdthreadstate,
                              JSDStackFrameInfo* jsdframe,
                              const char *bytes, uintN length,
                              const char *filename, uintN lineno, jsval *rval);





extern JSD_PUBLIC_API(JSString*)
JSD_ValToStringInStackFrame(JSDContext* jsdc,
                            JSDThreadState* jsdthreadstate,
                            JSDStackFrameInfo* jsdframe,
                            jsval val);






extern JSD_PUBLIC_API(JSDValue*)
JSD_GetException(JSDContext* jsdc, JSDThreadState* jsdthreadstate);





extern JSD_PUBLIC_API(JSBool)
JSD_SetException(JSDContext* jsdc, JSDThreadState* jsdthreadstate, 
                 JSDValue* jsdval);










#define JSD_ERROR_REPORTER_PASS_ALONG   0 /* pass along to regular reporter */
#define JSD_ERROR_REPORTER_RETURN       1 /* don't pass to error reporter */
#define JSD_ERROR_REPORTER_DEBUG        2 /* force call to DebugBreakHook */
#define JSD_ERROR_REPORTER_CLEAR_RETURN 3 /* clear exception and don't pass */




typedef uintN
(* JSD_ErrorReporter)(JSDContext*     jsdc,
                      JSContext*      cx,
                      const char*     message,
                      JSErrorReport*  report,
                      void*           callerdata);


extern JSD_PUBLIC_API(JSBool)
JSD_SetErrorReporter(JSDContext*       jsdc,
                     JSD_ErrorReporter reporter,
                     void*             callerdata);


extern JSD_PUBLIC_API(JSBool)
JSD_GetErrorReporter(JSDContext*        jsdc,
                     JSD_ErrorReporter* reporter,
                     void**             callerdata);







extern JSD_PUBLIC_API(JSBool)
JSD_IsLockingAndThreadIdSupported();




extern JSD_PUBLIC_API(void*)
JSD_CreateLock();





extern JSD_PUBLIC_API(void)
JSD_Lock(void* lock);





extern JSD_PUBLIC_API(void)
JSD_Unlock(void* lock);





extern JSD_PUBLIC_API(JSBool)
JSD_IsLocked(void* lock);




extern JSD_PUBLIC_API(JSBool)
JSD_IsUnlocked(void* lock);




extern JSD_PUBLIC_API(void*)
JSD_CurrentThread();
















extern JSD_PUBLIC_API(JSDValue*)
JSD_NewValue(JSDContext* jsdc, jsval val);





extern JSD_PUBLIC_API(void)
JSD_DropValue(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(jsval)
JSD_GetValueWrappedJSVal(JSDContext* jsdc, JSDValue* jsdval);









extern JSD_PUBLIC_API(void)
JSD_RefreshValue(JSDContext* jsdc, JSDValue* jsdval);







extern JSD_PUBLIC_API(JSBool)
JSD_IsValueObject(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(JSBool)
JSD_IsValueNumber(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(JSBool)
JSD_IsValueInt(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(JSBool)
JSD_IsValueDouble(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(JSBool)
JSD_IsValueString(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(JSBool)
JSD_IsValueBoolean(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(JSBool)
JSD_IsValueNull(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(JSBool)
JSD_IsValueVoid(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(JSBool)
JSD_IsValuePrimitive(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(JSBool)
JSD_IsValueFunction(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(JSBool)
JSD_IsValueNative(JSDContext* jsdc, JSDValue* jsdval);







extern JSD_PUBLIC_API(JSBool)
JSD_GetValueBoolean(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(int32)
JSD_GetValueInt(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(jsdouble*)
JSD_GetValueDouble(JSDContext* jsdc, JSDValue* jsdval);








extern JSD_PUBLIC_API(JSString*)
JSD_GetValueString(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(const char*)
JSD_GetValueFunctionName(JSDContext* jsdc, JSDValue* jsdval);







extern JSD_PUBLIC_API(uintN)
JSD_GetCountOfProperties(JSDContext* jsdc, JSDValue* jsdval);








extern JSD_PUBLIC_API(JSDProperty*)
JSD_IterateProperties(JSDContext* jsdc, JSDValue* jsdval, JSDProperty **iterp);






extern JSD_PUBLIC_API(JSDProperty*)
JSD_GetValueProperty(JSDContext* jsdc, JSDValue* jsdval, JSString* name);






extern JSD_PUBLIC_API(JSDValue*)
JSD_GetValuePrototype(JSDContext* jsdc, JSDValue* jsdval);






extern JSD_PUBLIC_API(JSDValue*)
JSD_GetValueParent(JSDContext* jsdc, JSDValue* jsdval);






extern JSD_PUBLIC_API(JSDValue*)
JSD_GetValueConstructor(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(const char*)
JSD_GetValueClassName(JSDContext* jsdc, JSDValue* jsdval);





extern JSD_PUBLIC_API(JSDScript*)
JSD_GetScriptForValue(JSDContext* jsdc, JSDValue* jsdval);







#define JSDPD_ENUMERATE  JSPD_ENUMERATE    /* visible to for/in loop */
#define JSDPD_READONLY   JSPD_READONLY     /* assignment is error */
#define JSDPD_PERMANENT  JSPD_PERMANENT    /* property cannot be deleted */
#define JSDPD_ALIAS      JSPD_ALIAS        /* property has an alias id */
#define JSDPD_ARGUMENT   JSPD_ARGUMENT     /* argument to function */
#define JSDPD_VARIABLE   JSPD_VARIABLE     /* local variable in function */
#define JSDPD_EXCEPTION  JSPD_EXCEPTION    /* exception occurred looking up */
                                           
#define JSDPD_ERROR      JSPD_ERROR        /* native getter returned JS_FALSE */
                                           

#define JSDPD_HINTED     0x800             /* found via explicit lookup */





extern JSD_PUBLIC_API(void)
JSD_DropProperty(JSDContext* jsdc, JSDProperty* jsdprop);






extern JSD_PUBLIC_API(JSDValue*)
JSD_GetPropertyName(JSDContext* jsdc, JSDProperty* jsdprop);






extern JSD_PUBLIC_API(JSDValue*)
JSD_GetPropertyValue(JSDContext* jsdc, JSDProperty* jsdprop);






extern JSD_PUBLIC_API(JSDValue*)
JSD_GetPropertyAlias(JSDContext* jsdc, JSDProperty* jsdprop);





extern JSD_PUBLIC_API(uintN)
JSD_GetPropertyFlags(JSDContext* jsdc, JSDProperty* jsdprop);





extern JSD_PUBLIC_API(uintN)
JSD_GetPropertyVarArgSlot(JSDContext* jsdc, JSDProperty* jsdprop);





















extern JSD_PUBLIC_API(void)
JSD_LockObjectSubsystem(JSDContext* jsdc);





extern JSD_PUBLIC_API(void)
JSD_UnlockObjectSubsystem(JSDContext* jsdc);







extern JSD_PUBLIC_API(JSDObject*)
JSD_IterateObjects(JSDContext* jsdc, JSDObject** iterp);





extern JSD_PUBLIC_API(JSObject*)
JSD_GetWrappedObject(JSDContext* jsdc, JSDObject* jsdobj);






extern JSD_PUBLIC_API(const char*)
JSD_GetObjectNewURL(JSDContext* jsdc, JSDObject* jsdobj);






extern JSD_PUBLIC_API(uintN)
JSD_GetObjectNewLineNumber(JSDContext* jsdc, JSDObject* jsdobj);






extern JSD_PUBLIC_API(const char*)
JSD_GetObjectConstructorURL(JSDContext* jsdc, JSDObject* jsdobj);






extern JSD_PUBLIC_API(uintN)
JSD_GetObjectConstructorLineNumber(JSDContext* jsdc, JSDObject* jsdobj);






extern JSD_PUBLIC_API(const char*)
JSD_GetObjectConstructorName(JSDContext* jsdc, JSDObject* jsdobj);






extern JSD_PUBLIC_API(JSDObject*)
JSD_GetJSDObjectForJSObject(JSDContext* jsdc, JSObject* jsobj);






extern JSD_PUBLIC_API(JSDObject*)
JSD_GetObjectForValue(JSDContext* jsdc, JSDValue* jsdval);






extern JSD_PUBLIC_API(JSDValue*)
JSD_GetValueForObject(JSDContext* jsdc, JSDObject* jsdobj);



#ifdef LIVEWIRE

extern JSD_PUBLIC_API(LWDBGScript*)
JSDLW_GetLWScript(JSDContext* jsdc, JSDScript* jsdscript);

extern JSD_PUBLIC_API(JSDSourceText*)
JSDLW_PreLoadSource(JSDContext* jsdc, LWDBGApp* app,
                    const char* filename, JSBool clear);

extern JSD_PUBLIC_API(JSDSourceText*)
JSDLW_ForceLoadSource(JSDContext* jsdc, JSDSourceText* jsdsrc);

extern JSD_PUBLIC_API(JSBool)
JSDLW_RawToProcessedLineNumber(JSDContext* jsdc, JSDScript* jsdscript,
                               uintN lineIn, uintN* lineOut);

extern JSD_PUBLIC_API(JSBool)
JSDLW_ProcessedToRawLineNumber(JSDContext* jsdc, JSDScript* jsdscript,
                               uintN lineIn, uintN* lineOut);

#endif


JS_END_EXTERN_C

#endif
