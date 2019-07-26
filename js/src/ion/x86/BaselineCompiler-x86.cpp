






#include "BaselineCompiler-x86.h"

using namespace js;
using namespace js::ion;

BaselineCompilerX86::BaselineCompilerX86(JSContext *cx, JSScript *script)
  : BaselineCompilerX86Shared(cx, script)
{
}
