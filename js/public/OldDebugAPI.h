





#ifndef js_OldDebugAPI_h
#define js_OldDebugAPI_h





#include "mozilla/NullPtr.h"

#include "jsapi.h"
#include "jsbytecode.h"

#include "js/CallArgs.h"
#include "js/TypeDecls.h"

class JSAtom;
class JSFreeOp;

namespace js {
class InterpreterFrame;
class FrameIter;
class ScriptSource;
}


extern JS_PUBLIC_API(unsigned)
JS_PCToLineNumber(JSContext *cx, JSScript *script, jsbytecode *pc);

extern JS_PUBLIC_API(const char *)
JS_GetScriptFilename(JSScript *script);

namespace JS {

class FrameDescription
{
  public:
    explicit FrameDescription(const js::FrameIter& iter);
    FrameDescription(const FrameDescription &rhs);
    ~FrameDescription();

    unsigned lineno() {
        if (!linenoComputed_) {
            lineno_ = JS_PCToLineNumber(nullptr, script_, pc_);
            linenoComputed_ = true;
        }
        return lineno_;
    }

    const char *filename() const {
        return filename_;
    }

    JSFlatString *funDisplayName() const {
        return funDisplayName_ ? JS_ASSERT_STRING_IS_FLAT(funDisplayName_) : nullptr;
    }

    
    
    Heap<JSScript*> &markedLocation1() {
        return script_;
    }
    Heap<JSString*> &markedLocation2() {
        return funDisplayName_;
    }

  private:
    void operator=(const FrameDescription &) MOZ_DELETE;

    
    Heap<JSString*> funDisplayName_;
    const char *filename_;

    
    Heap<JSScript*> script_;
    js::ScriptSource *scriptSource_;

    
    bool linenoComputed_;
    unsigned lineno_;

    
    jsbytecode *pc_;
};

struct StackDescription
{
    unsigned nframes;
    FrameDescription *frames;
};

extern JS_PUBLIC_API(StackDescription *)
DescribeStack(JSContext *cx, unsigned maxFrames);

extern JS_PUBLIC_API(void)
FreeStackDescription(JSContext *cx, StackDescription *desc);

extern JS_PUBLIC_API(char *)
FormatStackDump(JSContext *cx, char *buf, bool showArgs, bool showLocals, bool showThisProps);

} 

# ifdef JS_DEBUG
JS_FRIEND_API(void) js_DumpValue(const JS::Value &val);
JS_FRIEND_API(void) js_DumpId(jsid id);
JS_FRIEND_API(void) js_DumpInterpreterFrame(JSContext *cx, js::InterpreterFrame *start = nullptr);
# endif

JS_FRIEND_API(void)
js_DumpBacktrace(JSContext *cx);

typedef enum JSTrapStatus {
    JSTRAP_ERROR,
    JSTRAP_CONTINUE,
    JSTRAP_RETURN,
    JSTRAP_THROW,
    JSTRAP_LIMIT
} JSTrapStatus;

typedef JSTrapStatus
(* JSTrapHandler)(JSContext *cx, JSScript *script, jsbytecode *pc, JS::Value *rval,
                  JS::Value closure);

typedef JSTrapStatus
(* JSInterruptHook)(JSContext *cx, JSScript *script, jsbytecode *pc, JS::Value *rval,
                    void *closure);

typedef JSTrapStatus
(* JSDebuggerHandler)(JSContext *cx, JSScript *script, jsbytecode *pc, JS::Value *rval,
                      void *closure);

typedef JSTrapStatus
(* JSThrowHook)(JSContext *cx, JSScript *script, jsbytecode *pc, JS::Value *rval,
                void *closure);

typedef bool
(* JSWatchPointHandler)(JSContext *cx, JSObject *obj, jsid id, JS::Value old,
                        JS::Value *newp, void *closure);


typedef void
(* JSNewScriptHook)(JSContext  *cx,
                    const char *filename,  
                    unsigned   lineno,     
                    JSScript   *script,
                    JSFunction *fun,
                    void       *callerdata);


typedef void
(* JSDestroyScriptHook)(JSFreeOp *fop,
                        JSScript *script,
                        void     *callerdata);

typedef void
(* JSSourceHandler)(const char *filename, unsigned lineno, const jschar *str,
                    size_t length, void **listenerTSData, void *closure);



extern JS_PUBLIC_API(JSCompartment *)
JS_EnterCompartmentOfScript(JSContext *cx, JSScript *target);

extern JS_PUBLIC_API(JSString *)
JS_DecompileScript(JSContext *cx, JS::HandleScript script, const char *name, unsigned indent);





extern JS_PUBLIC_API(void)
JS_SetRuntimeDebugMode(JSRuntime *rt, bool debug);












extern JS_PUBLIC_API(bool)
JS_GetDebugMode(JSContext *cx);





