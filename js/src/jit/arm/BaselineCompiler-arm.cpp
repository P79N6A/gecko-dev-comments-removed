





#include "jit/arm/BaselineCompiler-arm.h"

using namespace js;
using namespace js::jit;

BaselineCompilerARM::BaselineCompilerARM(JSContext *cx, HandleScript script)
  : BaselineCompilerShared(cx, script)
{
}
