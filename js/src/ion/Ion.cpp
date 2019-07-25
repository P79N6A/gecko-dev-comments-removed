








































#include "Ion.h"
#include "IonAnalysis.h"
#include "IonBuilder.h"
#include "IonSpewer.h"
#include "IonLIR.h"
#include "GreedyAllocator.h"
#include "LICM.h"
#include "ValueNumbering.h"
#include "LinearScan.h"
#include "jscompartment.h"
#include "IonCompartment.h"
#include "CodeGenerator.h"

#if defined(JS_CPU_X86)
# include "x86/Lowering-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/Lowering-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/Lowering-arm.h"
#endif
#include "jsgcmark.h"
#include "jsgcinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "vm/Stack-inl.h"

using namespace js;
using namespace js::ion;

IonOptions ion::js_IonOptions;


JS_STATIC_ASSERT(sizeof(IonCode) % gc::Cell::CellSize == 0);

#ifdef JS_THREADSAFE
static bool IonTLSInitialized = false;
static PRUintn IonTLSIndex;

static inline IonContext *
CurrentIonContext()
{
    return (IonContext *)PR_GetThreadPrivate(IonTLSIndex);
}

bool
ion::SetIonContext(IonContext *ctx)
{
    return PR_SetThreadPrivate(IonTLSIndex, ctx) == PR_SUCCESS;
}

#else

static IonContext *GlobalIonContext;

static inline IonContext *
CurrentIonContext()
{
    return GlobalIonContext;
}

bool
ion::SetIonContext(IonContext *ctx)
{
    GlobalIonContext = ctx;
    return true;
}
#endif

IonContext *
ion::GetIonContext()
{
    JS_ASSERT(CurrentIonContext());
    return CurrentIonContext();
}

IonContext::IonContext(JSContext *cx, TempAllocator *temp)
  : cx(cx),
    temp(temp),
    prev_(CurrentIonContext())
{
    SetIonContext(this);
}

IonContext::~IonContext()
{
    SetIonContext(prev_);
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
    CheckLogging();
    return true;
}

IonCompartment::IonCompartment()
  : execAlloc_(NULL),
    enterJIT_(NULL),
    bailoutHandler_(NULL),
    returnError_(NULL),
    argumentsRectifier_(NULL),
    functionWrappers_(NULL)
{
}

bool
IonCompartment::initialize(JSContext *cx)
{
    execAlloc_ = js::OffTheBooks::new_<JSC::ExecutableAllocator>();
    if (!execAlloc_)
        return false;

    functionWrappers_ = cx->new_<VMWrapperMap>(cx);
    if (!functionWrappers_ || !functionWrappers_->init())
        return false;

    return true;
}

void
IonCompartment::mark(JSTracer *trc, JSCompartment *compartment)
{
    if (!compartment->active)
        return;

    
    if (enterJIT_)
        MarkIonCode(trc, enterJIT_, "enterJIT");
    if (returnError_)
        MarkIonCode(trc, returnError_, "returnError");

    
    
    if (bailoutHandler_)
        MarkIonCode(trc, bailoutHandler_, "bailoutHandler");
    for (size_t i = 0; i < bailoutTables_.length(); i++) {
        if (bailoutTables_[i])
            MarkIonCode(trc, bailoutTables_[i], "bailoutTable");
    }

    
    
}

void
IonCompartment::sweep(JSContext *cx)
{
    if (enterJIT_ && IsAboutToBeFinalized(cx, enterJIT_))
        enterJIT_ = NULL;
    if (bailoutHandler_ && IsAboutToBeFinalized(cx, bailoutHandler_))
        bailoutHandler_ = NULL;
    if (returnError_ && IsAboutToBeFinalized(cx, returnError_))
        returnError_ = NULL;
    if (argumentsRectifier_ && IsAboutToBeFinalized(cx, argumentsRectifier_))
        argumentsRectifier_ = NULL;

    for (size_t i = 0; i < bailoutTables_.length(); i++) {
        if (bailoutTables_[i] && IsAboutToBeFinalized(cx, bailoutTables_[i]))
            bailoutTables_[i] = NULL;
    }

    
    functionWrappers_->sweep(cx);
}

