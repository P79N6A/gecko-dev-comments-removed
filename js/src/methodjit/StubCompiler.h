







































#if !defined(jsstub_compiler_h__) && defined(JS_METHODJIT)
#define jsstub_compiler_h__

#include "jscntxt.h"
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
        CrossJumpInScript(Jump from, jsbytecode *pc, uint32 inlineIndex)
          : from(from), pc(pc), inlineIndex(inlineIndex)
        { }

        Jump from;
        jsbytecode *pc;
        uint32 inlineIndex;
    };

    JSContext *cx;
    Compiler &cc;
    FrameState &frame;

  public:
    Assembler masm;

  private:
    uint32 generation;
    uint32 lastGeneration;

    Vector<CrossPatch, 64, mjit::CompilerAllocPolicy> exits;
    Vector<CrossPatch, 64, mjit::CompilerAllocPolicy> joins;
    Vector<CrossJumpInScript, 64, mjit::CompilerAllocPolicy> scriptJoins;
    Vector<Jump, 8, SystemAllocPolicy> jumpList;

  public:
    StubCompiler(JSContext *cx, mjit::Compiler &cc, FrameState &frame);

    size_t size() {
        return masm.size();
    }

    uint8 *buffer() {
        return masm.buffer();
    }

    



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
    bool jumpInScript(Jump j, jsbytecode *target);
    unsigned crossJump(Jump j, Label l);

    Call emitStubCall(void *ptr, RejoinState rejoin, Uses uses);
    Call emitStubCall(void *ptr, RejoinState rejoin, Uses uses, int32 slots);

    void patchJoin(unsigned i, bool script, Assembler::Address address, AnyRegisterID reg);
};

} 
} 

#endif 

