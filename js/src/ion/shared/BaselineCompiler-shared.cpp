






#include "BaselineCompiler-shared.h"
#include "ion/BaselineIC.h"

using namespace js;
using namespace js::ion;

BaselineCompilerShared::BaselineCompilerShared(JSContext *cx, JSScript *script)
  : cx(cx),
    script(cx, script),
    pc(NULL),
    ionCompileable_(ion::IsEnabled(cx) && CanIonCompileScript(script)),
    frame(cx, script, masm),
    stubSpace_(),
    icEntries_(),
    icLoadLabels_()
{
}