IonCode *
IonCompartment::getBailoutTable(const FrameSizeClass &frameClass)
{
    JS_ASSERT(frameClass != FrameSizeClass::None());
    return bailoutTables_[frameClass.classId()];
}

IonCode *
IonCompartment::getBailoutTable(JSContext *cx, const FrameSizeClass &frameClass)
{
    uint32 id = frameClass.classId();

    if (id >= bailoutTables_.length()) {
        size_t numToPush = id - bailoutTables_.length() + 1;
        if (!bailoutTables_.reserve(bailoutTables_.length() + numToPush))
            return NULL;
        for (size_t i = 0; i < numToPush; i++)
            bailoutTables_.infallibleAppend(NULL);
    }

    if (!bailoutTables_[id])
        bailoutTables_[id] = generateBailoutTable(cx, id);

    return bailoutTables_[id];
}

IonCompartment::~IonCompartment()
{
    Foreground::delete_(execAlloc_);
    Foreground::delete_(functionWrappers_);
}

IonActivation::IonActivation(JSContext *cx, StackFrame *fp)
  : cx_(cx),
    prev_(cx->compartment->ionCompartment()->activation()),
    entryfp_(fp),
    oldFrameRegs_(cx->regs()),
    bailout_(NULL)
{
    cx->compartment->ionCompartment()->active_ = this;
    cx->stack.repointRegs(NULL);
}

IonActivation::~IonActivation()
{
    JS_ASSERT(cx_->compartment->ionCompartment()->active_ == this);
    JS_ASSERT(!bailout_);

    cx_->compartment->ionCompartment()->active_ = prev();
    cx_->stack.repointRegs(&oldFrameRegs_);
}

IonCode *
IonCode::New(JSContext *cx, uint8 *code, uint32 bufferSize, JSC::ExecutablePool *pool)
{
    IonCode *codeObj = NewGCThing<IonCode>(cx, gc::FINALIZE_IONCODE, sizeof(IonCode));
    if (!codeObj) {
        pool->release();
        return NULL;
    }

    new (codeObj) IonCode(code, bufferSize, pool);
    return codeObj;
}

void
IonCode::copyFrom(MacroAssembler &masm)
{
    
    
    *(IonCode **)(code_ - sizeof(IonCode *)) = this;

    insnSize_ = masm.instructionsSize();
    masm.executableCopy(code_);

    dataSize_ = masm.dataSize();
    masm.processDeferredData(this, code_ + dataOffset());

    jumpRelocTableBytes_ = masm.jumpRelocationTableBytes();
    masm.copyJumpRelocationTable(code_ + jumpRelocTableOffset());

    dataRelocTableBytes_ = masm.dataRelocationTableBytes();
    masm.copyDataRelocationTable(code_ + dataRelocTableOffset());

    masm.processCodeLabels(this);
}

void
IonCode::trace(JSTracer *trc)
{
    if (jumpRelocTableBytes_) {
        uint8 *start = code_ + jumpRelocTableOffset();
        CompactBufferReader reader(start, start + jumpRelocTableBytes_);
        MacroAssembler::TraceJumpRelocations(trc, this, reader);
    }
    if (dataRelocTableBytes_) {
        uint8 *start = code_ + dataRelocTableOffset();
        CompactBufferReader reader(start, start + dataRelocTableBytes_);
        MacroAssembler::TraceDataRelocations(trc, this, reader);
    }
}

void
IonCode::finalize(JSContext *cx)
{
    if (pool_)
        pool_->release();
}

IonScript::IonScript()
  : method_(NULL),
    deoptTable_(NULL),
    snapshots_(0),
    snapshotsSize_(0),
    bailoutTable_(0),
    bailoutEntries_(0)
{
}

