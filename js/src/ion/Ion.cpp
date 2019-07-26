






#include "Ion.h"
#include "IonAnalysis.h"
#include "IonBuilder.h"
#include "IonLinker.h"
#include "IonSpewer.h"
#include "LIR.h"
#include "AliasAnalysis.h"
#include "LICM.h"
#include "ValueNumbering.h"
#include "EdgeCaseAnalysis.h"
#include "RangeAnalysis.h"
#include "LinearScan.h"
#include "vm/ParallelDo.h"
#include "ParallelArrayAnalysis.h"
#include "jscompartment.h"
#include "vm/ThreadPool.h"
#include "vm/ForkJoin.h"
#include "IonCompartment.h"
#include "CodeGenerator.h"
#include "jsworkers.h"
#include "BacktrackingAllocator.h"
#include "StupidAllocator.h"
#include "UnreachableCodeElimination.h"

#if defined(JS_CPU_X86)
# include "x86/Lowering-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/Lowering-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/Lowering-arm.h"
#endif
#include "gc/Marking.h"
#include "jsgcinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "vm/Stack-inl.h"
#include "ion/IonFrames-inl.h"
#include "ion/CompilerRoot.h"
#include "methodjit/Retcon.h"
#include "ExecutionModeInlines.h"

#if JS_TRACE_LOGGING
#include "TraceLogging.h"
#endif

using namespace js;
using namespace js::ion;


IonOptions ion::js_IonOptions;


JS_STATIC_ASSERT(sizeof(IonCode) % gc::CellSize == 0);

#ifdef JS_THREADSAFE
static bool IonTLSInitialized = false;
static unsigned IonTLSIndex;

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

IonContext::IonContext(JSContext *cx, JSCompartment *compartment, TempAllocator *temp)
  : cx(cx),
    compartment(compartment),
    temp(temp),
    prev_(CurrentIonContext()),
    assemblerCount_(0)
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

IonRuntime::IonRuntime()
  : execAlloc_(NULL),
    enterJIT_(NULL),
    bailoutHandler_(NULL),
    argumentsRectifier_(NULL),
    invalidator_(NULL),
    functionWrappers_(NULL)
{
}

IonRuntime::~IonRuntime()
{
    js_delete(functionWrappers_);
}

bool
IonRuntime::initialize(JSContext *cx)
{
    AutoEnterAtomsCompartment ac(cx);

    if (!cx->compartment->ensureIonCompartmentExists(cx))
        return false;

    IonContext ictx(cx, cx->compartment, NULL);
    AutoFlushCache afc("IonRuntime::initialize");

    execAlloc_ = cx->runtime->getExecAlloc(cx);
    if (!execAlloc_)
        return false;

    functionWrappers_ = cx->new_<VMWrapperMap>(cx);
    if (!functionWrappers_ || !functionWrappers_->init())
        return false;

    if (!bailoutTables_.reserve(FrameSizeClass::ClassLimit().classId()))
        return false;

    for (uint32_t id = 0;; id++) {
        FrameSizeClass class_ = FrameSizeClass::FromClass(id);
        if (class_ == FrameSizeClass::ClassLimit())
            break;
        bailoutTables_.infallibleAppend(NULL);
        bailoutTables_[id] = generateBailoutTable(cx, id);
        if (!bailoutTables_[id])
            return false;
    }

    bailoutHandler_ = generateBailoutHandler(cx);
    if (!bailoutHandler_)
        return false;

    argumentsRectifier_ = generateArgumentsRectifier(cx);
    if (!argumentsRectifier_)
        return false;

    invalidator_ = generateInvalidator(cx);
    if (!invalidator_)
        return false;

    enterJIT_ = generateEnterJIT(cx);
    if (!enterJIT_)
        return false;

    valuePreBarrier_ = generatePreBarrier(cx, MIRType_Value);
    if (!valuePreBarrier_)
        return false;

    shapePreBarrier_ = generatePreBarrier(cx, MIRType_Shape);
    if (!shapePreBarrier_)
        return false;

    for (VMFunction *fun = VMFunction::functions; fun; fun = fun->next) {
        if (!generateVMWrapper(cx, *fun))
            return false;
    }

    return true;
}

IonCompartment::IonCompartment(IonRuntime *rt)
  : rt(rt),
    flusher_(NULL)
{
}

bool
IonCompartment::initialize(JSContext *cx)
{
    return true;
}

void
ion::FinishOffThreadBuilder(IonBuilder *builder)
{
    JS_ASSERT(builder->info().executionMode() == SequentialExecution);

    
    if (builder->script()->isIonCompilingOffThread()) {
        types::TypeCompartment &types = builder->script()->compartment()->types;
        builder->recompileInfo.compilerOutput(types)->invalidate();
        builder->script()->ion = NULL;
    }

    
    
    
    
    js_delete(builder->backgroundCodegen());
    js_delete(builder->temp().lifoAlloc());
}

static inline void
FinishAllOffThreadCompilations(IonCompartment *ion)
{
    OffThreadCompilationVector &compilations = ion->finishedOffThreadCompilations();

    for (size_t i = 0; i < compilations.length(); i++) {
        IonBuilder *builder = compilations[i];
        FinishOffThreadBuilder(builder);
    }
    compilations.clear();
}

 void
IonRuntime::Mark(JSTracer *trc)
{
    for (gc::CellIterUnderGC i(trc->runtime->atomsCompartment, gc::FINALIZE_IONCODE); !i.done(); i.next()) {
        IonCode *code = i.get<IonCode>();
        MarkIonCodeRoot(trc, &code, "wrapper");
    }
}

void
IonCompartment::mark(JSTracer *trc, JSCompartment *compartment)
{
    
    CancelOffThreadIonCompile(compartment, NULL);
    FinishAllOffThreadCompilations(this);
}

void
IonCompartment::sweep(FreeOp *fop)
{
}

IonCode *
IonCompartment::getBailoutTable(const FrameSizeClass &frameClass)
{
    JS_ASSERT(frameClass != FrameSizeClass::None());
    return rt->bailoutTables_[frameClass.classId()];
}

IonCode *
IonCompartment::getVMWrapper(const VMFunction &f)
{
    typedef MoveResolver::MoveOperand MoveOperand;

    JS_ASSERT(rt->functionWrappers_);
    JS_ASSERT(rt->functionWrappers_->initialized());
    IonRuntime::VMWrapperMap::Ptr p = rt->functionWrappers_->lookup(&f);
    JS_ASSERT(p);

    return p->value;
}

IonActivation::IonActivation(JSContext *cx, StackFrame *fp)
  : cx_(cx),
    compartment_(cx->compartment),
    prev_(cx->mainThread().ionActivation),
    entryfp_(fp),
    bailout_(NULL),
    prevIonTop_(cx->mainThread().ionTop),
    prevIonJSContext_(cx->mainThread().ionJSContext),
    prevpc_(NULL)
{
    if (fp)
        fp->setRunningInIon();
    cx->mainThread().ionJSContext = cx;
    cx->mainThread().ionActivation = this;
    cx->mainThread().ionStackLimit = cx->mainThread().nativeStackLimit;
}

IonActivation::~IonActivation()
{
    JS_ASSERT(cx_->mainThread().ionActivation == this);
    JS_ASSERT(!bailout_);

    if (entryfp_)
        entryfp_->clearRunningInIon();
    cx_->mainThread().ionActivation = prev();
    cx_->mainThread().ionTop = prevIonTop_;
    cx_->mainThread().ionJSContext = prevIonJSContext_;
}

IonCode *
IonCode::New(JSContext *cx, uint8_t *code, uint32_t bufferSize, JSC::ExecutablePool *pool)
{
    AssertCanGC();

    IonCode *codeObj = gc::NewGCThing<IonCode, CanGC>(cx, gc::FINALIZE_IONCODE, sizeof(IonCode), gc::DefaultHeap);
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

    jumpRelocTableBytes_ = masm.jumpRelocationTableBytes();
    masm.copyJumpRelocationTable(code_ + jumpRelocTableOffset());

    dataRelocTableBytes_ = masm.dataRelocationTableBytes();
    masm.copyDataRelocationTable(code_ + dataRelocTableOffset());

    preBarrierTableBytes_ = masm.preBarrierTableBytes();
    masm.copyPreBarrierTable(code_ + preBarrierTableOffset());

    masm.processCodeLabels(this);
}

