





#include "jit/x64/BaselineCompiler-x64.h"

using namespace js;
using namespace js::jit;

BaselineCompilerX64::BaselineCompilerX64(JSContext *cx, HandleScript script)
  : BaselineCompilerX86Shared(cx, script)
{
}
