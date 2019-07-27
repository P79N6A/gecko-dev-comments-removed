





#include "jit/mips/BaselineCompiler-mips.h"

using namespace js;
using namespace js::jit;

BaselineCompilerMIPS::BaselineCompilerMIPS(JSContext *cx, TempAllocator &alloc,
                                           JSScript *script)
  : BaselineCompilerShared(cx, alloc, script)
{
}