void
IonCode::trace(JSTracer *trc)
{
    
    
    if (invalidated())
        return;

    if (jumpRelocTableBytes_) {
        uint8_t *start = code_ + jumpRelocTableOffset();
        CompactBufferReader reader(start, start + jumpRelocTableBytes_);
        MacroAssembler::TraceJumpRelocations(trc, this, reader);
    }
    if (dataRelocTableBytes_) {
        uint8_t *start = code_ + dataRelocTableOffset();
        CompactBufferReader reader(start, start + dataRelocTableBytes_);
        MacroAssembler::TraceDataRelocations(trc, this, reader);
    }
}

void
IonCode::finalize(FreeOp *fop)
{
    
    JS_POISON(code_, JS_FREE_PATTERN, bufferSize_);

    
    
    if (pool_)
        pool_->release();
}

void
IonCode::togglePreBarriers(bool enabled)
{
    uint8_t *start = code_ + preBarrierTableOffset();
    CompactBufferReader reader(start, start + preBarrierTableBytes_);

    while (reader.more()) {
        size_t offset = reader.readUnsigned();
        CodeLocationLabel loc(this, offset);
        if (enabled)
            Assembler::ToggleToCmp(loc);
        else
            Assembler::ToggleToJmp(loc);
    }
}

void
IonCode::readBarrier(IonCode *code)
{
#ifdef JSGC_INCREMENTAL
    if (!code)
        return;

    Zone *zone = code->zone();
    if (zone->needsBarrier())
        MarkIonCodeUnbarriered(zone->barrierTracer(), &code, "ioncode read barrier");
#endif
}

