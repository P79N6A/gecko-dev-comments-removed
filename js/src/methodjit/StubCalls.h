







































#if !defined jslogic_h__ && defined JS_METHODJIT
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
void JS_FASTCALL NewInitArray(VMFrame &f, uint32 count);
void JS_FASTCALL NewInitObject(VMFrame &f, JSObject *base);
void JS_FASTCALL Trap(VMFrame &f, uint32 trapTypes);
void JS_FASTCALL DebuggerStatement(VMFrame &f, jsbytecode *pc);
void JS_FASTCALL Interrupt(VMFrame &f, jsbytecode *pc);
void JS_FASTCALL RecompileForInline(VMFrame &f);
void JS_FASTCALL InitElem(VMFrame &f, uint32 last);
void JS_FASTCALL InitProp(VMFrame &f, JSAtom *atom);
void JS_FASTCALL InitMethod(VMFrame &f, JSAtom *atom);

void JS_FASTCALL HitStackQuota(VMFrame &f);
void * JS_FASTCALL FixupArity(VMFrame &f, uint32 argc);
void * JS_FASTCALL CompileFunction(VMFrame &f, uint32 argc);
void JS_FASTCALL SlowNew(VMFrame &f, uint32 argc);
void JS_FASTCALL SlowCall(VMFrame &f, uint32 argc);
void * JS_FASTCALL UncachedNew(VMFrame &f, uint32 argc);
void * JS_FASTCALL UncachedCall(VMFrame &f, uint32 argc);
void * JS_FASTCALL UncachedLoweredCall(VMFrame &f, uint32 argc);
void JS_FASTCALL Eval(VMFrame &f, uint32 argc);
void JS_FASTCALL ScriptDebugPrologue(VMFrame &f);
void JS_FASTCALL ScriptDebugEpilogue(VMFrame &f);
void JS_FASTCALL ScriptProbeOnlyPrologue(VMFrame &f);
void JS_FASTCALL ScriptProbeOnlyEpilogue(VMFrame &f);












struct UncachedCallResult {
    JSObject   *callee;       
    JSFunction *fun;          
    void       *codeAddr;     
    bool       unjittable;    

    void init() {
        callee = NULL;
        fun = NULL;
        codeAddr = NULL;
        unjittable = false;
    }        
};






void UncachedCallHelper(VMFrame &f, uint32 argc, bool lowered, UncachedCallResult *ucr);
void UncachedNewHelper(VMFrame &f, uint32 argc, UncachedCallResult *ucr);

void JS_FASTCALL CreateThis(VMFrame &f, JSObject *proto);
void JS_FASTCALL Throw(VMFrame &f);
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
void JS_FASTCALL ToId(VMFrame &f);
void JS_FASTCALL CallName(VMFrame &f);
void JS_FASTCALL PushImplicitThisForGlobal(VMFrame &f);
void JS_FASTCALL GetUpvar(VMFrame &f, uint32 index);
void JS_FASTCALL GetGlobalName(VMFrame &f);

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
void JS_FASTCALL RegExp(VMFrame &f, JSObject *regex);
JSObject * JS_FASTCALL Lambda(VMFrame &f, JSFunction *fun);
JSObject * JS_FASTCALL LambdaJoinableForInit(VMFrame &f, JSFunction *fun);
JSObject * JS_FASTCALL LambdaJoinableForSet(VMFrame &f, JSFunction *fun);
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
void JS_FASTCALL IterNext(VMFrame &f, int32 offset);
JSBool JS_FASTCALL IterMore(VMFrame &f);
void JS_FASTCALL EndIter(VMFrame &f);

JSBool JS_FASTCALL ValueToBoolean(VMFrame &f);
JSString * JS_FASTCALL TypeOf(VMFrame &f);
JSBool JS_FASTCALL InstanceOf(VMFrame &f);
void JS_FASTCALL FastInstanceOf(VMFrame &f);
void JS_FASTCALL ArgCnt(VMFrame &f);
void JS_FASTCALL Unbrand(VMFrame &f);
void JS_FASTCALL UnbrandThis(VMFrame &f);





void JS_FASTCALL TypeBarrierHelper(VMFrame &f, uint32 which);
void JS_FASTCALL TypeBarrierReturn(VMFrame &f, Value *vp);
void JS_FASTCALL NegZeroHelper(VMFrame &f);

void JS_FASTCALL StubTypeHelper(VMFrame &f, int32 which);

void JS_FASTCALL CheckArgumentTypes(VMFrame &f);

#ifdef DEBUG
void JS_FASTCALL AssertArgumentTypes(VMFrame &f);
#endif

void JS_FASTCALL MissedBoundsCheckEntry(VMFrame &f);
void JS_FASTCALL MissedBoundsCheckHead(VMFrame &f);
void * JS_FASTCALL InvariantFailure(VMFrame &f, void *repatchCode);

template <bool strict> int32 JS_FASTCALL ConvertToTypedInt(JSContext *cx, Value *vp);
void JS_FASTCALL ConvertToTypedFloat(JSContext *cx, Value *vp);

void JS_FASTCALL Exception(VMFrame &f);

void JS_FASTCALL FunctionFramePrologue(VMFrame &f);
void JS_FASTCALL FunctionFrameEpilogue(VMFrame &f);

void JS_FASTCALL AnyFrameEpilogue(VMFrame &f);

JSObject * JS_FASTCALL
NewDenseUnallocatedArray(VMFrame &f, uint32 length);

void JS_FASTCALL
ArrayShift(VMFrame &f);

} 







template<typename FuncPtr>
inline FuncPtr FunctionTemplateConditional(bool cond, FuncPtr a, FuncPtr b) {
    return cond ? a : b;
}

}} 

extern "C" void *
js_InternalThrow(js::VMFrame &f);

extern "C" void *
js_InternalInterpret(void *returnData, void *returnType, void *returnReg, js::VMFrame &f);

#endif 

