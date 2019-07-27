





#ifndef js_OldDebugAPI_h
#define js_OldDebugAPI_h





#include "mozilla/NullPtr.h"

#include "jsapi.h"
#include "jsbytecode.h"

#include "js/CallArgs.h"
#include "js/TypeDecls.h"

class JSAtom;
struct JSFreeOp;

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

typedef bool
(* JSWatchPointHandler)(JSContext *cx, JSObject *obj, jsid id, JS::Value old,
                        JS::Value *newp, void *closure);



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







extern JS_PUBLIC_API(bool)
JS_DefineProfilingFunctions(JSContext *cx, JSObject *obj);


extern JS_PUBLIC_API(bool)
JS_DefineDebuggerObject(JSContext *cx, JS::HandleObject obj);

extern JS_PUBLIC_API(void)
JS_DumpPCCounts(JSContext *cx, JS::HandleScript script);

extern JS_PUBLIC_API(void)
JS_DumpCompartmentPCCounts(JSContext *cx);

#endif 
