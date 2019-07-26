






#include "BaselineCompiler-shared.h"

using namespace js;
using namespace js::ion;

BaselineCompilerShared::BaselineCompilerShared(JSContext *cx, JSScript *script)
  : cx(cx),
    script(script),
    pc(NULL),
    frame(cx, script, masm)
{
}
