






































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

    struct MICGenInfo {
        Label entry;
        Label stubEntry;
        DataLabelPtr shapeVal;
        Label load;
        Call call;
        ic::MICInfo::Type type;
        bool typeConst;
        bool dataConst;
        bool dataWrite;
    };

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
        RegisterID typeReg;
        Label shapeGuard;
        JSAtom *atom;
        StateRemat objRemat;
        Call callReturn;
        bool hasTypeCheck;
        ValueRemat vr;
    };

    struct Uses {
        Uses(uint32 nuses)
          : nuses(nuses)
        { }
        uint32 nuses;
    };

    struct Defs {
        Defs(uint32 ndefs)
          : ndefs(ndefs)
        { }
        uint32 ndefs;
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
    js::Vector<MICGenInfo, 64> mics;
    js::Vector<PICGenInfo, 64> pics;
    StubCompiler stubcc;
    Label invokeLabel;

  public:
    
    
    enum { LengthAtomIndex = uint32(-2) };

    Compiler(JSContext *cx, JSScript *script, JSFunction *fun, JSObject *scopeChain);
    ~Compiler();

    CompileStatus Compile();

    jsbytecode *getPC() { return PC; }
    Label getLabel() { return masm.label(); }
    bool knownJump(jsbytecode *pc);
    Label labelOf(jsbytecode *target);

  private:
    CompileStatus generatePrologue();
    CompileStatus generateMethod();
    CompileStatus generateEpilogue();
    CompileStatus finishThisUp();

    
    uint32 fullAtomIndex(jsbytecode *pc);
    void jumpInScript(Jump j, jsbytecode *pc);
    JSC::ExecutablePool *getExecPool(size_t size);
    bool compareTwoValues(JSContext *cx, JSOp op, const Value &lhs, const Value &rhs);

    
    void restoreFrameRegs();
    void emitStubCmpOp(BoolStub stub, jsbytecode *target, JSOp fused);
    void iterNext();
    void iterMore();

    
    void jsop_bindname(uint32 index);
    void jsop_setglobal(uint32 index);
    void jsop_getglobal(uint32 index);
    void jsop_getprop_slow();
    void jsop_getarg(uint32 index);
    void jsop_this();
    void jsop_binary(JSOp op, VoidStub stub);
    void emitReturn();
    void dispatchCall(VoidPtrStubUInt32 stub, uint32 argc);
    void inlineCallHelper(uint32 argc, bool callingNew);
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

    
    void jsop_bitop(JSOp op);
    void jsop_globalinc(JSOp op, uint32 index);
    void jsop_relational(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    void jsop_neg();
    void jsop_bitnot();
    void jsop_objtostr();
    void jsop_not();
    void jsop_typeof();
    void jsop_arginc(JSOp op, uint32 slot, bool popped);
    void jsop_localinc(JSOp op, uint32 slot, bool popped);
    void jsop_setelem();
    void jsop_getelem();
    void jsop_stricteq(JSOp op);
    void jsop_equality(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    void jsop_pos();

#define STUB_CALL_TYPE(type)                                            \
    Call stubCall(type stub, Uses uses, Defs defs) {                    \
        return stubCall(JS_FUNC_TO_DATA_PTR(void *, stub), uses, defs); \
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

#undef STUB_CALL_TYPE
    void prepareStubCall();
    Call stubCall(void *ptr, Uses uses, Defs defs);
};

} 
} 

#endif

