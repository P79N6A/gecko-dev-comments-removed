






































#if !defined jsjaeger_compiler_h__ && defined JS_METHODJIT
#define jsjaeger_compiler_h__

#include "jsanalyze.h"
#include "jscntxt.h"
#include "jstl.h"
#include "MethodJIT.h"
#include "CodeGenIncludes.h"
#include "BaseCompiler.h"
#include "StubCompiler.h"
#include "MonoIC.h"
#include "PolyIC.h"

namespace js {
namespace mjit {

class Compiler : public BaseCompiler
{
    friend class StubCompiler;

    struct BranchPatch {
        BranchPatch(const Jump &j, jsbytecode *pc)
          : jump(j), pc(pc)
        { }

        Jump jump;
        jsbytecode *pc;
    };

#if defined JS_MONOIC
    struct MICGenInfo {
        MICGenInfo(ic::MICInfo::Kind kind) : kind(kind)
        { }
        Label entry;
        Label stubEntry;
        DataLabel32 shape;
        DataLabelPtr addrLabel;
#if defined JS_PUNBOX64
        uint32 patchValueOffset;
#endif
        Label load;
        Call call;
        ic::MICInfo::Kind kind;
        jsbytecode *jumpTarget;
        Jump traceHint;
        MaybeJump slowTraceHint;
        union {
            struct {
                bool typeConst;
                bool dataConst;
                bool usePropertyCache;
            } name;
            struct {
                uint32 pcOffs;
            } tracer;
        } u;
    };

    struct EqualityGenInfo {
        DataLabelPtr addrLabel;
        Label stubEntry;
        Call stubCall;
        BoolStub stub;
        MaybeJump jumpToStub;
        Label fallThrough;
        jsbytecode *jumpTarget;
        bool trampoline;
        Label trampolineStart;
        ValueRemat lvr, rvr;
        Assembler::Condition cond;
        JSC::MacroAssembler::RegisterID tempReg;
    };
    
    struct TraceGenInfo {
        bool initialized;
        Label stubEntry;
        DataLabelPtr addrLabel;
        jsbytecode *jumpTarget;
        bool trampoline;
        Label trampolineStart;
        Jump traceHint;
        MaybeJump slowTraceHint;

        TraceGenInfo() : initialized(false) {}
    };

    
  public:
    struct CallGenInfo {
        CallGenInfo(jsbytecode *pc) : pc(pc) {}

        



        jsbytecode   *pc;
        DataLabelPtr funGuard;
        Jump         funJump;
        Jump         hotJump;
        Call         oolCall;
        Label        joinPoint;
        Label        slowJoinPoint;
        Label        slowPathStart;
        Label        hotPathLabel;
        DataLabelPtr addrLabel1;
        DataLabelPtr addrLabel2;
        Jump         oolJump;
        RegisterID   funObjReg;
        RegisterID   funPtrReg;
        FrameSize    frameSize;
    };

  private:
#endif

    



    struct CallPatchInfo {
        CallPatchInfo() : hasFastNcode(false), hasSlowNcode(false) {}
        Label joinPoint;
        DataLabelPtr fastNcodePatch;
        DataLabelPtr slowNcodePatch;
        bool hasFastNcode;
        bool hasSlowNcode;
    };

    struct BaseICInfo {
        BaseICInfo(JSOp op) : op(op)
        { }
        Label fastPathStart;
        Label fastPathRejoin;
        Label slowPathStart;
        Call slowPathCall;
        DataLabelPtr paramAddr;
        JSOp op;

        void copyTo(ic::BaseIC &to, JSC::LinkBuffer &full, JSC::LinkBuffer &stub) {
            to.fastPathStart = full.locationOf(fastPathStart);
            to.fastPathRejoin = full.locationOf(fastPathRejoin);
            to.slowPathStart = stub.locationOf(slowPathStart);
            to.slowPathCall = stub.locationOf(slowPathCall);
            to.op = op;
            JS_ASSERT(to.op == op);
        }
    };

    struct GetElementICInfo : public BaseICInfo {
        GetElementICInfo(JSOp op) : BaseICInfo(op)
        { }
        RegisterID  typeReg;
        RegisterID  objReg;
        ValueRemat  id;
        MaybeJump   typeGuard;
        Jump        claspGuard;
    };

    struct SetElementICInfo : public BaseICInfo {
        SetElementICInfo(JSOp op) : BaseICInfo(op)
        { }
        RegisterID  objReg;
        StateRemat  objRemat;
        ValueRemat  vr;
        Jump        capacityGuard;
        Jump        claspGuard;
        Jump        holeGuard;
        Int32Key    key;
    };