void
IonCode::writeBarrierPre(IonCode *code)
{
#ifdef JSGC_INCREMENTAL
    if (!code)
        return;

    Zone *zone = code->zone();
    if (zone->needsBarrier())
        MarkIonCodeUnbarriered(zone->barrierTracer(), &code, "ioncode write barrier");
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
    bailoutExpected_(false),
    runtimeData_(0),
    runtimeSize_(0),
    cacheIndex_(0),
    cacheEntries_(0),
    safepointIndexOffset_(0),
    safepointIndexEntries_(0),
    safepointsStart_(0),
    safepointsSize_(0),
    frameSlots_(0),
    frameSize_(0),
    bailoutTable_(0),
    bailoutEntries_(0),
    osiIndexOffset_(0),
    osiIndexEntries_(0),
    snapshots_(0),
    snapshotsSize_(0),
    constantTable_(0),
    constantEntries_(0),
    scriptList_(0),
    scriptEntries_(0),
    parallelInvalidatedScriptList_(0),
    refcount_(0),
    recompileInfo_(),
    slowCallCount(0)
{
}

static const int DataAlignment = 4;

IonScript *
IonScript::New(JSContext *cx, uint32_t frameSlots, uint32_t frameSize, size_t snapshotsSize,
               size_t bailoutEntries, size_t constants, size_t safepointIndices,
               size_t osiIndices, size_t cacheEntries, size_t runtimeSize,
               size_t safepointsSize, size_t scriptEntries,
               size_t parallelInvalidatedScriptEntries)
{
    if (snapshotsSize >= MAX_BUFFER_SIZE ||
        (bailoutEntries >= MAX_BUFFER_SIZE / sizeof(uint32_t)))
    {
        js_ReportOutOfMemory(cx);
        return NULL;
    }

    
    
    
    size_t paddedSnapshotsSize = AlignBytes(snapshotsSize, DataAlignment);
    size_t paddedBailoutSize = AlignBytes(bailoutEntries * sizeof(uint32_t), DataAlignment);
    size_t paddedConstantsSize = AlignBytes(constants * sizeof(Value), DataAlignment);
    size_t paddedSafepointIndicesSize = AlignBytes(safepointIndices * sizeof(SafepointIndex), DataAlignment);
    size_t paddedOsiIndicesSize = AlignBytes(osiIndices * sizeof(OsiIndex), DataAlignment);
    size_t paddedCacheEntriesSize = AlignBytes(cacheEntries * sizeof(uint32_t), DataAlignment);
    size_t paddedRuntimeSize = AlignBytes(runtimeSize, DataAlignment);
    size_t paddedSafepointSize = AlignBytes(safepointsSize, DataAlignment);
    size_t paddedScriptSize = AlignBytes(scriptEntries * sizeof(RawScript), DataAlignment);
    size_t paddedParallelInvalidatedScriptSize =
        AlignBytes(parallelInvalidatedScriptEntries * sizeof(RawScript), DataAlignment);
    size_t bytes = paddedSnapshotsSize +
                   paddedBailoutSize +
                   paddedConstantsSize +
                   paddedSafepointIndicesSize+
                   paddedOsiIndicesSize +
                   paddedCacheEntriesSize +
                   paddedRuntimeSize +
                   paddedSafepointSize +
                   paddedScriptSize +
                   paddedParallelInvalidatedScriptSize;
    uint8_t *buffer = (uint8_t *)cx->malloc_(sizeof(IonScript) + bytes);
    if (!buffer)
        return NULL;

    IonScript *script = reinterpret_cast<IonScript *>(buffer);
    new (script) IonScript();

    uint32_t offsetCursor = sizeof(IonScript);

    script->runtimeData_ = offsetCursor;
    script->runtimeSize_ = runtimeSize;
    offsetCursor += paddedRuntimeSize;

    script->cacheIndex_ = offsetCursor;
    script->cacheEntries_ = cacheEntries;
    offsetCursor += paddedCacheEntriesSize;

    script->safepointIndexOffset_ = offsetCursor;
    script->safepointIndexEntries_ = safepointIndices;
    offsetCursor += paddedSafepointIndicesSize;

    script->safepointsStart_ = offsetCursor;
    script->safepointsSize_ = safepointsSize;
    offsetCursor += paddedSafepointSize;

    script->bailoutTable_ = offsetCursor;
    script->bailoutEntries_ = bailoutEntries;
    offsetCursor += paddedBailoutSize;

    script->osiIndexOffset_ = offsetCursor;
    script->osiIndexEntries_ = osiIndices;
    offsetCursor += paddedOsiIndicesSize;

    script->snapshots_ = offsetCursor;
    script->snapshotsSize_ = snapshotsSize;
    offsetCursor += paddedSnapshotsSize;

    script->constantTable_ = offsetCursor;
    script->constantEntries_ = constants;
    offsetCursor += paddedConstantsSize;

    script->scriptList_ = offsetCursor;
    script->scriptEntries_ = scriptEntries;
    offsetCursor += paddedScriptSize;

    script->parallelInvalidatedScriptList_ = offsetCursor;
    script->parallelInvalidatedScriptEntries_ = parallelInvalidatedScriptEntries;
    offsetCursor += parallelInvalidatedScriptEntries;

    script->frameSlots_ = frameSlots;
    script->frameSize_ = frameSize;

    script->recompileInfo_ = cx->compartment->types.compiledInfo;

    return script;
}

void
IonScript::trace(JSTracer *trc)
{
    if (method_)
        MarkIonCode(trc, &method_, "method");

    if (deoptTable_)
        MarkIonCode(trc, &deoptTable_, "deoptimizationTable");

    for (size_t i = 0; i < numConstants(); i++)
        gc::MarkValue(trc, &getConstant(i), "constant");
}

void
IonScript::copySnapshots(const SnapshotWriter *writer)
{
    JS_ASSERT(writer->size() == snapshotsSize_);
    memcpy((uint8_t *)this + snapshots_, writer->buffer(), snapshotsSize_);
}

void
IonScript::copySafepoints(const SafepointWriter *writer)
{
    JS_ASSERT(writer->size() == safepointsSize_);
    memcpy((uint8_t *)this + safepointsStart_, writer->buffer(), safepointsSize_);
}

void
IonScript::copyBailoutTable(const SnapshotOffset *table)
{
    memcpy(bailoutTable(), table, bailoutEntries_ * sizeof(uint32_t));
}

void
IonScript::copyConstants(const HeapValue *vp)
{
    for (size_t i = 0; i < constantEntries_; i++)
        constants()[i].init(vp[i]);
}

void
IonScript::copyScriptEntries(JSScript **scripts)
{
    for (size_t i = 0; i < scriptEntries_; i++)
        scriptList()[i] = scripts[i];
}

void
IonScript::zeroParallelInvalidatedScripts()
{
    memset(parallelInvalidatedScriptList(), 0,
           parallelInvalidatedScriptEntries_ * sizeof(JSScript *));
}

void
IonScript::copySafepointIndices(const SafepointIndex *si, MacroAssembler &masm)
{
    
    
    
    SafepointIndex *table = safepointIndices();
    memcpy(table, si, safepointIndexEntries_ * sizeof(SafepointIndex));
    for (size_t i = 0; i < safepointIndexEntries_; i++)
        table[i].adjustDisplacement(masm.actualOffset(table[i].displacement()));
}

void
IonScript::copyOsiIndices(const OsiIndex *oi, MacroAssembler &masm)
{
    memcpy(osiIndices(), oi, osiIndexEntries_ * sizeof(OsiIndex));
    for (unsigned i = 0; i < osiIndexEntries_; i++)
        osiIndices()[i].fixUpOffset(masm);
}

void
IonScript::copyRuntimeData(const uint8_t *data)
{
    memcpy(runtimeData(), data, runtimeSize());
}

void
IonScript::copyCacheEntries(const uint32_t *caches, MacroAssembler &masm)
{
    memcpy(cacheIndex(), caches, numCaches() * sizeof(uint32_t));

    
    
    
    for (size_t i = 0; i < numCaches(); i++)
        getCache(i).updateBaseAddress(method_, masm);
}

const SafepointIndex *
IonScript::getSafepointIndex(uint32_t disp) const
{
    JS_ASSERT(safepointIndexEntries_ > 0);

    const SafepointIndex *table = safepointIndices();
    if (safepointIndexEntries_ == 1) {
        JS_ASSERT(disp == table[0].displacement());
        return &table[0];
    }

    size_t minEntry = 0;
    size_t maxEntry = safepointIndexEntries_ - 1;
    uint32_t min = table[minEntry].displacement();
    uint32_t max = table[maxEntry].displacement();

    
    JS_ASSERT(min <= disp && disp <= max);

    
    size_t guess = (disp - min) * (maxEntry - minEntry) / (max - min) + minEntry;
    uint32_t guessDisp = table[guess].displacement();

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
IonScript::getOsiIndex(uint32_t disp) const
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
IonScript::getOsiIndex(uint8_t *retAddr) const
{
    IonSpew(IonSpew_Invalidate, "IonScript %p has method %p raw %p", (void *) this, (void *)
            method(), method()->raw());

    JS_ASSERT(containsCodeAddress(retAddr));
    uint32_t disp = retAddr - method()->raw();
    return getOsiIndex(disp);
}

void
IonScript::Trace(JSTracer *trc, IonScript *script)
{
    if (script != ION_DISABLED_SCRIPT)
        script->trace(trc);
}

void
IonScript::Destroy(FreeOp *fop, IonScript *script)
{
    fop->free_(script);
}

void
IonScript::toggleBarriers(bool enabled)
{
    method()->togglePreBarriers(enabled);
}

void
IonScript::purgeCaches(JSCompartment *c)
{
    
    
    
    
    
    if (invalidated())
        return;

    
    
    
    js::ion::IonContext ictx(NULL, c, NULL);
    AutoFlushCache afc("purgeCaches");
    for (size_t i = 0; i < numCaches(); i++)
        getCache(i).reset();
}

void
ion::ToggleBarriers(JSCompartment *comp, bool needs)
{
    IonContext ictx(NULL, comp, NULL);
    AutoFlushCache afc("ToggleBarriers");
    for (gc::CellIterUnderGC i(comp, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        UnrootedScript script = i.get<JSScript>();
        if (script->hasIonScript())
            script->ion->toggleBarriers(needs);
    }
}

namespace js {
namespace ion {

bool
OptimizeMIR(MIRGenerator *mir)
{
    IonSpewPass("BuildSSA");
    
    

    MIRGraph &graph = mir->graph();

    if (mir->shouldCancel("Start"))
        return false;

    if (!SplitCriticalEdges(graph))
        return false;
    IonSpewPass("Split Critical Edges");
    AssertGraphCoherency(graph);

    if (mir->shouldCancel("Split Critical Edges"))
        return false;

    if (!RenumberBlocks(graph))
        return false;
    IonSpewPass("Renumber Blocks");
    AssertGraphCoherency(graph);

    if (mir->shouldCancel("Renumber Blocks"))
        return false;

    if (!BuildDominatorTree(graph))
        return false;
    

    if (mir->shouldCancel("Dominator Tree"))
        return false;

    
    if (!EliminatePhis(mir, graph, AggressiveObservability))
        return false;
    IonSpewPass("Eliminate phis");
    AssertGraphCoherency(graph);

    if (mir->shouldCancel("Eliminate phis"))
        return false;

    if (!BuildPhiReverseMapping(graph))
        return false;
    AssertExtendedGraphCoherency(graph);
    

    if (mir->shouldCancel("Phi reverse mapping"))
        return false;

    
    if (!ApplyTypeInformation(mir, graph))
        return false;
    IonSpewPass("Apply types");
    AssertExtendedGraphCoherency(graph);

    if (mir->shouldCancel("Apply types"))
        return false;

    
    
    if (js_IonOptions.licm || js_IonOptions.gvn) {
        AliasAnalysis analysis(mir, graph);
        if (!analysis.analyze())
            return false;
        IonSpewPass("Alias analysis");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("Alias analysis"))
            return false;

        
        
        
        if (!EliminateDeadResumePointOperands(mir, graph))
            return false;

        if (mir->shouldCancel("Eliminate dead resume point operands"))
            return false;
    }

    if (js_IonOptions.gvn) {
        ValueNumberer gvn(mir, graph, js_IonOptions.gvnIsOptimistic);
        if (!gvn.analyze())
            return false;
        IonSpewPass("GVN");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("GVN"))
            return false;
    }

    if (js_IonOptions.uce) {
        UnreachableCodeElimination uce(mir, graph);
        if (!uce.analyze())
            return false;
        IonSpewPass("UCE");
        AssertExtendedGraphCoherency(graph);
    }

    if (mir->shouldCancel("UCE"))
        return false;

    if (js_IonOptions.licm) {
        LICM licm(mir, graph);
        if (!licm.analyze())
            return false;
        IonSpewPass("LICM");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("LICM"))
            return false;
    }

    if (js_IonOptions.rangeAnalysis) {
        RangeAnalysis r(graph);
        if (!r.addBetaNobes())
            return false;
        IonSpewPass("Beta");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("RA Beta"))
            return false;

        if (!r.analyze())
            return false;
        IonSpewPass("Range Analysis");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("Range Analysis"))
            return false;

        if (!r.removeBetaNobes())
            return false;
        IonSpewPass("De-Beta");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("RA De-Beta"))
            return false;
    }

    if (!EliminateDeadCode(mir, graph))
        return false;
    IonSpewPass("DCE");
    AssertExtendedGraphCoherency(graph);

    if (mir->shouldCancel("DCE"))
        return false;

    
    

    if (js_IonOptions.edgeCaseAnalysis) {
        EdgeCaseAnalysis edgeCaseAnalysis(mir, graph);
        if (!edgeCaseAnalysis.analyzeLate())
            return false;
        IonSpewPass("Edge Case Analysis (Late)");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("Edge Case Analysis (Late)"))
            return false;
    }

    
    
    
    
    if (!EliminateRedundantChecks(graph))
        return false;
    IonSpewPass("Bounds Check Elimination");
    AssertGraphCoherency(graph);

    return true;
}

