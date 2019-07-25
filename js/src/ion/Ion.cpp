








































#include "Ion.h"
#include "IonAnalysis.h"
#include "IonBuilder.h"
#include "IonSpewer.h"
#include "LIR.h"
#include "AliasAnalysis.h"
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
    invalidator_(NULL),
    functionWrappers_(NULL)
{
}

bool
IonCompartment::initialize(JSContext *cx)
{
    execAlloc_ = cx->runtime->getExecutableAllocator(cx);
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
        MarkIonCodeRoot(trc, enterJIT_.unsafeGet(), "enterJIT");

    
    
    if (osrPrologue_)
        MarkIonCodeRoot(trc, osrPrologue_.unsafeGet(), "osrPrologue");
    if (bailoutHandler_)
        MarkIonCodeRoot(trc, bailoutHandler_.unsafeGet(), "bailoutHandler");
    if (argumentsRectifier_)
        MarkIonCodeRoot(trc, argumentsRectifier_.unsafeGet(), "argumentsRectifier");
    if (invalidator_)
        MarkIonCodeRoot(trc, invalidator_.unsafeGet(), "invalidator");
    if (preBarrier_)
        MarkIonCodeRoot(trc, preBarrier_.unsafeGet(), "preBarrier");
    for (size_t i = 0; i < bailoutTables_.length(); i++) {
        if (bailoutTables_[i])
            MarkIonCodeRoot(trc, bailoutTables_[i].unsafeGet(), "bailoutTable");
    }

    
    
}

