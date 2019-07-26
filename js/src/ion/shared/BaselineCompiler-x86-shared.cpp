





#include "BaselineCompiler-x86-shared.h"

using namespace js;
using namespace js::ion;

BaselineCompilerX86Shared::BaselineCompilerX86Shared(JSContext *cx, HandleScript script)
  : BaselineCompilerShared(cx, script)
{
}