CodeGenerator *
GenerateLIR(MIRGenerator *mir)
{
    MIRGraph &graph = mir->graph();

    LIRGraph *lir = mir->temp().lifoAlloc()->new_<LIRGraph>(&graph);
    if (!lir)
        return NULL;

    LIRGenerator lirgen(mir, graph, *lir);
    if (!lirgen.generate())
        return NULL;
    IonSpewPass("Generate LIR");

    if (mir->shouldCancel("Generate LIR"))
        return NULL;

    AllocationIntegrityState integrity(*lir);

    switch (js_IonOptions.registerAllocator) {
      case RegisterAllocator_LSRA: {
#ifdef DEBUG
        integrity.record();
#endif

        LinearScanAllocator regalloc(mir, &lirgen, *lir);
        if (!regalloc.go())
            return NULL;

#ifdef DEBUG
        integrity.check(false);
#endif

        IonSpewPass("Allocate Registers [LSRA]", &regalloc);
        break;
      }

      case RegisterAllocator_Backtracking: {
#ifdef DEBUG
        integrity.record();
#endif

        BacktrackingAllocator regalloc(mir, &lirgen, *lir);
        if (!regalloc.go())
            return NULL;

#ifdef DEBUG
        integrity.check(false);
#endif

        IonSpewPass("Allocate Registers [Backtracking]");
        break;
      }

      case RegisterAllocator_Stupid: {
        
        
        integrity.record();

        StupidAllocator regalloc(mir, &lirgen, *lir);
        if (!regalloc.go())
            return NULL;
        if (!integrity.check(true))
            return NULL;
        IonSpewPass("Allocate Registers [Stupid]");
        break;
      }

      default:
        JS_NOT_REACHED("Bad regalloc");
    }

    if (mir->shouldCancel("Allocate Registers"))
        return NULL;

    CodeGenerator *codegen = js_new<CodeGenerator>(mir, lir);
    if (!codegen || !codegen->generate()) {
        js_delete(codegen);
        return NULL;
    }

    return codegen;
}

CodeGenerator *
CompileBackEnd(MIRGenerator *mir)
{
    if (!OptimizeMIR(mir))
        return NULL;
    return GenerateLIR(mir);
}

class SequentialCompileContext {
public:
    ExecutionMode executionMode() {
        return SequentialExecution;
    }

    MethodStatus checkScriptSize(JSContext *cx, UnrootedScript script);
    AbortReason compile(IonBuilder *builder, MIRGraph *graph,
                        ScopedJSDeletePtr<LifoAlloc> &autoDelete);
};

void
AttachFinishedCompilations(JSContext *cx)
{
#ifdef JS_THREADSAFE
    AssertCanGC();
    IonCompartment *ion = cx->compartment->ionCompartment();
    if (!ion || !cx->runtime->workerThreadState)
        return;

    AutoLockWorkerThreadState lock(cx->runtime);

    OffThreadCompilationVector &compilations = ion->finishedOffThreadCompilations();

    
    
    
    while (!compilations.empty()) {
        IonBuilder *builder = compilations.popCopy();

        if (CodeGenerator *codegen = builder->backgroundCodegen()) {
            RootedScript script(cx, builder->script());
            IonContext ictx(cx, cx->compartment, &builder->temp());

            
            
            
            codegen->masm.constructRoot(cx);

            types::AutoEnterAnalysis enterTypes(cx);

            ExecutionMode executionMode = builder->info().executionMode();
            types::AutoEnterCompilation enterCompiler(cx, CompilerOutputKind(executionMode));
            enterCompiler.initExisting(builder->recompileInfo);

            bool success;
            {
                
                AutoTempAllocatorRooter root(cx, &builder->temp());
                AutoUnlockWorkerThreadState unlock(cx->runtime);
                AutoFlushCache afc("AttachFinishedCompilations");
                success = codegen->link();
            }

            if (success) {
                if (script->hasIonScript())
                    mjit::DisableScriptCodeForIon(script, script->ionScript()->osrPc());
            } else {
                
                
                cx->clearPendingException();
            }
        }

        FinishOffThreadBuilder(builder);
    }

    compilations.clear();
#endif
}

static const size_t BUILDER_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 1 << 12;

template <typename CompileContext>
static AbortReason
IonCompile(JSContext *cx, JSScript *script, JSFunction *fun, jsbytecode *osrPc, bool constructing,
           CompileContext &compileContext)
{
#if JS_TRACE_LOGGING
    AutoTraceLog logger(TraceLogging::defaultLogger(),
                        TraceLogging::ION_COMPILE_START,
                        TraceLogging::ION_COMPILE_STOP,
                        script);
#endif

    LifoAlloc *alloc = cx->new_<LifoAlloc>(BUILDER_LIFO_ALLOC_PRIMARY_CHUNK_SIZE);
    if (!alloc)
        return AbortReason_Alloc;

    ScopedJSDeletePtr<LifoAlloc> autoDelete(alloc);

    TempAllocator *temp = alloc->new_<TempAllocator>(alloc);
    if (!temp)
        return AbortReason_Alloc;

    IonContext ictx(cx, cx->compartment, temp);

    types::AutoEnterAnalysis enter(cx);

    if (!cx->compartment->ensureIonCompartmentExists(cx))
        return AbortReason_Alloc;

    MIRGraph *graph = alloc->new_<MIRGraph>(temp);
    ExecutionMode executionMode = compileContext.executionMode();
    CompileInfo *info = alloc->new_<CompileInfo>(script, fun, osrPc, constructing,
                                                 executionMode);
    if (!info)
        return AbortReason_Alloc;

    TypeInferenceOracle oracle;

    if (!oracle.init(cx, script))
        return AbortReason_Disable;

    AutoFlushCache afc("IonCompile");

    types::AutoEnterCompilation enterCompiler(cx, CompilerOutputKind(executionMode));
    if (!enterCompiler.init(script, false, 0))
        return AbortReason_Disable;

    AutoTempAllocatorRooter root(cx, temp);

    IonBuilder *builder = alloc->new_<IonBuilder>(cx, temp, graph, &oracle, info);
    if (!builder)
        return AbortReason_Alloc;

    AbortReason abortReason  = compileContext.compile(builder, graph, autoDelete);
    if (abortReason != AbortReason_NoAbort)
        IonSpew(IonSpew_Abort, "IM Compilation failed.");

    return abortReason;
}

static inline bool
OffThreadCompilationEnabled(JSContext *cx)
{
    return js_IonOptions.parallelCompilation
        && cx->runtime->useHelperThreads()
        && cx->runtime->helperThreadCount() != 0;
}

static inline bool
OffThreadCompilationAvailable(JSContext *cx)
{
    
    
    
    
    
    
    
    
    
    
    
    return OffThreadCompilationEnabled(cx)
        && cx->runtime->gcIncrementalState == gc::NO_INCREMENTAL
        && !cx->runtime->profilingScripts
        && !cx->runtime->spsProfiler.enabled();
}

AbortReason
SequentialCompileContext::compile(IonBuilder *builder, MIRGraph *graph,
                                  ScopedJSDeletePtr<LifoAlloc> &autoDelete)
{
    JS_ASSERT(!builder->script()->ion);
    JSContext *cx = GetIonContext()->cx;

    RootedScript builderScript(cx, builder->script());
    IonSpewNewFunction(graph, builderScript);

    if (!builder->build()) {
        IonSpew(IonSpew_Abort, "Builder failed to build.");
        return builder->abortReason();
    }
    builder->clearForBackEnd();

    
    if (OffThreadCompilationAvailable(cx)) {
        builder->script()->ion = ION_COMPILING_SCRIPT;

        if (!StartOffThreadIonCompile(cx, builder)) {
            IonSpew(IonSpew_Abort, "Unable to start off-thread ion compilation.");
            return AbortReason_Alloc;
        }

        
        
        autoDelete.forget();

        return AbortReason_NoAbort;
    }

    ScopedJSDeletePtr<CodeGenerator> codegen(CompileBackEnd(builder));
    if (!codegen) {
        IonSpew(IonSpew_Abort, "Failed during back-end compilation.");
        return AbortReason_Disable;
    }

    bool success = codegen->link();

    IonSpewEndFunction();

    return success ? AbortReason_NoAbort : AbortReason_Disable;
}

