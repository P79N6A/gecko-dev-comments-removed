







































#if !defined(jsstub_compiler_h__) && defined(JS_METHODJIT)
#define jsstub_compiler_h__

#include "jscntxt.h"
#include "jstl.h"
#include "MethodJIT.h"
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

    struct ExitPatch {
        ExitPatch(Jump from, Label to)
          : from(from), to(to)
        { }

        Jump from;
        Label to;
    };

    JSContext *cx;
    Compiler &cc;
    FrameState &frame;
    JSScript *script;
    Assembler masm;
    Vector<ExitPatch, 64, SystemAllocPolicy> exits;
    RegSnapshot snapshot;

  public:
    StubCompiler(JSContext *cx, mjit::Compiler &cc, FrameState &frame, JSScript *script);

    size_t size() {
        return masm.size();
    }

    uint8 *buffer() {
        return masm.buffer();
    }

#define STUB_CALL_TYPE(type)                                    \
    Call call(type stub) {                                      \
        return stubCall(JS_FUNC_TO_DATA_PTR(void *, stub));     \
    }

    STUB_CALL_TYPE(JSObjStub);

#undef STUB_CALL_TYPE

    
    void linkExit(Jump j);

    



    void leave();

    




    void rejoin(uint32 invalidationDepth);

    
    void finalize(uint8 *ncode);

  private:
    Call stubCall(void *ptr);
};

} 
} 

#endif 

