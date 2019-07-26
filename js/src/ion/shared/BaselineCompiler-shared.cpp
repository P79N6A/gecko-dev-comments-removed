






#include "BaselineCompiler-shared.h"
#include "ion/BaselineIC.h"

using namespace js;
using namespace js::ion;

BaselineCompilerShared::BaselineCompilerShared(JSContext *cx, JSScript *script)
  : cx(cx),
    script(script),
    pc(NULL),
    frame(cx, script, masm),
    stubSpace_(),
    icEntries_(),
    icLoadLabels_()
{
}