static bool
CheckFrame(AbstractFramePtr fp)
{
    if (fp.isEvalFrame()) {
        
        
        
        IonSpew(IonSpew_Abort, "eval frame");
        return false;
    }

    if (fp.isGeneratorFrame()) {
        
        IonSpew(IonSpew_Abort, "generator frame");
        return false;
    }

    if (fp.isDebuggerFrame()) {
        IonSpew(IonSpew_Abort, "debugger frame");
        return false;
    }

    
    
    if (fp.isFunctionFrame() &&
        (fp.numActualArgs() >= SNAPSHOT_MAX_NARGS ||
         fp.numActualArgs() > js_IonOptions.maxStackArgs))
    {
        IonSpew(IonSpew_Abort, "too many actual args");
        return false;
    }

    return true;
}

static bool
CheckScript(UnrootedScript script)
{
    if (script->needsArgsObj()) {
        
        IonSpew(IonSpew_Abort, "script has argsobj");
        return false;
    }

    if (!script->compileAndGo) {
        IonSpew(IonSpew_Abort, "not compile-and-go");
        return false;
    }

    return true;
}

MethodStatus
SequentialCompileContext::checkScriptSize(JSContext *cx, UnrootedScript script)
{
    if (!js_IonOptions.limitScriptSize)
        return Method_Compiled;

    
    
    static const uint32_t MAX_MAIN_THREAD_SCRIPT_SIZE = 2000;
    static const uint32_t MAX_OFF_THREAD_SCRIPT_SIZE = 20000;
    static const uint32_t MAX_LOCALS_AND_ARGS = 256;

    if (script->length > MAX_OFF_THREAD_SCRIPT_SIZE) {
        IonSpew(IonSpew_Abort, "Script too large (%u bytes)", script->length);
        return Method_CantCompile;
    }

    if (script->length > MAX_MAIN_THREAD_SCRIPT_SIZE) {
        if (OffThreadCompilationEnabled(cx)) {
            
            
            
            
            
            if (!OffThreadCompilationAvailable(cx) && !cx->runtime->profilingScripts) {
                IonSpew(IonSpew_Abort, "Script too large for main thread, skipping (%u bytes)", script->length);
                return Method_Skipped;
            }
        } else {
            IonSpew(IonSpew_Abort, "Script too large (%u bytes)", script->length);
            return Method_CantCompile;
        }
    }

    uint32_t numLocalsAndArgs = analyze::TotalSlots(script);
    if (numLocalsAndArgs > MAX_LOCALS_AND_ARGS) {
        IonSpew(IonSpew_Abort, "Too many locals and arguments (%u)", numLocalsAndArgs);
        return Method_CantCompile;
    }

    return Method_Compiled;
}

template <typename CompileContext>
static MethodStatus
Compile(JSContext *cx, HandleScript script, HandleFunction fun, jsbytecode *osrPc, bool constructing,
        CompileContext &compileContext)
{
    JS_ASSERT(ion::IsEnabled(cx));
    JS_ASSERT_IF(osrPc != NULL, (JSOp)*osrPc == JSOP_LOOPENTRY);

    if (cx->compartment->debugMode()) {
        IonSpew(IonSpew_Abort, "debugging");
        return Method_CantCompile;
    }

    if (!CheckScript(script)) {
        IonSpew(IonSpew_Abort, "Aborted compilation of %s:%d", script->filename, script->lineno);
        return Method_CantCompile;
    }

    MethodStatus status = compileContext.checkScriptSize(cx, script);
    if (status != Method_Compiled) {
        IonSpew(IonSpew_Abort, "Aborted compilation of %s:%d", script->filename, script->lineno);
        return status;
    }

    ExecutionMode executionMode = compileContext.executionMode();
    IonScript *scriptIon = GetIonScript(script, executionMode);
    if (scriptIon) {
        if (!scriptIon->method())
            return Method_CantCompile;
        return Method_Compiled;
    }

    if (executionMode == SequentialExecution) {
        if (cx->methodJitEnabled) {
            
            

            if (script->getUseCount() < js_IonOptions.usesBeforeCompile)
                return Method_Skipped;
        } else {
            if (script->incUseCount() < js_IonOptions.usesBeforeCompileNoJaeger)
                return Method_Skipped;
        }
    }

    AbortReason reason = IonCompile(cx, script, fun, osrPc, constructing, compileContext);
    if (reason == AbortReason_Disable)
        return Method_CantCompile;

    
    return HasIonScript(script, executionMode) ? Method_Compiled : Method_Skipped;
}

} 
} 



MethodStatus
ion::CanEnterAtBranch(JSContext *cx, JSScript *script, AbstractFramePtr fp,
                      jsbytecode *pc, bool isConstructing)
{
    JS_ASSERT(ion::IsEnabled(cx));
    JS_ASSERT((JSOp)*pc == JSOP_LOOPENTRY);

    
    if (script->ion == ION_DISABLED_SCRIPT)
        return Method_Skipped;

    
    if (script->ion == ION_COMPILING_SCRIPT)
        return Method_Skipped;

    
    if (script->ion && script->ion->bailoutExpected())
        return Method_Skipped;

    
    if (!js_IonOptions.osr)
        return Method_Skipped;

    
    if (!CheckFrame(fp)) {
        ForbidCompilation(cx, script);
        return Method_CantCompile;
    }

    
    RootedFunction fun(cx, fp.isFunctionFrame() ? fp.fun() : NULL);
    SequentialCompileContext compileContext;
    RootedScript rscript(cx, script);
    MethodStatus status = Compile(cx, rscript, fun, pc, isConstructing, compileContext);
    if (status != Method_Compiled) {
        if (status == Method_CantCompile)
            ForbidCompilation(cx, script);
        return status;
    }

    if (script->ion && script->ion->osrPc() != pc)
        return Method_Skipped;

    return Method_Compiled;
}

MethodStatus
ion::CanEnter(JSContext *cx, JSScript *script, AbstractFramePtr fp, bool isConstructing)
{
    JS_ASSERT(ion::IsEnabled(cx));

    
    if (script->ion == ION_DISABLED_SCRIPT)
        return Method_Skipped;

    
    if (script->ion == ION_COMPILING_SCRIPT)
        return Method_Skipped;

    
    if (script->ion && script->ion->bailoutExpected())
        return Method_Skipped;

    
    
    
    if (isConstructing && fp.thisValue().isPrimitive()) {
        RootedScript scriptRoot(cx, script);
        RootedObject callee(cx, &fp.callee());
        RootedObject obj(cx, CreateThisForFunction(cx, callee, fp.useNewType()));
        if (!obj)
            return Method_Skipped;
        fp.thisValue().setObject(*obj);
        script = scriptRoot;
    }

    
    if (!CheckFrame(fp)) {
        ForbidCompilation(cx, script);
        return Method_CantCompile;
    }

    
    RootedFunction fun(cx, fp.isFunctionFrame() ? fp.fun() : NULL);
    SequentialCompileContext compileContext;
    RootedScript rscript(cx, script);
    MethodStatus status = Compile(cx, rscript, fun, NULL, isConstructing, compileContext);
    if (status != Method_Compiled) {
        if (status == Method_CantCompile)
            ForbidCompilation(cx, script);
        return status;
    }

    return Method_Compiled;
}

