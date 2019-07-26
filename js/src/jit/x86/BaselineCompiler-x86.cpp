





#include "jit/x86/BaselineCompiler-x86.h"

using namespace js;
using namespace js::jit;

BaselineCompilerX86::BaselineCompilerX86(JSContext *cx, TempAllocator &alloc, HandleScript script)
  : BaselineCompilerX86Shared(cx, alloc, script)
{
}
