







































#ifndef jsinterp_h___
#define jsinterp_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsopcode.h"
#include "jsscript.h"

#include "vm/Stack.h"

namespace js {

extern JSObject *
GetBlockChain(JSContext *cx, StackFrame *fp);

extern JSObject *
GetBlockChainFast(JSContext *cx, StackFrame *fp, JSOp op, size_t oplen);

extern JSObject *
GetScopeChain(JSContext *cx);








extern JSObject *
GetScopeChain(JSContext *cx, StackFrame *fp);

extern JSObject *
GetScopeChainFast(JSContext *cx, StackFrame *fp, JSOp op, size_t oplen);





void
ReportIncompatibleMethod(JSContext *cx, Value *vp, Class *clasp);









template <typename T>
bool GetPrimitiveThis(JSContext *cx, Value *vp, T *v);







inline bool
ScriptPrologue(JSContext *cx, StackFrame *fp, JSScript *script);

inline bool
ScriptEpilogue(JSContext *cx, StackFrame *fp, bool ok);








inline bool
ScriptPrologueOrGeneratorResume(JSContext *cx, StackFrame *fp);

inline bool
ScriptEpilogueOrGeneratorYield(JSContext *cx, StackFrame *fp, bool ok);



extern void
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
InvokeKernel(JSContext *cx, const CallArgs &args, MaybeConstruct construct = NO_CONSTRUCT);





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





extern JS_REQUIRES_STACK bool
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





extern JS_REQUIRES_STACK bool
InvokeConstructorWithGivenThis(JSContext *cx, JSObject *thisobj, const Value &fval,
                               uintN argc, Value *argv, Value *rval);







extern bool
ExecuteKernel(JSContext *cx, JSScript *script, JSObject &scopeChain, const Value &thisv,
              ExecuteType type, StackFrame *evalInFrame, Value *result);


extern bool
Execute(JSContext *cx, JSScript *script, JSObject &scopeChain, Value *rval);


enum InterpMode
{
    JSINTERP_NORMAL    = 0, 
    JSINTERP_RECORD    = 1, 
    JSINTERP_SAFEPOINT = 2, 
    JSINTERP_PROFILE   = 3, 
    JSINTERP_REJOIN    = 4, 
    JSINTERP_SKIP_TRAP = 5  
};





extern JS_REQUIRES_STACK JS_NEVER_INLINE bool
Interpret(JSContext *cx, StackFrame *stopFp, InterpMode mode = JSINTERP_NORMAL);

extern JS_REQUIRES_STACK bool
RunScript(JSContext *cx, JSScript *script, StackFrame *fp);

extern bool
CheckRedeclaration(JSContext *cx, JSObject *obj, jsid id, uintN attrs);

extern bool
StrictlyEqual(JSContext *cx, const Value &lval, const Value &rval, JSBool *equal);

extern bool
LooselyEqual(JSContext *cx, const Value &lval, const Value &rval, JSBool *equal);


extern bool
SameValue(JSContext *cx, const Value &v1, const Value &v2, JSBool *same);

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

    
    void enableInterruptsIfRunning(JSScript *script) {
        if (script == regs->fp()->script())
            enabler.enableInterrupts();
    }

    InterpreterFrames *older;

  private:
    JSContext *context;
    FrameRegs *regs;
    const InterruptEnablerBase &enabler;
};





extern bool
UnwindScope(JSContext *cx, jsint stackDepth, JSBool normalUnwind);

extern bool
OnUnknownMethod(JSContext *cx, js::Value *vp);

extern bool
IsActiveWithOrBlock(JSContext *cx, JSObject &obj, int stackDepth);



static JS_ALWAYS_INLINE void
ClearValueRange(Value *vec, uintN len, bool useHoles)
{
    if (useHoles) {
        for (uintN i = 0; i < len; i++)
            vec[i].setMagic(JS_ARRAY_HOLE);
    } else {
        for (uintN i = 0; i < len; i++)
            vec[i].setUndefined();
    }
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Value *vec, size_t len)
{
    PodZero(vec, len);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Value *beg, Value *end)
{
    PodZero(beg, end - beg);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(jsid *beg, jsid *end)
{
    for (jsid *id = beg; id != end; ++id)
        *id = INT_TO_JSID(0);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(jsid *vec, size_t len)
{
    MakeRangeGCSafe(vec, vec + len);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(const Shape **beg, const Shape **end)
{
    PodZero(beg, end - beg);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(const Shape **vec, size_t len)
{
    PodZero(vec, len);
}

static JS_ALWAYS_INLINE void
SetValueRangeToUndefined(Value *beg, Value *end)
{
    for (Value *v = beg; v != end; ++v)
        v->setUndefined();
}

static JS_ALWAYS_INLINE void
SetValueRangeToUndefined(Value *vec, size_t len)
{
    SetValueRangeToUndefined(vec, vec + len);
}

static JS_ALWAYS_INLINE void
SetValueRangeToNull(Value *beg, Value *end)
{
    for (Value *v = beg; v != end; ++v)
        v->setNull();
}

static JS_ALWAYS_INLINE void
SetValueRangeToNull(Value *vec, size_t len)
{
    SetValueRangeToNull(vec, vec + len);
}







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

}  

#endif 