    struct PICGenInfo : public BaseICInfo {
        PICGenInfo(ic::PICInfo::Kind kind, JSOp op, bool usePropCache)
          : BaseICInfo(op), kind(kind), usePropCache(usePropCache)
        { }
        ic::PICInfo::Kind kind;
        Label typeCheck;
        RegisterID shapeReg;
        RegisterID objReg;
        RegisterID idReg;
        RegisterID typeReg;
        bool usePropCache;
        Label shapeGuard;
        jsbytecode *pc;
        JSAtom *atom;
        bool hasTypeCheck;
        ValueRemat vr;
# if defined JS_CPU_X64
        ic::PICLabels labels;
# endif

        void copySimpleMembersTo(ic::PICInfo &ic) const {
            ic.kind = kind;
            ic.shapeReg = shapeReg;
            ic.objReg = objReg;
            ic.atom = atom;
            ic.usePropCache = usePropCache;
            if (ic.isSet()) {
                ic.u.vr = vr;
            } else if (ic.isGet()) {
                ic.u.get.typeReg = typeReg;
                ic.u.get.hasTypeCheck = hasTypeCheck;
            }
        }

    };

    struct Defs {
        Defs(uint32 ndefs)
          : ndefs(ndefs)
        { }
        uint32 ndefs;
    };

    struct InternalCallSite {
        uint32 returnOffset;
        jsbytecode *pc;
        size_t id;
        bool call;
        bool ool;

        InternalCallSite(uint32 returnOffset, jsbytecode *pc, size_t id,
                         bool call, bool ool)
          : returnOffset(returnOffset), pc(pc), id(id), call(call), ool(ool)
        { }
    };

    JSStackFrame *fp;
    JSScript *script;
    JSObject *scopeChain;
    JSObject *globalObj;
    JSFunction *fun;
    bool isConstructing;
    analyze::Script *analysis;
    Label *jumpMap;
    bool *savedTraps;
    jsbytecode *PC;
    Assembler masm;
    FrameState frame;
    analyze::LifetimeScript liveness;
    js::Vector<BranchPatch, 64, CompilerAllocPolicy> branchPatches;
#if defined JS_MONOIC
    js::Vector<MICGenInfo, 64, CompilerAllocPolicy> mics;
    js::Vector<CallGenInfo, 64, CompilerAllocPolicy> callICs;
    js::Vector<EqualityGenInfo, 64, CompilerAllocPolicy> equalityICs;
    js::Vector<TraceGenInfo, 64, CompilerAllocPolicy> traceICs;
#endif
#if defined JS_POLYIC
    js::Vector<PICGenInfo, 16, CompilerAllocPolicy> pics;
    js::Vector<GetElementICInfo, 16, CompilerAllocPolicy> getElemICs;
    js::Vector<SetElementICInfo, 16, CompilerAllocPolicy> setElemICs;
#endif
    js::Vector<CallPatchInfo, 64, CompilerAllocPolicy> callPatches;
    js::Vector<InternalCallSite, 64, CompilerAllocPolicy> callSites;
    StubCompiler stubcc;
    Label invokeLabel;
    Label arityLabel;
    bool debugMode_;
    bool addTraceHints;
    bool recompiling;
#ifdef JS_TYPE_INFERENCE
    bool hasThisType;
    JSValueType thisType;
    js::Vector<JSValueType, 16> argumentTypes;
    js::Vector<JSValueType, 16> localTypes;
#endif
    bool oomInVector;       
    enum { NoApplyTricks, LazyArgsObj } applyTricks;

    Compiler *thisFromCtor() { return this; }

    friend class CompilerAllocPolicy;
  public:
    
    
    enum { LengthAtomIndex = uint32(-2) };

    Compiler(JSContext *cx, JSStackFrame *fp);
    ~Compiler();

    CompileStatus compile();

    jsbytecode *getPC() { return PC; }
    Label getLabel() { return masm.label(); }
    bool knownJump(jsbytecode *pc);
    Label labelOf(jsbytecode *target);
    void *findCallSite(const CallSite &callSite);
    void addCallSite(const InternalCallSite &callSite);
    void addReturnSite(Label joinPoint);
    bool loadOldTraps(const Vector<CallSite> &site);

    bool debugMode() { return debugMode_; }

