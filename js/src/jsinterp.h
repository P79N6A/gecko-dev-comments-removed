







































#ifndef jsinterp_h___
#define jsinterp_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsopcode.h"

#include "vm/Stack.h"

namespace js {









extern JSObject *
GetScopeChain(JSContext *cx);

extern JSObject *
GetScopeChain(JSContext *cx, StackFrame *fp);







inline bool
ScriptPrologue(JSContext *cx, StackFrame *fp, JSScript *script);

inline bool
ScriptEpilogue(JSContext *cx, StackFrame *fp, bool ok);








inline bool
ScriptPrologueOrGeneratorResume(JSContext *cx, StackFrame *fp);

inline bool
ScriptEpilogueOrGeneratorYield(JSContext *cx, StackFrame *fp, bool ok);




















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
Invoke(JSContext *cx, const Value &thisv, const Value &fval, uintN argc, Value *argv,
       Value *rval);





extern bool
InvokeGetterOrSetter(JSContext *cx, JSObject *obj, const Value &fval, uintN argc, Value *argv,
                     Value *rval);





extern bool
InvokeConstructorKernel(JSContext *cx, const CallArgs &args);


inline bool
InvokeConstructor(JSContext *cx, InvokeArgsGuard &args)
{
    args.setActive();
    bool ok = InvokeConstructorKernel(cx, ImplicitCast<CallArgs>(args));
    args.setInactive();
    return ok;
}


extern bool
InvokeConstructor(JSContext *cx, const Value &fval, uintN argc, Value *argv, Value *rval);







extern bool
ExecuteKernel(JSContext *cx, JSScript *script, JSObject &scopeChain, const Value &thisv,
              ExecuteType type, StackFrame *evalInFrame, Value *result);


extern bool
Execute(JSContext *cx, JSScript *script, JSObject &scopeChain, Value *rval);


enum InterpMode
{
    JSINTERP_NORMAL    = 0, 
    JSINTERP_REJOIN    = 1, 
    JSINTERP_SKIP_TRAP = 2, 
    JSINTERP_BAILOUT   = 3  
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
HasInstance(JSContext *cx, JSObject *obj, const js::Value *v, JSBool *bp);

extern bool
ValueToId(JSContext *cx, const Value &v, jsid *idp);








extern const Value &
GetUpvar(JSContext *cx, uintN level, UpvarCookie cookie);


extern StackFrame *
FindUpvarFrame(JSContext *cx, uintN targetLevel);
























class InterpreterFrames {
  public:
    class InterruptEnablerBase {
      public:
        virtual void enableInterrupts() const = 0;
    };

    InterpreterFrames(JSContext *cx, FrameRegs *regs, const InterruptEnablerBase &enabler);
    ~InterpreterFrames();

    
    inline void enableInterruptsIfRunning(JSScript *script);

    InterpreterFrames *older;

  private:
    JSContext *context;
    FrameRegs *regs;
    const InterruptEnablerBase &enabler;
};





extern void
UnwindScope(JSContext *cx, uint32_t stackDepth);

extern bool
OnUnknownMethod(JSContext *cx, JSObject *obj, Value idval, Value *vp);

extern bool
IsActiveWithOrBlock(JSContext *cx, JSObject &obj, uint32_t stackDepth);









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

JSObject*
NewInitArray(JSContext *cx, uint32_t count, types::TypeObject *type);

bool
Throw(JSContext *cx, const Value &v);

bool
GetObjectProperty(JSContext *cx, JSObject *obj, PropertyName *name, Value *vp);

bool
GetScopeName(JSContext *cx, JSObject *obj, PropertyName *name, Value *vp);

bool
GetScopeNameForTypeOf(JSContext *cx, JSObject *obj, PropertyName *name, Value *vp);

JSObject *
Lambda(JSContext *cx, JSFunction *fun, JSObject *parent);

JSObject *
LambdaJoinableForCall(JSContext *cx, JSFunction *fun, JSObject *parent, JSObject *callee, uint32_t argc);

JSObject *
LambdaJoinableForSet(JSContext *cx, JSFunction *fun, JSObject *parent, JSObject *target);

bool
GetElement(JSContext *cx, const Value &lref, const Value &rref, Value *res);

bool
SetObjectElement(JSContext *cx, JSObject *obj, const Value &index, const Value &value);

bool
AddValues(JSContext *cx, const Value &lhs, const Value &rhs, Value *res);

bool
SubValues(JSContext *cx, const Value &lhs, const Value &rhs, Value *res);

bool
MulValues(JSContext *cx, const Value &lhs, const Value &rhs, Value *res);

bool
DivValues(JSContext *cx, const Value &lhs, const Value &rhs, Value *res);

bool
ModValues(JSContext *cx, const Value &lhs, const Value &rhs, Value *res);

template <bool strict>
bool
SetProperty(JSContext *cx, JSObject *obj, JSAtom *atom, Value value);

}  

#endif 
