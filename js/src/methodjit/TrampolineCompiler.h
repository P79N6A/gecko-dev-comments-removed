







































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
    TrampolineCompiler(JSC::ExecutableAllocator *pool, Trampolines *tramps)
      : execPool(pool), trampolines(tramps)
    { }

    bool compile();
    static void release(Trampolines *tramps);

private:
    bool compileTrampoline(void **where, JSC::ExecutablePool **pool,
                           TrampolineGenerator generator);
    
    
    static bool generateForceReturn(Assembler &masm);

#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
    static bool generateForceReturnFast(Assembler &masm);
#endif

    JSC::ExecutableAllocator *execPool;
    Trampolines *trampolines;
};

} 
} 

#endif

