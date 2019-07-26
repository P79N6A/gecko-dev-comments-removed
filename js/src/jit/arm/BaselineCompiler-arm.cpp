





#include "jit/arm/BaselineCompiler-arm.h"

using namespace js;
using namespace js::jit;

BaselineCompilerARM::BaselineCompilerARM(JSContext *cx, TempAllocator &alloc, HandleScript script)
  : BaselineCompilerShared(cx, alloc, script)
{
}
