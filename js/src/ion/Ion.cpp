








































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
    osrPrologue_(NULL),
    bailoutHandler_(NULL),
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
        MarkRoot(trc, enterJIT_.unsafeGet(), "enterJIT");

    
    
    if (osrPrologue_)
        MarkRoot(trc, osrPrologue_.unsafeGet(), "osrPrologue");
    if (bailoutHandler_)
        MarkRoot(trc, bailoutHandler_.unsafeGet(), "bailoutHandler");
    if (argumentsRectifier_)
        MarkRoot(trc, argumentsRectifier_.unsafeGet(), "argumentsRectifier");
    for (size_t i = 0; i < bailoutTables_.length(); i++) {
        if (bailoutTables_[i])
            MarkRoot(trc, bailoutTables_[i].unsafeGet(), "bailoutTable");
    }

    
    
}

void
IonCompartment::sweep(JSContext *cx)
{
    if (enterJIT_ && IsAboutToBeFinalized(cx, enterJIT_))
        enterJIT_ = NULL;
    if (osrPrologue_ && IsAboutToBeFinalized(cx, osrPrologue_))
        osrPrologue_ = NULL;
    if (bailoutHandler_ && IsAboutToBeFinalized(cx, bailoutHandler_))
        bailoutHandler_ = NULL;
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
    prev_(JS_THREAD_DATA(cx)->ionActivation),
    entryfp_(fp),
    bailout_(NULL),
    prevIonTop_(JS_THREAD_DATA(cx)->ionTop),
    prevIonJSContext_(JS_THREAD_DATA(cx)->ionJSContext),
    failedInvalidation_(false)
{
    fp->setRunningInIon();
    JS_THREAD_DATA(cx)->ionJSContext = cx;
    JS_THREAD_DATA(cx)->ionActivation = this;
}

IonActivation::~IonActivation()
{
    JS_ASSERT(JS_THREAD_DATA(cx_)->ionActivation == this);
    JS_ASSERT(!bailout_);

    entryfp_->clearRunningInIon();
    JS_THREAD_DATA(cx_)->ionActivation = prev();
    JS_THREAD_DATA(cx_)->ionTop = prevIonTop_;
    JS_THREAD_DATA(cx_)->ionJSContext = prevIonJSContext_;
}

IonCode *
IonCode::New(JSContext *cx, uint8 *code, uint32 bufferSize, JSC::ExecutablePool *pool)
{
    IonCode *codeObj = gc::NewGCThing<IonCode>(cx, gc::FINALIZE_IONCODE, sizeof(IonCode));
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
IonCode::finalize(JSContext *cx, bool background)
{
    JS_ASSERT(!background);
    if (pool_)
        pool_->release();
}

void
IonCode::readBarrier(IonCode *code)
{
#ifdef JSGC_INCREMENTAL
    if (!code)
        return;

    JSCompartment *comp = code->compartment();
    if (comp->needsBarrier())
        MarkIonCodeUnbarriered(comp->barrierTracer(), code, "ioncode read barrier");
#endif
}

void
IonCode::writeBarrierPre(IonCode *code)
{
#ifdef JSGC_INCREMENTAL
    if (!code)
        return;

    JSCompartment *comp = code->compartment();
    if (comp->needsBarrier())
        MarkIonCodeUnbarriered(comp->barrierTracer(), code, "ioncode write barrier");
#endif
}

void
IonCode::writeBarrierPost(IonCode *code, void *addr)
{
#ifdef JSGC_INCREMENTAL
    
#endif
}

IonScript::IonScript()
  : method_(NULL),
    deoptTable_(NULL),
    osrPc_(NULL),
    osrEntryOffset_(0),
    forbidOsr_(false),
    snapshots_(0),
    snapshotsSize_(0),
    bailoutTable_(0),
    bailoutEntries_(0),
    constantTable_(0),
    constantEntries_(0),
    frameInfoTable_(0),
    frameInfoEntries_(0)
{
}

IonScript *
IonScript::New(JSContext *cx, size_t snapshotsSize, size_t bailoutEntries, size_t constants, size_t frameInfoEntries)
{
    if (snapshotsSize >= MAX_BUFFER_SIZE ||
        (bailoutEntries >= MAX_BUFFER_SIZE / sizeof(uint32)))
    {
        js_ReportOutOfMemory(cx);
        return NULL;
    }

    
    
    
    size_t bytes = snapshotsSize +
                   bailoutEntries * sizeof(uint32) +
                   constants * sizeof(Value) +
                   frameInfoEntries * sizeof(IonFrameInfo);
    uint8 *buffer = (uint8 *)cx->malloc_(sizeof(IonScript) + bytes);
    if (!buffer)
        return NULL;

    IonScript *script = reinterpret_cast<IonScript *>(buffer);
    new (script) IonScript();

    uint32 tableShift = sizeof(IonScript);

    script->snapshots_ = tableShift;
    script->snapshotsSize_ = snapshotsSize;
    tableShift += snapshotsSize;

    script->bailoutTable_ = tableShift;
    script->bailoutEntries_ = bailoutEntries;
    tableShift += bailoutEntries * sizeof(uint32);

    script->constantTable_ = tableShift;
    script->constantEntries_ = constants;
    tableShift += constants * sizeof(Value);

    script->frameInfoTable_ = tableShift;
    script->frameInfoEntries_ = frameInfoEntries;

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
IonScript::copyFrameInfoTable(const IonFrameInfo *fi)
{
    memcpy(frameInfoTable(), fi, frameInfoEntries_ * sizeof(IonFrameInfo));
}

const IonFrameInfo *
IonScript::getFrameInfo(ptrdiff_t disp) const
{
    JS_ASSERT(frameInfoEntries_ > 0);

    if (frameInfoEntries_ == 1) {
        JS_ASSERT(disp == frameInfoTable()[0].displacement);
        return &frameInfoTable()[0];
    }

    size_t minEntry = 0;
    size_t maxEntry = frameInfoEntries_ - 1;
    ptrdiff_t min = frameInfoTable()[minEntry].displacement;
    ptrdiff_t max = frameInfoTable()[maxEntry].displacement;
    size_t guess = frameInfoEntries_;

    JS_ASSERT(min <= disp && disp <= max);
    while (true) {
        DebugOnly<size_t> oldGuess = guess;
        guess = (disp - min) * (maxEntry - minEntry) / (max - min);
        ptrdiff_t guessDisp = frameInfoTable()[guess].displacement;

        if (guessDisp == disp)
            break;

        
        JS_ASSERT(guess != oldGuess);

        if (guessDisp > disp) {
            maxEntry = guess;
            max = guessDisp;
        } else {
            minEntry = guess;
            min = guessDisp;
        }
    }

    return &frameInfoTable()[guess];
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
    AssertGraphCoherency(graph);

    if (!SplitCriticalEdges(&builder, graph))
        return false;
    IonSpewPass("Split Critical Edges");
    AssertGraphCoherency(graph);

    if (!ReorderBlocks(graph))
        return false;
    IonSpewPass("Reorder Blocks");
    AssertGraphCoherency(graph);

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
IonCompile(JSContext *cx, JSScript *script, StackFrame *fp, jsbytecode *osrPc)
{
    TempAllocator temp(&cx->tempLifoAlloc());
    IonContext ictx(cx, &temp);

    if (!cx->compartment->ensureIonCompartmentExists(cx))
        return false;

    MIRGraph graph(temp);
    JSFunction *fun = fp->isFunctionFrame() ? fp->fun() : NULL;
    CompileInfo *info = cx->tempLifoAlloc().new_<CompileInfo>(script, fun, osrPc);
    if (!info)
        return false;

    if (cx->typeInferenceEnabled()) {
        types::AutoEnterTypeInference enter(cx, true);
        TypeInferenceOracle oracle;

        if (!oracle.init(cx, script))
            return false;

        types::AutoEnterCompilation enterCompiler(cx, script);

        IonBuilder builder(cx, temp, graph, &oracle, *info);
        if (!TestCompiler(builder, graph)) {
            IonSpew(IonSpew_Abort, "IM Compilation failed.");
            return false;
        }
    } else {
        DummyOracle oracle;
        IonBuilder builder(cx, temp, graph, &oracle, *info);
        if (!TestCompiler(builder, graph)) {
            IonSpew(IonSpew_Abort, "IM Compilation failed.");
            return false;
        }
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
ion::CanEnterAtBranch(JSContext *cx, JSScript *script, StackFrame *fp, jsbytecode *pc)
{
    JS_ASSERT(ion::IsEnabled());
    JS_ASSERT((JSOp)*pc == JSOP_LOOPHEAD);

    
    if (!js_IonOptions.osr)
        return Method_Skipped;

    
    if (script->ion && script->ion != ION_DISABLED_SCRIPT &&
        script->ion->isOsrForbidden())
    {
        return Method_Skipped;
    }

    
    MethodStatus status = Compile(cx, script, fp, pc);
    if (status != Method_Compiled)
        return status;

    if (script->ion->osrPc() != pc)
        return Method_Skipped;

    return Method_Compiled;
}

MethodStatus
ion::Compile(JSContext *cx, JSScript *script, js::StackFrame *fp, jsbytecode *osrPc)
{
    JS_ASSERT(ion::IsEnabled());
    JS_ASSERT_IF(osrPc != NULL, (JSOp)*osrPc == JSOP_LOOPHEAD);

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

    if (!IonCompile(cx, script, fp, osrPc)) {
        script->ion = ION_DISABLED_SCRIPT;
        return Method_CantCompile;
    }

    return Method_Compiled;
}


union CallTarget {
    EnterIonCode enterJIT;
    DoOsrIonCode osrPrologue;
};

static bool
EnterIon(JSContext *cx, StackFrame *fp, CallTarget target, void *jitcode, bool osr)
{
    JS_ASSERT(ion::IsEnabled());
    JS_ASSERT(CheckFrame(fp));

    int argc = 0;
    Value *argv = NULL;

    void *calleeToken;
    if (fp->isFunctionFrame()) {
        argc = CountArgSlots(fp->fun());
        argv = fp->formalArgs() - 1;
        calleeToken = CalleeToToken(fp->callee().toFunction());
    } else {
        calleeToken = CalleeToToken(fp->script());
    }

    Value result;
    {
        AssertCompartmentUnchanged pcc(cx);
        IonContext ictx(cx, NULL);
        IonActivation activation(cx, fp);
        JSAutoResolveFlags rf(cx, RESOLVE_INFER);

        
        if (osr)
            target.osrPrologue(jitcode, argc, argv, &result, calleeToken, fp);
        else
            target.enterJIT(jitcode, argc, argv, &result, calleeToken);

        JS_ASSERT_IF(result.isMagic(), result.isMagic(JS_ION_ERROR));
    }

    JS_ASSERT(fp == cx->fp());

    
    fp->setReturnValue(result);
    fp->markFunctionEpilogueDone();

    return !result.isMagic();
}

bool
ion::Cannon(JSContext *cx, StackFrame *fp)
{
    CallTarget target;
    target.enterJIT = cx->compartment->ionCompartment()->enterJIT(cx);
    if (!target.enterJIT)
        return false;

    JSScript *script = fp->script();
    IonScript *ion = script->ion;
    IonCode *code = ion->method();
    void *jitcode = code->raw();

    return EnterIon(cx, fp, target, jitcode, false);
}

bool
ion::SideCannon(JSContext *cx, StackFrame *fp, jsbytecode *pc)
{
    CallTarget target;
    target.osrPrologue = cx->compartment->ionCompartment()->osrPrologue(cx);
    if (!target.osrPrologue)
        return false;

    JSScript *script = fp->script();
    IonScript *ion = script->ion;
    IonCode *code = ion->method();
    void *osrcode = code->raw() + ion->osrEntryOffset();

    JS_ASSERT(ion->osrPc() == pc);

    return EnterIon(cx, fp, target, osrcode, true);
}

