





#include "jit/shared/BaselineCompiler-x86-shared.h"

using namespace js;
using namespace js::jit;

BaselineCompilerX86Shared::BaselineCompilerX86Shared(JSContext *cx, HandleScript script)
  : BaselineCompilerShared(cx, script)
{
}