MethodStatus
ParallelCompileContext::checkScriptSize(JSContext *cx, UnrootedScript script)
{
    if (!js_IonOptions.limitScriptSize)
        return Method_Compiled;

    
    
    static const uint32_t MAX_SCRIPT_SIZE = 5000;
    static const uint32_t MAX_LOCALS_AND_ARGS = 256;

    if (script->length > MAX_SCRIPT_SIZE) {
        IonSpew(IonSpew_Abort, "Script too large (%u bytes)", script->length);
        return Method_CantCompile;
    }

    uint32_t numLocalsAndArgs = analyze::TotalSlots(script);
    if (numLocalsAndArgs > MAX_LOCALS_AND_ARGS) {
        IonSpew(IonSpew_Abort, "Too many locals and arguments (%u)", numLocalsAndArgs);
        return Method_CantCompile;
    }

    return Method_Compiled;
}

MethodStatus
ParallelCompileContext::compileTransitively()
{
    using parallel::SpewBeginCompile;
    using parallel::SpewEndCompile;

    if (worklist_.empty())
        return Method_Skipped;

    RootedFunction fun(cx_);
    RootedScript script(cx_);
    while (!worklist_.empty()) {
        fun = worklist_.back()->toFunction();
        script = fun->nonLazyScript();
        worklist_.popBack();

        SpewBeginCompile(fun);

        
        
        if (script->hasParallelIonScript()) {
            IonScript *ion = script->parallelIonScript();
            JS_ASSERT(ion->parallelInvalidatedScriptEntries() > 0);

            RootedFunction invalidFun(cx_);
            for (uint32_t i = 0; i < ion->parallelInvalidatedScriptEntries(); i++) {
                if (JSScript *invalid = ion->getAndZeroParallelInvalidatedScript(i)) {
                    invalidFun = invalid->function();
                    parallel::Spew(parallel::SpewCompile,
                                   "Adding previously invalidated function %p:%s:%u",
                                   fun.get(), invalid->filename, invalid->lineno);
                    appendToWorklist(invalidFun);
                }
            }
        }

        
        MethodStatus status = Compile(cx_, script, fun, NULL, false, *this);
        if (status != Method_Compiled) {
            if (status == Method_CantCompile)
                ForbidCompilation(cx_, script, ParallelExecution);
            return SpewEndCompile(status);
        }

        
        if (!cx_->compartment->ionCompartment()->enterJIT())
            return SpewEndCompile(Method_Error);

        
        
        
        
        if (!script->hasParallelIonScript()) {
            parallel::Spew(parallel::SpewCompile,
                           "Function %p:%s:%u was garbage-collected or invalidated",
                           fun.get(), script->filename, script->lineno);
            return SpewEndCompile(Method_Skipped);
        }

        SpewEndCompile(Method_Compiled);
    }

    return Method_Compiled;
}

AbortReason
ParallelCompileContext::compile(IonBuilder *builder,
                                MIRGraph *graph,
                                ScopedJSDeletePtr<LifoAlloc> &autoDelete)
{
    JS_ASSERT(!builder->script()->parallelIon);

    RootedScript builderScript(cx_, builder->script());
    IonSpewNewFunction(graph, builderScript);

    if (!builder->build())
        return builder->abortReason();
    builder->clearForBackEnd();

    

    if (!OptimizeMIR(builder)) {
        IonSpew(IonSpew_Abort, "Failed during back-end compilation.");
        return AbortReason_Disable;
    }

    if (!analyzeAndGrowWorklist(builder, *graph)) {
        return AbortReason_Disable;
    }

    CodeGenerator *codegen = GenerateLIR(builder);
    if (!codegen)  {
        IonSpew(IonSpew_Abort, "Failed during back-end compilation.");
        return AbortReason_Disable;
    }

    bool success = codegen->link();
    js_delete(codegen);

    IonSpewEndFunction();

    return success ? AbortReason_NoAbort : AbortReason_Disable;
}

MethodStatus
ion::CanEnterUsingFastInvoke(JSContext *cx, HandleScript script, uint32_t numActualArgs)
{
    JS_ASSERT(ion::IsEnabled(cx));

    
    if (!script->hasIonScript() || script->ion->bailoutExpected())
        return Method_Skipped;

    
    
    if (numActualArgs < script->function()->nargs)
        return Method_Skipped;

    if (!cx->compartment->ensureIonCompartmentExists(cx))
        return Method_Error;

    
    AssertCanGC();
    if (!cx->compartment->ionCompartment()->enterJIT())
        return Method_Error;

    if (!script->ion)
        return Method_Skipped;

    return Method_Compiled;
}

static IonExecStatus
EnterIon(JSContext *cx, StackFrame *fp, void *jitcode)
{
    AssertCanGC();
    JS_CHECK_RECURSION(cx, return IonExec_Aborted);
    JS_ASSERT(ion::IsEnabled(cx));
    JS_ASSERT(CheckFrame(fp));
    JS_ASSERT(!fp->script()->ion->bailoutExpected());

    EnterIonCode enter = cx->compartment->ionCompartment()->enterJIT();

    
    
    int maxArgc = 0;
    Value *maxArgv = NULL;
    int numActualArgs = 0;
    RootedValue thisv(cx);

    void *calleeToken;
    if (fp->isFunctionFrame()) {
        fp->cleanupTornValues();

        
        maxArgc = CountArgSlots(fp->fun()) - 1; 
        maxArgv = fp->formals() - 1;            

        
        
        
        numActualArgs = fp->numActualArgs();

        
        
        if (fp->hasOverflowArgs()) {
            int formalArgc = maxArgc;
            Value *formalArgv = maxArgv;
            maxArgc = numActualArgs + 1; 
            maxArgv = fp->actuals() - 1; 

            
            
            
            memcpy(maxArgv, formalArgv, formalArgc * sizeof(Value));
        }
        calleeToken = CalleeToToken(&fp->callee());
    } else {
        calleeToken = CalleeToToken(fp->script());
        thisv = fp->thisValue();
        maxArgc = 1;
        maxArgv = thisv.address();
    }

    
    JS_ASSERT_IF(fp->isConstructing(), fp->functionThis().isObject());
    RootedValue result(cx, Int32Value(numActualArgs));
    {
        AssertCompartmentUnchanged pcc(cx);
        IonContext ictx(cx, cx->compartment, NULL);
        IonActivation activation(cx, fp);
        JSAutoResolveFlags rf(cx, RESOLVE_INFER);
        AutoFlushInhibitor afi(cx->compartment->ionCompartment());
        
        enter(jitcode, maxArgc, maxArgv, fp, calleeToken, result.address());
    }

    if (result.isMagic() && result.whyMagic() == JS_ION_BAILOUT) {
        if (!EnsureHasScopeObjects(cx, cx->fp()))
            return IonExec_Error;
        return IonExec_Bailout;
    }

    JS_ASSERT(fp == cx->fp());
    JS_ASSERT(!cx->runtime->hasIonReturnOverride());

    
    fp->setReturnValue(result);

    
    if (!result.isMagic() && fp->isConstructing() && fp->returnValue().isPrimitive())
        fp->setReturnValue(ObjectValue(fp->constructorThis()));

    JS_ASSERT_IF(result.isMagic(), result.isMagic(JS_ION_ERROR));
    return result.isMagic() ? IonExec_Error : IonExec_Ok;
}

IonExecStatus
ion::Cannon(JSContext *cx, StackFrame *fp)
{
    AssertCanGC();
    RootedScript script(cx, fp->script());
    IonScript *ion = script->ion;
    IonCode *code = ion->method();
    void *jitcode = code->raw();

#if JS_TRACE_LOGGING
    TraceLog(TraceLogging::defaultLogger(),
             TraceLogging::ION_CANNON_START,
             script);
#endif

    IonExecStatus status = EnterIon(cx, fp, jitcode);

#if JS_TRACE_LOGGING
    if (status == IonExec_Bailout) {
        TraceLog(TraceLogging::defaultLogger(),
                 TraceLogging::ION_CANNON_BAIL,
                 script);
    } else {
        TraceLog(TraceLogging::defaultLogger(),
                 TraceLogging::ION_CANNON_STOP,
                 script);
    }
#endif

    return status;
}

