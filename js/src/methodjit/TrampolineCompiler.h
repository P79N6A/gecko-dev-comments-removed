





#if !defined trampolines_h__ && defined JS_METHODJIT
#define trampolines_h__

#include "assembler/jit/ExecutableAllocator.h"
#include "methodjit/CodeGenIncludes.h"

namespace js {
namespace mjit {

class TrampolineCompiler
{
    typedef bool (*TrampolineGenerator)(Assembler &masm);

public:
    TrampolineCompiler(JSC::ExecutableAllocator *alloc, Trampolines *tramps)
      : execAlloc(alloc), trampolines(tramps)
    { }

    bool compile();
    static void release(Trampolines *tramps);

private:
    bool compileTrampoline(Trampolines::TrampolinePtr *where, JSC::ExecutablePool **pool,
                           TrampolineGenerator generator);

    
    static bool generateForceReturn(Assembler &masm);

#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
    static bool generateForceReturnFast(Assembler &masm);
#endif

    JSC::ExecutableAllocator *execAlloc;
    Trampolines *trampolines;
};

} 
} 

#endif

