






#ifndef jsinterp_h___
#define jsinterp_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsopcode.h"

#include "vm/Stack.h"

namespace js {




















extern JSTrapStatus
ScriptDebugPrologue(JSContext *cx, StackFrame *fp);
















extern bool
ScriptDebugEpilogue(JSContext *cx, StackFrame *fp, bool ok);







extern bool
BoxNonStrictThis(JSContext *cx, const CallReceiver &call);







inline bool
ComputeThis(JSContext *cx, StackFrame *fp);

enum MaybeConstruct {
    NO_CONSTRUCT = INITIAL_NONE,
    CONSTRUCT = INITIAL_CONSTRUCT
};

extern bool
ReportIsNotFunction(JSContext *cx, const Value &v, MaybeConstruct construct = NO_CONSTRUCT);

extern bool
ReportIsNotFunction(JSContext *cx, const Value *vp, MaybeConstruct construct = NO_CONSTRUCT);

extern JSObject *
ValueToCallable(JSContext *cx, const Value *vp, MaybeConstruct construct = NO_CONSTRUCT);

inline JSFunction *
ReportIfNotFunction(JSContext *cx, const Value &v, MaybeConstruct construct = NO_CONSTRUCT)
{
    if (v.isObject() && v.toObject().isFunction())
        return v.toObject().toFunction();

    ReportIsNotFunction(cx, v, construct);
    return NULL;
}






extern bool
InvokeKernel(JSContext *cx, CallArgs args, MaybeConstruct construct = NO_CONSTRUCT);





inline bool
Invoke(JSContext *cx, InvokeArgsGuard &args, MaybeConstruct construct = NO_CONSTRUCT)
{
    args.setActive();
    bool ok = InvokeKernel(cx, args, construct);
    args.setInactive();
    return ok;
}






extern bool
Invoke(JSContext *cx, const Value &thisv, const Value &fval, unsigned argc, Value *argv,
       Value *rval);





extern bool
InvokeGetterOrSetter(JSContext *cx, JSObject *obj, const Value &fval, unsigned argc, Value *argv,
                     Value *rval);





extern bool
InvokeConstructorKernel(JSContext *cx, CallArgs args);


inline bool
InvokeConstructor(JSContext *cx, InvokeArgsGuard &args)
{
    args.setActive();
    bool ok = InvokeConstructorKernel(cx, ImplicitCast<CallArgs>(args));
    args.setInactive();
    return ok;
}


extern bool
InvokeConstructor(JSContext *cx, const Value &fval, unsigned argc, Value *argv, Value *rval);







extern bool
ExecuteKernel(JSContext *cx, HandleScript script, JSObject &scopeChain, const Value &thisv,
              ExecuteType type, StackFrame *evalInFrame, Value *result);


extern bool
Execute(JSContext *cx, HandleScript script, JSObject &scopeChain, Value *rval);


enum InterpMode
{
    JSINTERP_NORMAL    = 0, 
    JSINTERP_REJOIN    = 1, 
    JSINTERP_SKIP_TRAP = 2  
};





extern JS_NEVER_INLINE bool
Interpret(JSContext *cx, StackFrame *stopFp, InterpMode mode = JSINTERP_NORMAL);

extern bool
RunScript(JSContext *cx, JSScript *script, StackFrame *fp);

extern bool
StrictlyEqual(JSContext *cx, const Value &lval, const Value &rval, bool *equal);

extern bool
LooselyEqual(JSContext *cx, const Value &lval, const Value &rval, bool *equal);


extern bool
SameValue(JSContext *cx, const Value &v1, const Value &v2, bool *same);

extern JSType
TypeOfValue(JSContext *cx, const Value &v);

extern JSBool
HasInstance(JSContext *cx, HandleObject obj, MutableHandleValue v, JSBool *bp);
























class InterpreterFrames {
  public:
    class InterruptEnablerBase {
      public:
        virtual void enable() const = 0;
    };

    InterpreterFrames(JSContext *cx, FrameRegs *regs, const InterruptEnablerBase &enabler);
    ~InterpreterFrames();

    
    inline void enableInterruptsIfRunning(JSScript *script);
    inline void enableInterruptsUnconditionally() { enabler.enable(); }

    InterpreterFrames *older;

  private:
    JSContext *context;
    FrameRegs *regs;
    const InterruptEnablerBase &enabler;
};





extern void
UnwindScope(JSContext *cx, uint32_t stackDepth);





extern void
UnwindForUncatchableException(JSContext *cx, const FrameRegs &regs);

extern bool
OnUnknownMethod(JSContext *cx, HandleObject obj, Value idval, MutableHandleValue vp);

class TryNoteIter
{
    const FrameRegs &regs;
    JSScript *script;
    uint32_t pcOffset;
    JSTryNote *tn, *tnEnd;
    void settle();
  public:
    TryNoteIter(const FrameRegs &regs);
    bool done() const;
    void operator++();
    JSTryNote *operator*() const { return tn; }
};









static JS_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(Value *beg, Value *end)
{
#ifdef DEBUG
    for (Value *v = beg; v != end; ++v)
        v->setObject(*reinterpret_cast<JSObject *>(0x42));
#endif
}

static JS_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(Value *vec, size_t len)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch(vec, vec + len);
#endif
}

static JS_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(HeapValue *vec, size_t len)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch((Value *) vec, len);
#endif
}

}  

#endif 
