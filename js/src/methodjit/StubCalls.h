







































#ifndef jslogic_h__
#define jslogic_h__

#include "MethodJIT.h"

namespace js {
namespace mjit {
namespace stubs {

typedef enum JSTrapType {
    JSTRAP_NONE = 0,
    JSTRAP_TRAP = 1,
    JSTRAP_SINGLESTEP = 2
} JSTrapType;

void JS_FASTCALL This(VMFrame &f);
JSObject * JS_FASTCALL NewInitArray(VMFrame &f, uint32 count);
JSObject * JS_FASTCALL NewInitObject(VMFrame &f, JSObject *base);
void JS_FASTCALL Trap(VMFrame &f, uint32 trapTypes);
void JS_FASTCALL Debugger(VMFrame &f, jsbytecode *pc);
void JS_FASTCALL Interrupt(VMFrame &f, jsbytecode *pc);
void JS_FASTCALL InitElem(VMFrame &f, uint32 last);
void JS_FASTCALL InitProp(VMFrame &f, JSAtom *atom);
void JS_FASTCALL InitMethod(VMFrame &f, JSAtom *atom);

void JS_FASTCALL HitStackQuota(VMFrame &f);
void * JS_FASTCALL FixupArity(VMFrame &f, uint32 argc);
void * JS_FASTCALL CompileFunction(VMFrame &f, uint32 argc);
void JS_FASTCALL Eval(VMFrame &f, uint32 argc);
void JS_FASTCALL ScriptDebugPrologue(VMFrame &f);
void JS_FASTCALL ScriptDebugEpilogue(VMFrame &f);

void JS_FASTCALL CreateThis(VMFrame &f, JSObject *proto);
void JS_FASTCALL Throw(VMFrame &f);
void JS_FASTCALL PutActivationObjects(VMFrame &f);
void JS_FASTCALL CreateFunCallObject(VMFrame &f);
#if JS_MONOIC
void * JS_FASTCALL InvokeTracer(VMFrame &f, ic::TraceICInfo *tic);
#else
void * JS_FASTCALL InvokeTracer(VMFrame &f);
#endif

void * JS_FASTCALL LookupSwitch(VMFrame &f, jsbytecode *pc);
void * JS_FASTCALL TableSwitch(VMFrame &f, jsbytecode *origPc);

void JS_FASTCALL BindName(VMFrame &f);
void JS_FASTCALL BindNameNoCache(VMFrame &f, JSAtom *atom);
JSObject * JS_FASTCALL BindGlobalName(VMFrame &f);
template<JSBool strict> void JS_FASTCALL SetName(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL SetPropNoCache(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL SetGlobalName(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL SetGlobalNameNoCache(VMFrame &f, JSAtom *atom);
void JS_FASTCALL Name(VMFrame &f);
void JS_FASTCALL GetProp(VMFrame &f);
void JS_FASTCALL GetPropNoCache(VMFrame &f, JSAtom *atom);
void JS_FASTCALL GetElem(VMFrame &f);
void JS_FASTCALL CallElem(VMFrame &f);
template<JSBool strict> void JS_FASTCALL SetElem(VMFrame &f);
void JS_FASTCALL Length(VMFrame &f);
void JS_FASTCALL CallName(VMFrame &f);
void JS_FASTCALL PushImplicitThisForGlobal(VMFrame &f);
void JS_FASTCALL GetUpvar(VMFrame &f, uint32 index);
void JS_FASTCALL GetGlobalName(VMFrame &f);

template<JSBool strict> void JS_FASTCALL NameInc(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL NameDec(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL IncName(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL DecName(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL GlobalNameInc(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL GlobalNameDec(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL IncGlobalName(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL DecGlobalName(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL PropInc(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL PropDec(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL IncProp(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL DecProp(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL ElemInc(VMFrame &f);
template<JSBool strict> void JS_FASTCALL ElemDec(VMFrame &f);
template<JSBool strict> void JS_FASTCALL IncElem(VMFrame &f);
template<JSBool strict> void JS_FASTCALL DecElem(VMFrame &f);
void JS_FASTCALL CallProp(VMFrame &f, JSAtom *atom);
template <JSBool strict> void JS_FASTCALL DelProp(VMFrame &f, JSAtom *atom);
template <JSBool strict> void JS_FASTCALL DelElem(VMFrame &f);
void JS_FASTCALL DelName(VMFrame &f, JSAtom *atom);
JSBool JS_FASTCALL In(VMFrame &f);

void JS_FASTCALL DefVarOrConst(VMFrame &f, JSAtom *atom);
void JS_FASTCALL SetConst(VMFrame &f, JSAtom *atom);
template<JSBool strict> void JS_FASTCALL DefFun(VMFrame &f, JSFunction *fun);
JSObject * JS_FASTCALL DefLocalFun(VMFrame &f, JSFunction *fun);
JSObject * JS_FASTCALL DefLocalFun_FC(VMFrame &f, JSFunction *fun);
JSObject * JS_FASTCALL RegExp(VMFrame &f, JSObject *regex);
JSObject * JS_FASTCALL Lambda(VMFrame &f, JSFunction *fun);
JSObject * JS_FASTCALL LambdaForInit(VMFrame &f, JSFunction *fun);
JSObject * JS_FASTCALL LambdaForSet(VMFrame &f, JSFunction *fun);
JSObject * JS_FASTCALL LambdaJoinableForCall(VMFrame &f, JSFunction *fun);
JSObject * JS_FASTCALL LambdaJoinableForNull(VMFrame &f, JSFunction *fun);
JSObject * JS_FASTCALL FlatLambda(VMFrame &f, JSFunction *fun);
void JS_FASTCALL Arguments(VMFrame &f);
void JS_FASTCALL ArgSub(VMFrame &f, uint32 n);
void JS_FASTCALL EnterBlock(VMFrame &f, JSObject *obj);
void JS_FASTCALL LeaveBlock(VMFrame &f, JSObject *blockChain);

JSBool JS_FASTCALL LessThan(VMFrame &f);
JSBool JS_FASTCALL LessEqual(VMFrame &f);
JSBool JS_FASTCALL GreaterThan(VMFrame &f);
JSBool JS_FASTCALL GreaterEqual(VMFrame &f);
JSBool JS_FASTCALL Equal(VMFrame &f);
JSBool JS_FASTCALL NotEqual(VMFrame &f);

void JS_FASTCALL BitOr(VMFrame &f);
void JS_FASTCALL BitXor(VMFrame &f);
void JS_FASTCALL BitAnd(VMFrame &f);
void JS_FASTCALL BitNot(VMFrame &f);
void JS_FASTCALL Lsh(VMFrame &f);
void JS_FASTCALL Rsh(VMFrame &f);
void JS_FASTCALL Ursh(VMFrame &f);
void JS_FASTCALL Add(VMFrame &f);
void JS_FASTCALL Sub(VMFrame &f);
void JS_FASTCALL Mul(VMFrame &f);
void JS_FASTCALL Div(VMFrame &f);
void JS_FASTCALL Mod(VMFrame &f);
void JS_FASTCALL Neg(VMFrame &f);
void JS_FASTCALL Pos(VMFrame &f);
void JS_FASTCALL Not(VMFrame &f);
void JS_FASTCALL StrictEq(VMFrame &f);
void JS_FASTCALL StrictNe(VMFrame &f);

void JS_FASTCALL Iter(VMFrame &f, uint32 flags);
void JS_FASTCALL IterNext(VMFrame &f);
JSBool JS_FASTCALL IterMore(VMFrame &f);
void JS_FASTCALL EndIter(VMFrame &f);

JSBool JS_FASTCALL ValueToBoolean(VMFrame &f);
JSString * JS_FASTCALL TypeOf(VMFrame &f);
JSBool JS_FASTCALL InstanceOf(VMFrame &f);
void JS_FASTCALL FastInstanceOf(VMFrame &f);
void JS_FASTCALL ArgCnt(VMFrame &f);
void JS_FASTCALL Unbrand(VMFrame &f);

template <bool strict> int32 JS_FASTCALL ConvertToTypedInt(JSContext *cx, Value *vp);
void JS_FASTCALL ConvertToTypedFloat(JSContext *cx, Value *vp);

void JS_FASTCALL Exception(VMFrame &f);

} 







template<typename FuncPtr>
inline FuncPtr FunctionTemplateConditional(bool cond, FuncPtr a, FuncPtr b) {
    return cond ? a : b;
}

}} 

extern "C" void *
js_InternalThrow(js::VMFrame &f);

#endif 