  private:
    CompileStatus performCompilation(JITScript **jitp);
    CompileStatus generatePrologue();
    CompileStatus generateMethod();
    CompileStatus generateEpilogue();
    CompileStatus finishThisUp(JITScript **jitp);

    
    void fixDoubleTypes(Uses uses);
    void restoreAnalysisTypes(uint32 stackDepth);
    JSValueType knownThisType();
    JSValueType knownArgumentType(uint32 arg);
    JSValueType knownLocalType(uint32 local);
    JSValueType knownPushedType(uint32 pushed);
    js::types::ObjectKind knownPoppedObjectKind(uint32 popped);
    bool arrayPrototypeHasIndexedSetter();

    
    uint32 fullAtomIndex(jsbytecode *pc);
    bool jumpInScript(Jump j, jsbytecode *pc);
    bool compareTwoValues(JSContext *cx, JSOp op, const Value &lhs, const Value &rhs);
    bool canUseApplyTricks();

    
    void restoreFrameRegs(Assembler &masm);
    bool emitStubCmpOp(BoolStub stub, jsbytecode *target, JSOp fused);
    void iter(uintN flags);
    void iterNext();
    bool iterMore();
    void iterEnd();
    MaybeJump loadDouble(FrameEntry *fe, FPRegisterID *fpReg, bool *allocated);
#ifdef JS_POLYIC
    void passICAddress(BaseICInfo *ic);
#endif
#ifdef JS_MONOIC
    void passMICAddress(MICGenInfo &mic);
#endif
    bool constructThis();
    void ensureDouble(FrameEntry *fe);

    



