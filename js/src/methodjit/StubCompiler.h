







































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

struct StubCallInfo {
    uint32 numSpills;
};

class StubCompiler
{
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
    Vector<ExitPatch, 64, ContextAllocPolicy> exits;

  public:
    StubCompiler(JSContext *cx, mjit::Compiler &cc, FrameState &frame, JSScript *script);
    void linkExit(Jump j);
    void syncAndSpill();
    void call(JSObjStub stub) { scall(JS_FUNC_TO_DATA_PTR(void *, stub)); }

  private:
    void scall(void *ptr);
    void *getCallTarget(void *fun);
};

} 
} 

#endif 

