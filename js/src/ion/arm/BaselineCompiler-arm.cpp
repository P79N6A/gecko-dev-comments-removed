






#include "BaselineCompiler-arm.h"

using namespace js;
using namespace js::ion;

BaselineCompilerARM::BaselineCompilerARM(JSContext *cx, JSScript *script)
  : BaselineCompilerShared(cx, script)
{
}
