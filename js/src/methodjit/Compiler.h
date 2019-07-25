






































#if !defined jsjaeger_compiler_h__ && defined JS_METHODJIT
#define jsjaeger_compiler_h__

#include "jscntxt.h"
#include "jstl.h"
#include "BytecodeAnalyzer.h"
#include "MethodJIT.h"
#include "CodeGenIncludes.h"
#include "StubCompiler.h"
#include "MonoIC.h"
#include "PolyIC.h"

namespace js {
namespace mjit {

class Compiler
{
    typedef JSC::MacroAssembler::Label Label;
    typedef JSC::MacroAssembler::Imm32 Imm32;
    typedef JSC::MacroAssembler::ImmPtr ImmPtr;
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::FPRegisterID FPRegisterID;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::BaseIndex BaseIndex;
    typedef JSC::MacroAssembler::Jump Jump;
    typedef JSC::MacroAssembler::Call Call;
    typedef JSC::MacroAssembler::DataLabelPtr DataLabelPtr;

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
        DataLabelPtr shapeVal;
        Label load;
        Call call;
        ic::MICInfo::Kind kind;
        jsbytecode *jumpTarget;
        Jump traceHint;
        union {
            struct {
                bool typeConst;
                bool dataConst;
                bool dataWrite;
            } name;
            struct {
                uint32 pcOffs;
            } tracer;
        } u;
    };
#endif

#if defined JS_POLYIC
    struct PICGenInfo {
        PICGenInfo(ic::PICInfo::Kind kind) : kind(kind)
        { }
        ic::PICInfo::Kind kind;
        Label hotPathBegin;
        Label storeBack;
        Label typeCheck;
        Label slowPathStart;
        RegisterID shapeReg;
        RegisterID objReg;
        RegisterID idReg;
        RegisterID typeReg;
        Label shapeGuard;
        JSAtom *atom;
        StateRemat objRemat;
        StateRemat idRemat;
        Call callReturn;
        bool hasTypeCheck;
        ValueRemat vr;

        void copySimpleMembersTo(ic::PICInfo &pi) const {
            pi.kind = kind;
            pi.shapeReg = shapeReg;
            pi.objReg = objReg;
            pi.atom = atom;
            if (kind == ic::PICInfo::SET) {
                pi.u.vr = vr;
            } else if (kind != ic::PICInfo::NAME) {
                pi.u.get.idReg = idReg;
                pi.u.get.typeReg = typeReg;
                pi.u.get.hasTypeCheck = hasTypeCheck;
                pi.u.get.objRemat = objRemat.offset;
            }
        }

    };
#endif

    struct Defs {
        Defs(uint32 ndefs)
          : ndefs(ndefs)
        { }
        uint32 ndefs;
    };

    class MaybeJump {
      public:
        MaybeJump()
          : set(false)
        { }

        inline Jump getJump() const { JS_ASSERT(set); return jump; }
        inline Jump get() const { JS_ASSERT(set); return jump; }
        inline void setJump(const Jump &j) { jump = j; set = true; }
        inline bool isSet() const { return set; }

        inline MaybeJump &operator=(Jump j) { setJump(j); return *this; }

      private:
        Jump jump;
        bool set;
    };

    struct InternalCallSite {
        bool stub;
        Label location;
        jsbytecode *pc;
        uint32 id;
    };

    struct DoublePatch {
        double d;
        DataLabelPtr label;
        bool ool;
    };

    JSContext *cx;
    JSScript *script;
    JSObject *scopeChain;
    JSObject *globalObj;
    JSFunction *fun;
    BytecodeAnalyzer analysis;
    Label *jumpMap;
    jsbytecode *PC;
    Assembler masm;
    FrameState frame;
    js::Vector<BranchPatch, 64> branchPatches;
#if defined JS_MONOIC
    js::Vector<MICGenInfo, 64> mics;
#endif
#if defined JS_POLYIC
    js::Vector<PICGenInfo, 64> pics;
#endif
    js::Vector<InternalCallSite, 64> callSites;
    js::Vector<DoublePatch, 16> doubleList;
    StubCompiler stubcc;
    Label invokeLabel;
    bool addTraceHints;

  public:
    
    
    enum { LengthAtomIndex = uint32(-2) };

    Compiler(JSContext *cx, JSScript *script, JSFunction *fun, JSObject *scopeChain);
    ~Compiler();

    CompileStatus Compile();

    jsbytecode *getPC() { return PC; }
    Label getLabel() { return masm.label(); }
    bool knownJump(jsbytecode *pc);
    Label labelOf(jsbytecode *target);
    void *findCallSite(const CallSite &callSite);