IonExecStatus
ion::SideCannon(JSContext *cx, StackFrame *fp, jsbytecode *pc)
{
    AssertCanGC();
    RootedScript script(cx, fp->script());
    IonScript *ion = script->ion;
    IonCode *code = ion->method();
    void *osrcode = code->raw() + ion->osrEntryOffset();

    JS_ASSERT(ion->osrPc() == pc);

#if JS_TRACE_LOGGING
    TraceLog(TraceLogging::defaultLogger(),
             TraceLogging::ION_SIDE_CANNON_START,
             script);
#endif

    IonExecStatus status = EnterIon(cx, fp, osrcode);

#if JS_TRACE_LOGGING
    if (status == IonExec_Bailout) {
        TraceLog(TraceLogging::defaultLogger(),
                 TraceLogging::ION_SIDE_CANNON_BAIL,
                 script);
    } else {
        TraceLog(TraceLogging::defaultLogger(),
                 TraceLogging::ION_SIDE_CANNON_STOP,
                 script);
    }
#endif

    return status;
}

IonExecStatus
ion::FastInvoke(JSContext *cx, HandleFunction fun, CallArgsList &args)
{
    JS_CHECK_RECURSION(cx, return IonExec_Error);

    IonScript *ion = fun->nonLazyScript()->ionScript();
    IonCode *code = ion->method();
    void *jitcode = code->raw();

    JS_ASSERT(ion::IsEnabled(cx));
    JS_ASSERT(!ion->bailoutExpected());

    bool clearCallingIntoIon = false;
    StackFrame *fp = cx->fp();

    
    
    
    
    
    
    
    
    
    
    IonActivation activation(cx, NULL);
    if (!fp->beginsIonActivation()) {
        fp->setCallingIntoIon();
        clearCallingIntoIon = true;
        activation.setEntryFp(fp);
    } else {
        JS_ASSERT(!activation.entryfp());
    }

    activation.setPrevPc(cx->regs().pc);

    EnterIonCode enter = cx->compartment->ionCompartment()->enterJIT();
    void *calleeToken = CalleeToToken(fun);

    RootedValue result(cx, Int32Value(args.length()));
    JS_ASSERT(args.length() >= fun->nargs);

    JSAutoResolveFlags rf(cx, RESOLVE_INFER);
    args.setActive();
    enter(jitcode, args.length() + 1, args.array() - 1, fp, calleeToken, result.address());
    args.setInactive();

    if (clearCallingIntoIon)
        fp->clearCallingIntoIon();

    JS_ASSERT(fp == cx->fp());
    JS_ASSERT(!cx->runtime->hasIonReturnOverride());

    args.rval().set(result);

    JS_ASSERT_IF(result.isMagic(), result.isMagic(JS_ION_ERROR));
    return result.isMagic() ? IonExec_Error : IonExec_Ok;
}

static void
InvalidateActivation(FreeOp *fop, uint8_t *ionTop, bool invalidateAll)
{
    AutoAssertNoGC nogc;
    IonSpew(IonSpew_Invalidate, "BEGIN invalidating activation");

    size_t frameno = 1;

    for (IonFrameIterator it(ionTop); !it.done(); ++it, ++frameno) {
        JS_ASSERT_IF(frameno == 1, it.type() == IonFrame_Exit);

#ifdef DEBUG
        switch (it.type()) {
          case IonFrame_Exit:
            IonSpew(IonSpew_Invalidate, "#%d exit frame @ %p", frameno, it.fp());
            break;
          case IonFrame_OptimizedJS:
          {
            JS_ASSERT(it.isScripted());
            IonSpew(IonSpew_Invalidate, "#%d JS frame @ %p, %s:%d (fun: %p, script: %p, pc %p)",
                    frameno, it.fp(), it.script()->filename, it.script()->lineno,
                    it.maybeCallee(), (RawScript)it.script(), it.returnAddressToFp());
            break;
          }
          case IonFrame_Rectifier:
            IonSpew(IonSpew_Invalidate, "#%d rectifier frame @ %p", frameno, it.fp());
            break;
          case IonFrame_Unwound_OptimizedJS:
            JS_NOT_REACHED("invalid");
            break;
          case IonFrame_Unwound_Rectifier:
            IonSpew(IonSpew_Invalidate, "#%d unwound rectifier frame @ %p", frameno, it.fp());
            break;
          case IonFrame_Osr:
            IonSpew(IonSpew_Invalidate, "#%d osr frame @ %p", frameno, it.fp());
            break;
          case IonFrame_Entry:
            IonSpew(IonSpew_Invalidate, "#%d entry frame @ %p", frameno, it.fp());
            break;
        }
#endif

        if (!it.isScripted())
            continue;

        
        if (it.checkInvalidation())
            continue;

        RawScript script = it.script();
        if (!script->hasIonScript())
            continue;

        if (!invalidateAll && !script->ion->invalidated())
            continue;

        IonScript *ionScript = script->ion;

        
        
        
        ionScript->purgeCaches(script->compartment());

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        ionScript->incref();

        const SafepointIndex *si = ionScript->getSafepointIndex(it.returnAddressToFp());
        IonCode *ionCode = ionScript->method();

        JS::Zone *zone = script->zone();
        if (zone->needsBarrier()) {
            
            
            
            
            ionCode->trace(zone->barrierTracer());
        }
        ionCode->setInvalidated();

        
        
        
        
        
        
        CodeLocationLabel dataLabelToMunge(it.returnAddressToFp());
        ptrdiff_t delta = ionScript->invalidateEpilogueDataOffset() -
                          (it.returnAddressToFp() - ionCode->raw());
        Assembler::patchWrite_Imm32(dataLabelToMunge, Imm32(delta));

        CodeLocationLabel osiPatchPoint = SafepointReader::InvalidationPatchPoint(ionScript, si);
        CodeLocationLabel invalidateEpilogue(ionCode, ionScript->invalidateEpilogueOffset());

        IonSpew(IonSpew_Invalidate, "   ! Invalidate ionScript %p (ref %u) -> patching osipoint %p",
                ionScript, ionScript->refcount(), (void *) osiPatchPoint.raw());
        Assembler::patchWrite_NearCall(osiPatchPoint, invalidateEpilogue);
    }

    IonSpew(IonSpew_Invalidate, "END invalidating activation");
}

void
ion::InvalidateAll(FreeOp *fop, JSCompartment *c)
{
    if (!c->ionCompartment())
        return;

    CancelOffThreadIonCompile(c, NULL);

    FinishAllOffThreadCompilations(c->ionCompartment());
    for (IonActivationIterator iter(fop->runtime()); iter.more(); ++iter) {
        if (iter.activation()->compartment() == c) {
            IonContext ictx(NULL, c, NULL);
            AutoFlushCache afc ("InvalidateAll", c->ionCompartment());
            IonSpew(IonSpew_Invalidate, "Invalidating all frames for GC");
            InvalidateActivation(fop, iter.top(), true);
        }
    }
}