IonScript *
IonScript::New(JSContext *cx, size_t snapshotsSize, size_t bailoutEntries, size_t constants)
{
    if (snapshotsSize >= MAX_BUFFER_SIZE ||
        (bailoutEntries >= MAX_BUFFER_SIZE / sizeof(uint32)))
    {
        js_ReportOutOfMemory(cx);
        return NULL;
    }

    
    
    
    size_t bytes = snapshotsSize +
                   bailoutEntries * sizeof(uint32) +
                   constants * sizeof(Value);
    uint8 *buffer = (uint8 *)cx->malloc_(sizeof(IonScript) + bytes);
    if (!buffer)
        return NULL;

    IonScript *script = reinterpret_cast<IonScript *>(buffer);
    new (script) IonScript();

    script->snapshots_ = sizeof(IonScript);
    script->snapshotsSize_ = snapshotsSize;

    script->bailoutTable_ = script->snapshots_ + snapshotsSize;
    script->bailoutEntries_ = bailoutEntries;

    script->constantTable_ = script->bailoutTable_ + bailoutEntries * sizeof(uint32);
    script->constantEntries_ = constants;

    return script;
}


void
IonScript::trace(JSTracer *trc, JSScript *script)
{
    if (method_)
        MarkIonCode(trc, method_, "method");
    if (deoptTable_)
        MarkIonCode(trc, deoptTable_, "deoptimizationTable");
}

void
IonScript::copySnapshots(const SnapshotWriter *writer)
{
    JS_ASSERT(writer->length() == snapshotsSize_);
    memcpy((uint8 *)this + snapshots_, writer->buffer(), snapshotsSize_);
}

void
IonScript::copyBailoutTable(const SnapshotOffset *table)
{
    memcpy(bailoutTable(), table, bailoutEntries_ * sizeof(uint32));
}

void
IonScript::copyConstants(const Value *vp)
{
    memcpy(constants(), vp, constantEntries_ * sizeof(Value));
}

void
IonScript::Trace(JSTracer *trc, JSScript *script)
{
    if (script->ion && script->ion != ION_DISABLED_SCRIPT)
        script->ion->trace(trc, script);
}

void
IonScript::Destroy(JSContext *cx, JSScript *script)
{
    if (!script->ion || script->ion == ION_DISABLED_SCRIPT)
        return;

    cx->free_(script->ion);
}

static bool
TestCompiler(IonBuilder &builder, MIRGraph &graph)
{
    IonSpewNewFunction(&graph, builder.script);

    if (!builder.build())
        return false;
    IonSpewPass("BuildSSA");

    if (!SplitCriticalEdges(&builder, graph))
        return false;
    IonSpewPass("Split Critical Edges");

    if (!ReorderBlocks(graph))
        return false;
    IonSpewPass("Reorder Blocks");

    if (!BuildDominatorTree(graph))
        return false;
    

    
    
    if (!EliminateDeadPhis(graph))
        return false;
    IonSpewPass("Eliminate dead phis");

    if (!BuildPhiReverseMapping(graph))
        return false;
    

    if (!ApplyTypeInformation(graph))
        return false;
    IonSpewPass("Apply types");

    if (js_IonOptions.gvn) {
        ValueNumberer gvn(graph, js_IonOptions.gvnIsOptimistic);
        if (!gvn.analyze())
            return false;
        IonSpewPass("GVN");
    }

    if (!EliminateDeadCode(graph))
        return false;
    IonSpewPass("DCE");

    if (js_IonOptions.licm) {
        LICM licm(graph);
        if (!licm.analyze())
            return false;
        IonSpewPass("LICM");
    }

    LIRGraph lir(graph);
    LIRGenerator lirgen(&builder, graph, lir);
    if (!lirgen.generate())
        return false;
    IonSpewPass("Generate LIR");

    if (js_IonOptions.lsra) {
        LinearScanAllocator regalloc(&lirgen, lir);
        if (!regalloc.go())
            return false;
        IonSpewPass("Allocate Registers", &regalloc);
    } else {
        GreedyAllocator greedy(&builder, lir);
        if (!greedy.allocate())
            return false;
        IonSpewPass("Allocate Registers");
    }

    CodeGenerator codegen(&builder, lir);
    if (!codegen.generate())
        return false;
    

    IonSpewEndFunction();

    return true;
}