    void ensureInteger(FrameEntry *fe, Uses uses);

    
    void truncateDoubleToInt32(FrameEntry *fe, Uses uses);

    
    bool jumpAndTrace(Jump j, jsbytecode *target, Jump *slow = NULL, bool *trampoline = NULL);
    bool finishLoop(jsbytecode *head);
    void jsop_bindname(uint32 index, bool usePropCache);
    void jsop_setglobal(uint32 index);
    void jsop_getglobal(uint32 index);
    void jsop_getprop_slow(JSAtom *atom, bool usePropCache = true);
    void jsop_getarg(uint32 slot);
    void jsop_setarg(uint32 slot, bool popped);
    void jsop_this();
    void emitReturn(FrameEntry *fe);
    void emitFinalReturn(Assembler &masm);
    void loadReturnValue(Assembler *masm, FrameEntry *fe);
    void emitReturnValue(Assembler *masm, FrameEntry *fe);
    void dispatchCall(VoidPtrStubUInt32 stub, uint32 argc);
    void interruptCheckHelper();
    void emitUncachedCall(uint32 argc, bool callingNew);
    void checkCallApplySpeculation(uint32 callImmArgc, uint32 speculatedArgc,
                                   FrameEntry *origCallee, FrameEntry *origThis,
                                   MaybeRegisterID origCalleeType, RegisterID origCalleeData,
                                   MaybeRegisterID origThisType, RegisterID origThisData,
                                   Jump *uncachedCallSlowRejoin, CallPatchInfo *uncachedCallPatch);
    void inlineCallHelper(uint32 argc, bool callingNew);
    void fixPrimitiveReturn(Assembler *masm, FrameEntry *fe);
    void jsop_gnameinc(JSOp op, VoidStubAtom stub, uint32 index);
    bool jsop_nameinc(JSOp op, VoidStubAtom stub, uint32 index);
    bool jsop_propinc(JSOp op, VoidStubAtom stub, uint32 index);
    void jsop_eleminc(JSOp op, VoidStub);
    void jsop_getgname(uint32 index, JSValueType type);
    void jsop_getgname_slow(uint32 index);
    void jsop_setgname(uint32 index, bool usePropertyCache);
    void jsop_setgname_slow(uint32 index, bool usePropertyCache);
    void jsop_bindgname();
    void jsop_setelem_slow();
    void jsop_getelem_slow();
    void jsop_callelem_slow();
    void jsop_unbrand();
    bool jsop_getprop(JSAtom *atom, JSValueType type, bool typeCheck = true, bool usePropCache = true);
    bool jsop_length();
    bool jsop_setprop(JSAtom *atom, bool usePropCache = true);
    void jsop_setprop_slow(JSAtom *atom, bool usePropCache = true);
    bool jsop_callprop_slow(JSAtom *atom);
    bool jsop_callprop(JSAtom *atom);
    bool jsop_callprop_obj(JSAtom *atom);
    bool jsop_callprop_str(JSAtom *atom);
    bool jsop_callprop_generic(JSAtom *atom);
    bool jsop_instanceof();
    void jsop_name(JSAtom *atom, JSValueType type);
    bool jsop_xname(JSAtom *atom);
    void enterBlock(JSObject *obj);
    void leaveBlock();
    void emitEval(uint32 argc);
    void jsop_arguments();

    
    void jsop_binary(JSOp op, VoidStub stub, JSValueType type);
    void jsop_binary_full(FrameEntry *lhs, FrameEntry *rhs, JSOp op, VoidStub stub,
                          JSValueType type);
    void jsop_binary_full_simple(FrameEntry *fe, JSOp op, VoidStub stub,
                                 JSValueType type);
    void jsop_binary_double(FrameEntry *lhs, FrameEntry *rhs, JSOp op, VoidStub stub,
                            JSValueType type);
    void slowLoadConstantDouble(Assembler &masm, FrameEntry *fe,
                                FPRegisterID fpreg);
    void maybeJumpIfNotInt32(Assembler &masm, MaybeJump &mj, FrameEntry *fe,
                             MaybeRegisterID &mreg);
    void maybeJumpIfNotDouble(Assembler &masm, MaybeJump &mj, FrameEntry *fe,
                              MaybeRegisterID &mreg);
    bool jsop_relational(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    bool jsop_relational_self(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    bool jsop_relational_full(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    bool jsop_relational_double(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    bool jsop_relational_int(JSOp op, jsbytecode *target, JSOp fused);

    void emitLeftDoublePath(FrameEntry *lhs, FrameEntry *rhs, FrameState::BinaryAlloc &regs,
                            MaybeJump &lhsNotDouble, MaybeJump &rhsNotNumber,
                            MaybeJump &lhsUnknownDone);
    void emitRightDoublePath(FrameEntry *lhs, FrameEntry *rhs, FrameState::BinaryAlloc &regs,
                             MaybeJump &rhsNotNumber2);
    bool tryBinaryConstantFold(JSContext *cx, FrameState &frame, JSOp op,
                               FrameEntry *lhs, FrameEntry *rhs, JSValueType type);

    
    void jsop_bitop(JSOp op);
    void jsop_rsh();
    RegisterID rightRegForShift(FrameEntry *rhs);
    void jsop_rsh_int_int(FrameEntry *lhs, FrameEntry *rhs);
    void jsop_rsh_const_int(FrameEntry *lhs, FrameEntry *rhs);
    void jsop_rsh_int_const(FrameEntry *lhs, FrameEntry *rhs);
    void jsop_rsh_int_unknown(FrameEntry *lhs, FrameEntry *rhs);
    void jsop_rsh_const_const(FrameEntry *lhs, FrameEntry *rhs);
    void jsop_rsh_const_unknown(FrameEntry *lhs, FrameEntry *rhs);
    void jsop_rsh_unknown_const(FrameEntry *lhs, FrameEntry *rhs);
    void jsop_rsh_unknown_any(FrameEntry *lhs, FrameEntry *rhs);
    void jsop_globalinc(JSOp op, uint32 index);
    void jsop_mod();
    void jsop_neg();
    void jsop_bitnot();
    void jsop_not();
    void jsop_typeof();
    bool booleanJumpScript(JSOp op, jsbytecode *target);
    bool jsop_ifneq(JSOp op, jsbytecode *target);
    bool jsop_andor(JSOp op, jsbytecode *target);
    void jsop_arginc(JSOp op, uint32 slot, bool popped);
    void jsop_localinc(JSOp op, uint32 slot, bool popped);
    void jsop_newinit();
    void jsop_initmethod();
    void jsop_initprop();
    void jsop_initelem();
    bool jsop_setelem();
    void jsop_setelem_dense();
    bool jsop_getelem(bool isCall);
    void jsop_getelem_dense(bool isPacked);
    bool isCacheableBaseAndIndex(FrameEntry *obj, FrameEntry *id);
    void jsop_stricteq(JSOp op);
    bool jsop_equality(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    bool jsop_equality_int_string(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    void jsop_pos();

   
    void prepareStubCall(Uses uses);
    Call emitStubCall(void *ptr);
};



#define INLINE_STUBCALL(stub)                                               \
    do {                                                                    \
        void *nstub = JS_FUNC_TO_DATA_PTR(void *, (stub));                  \
        Call cl = emitStubCall(nstub);                                      \
        InternalCallSite site(masm.callReturnOffset(cl), PC, (size_t)nstub, \
                              true, false);                                 \
        addCallSite(site);                                                  \
    } while (0)                                                             \




#define OOL_STUBCALL(stub)                                                  \
    stubcc.emitStubCall(JS_FUNC_TO_DATA_PTR(void *, (stub)))


#define OOL_STUBCALL_LOCAL_SLOTS(stub, slots)                               \
    stubcc.emitStubCall(JS_FUNC_TO_DATA_PTR(void *, (stub)), (slots))       \

} 
} 

#endif