void
ion::Invalidate(types::TypeCompartment &types, FreeOp *fop,
                const Vector<types::RecompileInfo> &invalid, bool resetUses)
{
    AutoAssertNoGC nogc;
    IonSpew(IonSpew_Invalidate, "Start invalidation.");
    AutoFlushCache afc ("Invalidate");

    
    
    bool anyInvalidation = false;
    for (size_t i = 0; i < invalid.length(); i++) {
        const types::CompilerOutput &co = *invalid[i].compilerOutput(types);
        switch (co.kind()) {
          case types::CompilerOutput::MethodJIT:
            break;
          case types::CompilerOutput::Ion:
          case types::CompilerOutput::ParallelIon:
            JS_ASSERT(co.isValid());
            IonSpew(IonSpew_Invalidate, " Invalidate %s:%u, IonScript %p",
                    co.script->filename, co.script->lineno, co.ion());

            
            
            
            co.ion()->incref();
            anyInvalidation = true;
        }
    }

    if (!anyInvalidation) {
        IonSpew(IonSpew_Invalidate, " No IonScript invalidation.");
        return;
    }

    for (IonActivationIterator iter(fop->runtime()); iter.more(); ++iter)
        InvalidateActivation(fop, iter.top(), false);

    
    
    
    for (size_t i = 0; i < invalid.length(); i++) {
        types::CompilerOutput &co = *invalid[i].compilerOutput(types);
        ExecutionMode executionMode = SequentialExecution;
        switch (co.kind()) {
          case types::CompilerOutput::MethodJIT:
            continue;
          case types::CompilerOutput::Ion:
            break;
          case types::CompilerOutput::ParallelIon:
            executionMode = ParallelExecution;
            break;
        }
        JS_ASSERT(co.isValid());
        UnrootedScript script = co.script;
        IonScript *ionScript = GetIonScript(script, executionMode);

        Zone *zone = script->zone();
        if (zone->needsBarrier()) {
            
            
            
            
            IonScript::Trace(zone->barrierTracer(), ionScript);
        }

        ionScript->decref(fop);
        SetIonScript(script, executionMode, NULL);
        co.invalidate();

        
        
        if (resetUses)
            script->resetUseCount();
    }
}

void
ion::Invalidate(JSContext *cx, const Vector<types::RecompileInfo> &invalid, bool resetUses)
{
    AutoAssertNoGC nogc;
    ion::Invalidate(cx->compartment->types, cx->runtime->defaultFreeOp(), invalid, resetUses);
}

bool
ion::Invalidate(JSContext *cx, UnrootedScript script, ExecutionMode mode, bool resetUses)
{
    AutoAssertNoGC nogc;
    JS_ASSERT(script->hasIonScript());

    Vector<types::RecompileInfo> scripts(cx);

    switch (mode) {
      case SequentialExecution:
        JS_ASSERT(script->hasIonScript());
        if (!scripts.append(script->ionScript()->recompileInfo()))
            return false;
        break;
      case ParallelExecution:
        JS_ASSERT(script->hasParallelIonScript());
        if (!scripts.append(script->parallelIonScript()->recompileInfo()))
            return false;
        break;
    }

    Invalidate(cx, scripts, resetUses);
    return true;
}

bool
ion::Invalidate(JSContext *cx, UnrootedScript script, bool resetUses)
{
    return Invalidate(cx, script, SequentialExecution, resetUses);
}

static void
FinishInvalidationOf(FreeOp *fop, UnrootedScript script, IonScript **ionField)
{
    
    
    if (!(*ionField)->invalidated()) {
        types::TypeCompartment &types = script->compartment()->types;
        (*ionField)->recompileInfo().compilerOutput(types)->invalidate();

        ion::IonScript::Destroy(fop, *ionField);
    }

    
    *ionField = NULL;
}

void
ion::FinishInvalidation(FreeOp *fop, UnrootedScript script)
{
    if (script->hasIonScript())
        FinishInvalidationOf(fop, script, &script->ion);

    if (script->hasParallelIonScript())
        FinishInvalidationOf(fop, script, &script->parallelIon);
}

void
ion::MarkValueFromIon(JSRuntime *rt, Value *vp)
{
    gc::MarkValueUnbarriered(&rt->gcMarker, vp, "write barrier");
}

void
ion::MarkShapeFromIon(JSRuntime *rt, Shape **shapep)
{
    gc::MarkShapeUnbarriered(&rt->gcMarker, shapep, "write barrier");
}

void
ion::ForbidCompilation(JSContext *cx, UnrootedScript script)
{
    ForbidCompilation(cx, script, SequentialExecution);
}

void
ion::ForbidCompilation(JSContext *cx, UnrootedScript script, ExecutionMode mode)
{
    IonSpew(IonSpew_Abort, "Disabling Ion mode %d compilation of script %s:%d",
            mode, script->filename, script->lineno);

    CancelOffThreadIonCompile(cx->compartment, script);

    switch (mode) {
      case SequentialExecution:
        if (script->hasIonScript()) {
            
            
            
            
            
            if (!Invalidate(cx, script, mode, false))
                return;
        }

        script->ion = ION_DISABLED_SCRIPT;
        return;

      case ParallelExecution:
        if (script->hasParallelIonScript()) {
            if (!Invalidate(cx, script, mode, false))
                return;
        }

        script->parallelIon = ION_DISABLED_SCRIPT;
        return;
    }

    JS_NOT_REACHED("No such execution mode");
}

uint32_t
ion::UsesBeforeIonRecompile(UnrootedScript script, jsbytecode *pc)
{
    JS_ASSERT(pc == script->code || JSOp(*pc) == JSOP_LOOPENTRY);

    uint32_t minUses = js_IonOptions.usesBeforeCompile;
    if (JSOp(*pc) != JSOP_LOOPENTRY || !script->hasAnalysis() || js_IonOptions.eagerCompilation)
        return minUses;

    analyze::LoopAnalysis *loop = script->analysis()->getLoop(pc);
    if (!loop)
        return minUses;

    
    
    
    return minUses + (loop->depth + 1) * 100;
}

void
AutoFlushCache::updateTop(uintptr_t p, size_t len)
{
    IonContext *ictx = GetIonContext();
    IonCompartment *icmp = ictx->compartment->ionCompartment();
    AutoFlushCache *afc = icmp->flusher();
    afc->update(p, len);
}

AutoFlushCache::AutoFlushCache(const char *nonce, IonCompartment *comp)
  : start_(0),
    stop_(0),
    name_(nonce),
    used_(false)
{
    if (CurrentIonContext() != NULL)
        comp = GetIonContext()->compartment->ionCompartment();
    
    if (comp) {
        if (comp->flusher())
            IonSpew(IonSpew_CacheFlush, "<%s ", nonce);
        else
            IonSpewCont(IonSpew_CacheFlush, "<%s ", nonce);
        comp->setFlusher(this);
    } else {
        IonSpew(IonSpew_CacheFlush, "<%s DEAD>\n", nonce);
    }
    myCompartment_ = comp;
}

AutoFlushInhibitor::AutoFlushInhibitor(IonCompartment *ic) : ic_(ic), afc(NULL)
{
    if (!ic)
        return;
    afc = ic->flusher();
    
    ic->setFlusher(NULL);
    
    if (afc) {
        afc->flushAnyway();
        IonSpewCont(IonSpew_CacheFlush, "}");
    }
}
AutoFlushInhibitor::~AutoFlushInhibitor()
{
    if (!ic_)
        return;
    JS_ASSERT(ic_->flusher() == NULL);
    
    ic_->setFlusher(afc);
    if (afc)
        IonSpewCont(IonSpew_CacheFlush, "{");
}

int js::ion::LabelBase::id_count = 0;

void
ion::PurgeCaches(UnrootedScript script, JSCompartment *c) {
    if (script->hasIonScript())
        script->ion->purgeCaches(c);

    if (script->hasParallelIonScript())
        script->parallelIon->purgeCaches(c);
}

size_t
ion::MemoryUsed(UnrootedScript script, JSMallocSizeOfFun mallocSizeOf) {
    size_t result = 0;

    if (script->hasIonScript())
        result += script->ion->sizeOfIncludingThis(mallocSizeOf);

    if (script->hasParallelIonScript())
        result += script->parallelIon->sizeOfIncludingThis(mallocSizeOf);

    return result;
}

void
ion::DestroyIonScripts(FreeOp *fop, UnrootedScript script) {
    if (script->hasIonScript())
        ion::IonScript::Destroy(fop, script->ion);

    if (script->hasParallelIonScript())
        ion::IonScript::Destroy(fop, script->parallelIon);
}

void
ion::TraceIonScripts(JSTracer* trc, UnrootedScript script) {
    if (script->hasIonScript())
        ion::IonScript::Trace(trc, script->ion);

    if (script->hasParallelIonScript())
        ion::IonScript::Trace(trc, script->parallelIon);
}
