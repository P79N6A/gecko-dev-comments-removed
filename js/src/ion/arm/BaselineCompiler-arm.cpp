





#include "BaselineCompiler-arm.h"

using namespace js;
using namespace js::ion;

BaselineCompilerARM::BaselineCompilerARM(JSContext *cx, HandleScript script)
  : BaselineCompilerShared(cx, script)
{
}
