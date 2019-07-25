








































#include "Ion.h"
#include "IonAnalysis.h"
#include "IonBuilder.h"
#include "IonSpew.h"
#include "IonLIR.h"

#if defined(JS_CPU_X86)
# include "x86/Lowering-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/Lowering-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/Lowering-arm.h"
#endif

using namespace js;
using namespace js::ion;

#ifdef JS_THREADSAFE
static bool IonTLSInitialized = false;
static PRUintn IonTLSIndex;
#else
static IonContext *GlobalIonContext;
#endif

IonContext::IonContext(JSContext *cx, TempAllocator *temp)
  : cx(cx),
    temp(temp)
{
    SetIonContext(this);
}

IonContext::~IonContext()
{
    SetIonContext(NULL);
}

bool ion::InitializeIon()
{
#ifdef JS_THREADSAFE
    if (!IonTLSInitialized) {
        PRStatus status = PR_NewThreadPrivateIndex(&IonTLSIndex, NULL);
        if (status != PR_SUCCESS)
            return false;
        IonTLSInitialized = true;
    }
#endif
    IonBuilder::SetupOpcodeFlags();
    return true;
}

#ifdef JS_THREADSAFE
IonContext *ion::GetIonContext()
{
    return (IonContext *)PR_GetThreadPrivate(IonTLSIndex);
}

bool ion::SetIonContext(IonContext *ctx)
{
    return PR_SetThreadPrivate(IonTLSIndex, ctx) == PR_SUCCESS;
}
#else
IonContext *ion::GetIonContext()
{
    JS_ASSERT(GlobalIonContext);
    return GlobalIonContext;
}

bool ion::SetIonContext(IonContext *ctx)
{
    GlobalIonContext = ctx;
    return true;
}
#endif

bool
ion::Go(JSContext *cx, JSScript *script, StackFrame *fp)
{
    TempAllocator temp(&cx->tempPool);
    IonContext ictx(cx, &temp);

    JSFunction *fun = fp->isFunctionFrame() ? fp->fun() : NULL;

    MIRGraph graph;

    DummyOracle oracle;
    C1Spewer spew(graph, script);
    IonBuilder builder(cx, script, fun, temp, graph, &oracle);
    spew.enable("/tmp/ion.cfg");

    if (!builder.build())
        return false;
    spew.spew("Build SSA");

    if (!ReorderBlocks(graph))
        return false;
    spew.spew("Reorder Blocks");

    if (!ApplyTypeInformation(graph))
        return false;
    spew.spew("Apply types");

    LIRBuilder lirgen(&builder, graph);
    if (!lirgen.generate())
        return false;
    spew.spew("Generate LIR");

    return false;
}

