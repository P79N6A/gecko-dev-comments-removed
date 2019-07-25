








































#include "Ion.h"
#include "IonAnalysis.h"
#include "IonBuilder.h"
#include "IonSpewer.h"
#include "IonLIR.h"
#include "GreedyAllocator.h"
#include "LICM.h"
#include "ValueNumbering.h"
#include "LinearScan.h"

#if defined(JS_CPU_X86)
# include "x86/Lowering-x86.h"
# include "x86/CodeGenerator-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/Lowering-x64.h"
# include "x64/CodeGenerator-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/Lowering-arm.h"
# include "arm/CodeGenerator-arm.h"
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

bool
ion::InitializeIon()
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
IonContext *
ion::GetIonContext()
{
    return (IonContext *)PR_GetThreadPrivate(IonTLSIndex);
}

bool
ion::SetIonContext(IonContext *ctx)
{
    return PR_SetThreadPrivate(IonTLSIndex, ctx) == PR_SUCCESS;
}
#else
IonContext *
ion::GetIonContext()
{
    JS_ASSERT(GlobalIonContext);
    return GlobalIonContext;
}

bool
ion::SetIonContext(IonContext *ctx)
{
    GlobalIonContext = ctx;
    return true;
}
#endif

void
IonCode::release()
{
    if (pool_)
        pool_->release();
#ifdef DEBUG
    pool_ = NULL;
    code_ = NULL;
#endif
}

void
IonScript::Destroy(JSContext *cx, JSScript *script)
{
    if (!script->ion)
        return;

    script->ion->method.release();
    cx->free_(script->ion);
}

static bool
TestCompiler(IonBuilder &builder, MIRGraph &graph)
{
    IonSpewer spew(&graph, builder.script);
    spew.init();

    if (!builder.build())
        return false;
    spew.spewPass("Build SSA");

    if (!SplitCriticalEdges(&builder, graph))
        return false;
    spew.spewPass("Split Critical Edges");

    if (!ReorderBlocks(graph))
        return false;
    spew.spewPass("Reorder Blocks");

    if (!BuildDominatorTree(graph))
        return false;
    spew.spewPass("Dominator tree");

    if (!BuildPhiReverseMapping(graph))
        return false;
    

    if (!ApplyTypeInformation(graph))
        return false;
    spew.spewPass("Apply types");

    
    ValueNumberer gvn(graph, ION_GVN_PESSIMISTIC);
    if (!gvn.analyze())
        return false;
    spew.spewPass("GVN");

    LICM licm(graph);
    if (!licm.analyze())
        return false;
    spew.spewPass("LICM");

    LIRGraph lir;
    LIRBuilder lirgen(&builder, graph, lir);
    if (!lirgen.generate())
        return false;
    spew.spewPass("Generate LIR");

    
#ifndef ION_LSRA
    GreedyAllocator greedy(&builder, lir);
    if (!greedy.allocate())
        return false;
    spew.spewPass("Allocate registers");
#else
    LinearScanAllocator regalloc(&lirgen, lir);
    if (!regalloc.go())
        return false;
    spew.spewPass("Allocate Registers", &regalloc);
#endif

    CodeGenerator codegen(&builder, lir);
    if (!codegen.generate())
        return false;
    spew.spewPass("Code generation");

    spew.finish();

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

