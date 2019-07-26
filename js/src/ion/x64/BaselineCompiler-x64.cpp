






#include "BaselineCompiler-x64.h"

using namespace js;
using namespace js::ion;

BaselineCompilerX64::BaselineCompilerX64(JSContext *cx, JSScript *script)
  : BaselineCompilerX86Shared(cx, script)
{
}
