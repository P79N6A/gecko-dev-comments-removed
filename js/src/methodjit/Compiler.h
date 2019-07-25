






































#if !defined jsjaeger_compiler_h__ && defined JS_METHODJIT
#define jsjaeger_compiler_h__

#include "jscntxt.h"
#include "jstl.h"
#include "BytecodeAnalyzer.h"
#include "MethodJIT.h"
#include "CodeGenIncludes.h"
#include "StubCompiler.h"

namespace js {
namespace mjit {

class Compiler
{
    typedef JSC::MacroAssembler::Label Label;
    typedef JSC::MacroAssembler::Imm32 Imm32;
    typedef JSC::MacroAssembler::ImmPtr ImmPtr;
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::Jump Jump;
    typedef JSC::MacroAssembler::Call Call;

    struct BranchPatch {
        BranchPatch(const Jump &j, jsbytecode *pc)
          : jump(j), pc(pc)
        { }

        Jump jump;
        jsbytecode *pc;
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
    StubCompiler stubcc;

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
    void jsop_binary(JSOp op, VoidStub stub);
    void emitReturn();
    void dispatchCall(VoidPtrStubUInt32 stub);
    void jsop_nameinc(JSOp op, VoidStubAtom stub, uint32 index);

    
    void jsop_bitop(JSOp op);
    void jsop_globalinc(JSOp op, uint32 index);
    void jsop_relational(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    void jsop_neg();
    void jsop_bitnot();
    void jsop_objtostr();
    void jsop_not();
    void jsop_typeof();

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

#undef STUB_CALL_TYPE
    void prepareStubCall();
    Call stubCall(void *ptr, Uses uses, Defs defs);
};

} 
} 

#endif

