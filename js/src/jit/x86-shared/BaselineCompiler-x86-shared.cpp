





#include "jit/x86-shared/BaselineCompiler-x86-shared.h"

using namespace js;
using namespace js::jit;

BaselineCompilerX86Shared::BaselineCompilerX86Shared(JSContext* cx, TempAllocator& alloc, JSScript* script)
  : BaselineCompilerShared(cx, alloc, script)
{
}