  private:
    CompileStatus generatePrologue();
    CompileStatus generateMethod();
    CompileStatus generateEpilogue();
    CompileStatus finishThisUp();

    
    uint32 fullAtomIndex(jsbytecode *pc);
    void jumpInScript(Jump j, jsbytecode *pc);
    JSC::ExecutablePool *getExecPool(size_t size);
    bool compareTwoValues(JSContext *cx, JSOp op, const Value &lhs, const Value &rhs);
    void addCallSite(uint32 id, bool stub);

    
    void restoreFrameRegs(Assembler &masm);
    void emitStubCmpOp(BoolStub stub, jsbytecode *target, JSOp fused);
    void iterNext();
    void iterMore();
    MaybeJump loadDouble(FrameEntry *fe, FPRegisterID fpReg);

    
    void jumpAndTrace(Jump j, jsbytecode *target);
    void jsop_bindname(uint32 index);
    void jsop_setglobal(uint32 index);
    void jsop_getglobal(uint32 index);
    void jsop_getprop_slow();
    void jsop_getarg(uint32 index);
    void jsop_this();
    void emitReturn();
    void dispatchCall(VoidPtrStubUInt32 stub, uint32 argc);
    void interruptCheckHelper();
    void inlineCallHelper(uint32 argc, bool callingNew);
    void jsop_gnameinc(JSOp op, VoidStubAtom stub, uint32 index);
    void jsop_nameinc(JSOp op, VoidStubAtom stub, uint32 index);
    void jsop_propinc(JSOp op, VoidStubAtom stub, uint32 index);
    void jsop_eleminc(JSOp op, VoidStub);
    void jsop_getgname(uint32 index);
    void jsop_getgname_slow(uint32 index);
    void jsop_setgname(uint32 index);
    void jsop_setgname_slow(uint32 index);
    void jsop_bindgname();
    void jsop_setelem_slow();
    void jsop_getelem_slow();
    void jsop_unbrand();
    void jsop_getprop(JSAtom *atom, bool typeCheck = true);
    void jsop_length();
    void jsop_setprop(JSAtom *atom);
    void jsop_setprop_slow(JSAtom *atom);
    bool jsop_callprop_slow(JSAtom *atom);
    bool jsop_callprop(JSAtom *atom);
    bool jsop_callprop_obj(JSAtom *atom);
    bool jsop_callprop_str(JSAtom *atom);
    bool jsop_callprop_generic(JSAtom *atom);
    void jsop_instanceof();
    void jsop_name(JSAtom *atom);

    
    void jsop_binary(JSOp op, VoidStub stub);
    void jsop_binary_full(FrameEntry *lhs, FrameEntry *rhs, JSOp op, VoidStub stub);
    void jsop_binary_full_simple(FrameEntry *fe, JSOp op, VoidStub stub);
    void jsop_binary_double(FrameEntry *lhs, FrameEntry *rhs, JSOp op, VoidStub stub);
    void slowLoadConstantDouble(Assembler &masm, FrameEntry *fe,
                                FPRegisterID fpreg);
    void maybeJumpIfNotInt32(Assembler &masm, MaybeJump &mj, FrameEntry *fe,
                             MaybeRegisterID &mreg);
    void maybeJumpIfNotDouble(Assembler &masm, MaybeJump &mj, FrameEntry *fe,
                              MaybeRegisterID &mreg);
    void jsop_relational(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    void jsop_relational_int(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    void jsop_relational_self(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    void jsop_relational_full(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    void jsop_relational_double(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);

    void emitLeftDoublePath(FrameEntry *lhs, FrameEntry *rhs, FrameState::BinaryAlloc &regs,
                            MaybeJump &lhsNotDouble, MaybeJump &rhsNotNumber,
                            MaybeJump &lhsUnknownDone);
    void emitRightDoublePath(FrameEntry *lhs, FrameEntry *rhs, FrameState::BinaryAlloc &regs,
                             MaybeJump &rhsNotNumber2);

    
    void jsop_bitop(JSOp op);
    void jsop_globalinc(JSOp op, uint32 index);
    void jsop_mod();
    void jsop_neg();
    void jsop_bitnot();
    void jsop_objtostr();
    void jsop_not();
    void jsop_typeof();
    void booleanJumpScript(JSOp op, jsbytecode *target);
    void jsop_ifneq(JSOp op, jsbytecode *target);
    void jsop_andor(JSOp op, jsbytecode *target);
    void jsop_arginc(JSOp op, uint32 slot, bool popped);
    void jsop_localinc(JSOp op, uint32 slot, bool popped);
    void jsop_setelem();
    void jsop_getelem();
    void jsop_getelem_known_type(FrameEntry *obj, FrameEntry *id, RegisterID tmpReg);
    void jsop_getelem_with_pic(FrameEntry *obj, FrameEntry *id, RegisterID tmpReg);
    void jsop_getelem_nopic(FrameEntry *obj, FrameEntry *id, RegisterID tmpReg);
    void jsop_getelem_pic(FrameEntry *obj, FrameEntry *id, RegisterID objReg, RegisterID idReg,
                          RegisterID shapeReg);
    void jsop_getelem_dense(FrameEntry *obj, FrameEntry *id, RegisterID objReg,
                            MaybeRegisterID &idReg, RegisterID shapeReg);
    void jsop_stricteq(JSOp op);
    void jsop_equality(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    void jsop_pos();

#define STUB_CALL_TYPE(type)                                            \
    Call stubCall(type stub) {                                          \
        return stubCall(JS_FUNC_TO_DATA_PTR(void *, stub));             \
    }

    STUB_CALL_TYPE(JSObjStub);
    STUB_CALL_TYPE(VoidStubUInt32);
    STUB_CALL_TYPE(VoidStub);
    STUB_CALL_TYPE(VoidPtrStubUInt32);
    STUB_CALL_TYPE(VoidPtrStub);
    STUB_CALL_TYPE(BoolStub);
    STUB_CALL_TYPE(JSObjStubUInt32);
    STUB_CALL_TYPE(JSObjStubFun);
    STUB_CALL_TYPE(JSObjStubJSObj);
    STUB_CALL_TYPE(VoidStubAtom);
    STUB_CALL_TYPE(JSStrStub);
    STUB_CALL_TYPE(JSStrStubUInt32);
    STUB_CALL_TYPE(VoidStubJSObj);
    STUB_CALL_TYPE(VoidPtrStubPC);
    STUB_CALL_TYPE(VoidVpStub);
    STUB_CALL_TYPE(VoidStubPC);

#undef STUB_CALL_TYPE
    void prepareStubCall(Uses uses);
    Call stubCall(void *ptr);
};

} 
} 

#endif