void
IonCompartment::sweep(JSContext *cx)
{
    if (enterJIT_ && IsAboutToBeFinalized(enterJIT_))
        enterJIT_ = NULL;
    if (osrPrologue_ && IsAboutToBeFinalized(osrPrologue_))
        osrPrologue_ = NULL;
    if (bailoutHandler_ && IsAboutToBeFinalized(bailoutHandler_))
        bailoutHandler_ = NULL;
    if (argumentsRectifier_ && IsAboutToBeFinalized(argumentsRectifier_))
        argumentsRectifier_ = NULL;
    if (invalidator_ && IsAboutToBeFinalized(invalidator_))
        invalidator_ = NULL;
    if (preBarrier_ && IsAboutToBeFinalized(preBarrier_))
        preBarrier_ = NULL;

    for (size_t i = 0; i < bailoutTables_.length(); i++) {
        if (bailoutTables_[i] && IsAboutToBeFinalized(bailoutTables_[i]))
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
    Foreground::delete_(functionWrappers_);
}

IonActivation::IonActivation(JSContext *cx, StackFrame *fp)
  : cx_(cx),
    compartment_(cx->compartment),
    prev_(cx->runtime->ionActivation),
    entryfp_(fp),
    bailout_(NULL),
    prevIonTop_(cx->runtime->ionTop),
    prevIonJSContext_(cx->runtime->ionJSContext)
{
    fp->setRunningInIon();
    cx->runtime->ionJSContext = cx;
    cx->runtime->ionActivation = this;
    cx->runtime->ionStackLimit = cx->runtime->nativeStackLimit;
}

IonActivation::~IonActivation()
{
    JS_ASSERT(cx_->runtime->ionActivation == this);
    JS_ASSERT(!bailout_);

    entryfp_->clearRunningInIon();
    cx_->runtime->ionActivation = prev();
    cx_->runtime->ionTop = prevIonTop_;
    cx_->runtime->ionJSContext = prevIonJSContext_;
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
    
    
    if (invalidated())
        return;

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
    invalidateEpilogueOffset_(0),
    invalidateEpilogueDataOffset_(0),
    forbidOsr_(false),
    snapshots_(0),
    snapshotsSize_(0),
    bailoutTable_(0),
    bailoutEntries_(0),
    constantTable_(0),
    constantEntries_(0),
    safepointIndexOffset_(0),
    safepointIndexEntries_(0),
    frameLocals_(0),
    frameSize_(0),
    safepointsStart_(0),
    safepointsSize_(0),
    cacheList_(0),
    cacheEntries_(0),
    refcount_(0)
{
}

IonScript *
IonScript::New(JSContext *cx, uint32 frameLocals, uint32 frameSize, size_t snapshotsSize,
               size_t bailoutEntries, size_t constants, size_t safepointIndices,
               size_t osiIndices, size_t cacheEntries, size_t safepointsSize)
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
                   safepointIndices * sizeof(SafepointIndex) +
                   osiIndices * sizeof(OsiIndex) +
                   cacheEntries * sizeof(IonCache) +
                   safepointsSize;
    uint8 *buffer = (uint8 *)cx->malloc_(sizeof(IonScript) + bytes);
    if (!buffer)
        return NULL;

    IonScript *script = reinterpret_cast<IonScript *>(buffer);
    new (script) IonScript();

    uint32 offsetCursor = sizeof(IonScript);

    script->snapshots_ = offsetCursor;
    script->snapshotsSize_ = snapshotsSize;
    offsetCursor += snapshotsSize;

    script->bailoutTable_ = offsetCursor;
    script->bailoutEntries_ = bailoutEntries;
    offsetCursor += bailoutEntries * sizeof(uint32);

    script->constantTable_ = offsetCursor;
    script->constantEntries_ = constants;
    offsetCursor += constants * sizeof(Value);

    script->safepointIndexOffset_ = offsetCursor;
    script->safepointIndexEntries_ = safepointIndices;
    offsetCursor += safepointIndices * sizeof(SafepointIndex);

    script->osiIndexOffset_ = offsetCursor;
    script->osiIndexEntries_ = osiIndices;
    offsetCursor += osiIndices * sizeof(OsiIndex);

    script->cacheList_ = offsetCursor;
    script->cacheEntries_ = cacheEntries;
    offsetCursor += cacheEntries * sizeof(IonCache);

    script->safepointsStart_ = offsetCursor;
    script->safepointsSize_ = safepointsSize;
    offsetCursor += safepointsSize;

    script->frameLocals_ = frameLocals;
    script->frameSize_ = frameSize;

    return script;
}

void
IonScript::trace(JSTracer *trc)
{
    if (method_)
        MarkIonCode(trc, method_, "method");

    if (deoptTable_)
        MarkIonCode(trc, deoptTable_, "deoptimizationTable");

    for (size_t i = 0; i < numConstants(); i++)
        gc::MarkValue(trc, &getConstant(i), "constant");
}

void
IonScript::copySnapshots(const SnapshotWriter *writer)
{
    JS_ASSERT(writer->size() == snapshotsSize_);
    memcpy((uint8 *)this + snapshots_, writer->buffer(), snapshotsSize_);
}

void
IonScript::copySafepoints(const SafepointWriter *writer)
{
    JS_ASSERT(writer->size() == safepointsSize_);
    memcpy((uint8 *)this + safepointsStart_, writer->buffer(), safepointsSize_);
}

void
IonScript::copyBailoutTable(const SnapshotOffset *table)
{
    memcpy(bailoutTable(), table, bailoutEntries_ * sizeof(uint32));
}

void
IonScript::copyConstants(const HeapValue *vp)
{
    for (size_t i = 0; i < constantEntries_; i++)
        constants()[i].init(vp[i]);
}

void
IonScript::copySafepointIndices(const SafepointIndex *si, MacroAssembler &masm)
{
    




    SafepointIndex *table = safepointIndices();
    memcpy(table, si, safepointIndexEntries_ * sizeof(SafepointIndex));
    for (size_t i = 0; i < safepointIndexEntries_; i++)
        table[i].adjustDisplacement(masm.actualOffset((uint8*)table[i].displacement()));
}

void
IonScript::copyOsiIndices(const OsiIndex *oi, MacroAssembler &masm)
{
    memcpy(osiIndices(), oi, osiIndexEntries_ * sizeof(OsiIndex));
    for (int i = 0; i < osiIndexEntries_; i++)
        osiIndices()[i].fixUpOffset(masm);
}

void
IonScript::copyCacheEntries(const IonCache *caches, MacroAssembler &masm)
{
    memcpy(cacheList(), caches, numCaches() * sizeof(IonCache));

    




    for (size_t i = 0; i < numCaches(); i++)
        getCache(i).updateBaseAddress(method_, masm);
}

const SafepointIndex *
IonScript::getSafepointIndex(uint32 disp) const
{
    JS_ASSERT(safepointIndexEntries_ > 0);

    const SafepointIndex *table = safepointIndices();
    if (safepointIndexEntries_ == 1) {
        JS_ASSERT(disp == table[0].displacement());
        return &table[0];
    }

    size_t minEntry = 0;
    size_t maxEntry = safepointIndexEntries_ - 1;
    uint32 min = table[minEntry].displacement();
    uint32 max = table[maxEntry].displacement();

    
    JS_ASSERT(min <= disp && disp <= max);

    
    size_t guess = (disp - min) * (maxEntry - minEntry) / (max - min) + minEntry;
    uint32 guessDisp = table[guess].displacement();

    if (table[guess].displacement() == disp)
        return &table[guess];

    
    
    
    
    if (guessDisp > disp) {
        while (--guess >= minEntry) {
            guessDisp = table[guess].displacement();
            JS_ASSERT(guessDisp >= disp);
            if (guessDisp == disp)
                return &table[guess];
        }
    } else {
        while (++guess <= maxEntry) {
            guessDisp = table[guess].displacement();
            JS_ASSERT(guessDisp <= disp);
            if (guessDisp == disp)
                return &table[guess];
        }
    }

    JS_NOT_REACHED("displacement not found.");
    return NULL;
}

const OsiIndex *
IonScript::getOsiIndex(uint32 disp) const
{
    for (const OsiIndex *it = osiIndices(), *end = osiIndices() + osiIndexEntries_;
         it != end;
         ++it)
    {
        if (it->returnPointDisplacement() == disp)
            return it;
    }

    JS_NOT_REACHED("Failed to find OSI point return address");
    return NULL;
}

const OsiIndex *
IonScript::getOsiIndex(uint8 *retAddr) const
{
    IonSpew(IonSpew_Invalidate, "IonScript %p has method %p raw %p", (void *) this, (void *)
            method(), method()->raw());
    JS_ASSERT(method()->raw() <= retAddr);
    JS_ASSERT(retAddr <= method()->raw() + method()->instructionsSize());

    uint32 disp = retAddr - method()->raw();
    return getOsiIndex(disp);
}

void
IonScript::Trace(JSTracer *trc, IonScript *script)
{
    if (script != ION_DISABLED_SCRIPT)
        script->trace(trc);
}

void
IonScript::Destroy(JSContext *cx, IonScript *script)
{
    cx->free_(script);
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
    

    EliminateCopies(graph);
    IonSpewPass("Eliminate copies");

    
    if (!ApplyTypeInformation(graph))
        return false;
    IonSpewPass("Apply types");

    
    
    if (js_IonOptions.licm || js_IonOptions.gvn) {
        AliasAnalysis analysis(graph);
        if (!analysis.analyze())
            return false;
        IonSpewPass("Alias analysis");
    }

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

        types::AutoEnterCompilation enterCompiler(cx, script, false, 0);

        IonBuilder builder(cx, &fp->scopeChain(), temp, graph, &oracle, *info);
        if (!TestCompiler(builder, graph)) {
            IonSpew(IonSpew_Abort, "IM Compilation failed.");
            return false;
        }
    } else {
        DummyOracle oracle;
        IonBuilder builder(cx, &fp->scopeChain(), temp, graph, &oracle, *info);
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

    
    
    if (fp->isFunctionFrame() && fp->numActualArgs() >= SNAPSHOT_MAX_NARGS) {
        IonSpew(IonSpew_Abort, "too many actual args");
        return false;
    }

    if (!fp->script()->compileAndGo) {
        IonSpew(IonSpew_Abort, "not compile-and-go");
        return false;
    }

    JS_ASSERT_IF(fp->maybeFun(), !fp->fun()->isHeavyweight());
    return true;
}



MethodStatus
ion::CanEnterAtBranch(JSContext *cx, JSScript *script, StackFrame *fp, jsbytecode *pc)
{
    JS_ASSERT(ion::IsEnabled());
    JS_ASSERT((JSOp)*pc == JSOP_LOOPENTRY);

    
    if (script->ion == ION_DISABLED_SCRIPT)
        return Method_Skipped;

    
    if (script->ion && script->ion->isOsrForbidden())
        return Method_Skipped;

    
    if (!js_IonOptions.osr)
        return Method_Skipped;

    
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
    JS_ASSERT_IF(osrPc != NULL, (JSOp)*osrPc == JSOP_LOOPENTRY);

    if (script->ion == ION_DISABLED_SCRIPT)
        return Method_CantCompile;

    if (cx->compartment->debugMode()) {
        IonSpew(IonSpew_Abort, "debugging");
        return Method_CantCompile;
    }

    if (!CheckFrame(fp)) {
        JS_ASSERT(script->ion != ION_DISABLED_SCRIPT);
        script->ion = ION_DISABLED_SCRIPT;
        return Method_CantCompile;
    }

    if (script->ion) {
        if (!script->ion->method())
            return Method_CantCompile;
        return Method_Compiled;
    }

    if (script->incUseCount() <= js_IonOptions.usesBeforeCompile)
        return Method_Skipped;

    if (!IonCompile(cx, script, fp, osrPc)) {
        JS_ASSERT(script->ion != ION_DISABLED_SCRIPT);
        script->ion = ION_DISABLED_SCRIPT;
        return Method_CantCompile;
    }

    
    return script->hasIonScript() ? Method_Compiled : Method_Skipped;
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
        argc = CountArgSlots(fp->fun()) - 1;
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
    if (fp->isFunctionFrame())
        fp->updateEpilogueFlags();

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

static void
InvalidateActivation(JSContext *cx, uint8 *ionTop, bool invalidateAll)
{
    IonSpew(IonSpew_Invalidate, "BEGIN invalidating activation");

    size_t frameno = 1;

    for (IonFrameIterator it(ionTop); it.more(); ++it, ++frameno) {
        JS_ASSERT_IF(frameno == 1, it.type() == IonFrame_Exit);

#ifdef DEBUG
        switch (it.type()) {
          case IonFrame_Exit:
            IonSpew(IonSpew_Invalidate, "#%d exit frame @ %p", frameno, it.fp());
            break;
          case IonFrame_JS:
          {
            JS_ASSERT(it.hasScript());
            IonSpew(IonSpew_Invalidate, "#%d JS frame @ %p", frameno, it.fp());
            IonSpew(IonSpew_Invalidate, "   token: %p", it.jsFrame()->calleeToken());
            IonSpew(IonSpew_Invalidate, "   script: %p; %s:%d", it.script(),
                    it.script()->filename, it.script()->lineno);
            break;
          }
          case IonFrame_Rectifier:
            IonSpew(IonSpew_Invalidate, "#%d rectifier frame @ %p", frameno, it.fp());
            break;
          case IonFrame_Bailed_Rectifier:
            IonSpew(IonSpew_Invalidate, "#%d bailed rectifier frame @ %p", frameno, it.fp());
            break;
          case IonFrame_Entry:
            IonSpew(IonSpew_Invalidate, "#%d entry frame @ %p", frameno, it.fp());
            break;
        }
        IonSpew(IonSpew_Invalidate, "   return address %p", it.returnAddress());
#endif

        if (!it.hasScript())
            continue;

        
        if (it.checkInvalidation())
            continue;

        JSScript *script = it.script();
        if (!script->hasIonScript())
            continue;

        if (!invalidateAll && !script->ion->invalidated())
            continue;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        IonSpew(IonSpew_Invalidate, "   ! requires invalidation");
        IonScript *ionScript = script->ion;
        ionScript->incref();
        IonSpew(IonSpew_Invalidate, "   ionScript %p ref %u", (void *) ionScript,
                unsigned(ionScript->refcount()));

        const SafepointIndex *si = ionScript->getSafepointIndex(it.returnAddressToFp());
        IonCode *ionCode = ionScript->method();

        ionCode->setInvalidated();

        
        
        
        
        
        
        CodeLocationLabel dataLabelToMunge(it.returnAddressToFp());
        ptrdiff_t delta = ionScript->invalidateEpilogueDataOffset() -
                          (it.returnAddressToFp() - ionCode->raw());
        Assembler::patchWrite_Imm32(dataLabelToMunge, Imm32(delta));

        CodeLocationLabel osiPatchPoint = SafepointReader::InvalidationPatchPoint(ionScript, si);
        CodeLocationLabel invalidateEpilogue(ionCode, ionScript->invalidateEpilogueOffset());

        IonSpew(IonSpew_Invalidate, "   -> patching address to call instruction %p",
                (void *) osiPatchPoint.raw());
        Assembler::patchWrite_NearCall(osiPatchPoint, invalidateEpilogue);
    }

    IonSpew(IonSpew_Invalidate, "END invalidating activation");
}

void
ion::InvalidateAll(JSContext *cx, JSCompartment *c)
{
    for (IonActivationIterator iter(cx); iter.more(); ++iter) {
        if (iter.activation()->compartment() == c) {
            IonSpew(IonSpew_Invalidate, "Invalidating all frames for GC");
            InvalidateActivation(cx, iter.top(), true);
        }
    }
}

void
ion::Invalidate(JSContext *cx, const Vector<types::RecompileInfo> &invalid, bool resetUses)
{
    
    
    for (size_t i = 0; i < invalid.length(); i++) {
        if (invalid[i].script->hasIonScript())
            invalid[i].script->ion->incref();
    }

    for (IonActivationIterator iter(cx); iter.more(); ++iter)
        InvalidateActivation(cx, iter.top(), false);

    
    
    
    for (size_t i = 0; i < invalid.length(); i++) {
        if (invalid[i].script->hasIonScript()) {
            invalid[i].script->ion->decref(cx);
            invalid[i].script->ion = NULL;
        }
    }

    
    
    if (resetUses) {
        for (size_t i = 0; i < invalid.length(); i++)
            invalid[i].script->resetUseCount();
    }
}

void
ion::FinishInvalidation(JSContext *cx, JSScript *script)
{
    if (!script->hasIonScript())
        return;

    



    if (!script->ion->invalidated())
        ion::IonScript::Destroy(cx, script->ion);

    
    script->ion = NULL;
}

void
ion::MarkFromIon(JSCompartment *comp, Value *vp)
{
    gc::MarkValueUnbarriered(comp->barrierTracer(), vp, "write barrier");
}