JS_FRIEND_API(bool)
JS_SetDebugModeForAllCompartments(JSContext *cx, bool debug);






JS_FRIEND_API(bool)
JS_SetDebugModeForCompartment(JSContext *cx, JSCompartment *comp, bool debug);




JS_FRIEND_API(bool)
JS_SetDebugMode(JSContext *cx, bool debug);


extern JS_PUBLIC_API(bool)
JS_SetSingleStepMode(JSContext *cx, JS::HandleScript script, bool singleStep);


extern JS_PUBLIC_API(bool)
JS_SetTrap(JSContext *cx, JS::HandleScript script, jsbytecode *pc,
           JSTrapHandler handler, JS::HandleValue closure);

extern JS_PUBLIC_API(void)
JS_ClearTrap(JSContext *cx, JSScript *script, jsbytecode *pc,
             JSTrapHandler *handlerp, JS::Value *closurep);

extern JS_PUBLIC_API(void)
JS_ClearScriptTraps(JSRuntime *rt, JSScript *script);

extern JS_PUBLIC_API(void)
JS_ClearAllTrapsForCompartment(JSContext *cx);

extern JS_PUBLIC_API(bool)
JS_SetInterrupt(JSRuntime *rt, JSInterruptHook handler, void *closure);

extern JS_PUBLIC_API(bool)
JS_ClearInterrupt(JSRuntime *rt, JSInterruptHook *handlerp, void **closurep);



extern JS_PUBLIC_API(bool)
JS_SetWatchPoint(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                 JSWatchPointHandler handler, JS::HandleObject closure);

extern JS_PUBLIC_API(bool)
JS_ClearWatchPoint(JSContext *cx, JSObject *obj, jsid id,
                   JSWatchPointHandler *handlerp, JSObject **closurep);

extern JS_PUBLIC_API(bool)
JS_ClearWatchPointsForObject(JSContext *cx, JSObject *obj);



extern JS_PUBLIC_API(jsbytecode *)
JS_LineNumberToPC(JSContext *cx, JSScript *script, unsigned lineno);

