








































#include "Ion.h"
#include "IonAnalysis.h"
#include "IonBuilder.h"
#include "IonSpew.h"
#include "IonLIR.h"
#include "GreedyAllocator.h"

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
    CheckLogging();
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

static bool
TestCompiler(IonBuilder &builder, MIRGraph &graph)
{
    C1Spewer spew(graph, builder.script);
    spew.enable("/tmp/ion.cfg");

    if (!builder.build())
        return false;
    spew.spew("Build SSA");

    if (!ReorderBlocks(graph))
        return false;
    spew.spew("Reorder Blocks");

    if (!BuildPhiReverseMapping(graph))
        return false;
    

    if (!ApplyTypeInformation(graph))
        return false;
    spew.spew("Apply types");

    LIRGraph lir;
    LIRBuilder lirgen(&builder, graph, lir);
    if (!lirgen.generate())
        return false;
    spew.spew("Generate LIR");

    GreedyAllocator greedy(&builder, lir);
    if (!greedy.allocate())
        return false;
    spew.spew("Allocate registers");

    return true;
}

static bool
IonCompile(JSContext *cx, JSScript *script, StackFrame *fp)
{
    TempAllocator temp(&cx->tempPool);
    IonContext ictx(cx, &temp);

    MIRGraph graph;
    DummyOracle oracle;

    JSFunction *fun = fp->isFunctionFrame() ? fp->fun() : NULL;
    IonBuilder builder(cx, script, fun, temp, graph, &oracle);
    if (!TestCompiler(builder, graph))
        return false;

    return true;
}

bool
ion::Go(JSContext *cx, JSScript *script, StackFrame *fp)
{
    if (!IonCompile(cx, script, fp))
        return false;

    
    return false;
}

