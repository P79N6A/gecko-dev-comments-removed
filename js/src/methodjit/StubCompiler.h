







































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

    Vector<CrossPatch, 64, mjit::CompilerAllocPolicy> exits;
    Vector<CrossPatch, 64, mjit::CompilerAllocPolicy> joins;
    Vector<CrossJumpInScript, 64, mjit::CompilerAllocPolicy> scriptJoins;
    Vector<Jump, 8, SystemAllocPolicy> jumpList;

  public:
    StubCompiler(JSContext *cx, mjit::Compiler &cc, FrameState &frame, JSScript *script);

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
    void crossJump(Jump j, Label l);

    Call emitStubCall(void *ptr, uint32 id);
    Call emitStubCall(void *ptr, int32 slots, uint32 id);
};

} 
} 

#endif 