extern JS_PUBLIC_API(jsbytecode *)
JS_EndPC(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(bool)
JS_GetLinePCs(JSContext *cx, JSScript *script,
              unsigned startLine, unsigned maxLines,
              unsigned* count, unsigned** lines, jsbytecode*** pcs);

extern JS_PUBLIC_API(unsigned)
JS_GetFunctionArgumentCount(JSContext *cx, JSFunction *fun);

extern JS_PUBLIC_API(bool)
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
JS_GetFunctionScript(JSContext *cx, JS::HandleFunction fun);

extern JS_PUBLIC_API(JSNative)
JS_GetFunctionNative(JSContext *cx, JSFunction *fun);

extern JS_PUBLIC_API(JSPrincipals *)
JS_GetScriptPrincipals(JSScript *script);

extern JS_PUBLIC_API(JSPrincipals *)
JS_GetScriptOriginPrincipals(JSScript *script);

JS_PUBLIC_API(JSFunction *)
JS_GetScriptFunction(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(JSObject *)
JS_GetParentOrScopeChain(JSContext *cx, JSObject *obj);









extern JS_PUBLIC_API(const char *)
JS_GetDebugClassName(JSObject *obj);



extern JS_PUBLIC_API(const jschar *)
JS_GetScriptSourceMap(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(unsigned)
JS_GetScriptBaseLineNumber(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(unsigned)
JS_GetScriptLineExtent(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(JSVersion)
JS_GetScriptVersion(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(bool)
JS_GetScriptIsSelfHosted(JSScript *script);







#define JS_SetNewScriptHook     JS_SetNewScriptHookProc
#define JS_SetDestroyScriptHook JS_SetDestroyScriptHookProc

extern JS_PUBLIC_API(void)
JS_SetNewScriptHook(JSRuntime *rt, JSNewScriptHook hook, void *callerdata);

extern JS_PUBLIC_API(void)
JS_SetDestroyScriptHook(JSRuntime *rt, JSDestroyScriptHook hook,
                        void *callerdata);



typedef struct JSPropertyDesc {
    JS::Value       id;         
    JS::Value       value;      
    uint8_t         flags;      
    uint8_t         spare;      
    JS::Value       alias;      
} JSPropertyDesc;

#define JSPD_ENUMERATE  0x01    /* visible to for/in loop */
#define JSPD_READONLY   0x02    /* assignment is error */
#define JSPD_PERMANENT  0x04    /* property cannot be deleted */
#define JSPD_ALIAS      0x08    /* property has an alias id */
#define JSPD_EXCEPTION  0x40    /* exception occurred fetching the property, */
                                
#define JSPD_ERROR      0x80    /* native getter returned false without */
                                

typedef struct JSPropertyDescArray {
    uint32_t        length;     
    JSPropertyDesc  *array;     
} JSPropertyDescArray;

typedef struct JSScopeProperty JSScopeProperty;

extern JS_PUBLIC_API(bool)
JS_GetPropertyDescArray(JSContext *cx, JS::HandleObject obj, JSPropertyDescArray *pda);

extern JS_PUBLIC_API(void)
JS_PutPropertyDescArray(JSContext *cx, JSPropertyDescArray *pda);







class JS_PUBLIC_API(JSAbstractFramePtr)
{
    uintptr_t ptr_;
    jsbytecode *pc_;

  protected:
    JSAbstractFramePtr()
      : ptr_(0), pc_(nullptr)
    { }

  public:
    JSAbstractFramePtr(void *raw, jsbytecode *pc);

    uintptr_t raw() const { return ptr_; }
    jsbytecode *pc() const { return pc_; }

    operator bool() const { return !!ptr_; }

    JSObject *scopeChain(JSContext *cx);
    JSObject *callObject(JSContext *cx);

    JSFunction *maybeFun();
    JSScript *script();

    bool getThisValue(JSContext *cx, JS::MutableHandleValue thisv);

    bool isDebuggerFrame();

    bool evaluateInStackFrame(JSContext *cx,
                              const char *bytes, unsigned length,
                              const char *filename, unsigned lineno,
                              JS::MutableHandleValue rval);

    bool evaluateUCInStackFrame(JSContext *cx,
                                const jschar *chars, unsigned length,
                                const char *filename, unsigned lineno,
                                JS::MutableHandleValue rval);
};

class JS_PUBLIC_API(JSNullFramePtr) : public JSAbstractFramePtr
{
  public:
    JSNullFramePtr()
      : JSAbstractFramePtr()
    {}
};








class JS_PUBLIC_API(JSBrokenFrameIterator)
{
    void *data_;

  public:
    explicit JSBrokenFrameIterator(JSContext *cx);
    ~JSBrokenFrameIterator();

    bool done() const;
    JSBrokenFrameIterator& operator++();

    JSAbstractFramePtr abstractFramePtr() const;
    jsbytecode *pc() const;

    bool isConstructing() const;
};


























typedef void *
(* JSInterpreterHook)(JSContext *cx, JSAbstractFramePtr frame, bool isConstructing,
                      bool before, bool *ok, void *closure);

typedef bool
(* JSDebugErrorHook)(JSContext *cx, const char *message, JSErrorReport *report,
                     void *closure);

typedef struct JSDebugHooks {
    JSInterruptHook     interruptHook;
    void                *interruptHookData;
    JSNewScriptHook     newScriptHook;
    void                *newScriptHookData;
    JSDestroyScriptHook destroyScriptHook;
    void                *destroyScriptHookData;
    JSDebuggerHandler   debuggerHandler;
    void                *debuggerHandlerData;
    JSSourceHandler     sourceHandler;
    void                *sourceHandlerData;
    JSInterpreterHook   executeHook;
    void                *executeHookData;
    JSInterpreterHook   callHook;
    void                *callHookData;
    JSThrowHook         throwHook;
    void                *throwHookData;
    JSDebugErrorHook    debugErrorHook;
    void                *debugErrorHookData;
} JSDebugHooks;



extern JS_PUBLIC_API(bool)
JS_SetDebuggerHandler(JSRuntime *rt, JSDebuggerHandler hook, void *closure);

extern JS_PUBLIC_API(bool)
JS_SetSourceHandler(JSRuntime *rt, JSSourceHandler handler, void *closure);

extern JS_PUBLIC_API(bool)
JS_SetExecuteHook(JSRuntime *rt, JSInterpreterHook hook, void *closure);

extern JS_PUBLIC_API(bool)
JS_SetCallHook(JSRuntime *rt, JSInterpreterHook hook, void *closure);

extern JS_PUBLIC_API(bool)
JS_SetThrowHook(JSRuntime *rt, JSThrowHook hook, void *closure);

extern JS_PUBLIC_API(bool)
JS_SetDebugErrorHook(JSRuntime *rt, JSDebugErrorHook hook, void *closure);



extern JS_PUBLIC_API(const JSDebugHooks *)
JS_GetGlobalDebugHooks(JSRuntime *rt);




extern JS_PUBLIC_API(bool)
JS_DefineProfilingFunctions(JSContext *cx, JSObject *obj);


extern JS_PUBLIC_API(bool)
JS_DefineDebuggerObject(JSContext *cx, JS::HandleObject obj);

extern JS_PUBLIC_API(void)
JS_DumpPCCounts(JSContext *cx, JS::HandleScript script);

extern JS_PUBLIC_API(void)
JS_DumpCompartmentPCCounts(JSContext *cx);

namespace js {
extern JS_FRIEND_API(bool)
CanCallContextDebugHandler(JSContext *cx);
}


extern JS_FRIEND_API(bool)
js_CallContextDebugHandler(JSContext *cx);

#endif 
