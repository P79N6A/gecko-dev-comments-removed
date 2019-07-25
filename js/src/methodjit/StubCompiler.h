







































#if !defined(jsstub_compiler_h__) && defined(JS_METHODJIT)
#define jsstub_compiler_h__

#include "jscntxt.h"
#include "jstl.h"
#include "MethodJIT.h"
#include "methodjit/FrameState.h"
#include "CodeGenIncludes.h"

namespace js {
namespace mjit {

class Compiler;

class StubCompiler
{
    typedef JSC::MacroAssembler::Call Call;
    typedef JSC::MacroAssembler::Jump Jump;
    typedef JSC::MacroAssembler::Label Label;

    struct CrossPatch {
        CrossPatch(Jump from, Label to)
          : from(from), to(to)
        { }

        Jump from;
        Label to;
    };

    struct CrossJumpInScript {
        CrossJumpInScript(Jump from, jsbytecode *pc)
          : from(from), pc(pc)
        { }

        Jump from;
        jsbytecode *pc;
    };

    JSContext *cx;
    Compiler &cc;
    FrameState &frame;
    JSScript *script;

  public:
    Assembler masm;

  private:
    uint32 generation;
    uint32 lastGeneration;

    
    Vector<CrossPatch, 64, SystemAllocPolicy> exits;
    Vector<CrossPatch, 64, SystemAllocPolicy> joins;
    Vector<CrossJumpInScript, 64, SystemAllocPolicy> scriptJoins;
    Vector<Jump, 8, SystemAllocPolicy> jumpList;

  public:
    StubCompiler(JSContext *cx, mjit::Compiler &cc, FrameState &frame, JSScript *script);

    bool init(uint32 nargs);

    size_t size() {
        return masm.size();
    }

    uint8 *buffer() {
        return masm.buffer();
    }

    Call vpInc(JSOp op, uint32 depth);

#define STUB_CALL_TYPE(type)                                    \
    Call call(type stub) {                                      \
        return stubCall(JS_FUNC_TO_DATA_PTR(void *, stub));     \
    }

    STUB_CALL_TYPE(JSObjStub);
    STUB_CALL_TYPE(VoidStub);
    STUB_CALL_TYPE(VoidStubUInt32);
    STUB_CALL_TYPE(VoidPtrStubUInt32);
    STUB_CALL_TYPE(VoidPtrStub);
    STUB_CALL_TYPE(BoolStub);
    STUB_CALL_TYPE(VoidStubAtom);
    STUB_CALL_TYPE(VoidStubPC);

#undef STUB_CALL_TYPE

    



    JSC::MacroAssembler::Label syncExit(Uses uses);

    



    JSC::MacroAssembler::Label syncExitAndJump(Uses uses);

    
    JSC::MacroAssembler::Label linkExit(Jump j, Uses uses);
    void linkExitForBranch(Jump j);
    void linkExitDirect(Jump j, Label L);

    void leave();
    void leaveWithDepth(uint32 depth);

    




    void rejoin(Changes changes);
    void linkRejoin(Jump j);

    
    void fixCrossJumps(uint8 *ncode, size_t offset, size_t total);
    void finalize(uint8 *ncode);
    void jumpInScript(Jump j, jsbytecode *target);
    void crossJump(Jump j, Label l);

  private:
    Call stubCall(void *ptr);
    Call stubCall(void *ptr, uint32 slots);
};

} 
} 

#endif 

