






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
#include "jscompartment.h"
#include "jsworkers.h"
#include "IonCompartment.h"
#include "CodeGenerator.h"

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

#if JS_TRACE_LOGGING
#include "TraceLogging.h"
#endif

using namespace js;
using namespace js::ion;


IonOptions ion::js_IonOptions;


JS_STATIC_ASSERT(sizeof(IonCode) % gc::Cell::CellSize == 0);

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

    execAlloc_ = cx->runtime->getExecAlloc(cx);
    if (!execAlloc_)
        return false;

    functionWrappers_ = cx->new_<VMWrapperMap>(cx);
    if (!functionWrappers_ || !functionWrappers_->init())
        return false;

    if (!bailoutTables_.reserve(FrameSizeClass::ClassLimit().classId()))
        return false;

    for (uint32 id = 0;; id++) {
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

    preBarrier_ = generatePreBarrier(cx);
    if (!preBarrier_)
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

void
ion::FinishOffThreadBuilder(IonBuilder *builder)
{
    if (builder->script()->isIonCompilingOffThread()) {
        types::TypeCompartment &types = builder->script()->compartment()->types;
        builder->recompileInfo.compilerOutput(types)->invalidate();
        builder->script()->ion = NULL;
    }
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
IonCompartment::mark(JSTracer *trc, JSCompartment *compartment)
{
    
    
    
    
    

    bool mustMarkEnterJIT = false;
    for (IonActivationIterator iter(trc->runtime); iter.more(); ++iter) {
        IonActivation *activation = iter.activation();

        if (activation->compartment() != compartment)
            continue;

        
        
        mustMarkEnterJIT = true;
    }

    
    

    
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
    prev_(cx->runtime->ionActivation),
    entryfp_(fp),
    bailout_(NULL),
    prevIonTop_(cx->runtime->ionTop),
    prevIonJSContext_(cx->runtime->ionJSContext),
    prevpc_(NULL)
{
    if (fp)
        fp->setRunningInIon();
    cx->runtime->ionJSContext = cx;
    cx->runtime->ionActivation = this;
    cx->runtime->ionStackLimit = cx->runtime->nativeStackLimit;
}

IonActivation::~IonActivation()
{
    JS_ASSERT(cx_->runtime->ionActivation == this);
    JS_ASSERT(!bailout_);

    if (entryfp_)
        entryfp_->clearRunningInIon();
    cx_->runtime->ionActivation = prev();
    cx_->runtime->ionTop = prevIonTop_;
    cx_->runtime->ionJSContext = prevIonJSContext_;
}

IonCode *
IonCode::New(JSContext *cx, uint8 *code, uint32 bufferSize, JSC::ExecutablePool *pool)
{
    AssertCanGC();

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
IonCode::finalize(FreeOp *fop)
{
    
    JS_POISON(code_, JS_FREE_PATTERN, bufferSize_);

    
    
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
        MarkIonCodeUnbarriered(comp->barrierTracer(), &code, "ioncode read barrier");
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
        MarkIonCodeUnbarriered(comp->barrierTracer(), &code, "ioncode write barrier");
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
    snapshots_(0),
    snapshotsSize_(0),
    bailoutTable_(0),
    bailoutEntries_(0),
    constantTable_(0),
    constantEntries_(0),
    safepointIndexOffset_(0),
    safepointIndexEntries_(0),
    frameSlots_(0),
    frameSize_(0),
    osiIndexOffset_(0),
    osiIndexEntries_(0),
    cacheList_(0),
    cacheEntries_(0),
    prebarrierList_(0),
    prebarrierEntries_(0),
    safepointsStart_(0),
    safepointsSize_(0),
    scriptList_(0),
    scriptEntries_(0),
    refcount_(0),
    recompileInfo_(),
    slowCallCount(0)
{
}

static const int DataAlignment = 4;

IonScript *
IonScript::New(JSContext *cx, uint32 frameSlots, uint32 frameSize, size_t snapshotsSize,
               size_t bailoutEntries, size_t constants, size_t safepointIndices,
               size_t osiIndices, size_t cacheEntries, size_t prebarrierEntries,
               size_t safepointsSize, size_t scriptEntries)
{
    if (snapshotsSize >= MAX_BUFFER_SIZE ||
        (bailoutEntries >= MAX_BUFFER_SIZE / sizeof(uint32)))
    {
        js_ReportOutOfMemory(cx);
        return NULL;
    }

    
    
    
    size_t paddedSnapshotsSize = AlignBytes(snapshotsSize, DataAlignment);
    size_t paddedBailoutSize = AlignBytes(bailoutEntries * sizeof(uint32), DataAlignment);
    size_t paddedConstantsSize = AlignBytes(constants * sizeof(Value), DataAlignment);
    size_t paddedSafepointIndicesSize = AlignBytes(safepointIndices * sizeof(SafepointIndex), DataAlignment);
    size_t paddedOsiIndicesSize = AlignBytes(osiIndices * sizeof(OsiIndex), DataAlignment);
    size_t paddedCacheEntriesSize = AlignBytes(cacheEntries * sizeof(IonCache), DataAlignment);
    size_t paddedPrebarrierEntriesSize =
        AlignBytes(prebarrierEntries * sizeof(CodeOffsetLabel), DataAlignment);
    size_t paddedSafepointSize = AlignBytes(safepointsSize, DataAlignment);
    size_t paddedScriptSize = AlignBytes(scriptEntries * sizeof(JSScript *), DataAlignment);
    size_t bytes = paddedSnapshotsSize +
                   paddedBailoutSize +
                   paddedConstantsSize +
                   paddedSafepointIndicesSize+
                   paddedOsiIndicesSize +
                   paddedCacheEntriesSize +
                   paddedPrebarrierEntriesSize +
                   paddedSafepointSize +
                   paddedScriptSize;
    uint8 *buffer = (uint8 *)cx->malloc_(sizeof(IonScript) + bytes);
    if (!buffer)
        return NULL;

    IonScript *script = reinterpret_cast<IonScript *>(buffer);
    new (script) IonScript();

    uint32 offsetCursor = sizeof(IonScript);

    script->snapshots_ = offsetCursor;
    script->snapshotsSize_ = snapshotsSize;
    offsetCursor += paddedSnapshotsSize;

    script->bailoutTable_ = offsetCursor;
    script->bailoutEntries_ = bailoutEntries;
    offsetCursor += paddedBailoutSize;

    script->constantTable_ = offsetCursor;
    script->constantEntries_ = constants;
    offsetCursor += paddedConstantsSize;

    script->safepointIndexOffset_ = offsetCursor;
    script->safepointIndexEntries_ = safepointIndices;
    offsetCursor += paddedSafepointIndicesSize;

    script->osiIndexOffset_ = offsetCursor;
    script->osiIndexEntries_ = osiIndices;
    offsetCursor += paddedOsiIndicesSize;

    script->cacheList_ = offsetCursor;
    script->cacheEntries_ = cacheEntries;
    offsetCursor += paddedCacheEntriesSize;

    script->prebarrierList_ = offsetCursor;
    script->prebarrierEntries_ = prebarrierEntries;
    offsetCursor += paddedPrebarrierEntriesSize;

    script->safepointsStart_ = offsetCursor;
    script->safepointsSize_ = safepointsSize;
    offsetCursor += paddedSafepointSize;

    script->scriptList_ = offsetCursor;
    script->scriptEntries_ = scriptEntries;
    offsetCursor += paddedScriptSize;

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
IonScript::copyScriptEntries(JSScript **scripts)
{
    for (size_t i = 0; i < scriptEntries_; i++)
        scriptList()[i] = scripts[i];
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
IonScript::copyCacheEntries(const IonCache *caches, MacroAssembler &masm)
{
    memcpy(cacheList(), caches, numCaches() * sizeof(IonCache));

    




    for (size_t i = 0; i < numCaches(); i++)
        getCache(i).updateBaseAddress(method_, masm);
}

inline CodeOffsetLabel &
IonScript::getPrebarrier(size_t index)
{
    JS_ASSERT(index < numPrebarriers());
    return prebarrierList()[index];
}

void
IonScript::copyPrebarrierEntries(const CodeOffsetLabel *barriers, MacroAssembler &masm)
{
    memcpy(prebarrierList(), barriers, numPrebarriers() * sizeof(CodeOffsetLabel));

    
    for (size_t i = 0; i < numPrebarriers(); i++)
        getPrebarrier(i).fixup(&masm);
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

    JS_ASSERT(containsCodeAddress(retAddr));
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
IonScript::Destroy(FreeOp *fop, IonScript *script)
{
    fop->free_(script);
}

void
IonScript::toggleBarriers(bool enabled)
{
    for (size_t i = 0; i < numPrebarriers(); i++) {
        CodeLocationLabel loc(method(), getPrebarrier(i));

        if (enabled)
            Assembler::ToggleToCmp(loc);
        else
            Assembler::ToggleToJmp(loc);
    }
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
        JSScript *script = i.get<JSScript>();
        if (script->hasIonScript())
            script->ion->toggleBarriers(needs);
    }
}

namespace js {
namespace ion {

LIRGraph *
CompileBackEnd(MIRGenerator *mir)
{
    IonSpewPass("BuildSSA");
    
    

    MIRGraph &graph = mir->graph();

    if (mir->shouldCancel("Start"))
        return NULL;

    if (!SplitCriticalEdges(graph))
        return NULL;
    IonSpewPass("Split Critical Edges");
    AssertGraphCoherency(graph);

    if (mir->shouldCancel("Split Critical Edges"))
        return NULL;

    if (!RenumberBlocks(graph))
        return NULL;
    IonSpewPass("Renumber Blocks");
    AssertGraphCoherency(graph);

    if (mir->shouldCancel("Renumber Blocks"))
        return NULL;

    if (!BuildDominatorTree(graph))
        return NULL;
    

    if (mir->shouldCancel("Dominator Tree"))
        return NULL;

    
    if (!EliminatePhis(mir, graph))
        return NULL;
    IonSpewPass("Eliminate phis");
    AssertGraphCoherency(graph);

    if (mir->shouldCancel("Eliminate phis"))
        return NULL;

    if (!BuildPhiReverseMapping(graph))
        return NULL;
    

    if (mir->shouldCancel("Phi reverse mapping"))
        return NULL;

    
    if (!ApplyTypeInformation(mir, graph))
        return NULL;
    IonSpewPass("Apply types");
    AssertGraphCoherency(graph);

    if (mir->shouldCancel("Apply types"))
        return NULL;

    
    
    if (js_IonOptions.licm || js_IonOptions.gvn) {
        AliasAnalysis analysis(mir, graph);
        if (!analysis.analyze())
            return NULL;
        IonSpewPass("Alias analysis");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("Alias analysis"))
            return NULL;
    }

    if (js_IonOptions.edgeCaseAnalysis) {
        EdgeCaseAnalysis edgeCaseAnalysis(mir, graph);
        if (!edgeCaseAnalysis.analyzeEarly())
            return NULL;
        IonSpewPass("Edge Case Analysis (Early)");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("Edge Case Analysis (Early)"))
            return NULL;
    }

    if (js_IonOptions.gvn) {
        ValueNumberer gvn(mir, graph, js_IonOptions.gvnIsOptimistic);
        if (!gvn.analyze())
            return NULL;
        IonSpewPass("GVN");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("GVN"))
            return NULL;
    }

    if (js_IonOptions.rangeAnalysis) {
        RangeAnalysis r(graph);
        if (!r.addBetaNobes())
            return NULL;
        IonSpewPass("Beta");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("RA Beta"))
            return NULL;

        if (!r.analyze())
            return NULL;
        IonSpewPass("Range Analysis");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("Range Analysis"))
            return NULL;

        if (!r.removeBetaNobes())
            return NULL;
        IonSpewPass("De-Beta");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("RA De-Beta"))
            return NULL;
    }

    if (!EliminateDeadCode(mir, graph))
        return NULL;
    IonSpewPass("DCE");
    AssertGraphCoherency(graph);

    if (mir->shouldCancel("DCE"))
        return NULL;

    if (js_IonOptions.licm) {
        LICM licm(mir, graph);
        if (!licm.analyze())
            return NULL;
        IonSpewPass("LICM");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("LICM"))
            return NULL;
    }

    if (js_IonOptions.edgeCaseAnalysis) {
        EdgeCaseAnalysis edgeCaseAnalysis(mir, graph);
        if (!edgeCaseAnalysis.analyzeLate())
            return NULL;
        IonSpewPass("Edge Case Analysis (Late)");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("Edge Case Analysis (Late)"))
            return NULL;
    }

    
    
    
    
    if (!EliminateRedundantBoundsChecks(graph))
        return NULL;
    IonSpewPass("Bounds Check Elimination");
    AssertGraphCoherency(graph);

    if (mir->shouldCancel("Bounds Check Elimination"))
        return NULL;

    LIRGraph *lir = mir->temp().lifoAlloc()->new_<LIRGraph>(&graph);
    if (!lir)
        return NULL;

    LIRGenerator lirgen(mir, graph, *lir);
    if (!lirgen.generate())
        return NULL;
    IonSpewPass("Generate LIR");

    if (mir->shouldCancel("Generate LIR"))
        return NULL;

    if (js_IonOptions.lsra) {
        LinearScanAllocator regalloc(mir, &lirgen, *lir);
        if (!regalloc.go())
            return NULL;
        IonSpewPass("Allocate Registers", &regalloc);

        if (mir->shouldCancel("Allocate Registers"))
            return NULL;
    }

    return lir;
}

class AutoDestroyAllocator
{
    LifoAlloc *alloc;

  public:
    AutoDestroyAllocator(LifoAlloc *alloc) : alloc(alloc) {}

    void cancel()
    {
        alloc = NULL;
    }

    ~AutoDestroyAllocator()
    {
        if (alloc)
            js_delete(alloc);
    }
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

        if (builder->backgroundCompiledLir) {
            RootedScript script(cx, builder->script());
            IonContext ictx(cx, cx->compartment, &builder->temp());

            CodeGenerator codegen(builder, *builder->backgroundCompiledLir);

            types::AutoEnterTypeInference enterTypes(cx);
            types::AutoEnterCompilation enterCompiler(cx, types::AutoEnterCompilation::Ion);
            enterCompiler.initExisting(builder->recompileInfo);

            bool success;
            {
                
                AutoTempAllocatorRooter root(cx, &builder->temp());
                AutoUnlockWorkerThreadState unlock(cx->runtime);
                success = codegen.generate();
            }

            if (success) {
                if (script->hasIonScript())
                    mjit::ReleaseScriptCodeFromVM(cx, script);
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

static bool
IonCompile(JSContext *cx, JSScript *script, JSFunction *fun, jsbytecode *osrPc, bool constructing)
{
    AssertCanGC();
#if JS_TRACE_LOGGING
    AutoTraceLog logger(TraceLogging::defaultLogger(),
                        TraceLogging::ION_COMPILE_START,
                        TraceLogging::ION_COMPILE_STOP,
                        script);
#endif

    LifoAlloc *alloc = cx->new_<LifoAlloc>(BUILDER_LIFO_ALLOC_PRIMARY_CHUNK_SIZE);
    if (!alloc)
        return false;

    AutoDestroyAllocator autoDestroy(alloc);

    TempAllocator *temp = alloc->new_<TempAllocator>(alloc);
    if (!temp)
        return false;

    IonContext ictx(cx, cx->compartment, temp);

    if (!cx->compartment->ensureIonCompartmentExists(cx))
        return false;

    MIRGraph *graph = alloc->new_<MIRGraph>(temp);
    CompileInfo *info = alloc->new_<CompileInfo>(script, fun, osrPc, constructing);
    if (!info)
        return false;

    types::AutoEnterTypeInference enter(cx, true);
    TypeInferenceOracle oracle;

    if (!oracle.init(cx, script))
        return false;

    AutoFlushCache afc("IonCompile");

    types::AutoEnterCompilation enterCompiler(cx, types::AutoEnterCompilation::Ion);
    enterCompiler.init(script, false, 0);

    AutoTempAllocatorRooter root(cx, temp);

    IonBuilder *builder = alloc->new_<IonBuilder>(cx, temp, graph, &oracle, info);
    JS_ASSERT(!builder->script()->ion);

    IonSpewNewFunction(graph, builder->script().unsafeGet());

    if (!builder->build()) {
        IonSpew(IonSpew_Abort, "Builder failed to build.");
        return false;
    }
    builder->clearForBackEnd();

    if (js_IonOptions.parallelCompilation && OffThreadCompilationAvailable(cx)) {
        builder->script()->ion = ION_COMPILING_SCRIPT;

        if (!StartOffThreadIonCompile(cx, builder)) {
            IonSpew(IonSpew_Abort, "Unable to start off-thread ion compilation.");
            return false;
        }

        
        
        autoDestroy.cancel();

        return true;
    }

    LIRGraph *lir = CompileBackEnd(builder);
    if (!lir) {
        IonSpew(IonSpew_Abort, "Failed during back-end compilation.");
        return false;
    }

    CodeGenerator codegen(builder, *lir);
    if (!codegen.generate()) {
        IonSpew(IonSpew_Abort, "Failed during code generation.");
        return false;
    }

    IonSpewEndFunction();

    return true;
}

bool
TestIonCompile(JSContext *cx, JSScript *script, JSFunction *fun, jsbytecode *osrPc, bool constructing)
{
    if (!IonCompile(cx, script, fun, osrPc, constructing)) {
        if (!cx->isExceptionPending())
            ForbidCompilation(cx, script);
        return false;
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

    if (fp->isGeneratorFrame()) {
        
        IonSpew(IonSpew_Abort, "generator frame");
        return false;
    }

    if (fp->isDebuggerFrame()) {
        IonSpew(IonSpew_Abort, "debugger frame");
        return false;
    }

    if (fp->annotation()) {
        IonSpew(IonSpew_Abort, "frame is annotated");
        return false;
    }

    
    
    if (fp->isFunctionFrame() &&
        (fp->numActualArgs() >= SNAPSHOT_MAX_NARGS ||
         fp->numActualArgs() > js_IonOptions.maxStackArgs))
    {
        IonSpew(IonSpew_Abort, "too many actual args");
        return false;
    }

    return true;
}

static bool
CheckScript(JSScript *script)
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

static bool
CheckScriptSize(JSScript *script)
{
    if (!js_IonOptions.limitScriptSize)
        return true;

    static const uint32_t MAX_SCRIPT_SIZE = 1500;
    static const uint32_t MAX_LOCALS_AND_ARGS = 256;

    if (script->length > MAX_SCRIPT_SIZE) {
        IonSpew(IonSpew_Abort, "Script too large (%u bytes)", script->length);
        return false;
    }

    uint32_t numLocalsAndArgs = analyze::TotalSlots(script);
    if (numLocalsAndArgs > MAX_LOCALS_AND_ARGS) {
        IonSpew(IonSpew_Abort, "Too many locals and arguments (%u)", numLocalsAndArgs);
        return false;
    }

    return true;
}

static MethodStatus
Compile(JSContext *cx, JSScript *script, JSFunction *fun, jsbytecode *osrPc, bool constructing)
{
    JS_ASSERT(ion::IsEnabled(cx));
    JS_ASSERT_IF(osrPc != NULL, (JSOp)*osrPc == JSOP_LOOPENTRY);

    if (cx->compartment->debugMode()) {
        IonSpew(IonSpew_Abort, "debugging");
        return Method_CantCompile;
    }

    if (!CheckScript(script) || !CheckScriptSize(script)) {
        IonSpew(IonSpew_Abort, "Aborted compilation of %s:%d", script->filename, script->lineno);
        return Method_CantCompile;
    }

    if (script->ion) {
        if (!script->ion->method())
            return Method_CantCompile;
        return Method_Compiled;
    }

    if (cx->methodJitEnabled) {
        
        
        if (script->getUseCount() < js_IonOptions.usesBeforeCompile)
            return Method_Skipped;
    } else {
        if (script->incUseCount() < js_IonOptions.usesBeforeCompileNoJaeger)
            return Method_Skipped;
    }

    if (!IonCompile(cx, script, fun, osrPc, constructing))
        return Method_CantCompile;

    
    return script->hasIonScript() ? Method_Compiled : Method_Skipped;
}

} 
} 



MethodStatus
ion::CanEnterAtBranch(JSContext *cx, HandleScript script, StackFrame *fp, jsbytecode *pc)
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

    
    JSFunction *fun = fp->isFunctionFrame() ? fp->fun() : NULL;
    MethodStatus status = Compile(cx, script, fun, pc, false);
    if (status != Method_Compiled) {
        if (status == Method_CantCompile)
            ForbidCompilation(cx, script);
        return status;
    }

    if (script->ion->osrPc() != pc)
        return Method_Skipped;

    return Method_Compiled;
}

MethodStatus
ion::CanEnter(JSContext *cx, HandleScript script, StackFrame *fp, bool newType)
{
    JS_ASSERT(ion::IsEnabled(cx));

    
    if (script->ion == ION_DISABLED_SCRIPT)
        return Method_Skipped;

    
    if (script->ion == ION_COMPILING_SCRIPT)
        return Method_Skipped;

    
    if (script->ion && script->ion->bailoutExpected())
        return Method_Skipped;

    
    
    
    if (fp->isConstructing() && fp->functionThis().isPrimitive()) {
        RootedObject callee(cx, &fp->callee());
        RootedObject obj(cx, js_CreateThisForFunction(cx, callee, newType));
        if (!obj)
            return Method_Skipped;
        fp->functionThis().setObject(*obj);
    }

    
    if (!CheckFrame(fp)) {
        ForbidCompilation(cx, script);
        return Method_CantCompile;
    }

    
    JSFunction *fun = fp->isFunctionFrame() ? fp->fun() : NULL;
    MethodStatus status = Compile(cx, script, fun, NULL, fp->isConstructing());
    if (status != Method_Compiled) {
        if (status == Method_CantCompile)
            ForbidCompilation(cx, script);
        return status;
    }

    return Method_Compiled;
}

MethodStatus
ion::CanEnterUsingFastInvoke(JSContext *cx, HandleScript script)
{
    JS_ASSERT(ion::IsEnabled(cx));

    
    if (!script->hasIonScript() || script->ion->bailoutExpected())
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
    JS_CHECK_RECURSION(cx, return IonExec_Error);
    JS_ASSERT(ion::IsEnabled(cx));
    JS_ASSERT(CheckFrame(fp));
    JS_ASSERT(!fp->script()->ion->bailoutExpected());

    EnterIonCode enter = cx->compartment->ionCompartment()->enterJIT();

    
    
    int maxArgc = 0;
    Value *maxArgv = NULL;
    int numActualArgs = 0;

    void *calleeToken;
    if (fp->isFunctionFrame()) {
        
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
        calleeToken = CalleeToToken(fp->script().unsafeGet());
    }

    
    JS_ASSERT_IF(fp->isConstructing(), fp->functionThis().isObject());

    Value result = Int32Value(numActualArgs);
    {
        AssertCompartmentUnchanged pcc(cx);
        IonContext ictx(cx, cx->compartment, NULL);
        IonActivation activation(cx, fp);
        JSAutoResolveFlags rf(cx, RESOLVE_INFER);

        
        enter(jitcode, maxArgc, maxArgv, fp, calleeToken, &result);
    }

    if (result.isMagic() && result.whyMagic() == JS_ION_BAILOUT) {
        if (!EnsureHasCallObject(cx, cx->fp()))
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
ion::FastInvoke(JSContext *cx, HandleFunction fun, CallArgs &args)
{
    JS_CHECK_RECURSION(cx, return IonExec_Error);

    RootedScript script(cx, fun->script());
    IonScript *ion = script->ionScript();
    IonCode *code = ion->method();
    void *jitcode = code->raw();

    JS_ASSERT(ion::IsEnabled(cx));
    JS_ASSERT(!script->ion->bailoutExpected());

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

    Value result = Int32Value(fun->nargs);

    JSAutoResolveFlags rf(cx, RESOLVE_INFER);
    enter(jitcode, args.length() + 1, &args[0] - 1, fp, calleeToken, &result);

    if (clearCallingIntoIon)
        fp->clearCallingIntoIon();

    JS_ASSERT(fp == cx->fp());
    JS_ASSERT(!cx->runtime->hasIonReturnOverride());

    args.rval().set(result);

    JS_ASSERT_IF(result.isMagic(), result.isMagic(JS_ION_ERROR));
    return result.isMagic() ? IonExec_Error : IonExec_Ok;
}

static void
InvalidateActivation(FreeOp *fop, uint8 *ionTop, bool invalidateAll)
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
          case IonFrame_JS:
          {
            JS_ASSERT(it.isScripted());
            IonSpew(IonSpew_Invalidate, "#%d JS frame @ %p, %s:%d (fun: %p, script: %p, pc %p)",
                    frameno, it.fp(), it.script()->filename, it.script()->lineno,
                    it.maybeCallee(), it.script(), it.returnAddressToFp());
            break;
          }
          case IonFrame_Rectifier:
            IonSpew(IonSpew_Invalidate, "#%d rectifier frame @ %p", frameno, it.fp());
            break;
          case IonFrame_Bailed_JS:
            JS_NOT_REACHED("invalid");
            break;
          case IonFrame_Bailed_Rectifier:
            IonSpew(IonSpew_Invalidate, "#%d bailed rectifier frame @ %p", frameno, it.fp());
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

        JSCompartment *compartment = script->compartment();
        if (compartment->needsBarrier()) {
            
            
            
            
            ionCode->trace(compartment->barrierTracer());
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
    IonSpew(IonSpew_Invalidate, "Start invalidation.");
    AutoFlushCache afc ("Invalidate");

    
    
    bool anyInvalidation = false;
    for (size_t i = 0; i < invalid.length(); i++) {
        const types::CompilerOutput &co = *invalid[i].compilerOutput(types);
        if (co.isIon()) {
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
        if (co.isIon()) {
            JS_ASSERT(co.isValid());
            JSScript *script = co.script;
            IonScript *ionScript = script->ionScript();

            JSCompartment *compartment = script->compartment();
            if (compartment->needsBarrier()) {
                
                
                
                
                IonScript::Trace(compartment->barrierTracer(), ionScript);
            }

            ionScript->decref(fop);
            script->ion = NULL;
            co.invalidate();

            
            
            if (resetUses)
                script->resetUseCount();
        }
    }
}

void
ion::Invalidate(JSContext *cx, const Vector<types::RecompileInfo> &invalid, bool resetUses)
{
    ion::Invalidate(cx->compartment->types, cx->runtime->defaultFreeOp(), invalid, resetUses);
}

bool
ion::Invalidate(JSContext *cx, JSScript *script, bool resetUses)
{
    JS_ASSERT(script->hasIonScript());

    Vector<types::RecompileInfo> scripts(cx);
    if (!scripts.append(script->ionScript()->recompileInfo()))
        return false;

    Invalidate(cx, scripts, resetUses);
    return true;
}

void
ion::FinishInvalidation(FreeOp *fop, JSScript *script)
{
    if (!script->hasIonScript())
        return;

    



    if (!script->ion->invalidated()) {
        types::TypeCompartment &types = script->compartment()->types;
        script->ion->recompileInfo().compilerOutput(types)->invalidate();

        ion::IonScript::Destroy(fop, script->ion);
    }

    
    script->ion = NULL;
}

void
ion::MarkFromIon(JSCompartment *comp, Value *vp)
{
    gc::MarkValueUnbarriered(comp->barrierTracer(), vp, "write barrier");
}

void
ion::ForbidCompilation(JSContext *cx, JSScript *script)
{
    IonSpew(IonSpew_Abort, "Disabling Ion compilation of script %s:%d",
            script->filename, script->lineno);

    if (script->hasIonScript()) {
        
        
        
        
        
        if (!Invalidate(cx, script, false))
            return;
    }

    script->ion = ION_DISABLED_SCRIPT;
}

uint32_t
ion::UsesBeforeIonRecompile(JSScript *script, jsbytecode *pc)
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
    if (comp == NULL) {
        if (CurrentIonContext() != NULL)
            comp = GetIonContext()->compartment->ionCompartment();
    }
    
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
int js::ion::LabelBase::id_count = 0;