static bool
IonCompile(JSContext *cx, JSScript *script, StackFrame *fp)
{
    TempAllocator temp(&cx->tempLifoAlloc());
    IonContext ictx(cx, &temp);

    if (!cx->compartment->ensureIonCompartmentExists(cx))
        return false;

    MIRGraph graph(temp);
    JSFunction *fun = fp->isFunctionFrame() ? fp->fun() : NULL;
    CompileInfo *info = cx->tempLifoAlloc().new_<CompileInfo>(script, fun);
    if (!info)
        return false;

    if (cx->typeInferenceEnabled()) {
        types::AutoEnterTypeInference enter(cx, true);
        TypeInferenceOracle oracle;

        if (!oracle.init(cx, script))
            return false;

        types::AutoEnterCompilation enterCompiler(cx, script);

        IonBuilder builder(cx, temp, graph, &oracle, *info);
        if (!TestCompiler(builder, graph))
            return false;
    } else {
        DummyOracle oracle;
        IonBuilder builder(cx, temp, graph, &oracle, *info);
        if (!TestCompiler(builder, graph))
            return false;
    }

    return true;
}

static bool
CheckFrame(StackFrame *fp)
{
    if (!fp->isFunctionFrame()) {
        
        
        
        IonSpew(IonSpew_Abort, "global frame");
        return false;
    }

    if (fp->isEvalFrame()) {
        
        
        IonSpew(IonSpew_Abort, "eval frame");
        return false;
    }

    if (fp->isConstructing()) {
        
        
        IonSpew(IonSpew_Abort, "constructing frame");
        return false;
    }

    if (fp->hasCallObj()) {
        
        
        
        IonSpew(IonSpew_Abort, "frame has callobj");
        return false;
    }

    if (fp->hasArgsObj() || fp->script()->usesArguments) {
        
        
        IonSpew(IonSpew_Abort, "frame has argsobj");
        return false;
    }

    if (fp->isGeneratorFrame()) {
        
        IonSpew(IonSpew_Abort, "generator frame");
        return false;
    }

    if (fp->isDebuggerFrame()) {
        IonSpew(IonSpew_Abort, "debugger frame");
        return false;
    }

    
    
    if (fp->numActualArgs() >= SNAPSHOT_MAX_NARGS) {
        IonSpew(IonSpew_Abort, "too many actual args");
        return false;
    }

    JS_ASSERT_IF(fp->fun(), !fp->fun()->isHeavyweight());
    return true;
}

MethodStatus
ion::Compile(JSContext *cx, JSScript *script, js::StackFrame *fp)
{
    JS_ASSERT(ion::IsEnabled());

    if (cx->compartment->debugMode()) {
        IonSpew(IonSpew_Abort, "debugging");
        return Method_CantCompile;
    }

    if (!CheckFrame(fp))
        return Method_CantCompile;

    if (script->ion) {
        if (script->ion == ION_DISABLED_SCRIPT || !script->ion->method())
            return Method_CantCompile;

        return Method_Compiled;
    }

    if (script->incUseCount() <= js_IonOptions.invokesBeforeCompile)
        return Method_Skipped;

    if (!IonCompile(cx, script, fp)) {
        script->ion = ION_DISABLED_SCRIPT;
        return Method_CantCompile;
    }

    return Method_Compiled;
}

bool
ion::Cannon(JSContext *cx, StackFrame *fp)
{
    JS_ASSERT(ion::IsEnabled());
    JS_ASSERT(CheckFrame(fp));

    EnterIonCode enterJIT = cx->compartment->ionCompartment()->enterJIT(cx);
    if (!enterJIT)
        return false;

    int argc = 0;
    Value *argv = NULL;

    void *calleeToken;
    if (fp->isFunctionFrame()) {
        argc = CountArgSlots(fp->fun());
        argv = fp->formalArgs() - 1;
        calleeToken = CalleeToToken(&fp->callee());
    } else {
        calleeToken = CalleeToToken(fp->script());
    }

    JSScript *script = fp->script();
    IonScript *ion = script->ion;
    IonCode *code = ion->method();
    void *jitcode = code->raw();

    JSBool ok;
    Value result;
    {
        AssertCompartmentUnchanged pcc(cx);
        IonContext ictx(cx, NULL);
        IonActivation activation(cx, fp);
        JSAutoResolveFlags rf(cx, RESOLVE_INFER);

        ok = enterJIT(jitcode, argc, argv, &result, calleeToken);
    }

    JS_ASSERT(fp == cx->fp());

    
    fp->setReturnValue(result);
    fp->markFunctionEpilogueDone();

    return !!ok;
}

