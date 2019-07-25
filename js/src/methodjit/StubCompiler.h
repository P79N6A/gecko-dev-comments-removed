







































#if !defined(jsstub_compiler_h__) && defined(JS_METHODJIT)
#define jsstub_compiler_h__

#include "jscntxt.h"
#include "jstl.h"
#include "MethodJIT.h"
#include "methodjit/FrameState.h"
#include "methodjit/nunbox/Assembler.h"
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

    Call vpInc(JSOp op, bool pushed);

#define STUB_CALL_TYPE(type)                                    \
    Call call(type stub) {                                      \
        return stubCall(JS_FUNC_TO_DATA_PTR(void *, stub));     \
    }

    STUB_CALL_TYPE(JSObjStub);
    STUB_CALL_TYPE(VoidStub);
    STUB_CALL_TYPE(BoolStub);

#undef STUB_CALL_TYPE

    
    void linkExit(Jump j);

    void leave();

    




    void rejoin(uint32 invalidationDepth);

    
    void fixCrossJumps(uint8 *ncode, size_t offset, size_t total);
    void finalize(uint8 *ncode);
    void jumpInScript(Jump j, jsbytecode *target);

  private:
    Call stubCall(void *ptr);
    Call stubCall(void *ptr, uint32 slots);
};

} 
} 

#endif 

