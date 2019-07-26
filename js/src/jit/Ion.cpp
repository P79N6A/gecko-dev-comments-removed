





#include "jit/Ion.h"

#include "mozilla/MemoryReporting.h"
#include "mozilla/ThreadLocal.h"

#include "jscompartment.h"
#include "jsworkers.h"
#if JS_TRACE_LOGGING
#include "TraceLogging.h"
#endif

#include "gc/Marking.h"
#include "jit/AliasAnalysis.h"
#include "jit/AsmJSModule.h"
#include "jit/BacktrackingAllocator.h"
#include "jit/BaselineFrame.h"
#include "jit/BaselineInspector.h"
#include "jit/BaselineJIT.h"
#include "jit/CodeGenerator.h"
#include "jit/EdgeCaseAnalysis.h"
#include "jit/EffectiveAddressAnalysis.h"
#include "jit/ExecutionModeInlines.h"
#include "jit/IonAnalysis.h"
#include "jit/IonBuilder.h"
#include "jit/IonOptimizationLevels.h"
#include "jit/IonSpewer.h"
#include "jit/JitCommon.h"
#include "jit/JitCompartment.h"
#include "jit/LICM.h"
#include "jit/LinearScan.h"
#include "jit/LIR.h"
#include "jit/Lowering.h"
#include "jit/ParallelSafetyAnalysis.h"
#include "jit/PerfSpewer.h"
#include "jit/RangeAnalysis.h"
#include "jit/StupidAllocator.h"
#include "jit/UnreachableCodeElimination.h"
#include "jit/ValueNumbering.h"
#include "vm/ForkJoin.h"

#include "jscompartmentinlines.h"
#include "jsgcinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"

using namespace js;
using namespace js::jit;

using mozilla::Maybe;
using mozilla::ThreadLocal;


JS_STATIC_ASSERT(sizeof(JitCode) % gc::CellSize == 0);

static ThreadLocal<IonContext*> TlsIonContext;

static IonContext *
CurrentIonContext()
{
    if (!TlsIonContext.initialized())
        return nullptr;
    return TlsIonContext.get();
}

void
jit::SetIonContext(IonContext *ctx)
{
    TlsIonContext.set(ctx);
}

IonContext *
jit::GetIonContext()
{
    MOZ_ASSERT(CurrentIonContext());
    return CurrentIonContext();
}

IonContext *
jit::MaybeGetIonContext()
{
    return CurrentIonContext();
}

IonContext::IonContext(JSContext *cx, TempAllocator *temp)
  : cx(cx),
    temp(temp),
    runtime(CompileRuntime::get(cx->runtime())),
    compartment(CompileCompartment::get(cx->compartment())),
    prev_(CurrentIonContext()),
    assemblerCount_(0)
{
    SetIonContext(this);
}

IonContext::IonContext(ExclusiveContext *cx, TempAllocator *temp)
  : cx(nullptr),
    temp(temp),
    runtime(CompileRuntime::get(cx->runtime_)),
    compartment(nullptr),
    prev_(CurrentIonContext()),
    assemblerCount_(0)
{
    SetIonContext(this);
}

IonContext::IonContext(CompileRuntime *rt, CompileCompartment *comp, TempAllocator *temp)
  : cx(nullptr),
    temp(temp),
    runtime(rt),
    compartment(comp),
    prev_(CurrentIonContext()),
    assemblerCount_(0)
{
    SetIonContext(this);
}

IonContext::IonContext(CompileRuntime *rt)
  : cx(nullptr),
    temp(nullptr),
    runtime(rt),
    compartment(nullptr),
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
jit::InitializeIon()
{
    if (!TlsIonContext.initialized() && !TlsIonContext.init())
        return false;
    CheckLogging();
    CheckPerf();
    return true;
}

JitRuntime::JitRuntime()
  : execAlloc_(nullptr),
    ionAlloc_(nullptr),
    exceptionTail_(nullptr),
    bailoutTail_(nullptr),
    enterJIT_(nullptr),
    bailoutHandler_(nullptr),
    argumentsRectifier_(nullptr),
    argumentsRectifierReturnAddr_(nullptr),
    parallelArgumentsRectifier_(nullptr),
    invalidator_(nullptr),
    debugTrapHandler_(nullptr),
    functionWrappers_(nullptr),
    osrTempData_(nullptr),
    flusher_(nullptr),
    ionCodeProtected_(false)
{
}

JitRuntime::~JitRuntime()
{
    js_delete(functionWrappers_);
    freeOsrTempData();

    
    
    js_delete(ionAlloc_);
}

bool
JitRuntime::initialize(JSContext *cx)
{
    JS_ASSERT(cx->runtime()->currentThreadHasExclusiveAccess());
    JS_ASSERT(cx->runtime()->currentThreadOwnsOperationCallbackLock());

    AutoCompartment ac(cx, cx->atomsCompartment());

    IonContext ictx(cx, nullptr);
    AutoFlushCache afc("JitRuntime::initialize", this);

    execAlloc_ = cx->runtime()->getExecAlloc(cx);
    if (!execAlloc_)
        return false;

    if (!cx->compartment()->ensureJitCompartmentExists(cx))
        return false;

    functionWrappers_ = cx->new_<VMWrapperMap>(cx);
    if (!functionWrappers_ || !functionWrappers_->init())
        return false;

    IonSpew(IonSpew_Codegen, "# Emitting exception tail stub");
    exceptionTail_ = generateExceptionTailStub(cx);
    if (!exceptionTail_)
        return false;

    IonSpew(IonSpew_Codegen, "# Emitting bailout tail stub");
    bailoutTail_ = generateBailoutTailStub(cx);
    if (!bailoutTail_)
        return false;

    if (cx->runtime()->jitSupportsFloatingPoint) {
        IonSpew(IonSpew_Codegen, "# Emitting bailout tables");

        
        if (!bailoutTables_.reserve(FrameSizeClass::ClassLimit().classId()))
            return false;

        for (uint32_t id = 0;; id++) {
            FrameSizeClass class_ = FrameSizeClass::FromClass(id);
            if (class_ == FrameSizeClass::ClassLimit())
                break;
            bailoutTables_.infallibleAppend((JitCode *)nullptr);
            bailoutTables_[id] = generateBailoutTable(cx, id);
            if (!bailoutTables_[id])
                return false;
        }

        IonSpew(IonSpew_Codegen, "# Emitting bailout handler");
        bailoutHandler_ = generateBailoutHandler(cx);
        if (!bailoutHandler_)
            return false;

        IonSpew(IonSpew_Codegen, "# Emitting invalidator");
        invalidator_ = generateInvalidator(cx);
        if (!invalidator_)
            return false;
    }

    IonSpew(IonSpew_Codegen, "# Emitting sequential arguments rectifier");
    argumentsRectifier_ = generateArgumentsRectifier(cx, SequentialExecution, &argumentsRectifierReturnAddr_);
    if (!argumentsRectifier_)
        return false;

#ifdef JS_THREADSAFE
    IonSpew(IonSpew_Codegen, "# Emitting parallel arguments rectifier");
    parallelArgumentsRectifier_ = generateArgumentsRectifier(cx, ParallelExecution, nullptr);
    if (!parallelArgumentsRectifier_)
        return false;
#endif

    IonSpew(IonSpew_Codegen, "# Emitting EnterJIT sequence");
    enterJIT_ = generateEnterJIT(cx, EnterJitOptimized);
    if (!enterJIT_)
        return false;

    IonSpew(IonSpew_Codegen, "# Emitting EnterBaselineJIT sequence");
    enterBaselineJIT_ = generateEnterJIT(cx, EnterJitBaseline);
    if (!enterBaselineJIT_)
        return false;

    IonSpew(IonSpew_Codegen, "# Emitting Pre Barrier for Value");
    valuePreBarrier_ = generatePreBarrier(cx, MIRType_Value);
    if (!valuePreBarrier_)
        return false;

    IonSpew(IonSpew_Codegen, "# Emitting Pre Barrier for Shape");
    shapePreBarrier_ = generatePreBarrier(cx, MIRType_Shape);
    if (!shapePreBarrier_)
        return false;

    IonSpew(IonSpew_Codegen, "# Emitting VM function wrappers");
    for (VMFunction *fun = VMFunction::functions; fun; fun = fun->next) {
        if (!generateVMWrapper(cx, *fun))
            return false;
    }

    return true;
}

JitCode *
JitRuntime::debugTrapHandler(JSContext *cx)
{
    if (!debugTrapHandler_) {
        
        
        AutoLockForExclusiveAccess lock(cx);
        AutoCompartment ac(cx, cx->runtime()->atomsCompartment());
        debugTrapHandler_ = generateDebugTrapHandler(cx);
    }
    return debugTrapHandler_;
}

uint8_t *
JitRuntime::allocateOsrTempData(size_t size)
{
    osrTempData_ = (uint8_t *)js_realloc(osrTempData_, size);
    return osrTempData_;
}

void
JitRuntime::freeOsrTempData()
{
    js_free(osrTempData_);
    osrTempData_ = nullptr;
}

JSC::ExecutableAllocator *
JitRuntime::createIonAlloc(JSContext *cx)
{
    JS_ASSERT(cx->runtime()->currentThreadOwnsOperationCallbackLock());

    ionAlloc_ = js_new<JSC::ExecutableAllocator>();
    if (!ionAlloc_)
        js_ReportOutOfMemory(cx);
    return ionAlloc_;
}

void
JitRuntime::ensureIonCodeProtected(JSRuntime *rt)
{
    JS_ASSERT(rt->currentThreadOwnsOperationCallbackLock());

    if (!rt->signalHandlersInstalled() || ionCodeProtected_ || !ionAlloc_)
        return;

    
    
    ionAlloc_->toggleAllCodeAsAccessible(false);
    ionCodeProtected_ = true;
}

bool
JitRuntime::handleAccessViolation(JSRuntime *rt, void *faultingAddress)
{
    if (!rt->signalHandlersInstalled() || !ionAlloc_ || !ionAlloc_->codeContains((char *) faultingAddress))
        return false;

#ifdef JS_THREADSAFE
    
    
    
    
    JS_ASSERT(!rt->currentThreadOwnsOperationCallbackLock());
#endif

    
    
    
    JSRuntime::AutoLockForOperationCallback lock(rt);

    ensureIonCodeAccessible(rt);
    return true;
}

void
JitRuntime::ensureIonCodeAccessible(JSRuntime *rt)
{
    JS_ASSERT(rt->currentThreadOwnsOperationCallbackLock());

    
    
#ifndef XP_MACOSX
    JS_ASSERT(CurrentThreadCanAccessRuntime(rt));
#endif

    if (!ionCodeProtected_)
        return;

    
    
    
    ionAlloc_->toggleAllCodeAsAccessible(true);
    ionCodeProtected_ = false;

    if (rt->interrupt) {
        
        
        
        
        
        
        patchIonBackedges(rt, BackedgeInterruptCheck);
    }
}

void
JitRuntime::patchIonBackedges(JSRuntime *rt, BackedgeTarget target)
{
#ifndef XP_MACOSX
    JS_ASSERT(CurrentThreadCanAccessRuntime(rt));
#endif

    
    
    for (InlineListIterator<PatchableBackedge> iter(backedgeList_.begin());
         iter != backedgeList_.end();
         iter++)
    {
        PatchableBackedge *patchableBackedge = *iter;
        PatchJump(patchableBackedge->backedge, target == BackedgeLoopHeader
                                               ? patchableBackedge->loopHeader
                                               : patchableBackedge->interruptCheck);
    }
}

void
jit::TriggerOperationCallbackForIonCode(JSRuntime *rt,
                                        JSRuntime::OperationCallbackTrigger trigger)
{
    JitRuntime *jitRuntime = rt->jitRuntime();
    if (!jitRuntime)
        return;

    JS_ASSERT(rt->currentThreadOwnsOperationCallbackLock());

    
    
    switch (trigger) {
      case JSRuntime::TriggerCallbackMainThread:
        
        
        
        
        JS_ASSERT(CurrentThreadCanAccessRuntime(rt));
        jitRuntime->ensureIonCodeAccessible(rt);
        break;

      case JSRuntime::TriggerCallbackAnyThread:
        
        
        
        
        
        jitRuntime->ensureIonCodeProtected(rt);
        break;

      case JSRuntime::TriggerCallbackAnyThreadDontStopIon:
        
        
        break;

      default:
        MOZ_ASSUME_UNREACHABLE("Bad trigger");
    }
}

JitCompartment::JitCompartment(JitRuntime *rt)
  : rt(rt),
    stubCodes_(nullptr),
    baselineCallReturnAddr_(nullptr),
    baselineGetPropReturnAddr_(nullptr),
    baselineSetPropReturnAddr_(nullptr),
    stringConcatStub_(nullptr),
    parallelStringConcatStub_(nullptr)
{
}

JitCompartment::~JitCompartment()
{
    js_delete(stubCodes_);
}

bool
JitCompartment::initialize(JSContext *cx)
{
    stubCodes_ = cx->new_<ICStubCodeMap>(cx);
    if (!stubCodes_ || !stubCodes_->init())
        return false;

    return true;
}

bool
JitCompartment::ensureIonStubsExist(JSContext *cx)
{
    if (!stringConcatStub_) {
        stringConcatStub_ = generateStringConcatStub(cx, SequentialExecution);
        if (!stringConcatStub_)
            return false;
    }

#ifdef JS_THREADSAFE
    if (!parallelStringConcatStub_) {
        parallelStringConcatStub_ = generateStringConcatStub(cx, ParallelExecution);
        if (!parallelStringConcatStub_)
            return false;
    }
#endif

    return true;
}

void
jit::FinishOffThreadBuilder(IonBuilder *builder)
{
    builder->script()->runtimeFromMainThread()->removeCompilationThread();

    ExecutionMode executionMode = builder->info().executionMode();

    
    if (builder->script()->hasIonScript())
        builder->script()->ionScript()->clearRecompiling();

    
    if (CompilingOffThread(builder->script(), executionMode))
        SetIonScript(builder->script(), executionMode, nullptr);

    
    
    
    
    js_delete(builder->backgroundCodegen());
    js_delete(builder->alloc().lifoAlloc());
}

static inline void
FinishAllOffThreadCompilations(JitCompartment *ion)
{
    OffThreadCompilationVector &compilations = ion->finishedOffThreadCompilations();

    for (size_t i = 0; i < compilations.length(); i++) {
        IonBuilder *builder = compilations[i];
        FinishOffThreadBuilder(builder);
    }
    compilations.clear();
}

 void
JitRuntime::Mark(JSTracer *trc)
{
    JS_ASSERT(!trc->runtime->isHeapMinorCollecting());
    Zone *zone = trc->runtime->atomsCompartment()->zone();
    for (gc::CellIterUnderGC i(zone, gc::FINALIZE_JITCODE); !i.done(); i.next()) {
        JitCode *code = i.get<JitCode>();
        MarkJitCodeRoot(trc, &code, "wrapper");
    }
}

void
JitCompartment::mark(JSTracer *trc, JSCompartment *compartment)
{
    
    
    
    JS_ASSERT(!trc->runtime->isHeapMinorCollecting());
    CancelOffThreadIonCompile(compartment, nullptr);
    FinishAllOffThreadCompilations(this);

    
    rt->freeOsrTempData();
}

void
JitCompartment::sweep(FreeOp *fop)
{
    stubCodes_->sweep(fop);

    
    if (!stubCodes_->lookup(static_cast<uint32_t>(ICStub::Call_Fallback)))
        baselineCallReturnAddr_ = nullptr;
    
    if (!stubCodes_->lookup(static_cast<uint32_t>(ICStub::GetProp_Fallback)))
        baselineGetPropReturnAddr_ = nullptr;
    if (!stubCodes_->lookup(static_cast<uint32_t>(ICStub::SetProp_Fallback)))
        baselineSetPropReturnAddr_ = nullptr;

    if (stringConcatStub_ && !IsJitCodeMarked(stringConcatStub_.unsafeGet()))
        stringConcatStub_ = nullptr;

    if (parallelStringConcatStub_ && !IsJitCodeMarked(parallelStringConcatStub_.unsafeGet()))
        parallelStringConcatStub_ = nullptr;
}

JitCode *
JitRuntime::getBailoutTable(const FrameSizeClass &frameClass) const
{
    JS_ASSERT(frameClass != FrameSizeClass::None());
    return bailoutTables_[frameClass.classId()];
}

JitCode *
JitRuntime::getVMWrapper(const VMFunction &f) const
{
    JS_ASSERT(functionWrappers_);
    JS_ASSERT(functionWrappers_->initialized());
    JitRuntime::VMWrapperMap::Ptr p = functionWrappers_->readonlyThreadsafeLookup(&f);
    JS_ASSERT(p);

    return p->value();
}

template <AllowGC allowGC>
JitCode *
JitCode::New(JSContext *cx, uint8_t *code, uint32_t bufferSize, JSC::ExecutablePool *pool)
{
    JitCode *codeObj = gc::NewGCThing<JitCode, allowGC>(cx, gc::FINALIZE_JITCODE, sizeof(JitCode), gc::DefaultHeap);
    if (!codeObj) {
        pool->release();
        return nullptr;
    }

    new (codeObj) JitCode(code, bufferSize, pool);
    return codeObj;
}

template
JitCode *
JitCode::New<CanGC>(JSContext *cx, uint8_t *code, uint32_t bufferSize, JSC::ExecutablePool *pool);

template
JitCode *
JitCode::New<NoGC>(JSContext *cx, uint8_t *code, uint32_t bufferSize, JSC::ExecutablePool *pool);

void
JitCode::copyFrom(MacroAssembler &masm)
{
    
    
    *(JitCode **)(code_ - sizeof(JitCode *)) = this;
    insnSize_ = masm.instructionsSize();
    masm.executableCopy(code_);

    jumpRelocTableBytes_ = masm.jumpRelocationTableBytes();
    masm.copyJumpRelocationTable(code_ + jumpRelocTableOffset());

    dataRelocTableBytes_ = masm.dataRelocationTableBytes();
    masm.copyDataRelocationTable(code_ + dataRelocTableOffset());

    preBarrierTableBytes_ = masm.preBarrierTableBytes();
    masm.copyPreBarrierTable(code_ + preBarrierTableOffset());

    masm.processCodeLabels(code_);
}

void
JitCode::trace(JSTracer *trc)
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
JitCode::finalize(FreeOp *fop)
{
    
    
    JS_ASSERT(fop->runtime()->currentThreadOwnsOperationCallbackLock());

#ifdef DEBUG
    
    
    
    if (fop->runtime()->jitRuntime() && !fop->runtime()->jitRuntime()->ionCodeProtected())
        JS_POISON(code_, JS_FREE_PATTERN, bufferSize_);
#endif

    
    
    if (PerfEnabled())
        return;

    
    
    if (pool_)
        pool_->release();
}

void
JitCode::togglePreBarriers(bool enabled)
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

IonScript::IonScript()
  : method_(nullptr),
    deoptTable_(nullptr),
    osrPc_(nullptr),
    osrEntryOffset_(0),
    skipArgCheckEntryOffset_(0),
    invalidateEpilogueOffset_(0),
    invalidateEpilogueDataOffset_(0),
    numBailouts_(0),
    hasUncompiledCallTarget_(false),
    hasSPSInstrumentation_(false),
    recompiling_(false),
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
    callTargetList_(0),
    callTargetEntries_(0),
    backedgeList_(0),
    backedgeEntries_(0),
    refcount_(0),
    recompileInfo_(),
    osrPcMismatchCounter_(0),
    dependentAsmJSModules(nullptr)
{
}

IonScript *
IonScript::New(JSContext *cx, types::RecompileInfo recompileInfo,
               uint32_t frameSlots, uint32_t frameSize, size_t snapshotsSize,
               size_t bailoutEntries, size_t constants, size_t safepointIndices,
               size_t osiIndices, size_t cacheEntries, size_t runtimeSize,
               size_t safepointsSize, size_t callTargetEntries, size_t backedgeEntries,
               OptimizationLevel optimizationLevel)
{
    static const int DataAlignment = sizeof(void *);

    if (snapshotsSize >= MAX_BUFFER_SIZE ||
        (bailoutEntries >= MAX_BUFFER_SIZE / sizeof(uint32_t)))
    {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }

    
    
    
    size_t paddedSnapshotsSize = AlignBytes(snapshotsSize, DataAlignment);
    size_t paddedBailoutSize = AlignBytes(bailoutEntries * sizeof(uint32_t), DataAlignment);
    size_t paddedConstantsSize = AlignBytes(constants * sizeof(Value), DataAlignment);
    size_t paddedSafepointIndicesSize = AlignBytes(safepointIndices * sizeof(SafepointIndex), DataAlignment);
    size_t paddedOsiIndicesSize = AlignBytes(osiIndices * sizeof(OsiIndex), DataAlignment);
    size_t paddedCacheEntriesSize = AlignBytes(cacheEntries * sizeof(uint32_t), DataAlignment);
    size_t paddedRuntimeSize = AlignBytes(runtimeSize, DataAlignment);
    size_t paddedSafepointSize = AlignBytes(safepointsSize, DataAlignment);
    size_t paddedCallTargetSize = AlignBytes(callTargetEntries * sizeof(JSScript *), DataAlignment);
    size_t paddedBackedgeSize = AlignBytes(backedgeEntries * sizeof(PatchableBackedge), DataAlignment);
    size_t bytes = paddedSnapshotsSize +
                   paddedBailoutSize +
                   paddedConstantsSize +
                   paddedSafepointIndicesSize+
                   paddedOsiIndicesSize +
                   paddedCacheEntriesSize +
                   paddedRuntimeSize +
                   paddedSafepointSize +
                   paddedCallTargetSize +
                   paddedBackedgeSize;
    uint8_t *buffer = (uint8_t *)cx->malloc_(sizeof(IonScript) + bytes);
    if (!buffer)
        return nullptr;

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

    script->callTargetList_ = offsetCursor;
    script->callTargetEntries_ = callTargetEntries;
    offsetCursor += paddedCallTargetSize;

    script->backedgeList_ = offsetCursor;
    script->backedgeEntries_ = backedgeEntries;
    offsetCursor += paddedBackedgeSize;

    script->frameSlots_ = frameSlots;
    script->frameSize_ = frameSize;

    script->recompileInfo_ = recompileInfo;
    script->optimizationLevel_ = optimizationLevel;

    return script;
}

void
IonScript::trace(JSTracer *trc)
{
    if (method_)
        MarkJitCode(trc, &method_, "method");

    if (deoptTable_)
        MarkJitCode(trc, &deoptTable_, "deoptimizationTable");

    for (size_t i = 0; i < numConstants(); i++)
        gc::MarkValue(trc, &getConstant(i), "constant");

    
    
    for (size_t i = 0; i < callTargetEntries(); i++)
        gc::MarkScriptUnbarriered(trc, &callTargetList()[i], "callTarget");
}

 void
IonScript::writeBarrierPre(Zone *zone, IonScript *ionScript)
{
#ifdef JSGC_INCREMENTAL
    if (zone->needsBarrier())
        ionScript->trace(zone->barrierTracer());
#endif
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
IonScript::copyConstants(const Value *vp)
{
    for (size_t i = 0; i < constantEntries_; i++)
        constants()[i].init(vp[i]);
}

void
IonScript::copyCallTargetEntries(JSScript **callTargets)
{
    for (size_t i = 0; i < callTargetEntries_; i++)
        callTargetList()[i] = callTargets[i];
}

void
IonScript::copyPatchableBackedges(JSContext *cx, JitCode *code,
                                  PatchableBackedgeInfo *backedges)
{
    for (size_t i = 0; i < backedgeEntries_; i++) {
        const PatchableBackedgeInfo &info = backedges[i];
        PatchableBackedge *patchableBackedge = &backedgeList()[i];

        CodeLocationJump backedge(code, info.backedge);
        CodeLocationLabel loopHeader(code, CodeOffsetLabel(info.loopHeader->offset()));
        CodeLocationLabel interruptCheck(code, CodeOffsetLabel(info.interruptCheck->offset()));
        new(patchableBackedge) PatchableBackedge(backedge, loopHeader, interruptCheck);

        
        
        
        
        PatchJump(backedge, cx->runtime()->interrupt ? interruptCheck : loopHeader);

        cx->runtime()->jitRuntime()->addPatchableBackedge(patchableBackedge);
    }
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
        getCacheFromIndex(i).updateBaseAddress(method_, masm);
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

    MOZ_ASSUME_UNREACHABLE("displacement not found.");
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

    MOZ_ASSUME_UNREACHABLE("Failed to find OSI point return address");
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
    script->destroyCaches();
    script->unlinkFromRuntime(fop);
    fop->free_(script);
}

void
IonScript::toggleBarriers(bool enabled)
{
    method()->togglePreBarriers(enabled);
}

void
IonScript::purgeCaches(Zone *zone)
{
    
    
    
    
    
    if (invalidated())
        return;

    JSRuntime *rt = zone->runtimeFromMainThread();
    IonContext ictx(CompileRuntime::get(rt));
    AutoFlushCache afc("purgeCaches", rt->jitRuntime());
    for (size_t i = 0; i < numCaches(); i++)
        getCacheFromIndex(i).reset();
}

void
IonScript::destroyCaches()
{
    for (size_t i = 0; i < numCaches(); i++)
        getCacheFromIndex(i).destroy();
}

bool
IonScript::addDependentAsmJSModule(JSContext *cx, DependentAsmJSModuleExit exit)
{
    if (!dependentAsmJSModules) {
        dependentAsmJSModules = cx->new_<Vector<DependentAsmJSModuleExit> >(cx);
        if (!dependentAsmJSModules)
            return false;
    }
    return dependentAsmJSModules->append(exit);
}

void
IonScript::unlinkFromRuntime(FreeOp *fop)
{
    
    
    if (dependentAsmJSModules) {
        for (size_t i = 0; i < dependentAsmJSModules->length(); i++) {
            DependentAsmJSModuleExit exit = dependentAsmJSModules->begin()[i];
            exit.module->detachIonCompilation(exit.exitIndex);
        }

        fop->delete_(dependentAsmJSModules);
        dependentAsmJSModules = nullptr;
    }

    
    
    
    JSRuntime *rt = fop->runtime();
    for (size_t i = 0; i < backedgeEntries_; i++) {
        PatchableBackedge *backedge = &backedgeList()[i];
        rt->jitRuntime()->removePatchableBackedge(backedge);
    }

    
    
    
    backedgeEntries_ = 0;
}

void
jit::ToggleBarriers(JS::Zone *zone, bool needs)
{
    JSRuntime *rt = zone->runtimeFromMainThread();
    if (!rt->hasJitRuntime())
        return;

    IonContext ictx(CompileRuntime::get(rt));
    AutoFlushCache afc("ToggleBarriers", rt->jitRuntime());
    for (gc::CellIterUnderGC i(zone, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->hasIonScript())
            script->ionScript()->toggleBarriers(needs);
        if (script->hasBaselineScript())
            script->baselineScript()->toggleBarriers(needs);
    }

    for (CompartmentsInZoneIter comp(zone); !comp.done(); comp.next()) {
        if (comp->jitCompartment())
            comp->jitCompartment()->toggleBaselineStubBarriers(needs);
    }
}

namespace js {
namespace jit {

bool
OptimizeMIR(MIRGenerator *mir)
{
    MIRGraph &graph = mir->graph();

    IonSpewPass("BuildSSA");
    AssertBasicGraphCoherency(graph);

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

    
    
    
    
    Observability observability = graph.hasTryBlock()
                                  ? ConservativeObservability
                                  : AggressiveObservability;
    if (!EliminatePhis(mir, graph, observability))
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

    if (!mir->compilingAsmJS()) {
        if (!ApplyTypeInformation(mir, graph))
            return false;
        IonSpewPass("Apply types");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("Apply types"))
            return false;
    }

    if (graph.entryBlock()->info().executionMode() == ParallelExecution) {
        ParallelSafetyAnalysis analysis(mir, graph);
        if (!analysis.analyze())
            return false;
    }

    
    
    if (mir->optimizationInfo().licmEnabled() ||
        mir->optimizationInfo().gvnEnabled())
    {
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

    if (mir->optimizationInfo().gvnEnabled()) {
        ValueNumberer gvn(mir, graph, mir->optimizationInfo().gvnKind() == GVN_Optimistic);
        if (!gvn.analyze())
            return false;
        IonSpewPass("GVN");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("GVN"))
            return false;
    }

    if (mir->optimizationInfo().uceEnabled()) {
        UnreachableCodeElimination uce(mir, graph);
        if (!uce.analyze())
            return false;
        IonSpewPass("UCE");
        AssertExtendedGraphCoherency(graph);
    }

    if (mir->shouldCancel("UCE"))
        return false;

    if (mir->optimizationInfo().licmEnabled()) {
        
        
        
        JSScript *script = mir->info().script();
        if (!script || !script->hadFrequentBailouts()) {
            LICM licm(mir, graph);
            if (!licm.analyze())
                return false;
            IonSpewPass("LICM");
            AssertExtendedGraphCoherency(graph);

            if (mir->shouldCancel("LICM"))
                return false;
        }
    }

    if (mir->optimizationInfo().rangeAnalysisEnabled()) {
        RangeAnalysis r(mir, graph);
        if (!r.addBetaNodes())
            return false;
        IonSpewPass("Beta");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("RA Beta"))
            return false;

        if (!r.analyze() || !r.addRangeAssertions())
            return false;
        IonSpewPass("Range Analysis");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("Range Analysis"))
            return false;

        if (!r.removeBetaNodes())
            return false;
        IonSpewPass("De-Beta");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("RA De-Beta"))
            return false;

        if (mir->optimizationInfo().uceEnabled()) {
            bool shouldRunUCE = false;
            if (!r.prepareForUCE(&shouldRunUCE))
                return false;
            IonSpewPass("RA check UCE");
            AssertExtendedGraphCoherency(graph);

            if (mir->shouldCancel("RA check UCE"))
                return false;

            if (shouldRunUCE) {
                UnreachableCodeElimination uce(mir, graph);
                uce.disableAliasAnalysis();
                if (!uce.analyze())
                    return false;
                IonSpewPass("UCE After RA");
                AssertExtendedGraphCoherency(graph);

                if (mir->shouldCancel("UCE After RA"))
                    return false;
            }
        }

        if (!r.truncate())
            return false;
        IonSpewPass("Truncate Doubles");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("Truncate Doubles"))
            return false;
    }

    if (mir->optimizationInfo().eaaEnabled()) {
        EffectiveAddressAnalysis eaa(graph);
        if (!eaa.analyze())
            return false;
        IonSpewPass("Effective Address Analysis");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("Effective Address Analysis"))
            return false;
    }

    if (!EliminateDeadCode(mir, graph))
        return false;
    IonSpewPass("DCE");
    AssertExtendedGraphCoherency(graph);

    if (mir->shouldCancel("DCE"))
        return false;

    
    

    if (mir->optimizationInfo().edgeCaseAnalysisEnabled()) {
        EdgeCaseAnalysis edgeCaseAnalysis(mir, graph);
        if (!edgeCaseAnalysis.analyzeLate())
            return false;
        IonSpewPass("Edge Case Analysis (Late)");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("Edge Case Analysis (Late)"))
            return false;
    }

    if (mir->optimizationInfo().eliminateRedundantChecksEnabled()) {
        
        
        
        
        if (!EliminateRedundantChecks(graph))
            return false;
        IonSpewPass("Bounds Check Elimination");
        AssertGraphCoherency(graph);
    }

    return true;
}

LIRGraph *
GenerateLIR(MIRGenerator *mir)
{
    MIRGraph &graph = mir->graph();

    LIRGraph *lir = mir->alloc().lifoAlloc()->new_<LIRGraph>(&graph);
    if (!lir)
        return nullptr;

    LIRGenerator lirgen(mir, graph, *lir);
    if (!lirgen.generate())
        return nullptr;
    IonSpewPass("Generate LIR");

    if (mir->shouldCancel("Generate LIR"))
        return nullptr;

    AllocationIntegrityState integrity(*lir);

    switch (mir->optimizationInfo().registerAllocator()) {
      case RegisterAllocator_LSRA: {
#ifdef DEBUG
        if (!integrity.record())
            return nullptr;
#endif

        LinearScanAllocator regalloc(mir, &lirgen, *lir);
        if (!regalloc.go())
            return nullptr;

#ifdef DEBUG
        if (!integrity.check(false))
            return nullptr;
#endif

        IonSpewPass("Allocate Registers [LSRA]", &regalloc);
        break;
      }

      case RegisterAllocator_Backtracking: {
#ifdef DEBUG
        if (!integrity.record())
            return nullptr;
#endif

        BacktrackingAllocator regalloc(mir, &lirgen, *lir);
        if (!regalloc.go())
            return nullptr;

#ifdef DEBUG
        if (!integrity.check(false))
            return nullptr;
#endif

        IonSpewPass("Allocate Registers [Backtracking]");
        break;
      }

      case RegisterAllocator_Stupid: {
        
        
        if (!integrity.record())
            return nullptr;

        StupidAllocator regalloc(mir, &lirgen, *lir);
        if (!regalloc.go())
            return nullptr;
        if (!integrity.check(true))
            return nullptr;
        IonSpewPass("Allocate Registers [Stupid]");
        break;
      }

      default:
        MOZ_ASSUME_UNREACHABLE("Bad regalloc");
    }

    if (mir->shouldCancel("Allocate Registers"))
        return nullptr;

    
    
    if (!UnsplitEdges(lir))
        return nullptr;
    IonSpewPass("Unsplit Critical Edges");
    AssertBasicGraphCoherency(graph);

    return lir;
}

CodeGenerator *
GenerateCode(MIRGenerator *mir, LIRGraph *lir, MacroAssembler *maybeMasm)
{
    CodeGenerator *codegen = js_new<CodeGenerator>(mir, lir, maybeMasm);
    if (!codegen)
        return nullptr;

    if (mir->compilingAsmJS()) {
        if (!codegen->generateAsmJS()) {
            js_delete(codegen);
            return nullptr;
        }
    } else {
        if (!codegen->generate()) {
            js_delete(codegen);
            return nullptr;
        }
    }

    return codegen;
}

CodeGenerator *
CompileBackEnd(MIRGenerator *mir, MacroAssembler *maybeMasm)
{
    if (!OptimizeMIR(mir))
        return nullptr;

    LIRGraph *lir = GenerateLIR(mir);
    if (!lir)
        return nullptr;

    return GenerateCode(mir, lir, maybeMasm);
}

void
AttachFinishedCompilations(JSContext *cx)
{
#ifdef JS_THREADSAFE
    JitCompartment *ion = cx->compartment()->jitCompartment();
    if (!ion || !cx->runtime()->workerThreadState)
        return;

    types::AutoEnterAnalysis enterTypes(cx);
    AutoLockWorkerThreadState lock(*cx->runtime()->workerThreadState);

    OffThreadCompilationVector &compilations = ion->finishedOffThreadCompilations();

    
    
    while (!compilations.empty()) {
        IonBuilder *builder = compilations.popCopy();

        if (CodeGenerator *codegen = builder->backgroundCodegen()) {
            RootedScript script(cx, builder->script());
            IonContext ictx(cx, &builder->alloc());

            
            
            
            codegen->masm.constructRoot(cx);

            bool success;
            {
                
                AutoTempAllocatorRooter root(cx, &builder->alloc());
                AutoUnlockWorkerThreadState unlock(cx->runtime());
                AutoFlushCache afc("AttachFinishedCompilations", cx->runtime()->jitRuntime());
                success = codegen->link(cx, builder->constraints());
            }

            if (!success) {
                
                
                cx->clearPendingException();
            }
        } else {
            if (builder->abortReason() == AbortReason_Disable)
                SetIonScript(builder->script(), builder->info().executionMode(), ION_DISABLED_SCRIPT);
        }

        FinishOffThreadBuilder(builder);
    }

    compilations.clear();
#endif
}

static const size_t BUILDER_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 1 << 12;

static inline bool
OffThreadCompilationAvailable(JSContext *cx)
{
    
    
    
    
    
    
    
    return cx->runtime()->canUseParallelIonCompilation()
        && cx->runtime()->gcIncrementalState == gc::NO_INCREMENTAL
        && !cx->runtime()->profilingScripts;
}

static void
TrackAllProperties(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->hasSingletonType());

    for (Shape::Range<NoGC> range(obj->lastProperty()); !range.empty(); range.popFront())
        types::EnsureTrackPropertyTypes(cx, obj, range.front().propid());
}

static void
TrackPropertiesForSingletonScopes(JSContext *cx, JSScript *script, BaselineFrame *baselineFrame)
{
    
    
    
    
    JSObject *environment = script->functionNonDelazifying()
                            ? script->functionNonDelazifying()->environment()
                            : nullptr;

    while (environment && !environment->is<GlobalObject>()) {
        if (environment->is<CallObject>() && environment->hasSingletonType())
            TrackAllProperties(cx, environment);
        environment = environment->enclosingScope();
    }

    if (baselineFrame) {
        JSObject *scope = baselineFrame->scopeChain();
        if (scope->is<CallObject>() && scope->hasSingletonType())
            TrackAllProperties(cx, scope);
    }
}

static AbortReason
IonCompile(JSContext *cx, JSScript *script,
           BaselineFrame *baselineFrame, jsbytecode *osrPc, bool constructing,
           ExecutionMode executionMode, bool recompile,
           OptimizationLevel optimizationLevel)
{
#if JS_TRACE_LOGGING
    AutoTraceLog logger(TraceLogging::defaultLogger(),
                        TraceLogging::ION_COMPILE_START,
                        TraceLogging::ION_COMPILE_STOP,
                        script);
#endif
    JS_ASSERT(optimizationLevel > Optimization_DontCompile);

    
    
    script->ensureNonLazyCanonicalFunction(cx);

    TrackPropertiesForSingletonScopes(cx, script, baselineFrame);

    LifoAlloc *alloc = cx->new_<LifoAlloc>(BUILDER_LIFO_ALLOC_PRIMARY_CHUNK_SIZE);
    if (!alloc)
        return AbortReason_Alloc;

    ScopedJSDeletePtr<LifoAlloc> autoDelete(alloc);

    TempAllocator *temp = alloc->new_<TempAllocator>(alloc);
    if (!temp)
        return AbortReason_Alloc;

    IonContext ictx(cx, temp);

    types::AutoEnterAnalysis enter(cx);

    if (!cx->compartment()->ensureJitCompartmentExists(cx))
        return AbortReason_Alloc;

    if (!cx->compartment()->jitCompartment()->ensureIonStubsExist(cx))
        return AbortReason_Alloc;

    MIRGraph *graph = alloc->new_<MIRGraph>(temp);
    if (!graph)
        return AbortReason_Alloc;

    CompileInfo *info = alloc->new_<CompileInfo>(script, script->functionNonDelazifying(), osrPc,
                                                 constructing, executionMode,
                                                 script->needsArgsObj());
    if (!info)
        return AbortReason_Alloc;

    BaselineInspector *inspector = alloc->new_<BaselineInspector>(script);
    if (!inspector)
        return AbortReason_Alloc;

    BaselineFrameInspector *baselineFrameInspector = nullptr;
    if (baselineFrame) {
        baselineFrameInspector = NewBaselineFrameInspector(temp, baselineFrame);
        if (!baselineFrameInspector)
            return AbortReason_Alloc;
    }

    AutoFlushCache afc("IonCompile", cx->runtime()->jitRuntime());

    AutoTempAllocatorRooter root(cx, temp);
    types::CompilerConstraintList *constraints = types::NewCompilerConstraintList(*temp);
    if (!constraints)
        return AbortReason_Alloc;

    const OptimizationInfo *optimizationInfo = js_IonOptimizations.get(optimizationLevel);
    const JitCompileOptions options(cx);

    IonBuilder *builder = alloc->new_<IonBuilder>((JSContext *) nullptr,
                                                  CompileCompartment::get(cx->compartment()),
                                                  options, temp, graph, constraints,
                                                  inspector, info, optimizationInfo,
                                                  baselineFrameInspector);
    if (!builder)
        return AbortReason_Alloc;

    JS_ASSERT(recompile == HasIonScript(builder->script(), executionMode));
    JS_ASSERT(CanIonCompile(builder->script(), executionMode));

    RootedScript builderScript(cx, builder->script());

    if (recompile) {
        JS_ASSERT(executionMode == SequentialExecution);
        builderScript->ionScript()->setRecompiling();
    }

    
    if (OffThreadCompilationAvailable(cx)) {
        if (!recompile)
            SetIonScript(builderScript, executionMode, ION_COMPILING_SCRIPT);

        IonSpew(IonSpew_Logs, "Can't log script %s:%d. (Compiled on background thread.)",
                              builderScript->filename(), builderScript->lineno());

        if (!StartOffThreadIonCompile(cx, builder)) {
            IonSpew(IonSpew_Abort, "Unable to start off-thread ion compilation.");
            return AbortReason_Alloc;
        }

        
        
        autoDelete.forget();

        return AbortReason_NoAbort;
    }

    Maybe<AutoEnterIonCompilation> ionCompiling;
    if (!cx->runtime()->profilingScripts && !IonSpewEnabled(IonSpew_Logs)) {
        
        
        
        
        ionCompiling.construct();
    }

    Maybe<AutoProtectHeapForIonCompilation> protect;
    if (js_JitOptions.checkThreadSafety &&
        cx->runtime()->gcIncrementalState == gc::NO_INCREMENTAL &&
        !cx->runtime()->profilingScripts)
    {
        protect.construct(cx->runtime());
    }

    IonSpewNewFunction(graph, builderScript);

    bool succeeded = builder->build();
    builder->clearForBackEnd();

    if (!succeeded)
        return builder->abortReason();

    ScopedJSDeletePtr<CodeGenerator> codegen(CompileBackEnd(builder));
    if (!codegen) {
        IonSpew(IonSpew_Abort, "Failed during back-end compilation.");
        return AbortReason_Disable;
    }

    if (!protect.empty())
        protect.destroy();
    if (!ionCompiling.empty())
        ionCompiling.destroy();

    bool success = codegen->link(cx, builder->constraints());

    IonSpewEndFunction();

    return success ? AbortReason_NoAbort : AbortReason_Disable;
}

static bool
CheckFrame(BaselineFrame *frame)
{
    JS_ASSERT(!frame->isGeneratorFrame());
    JS_ASSERT(!frame->isDebuggerFrame());

    
    if (frame->isFunctionFrame() && TooManyArguments(frame->numActualArgs())) {
        IonSpew(IonSpew_Abort, "too many actual args");
        return false;
    }

    return true;
}

static bool
CheckScript(JSContext *cx, JSScript *script, bool osr)
{
    if (script->isForEval()) {
        
        
        
        IonSpew(IonSpew_Abort, "eval script");
        return false;
    }

    if (!script->analyzedArgsUsage() && !script->ensureRanAnalysis(cx)) {
        IonSpew(IonSpew_Abort, "OOM under ensureRanAnalysis");
        return false;
    }

    if (!script->compileAndGo()) {
        IonSpew(IonSpew_Abort, "not compile-and-go");
        return false;
    }

    return true;
}

static MethodStatus
CheckScriptSize(JSContext *cx, JSScript* script)
{
    if (!js_JitOptions.limitScriptSize)
        return Method_Compiled;

    if (script->length() > MAX_OFF_THREAD_SCRIPT_SIZE) {
        
        IonSpew(IonSpew_Abort, "Script too large (%u bytes)", script->length());
        return Method_CantCompile;
    }

    uint32_t numLocalsAndArgs = analyze::TotalSlots(script);
    if (cx->runtime()->isWorkerRuntime()) {
        
        
        
        JS_ASSERT(!cx->runtime()->canUseParallelIonCompilation());

        if (script->length() > MAX_DOM_WORKER_SCRIPT_SIZE ||
            numLocalsAndArgs > MAX_DOM_WORKER_LOCALS_AND_ARGS)
        {
            return Method_CantCompile;
        }

        return Method_Compiled;
    }

    if (script->length() > MAX_MAIN_THREAD_SCRIPT_SIZE ||
        numLocalsAndArgs > MAX_MAIN_THREAD_LOCALS_AND_ARGS)
    {
        if (cx->runtime()->canUseParallelIonCompilation()) {
            
            
            
            
            
            if (!OffThreadCompilationAvailable(cx) && !cx->runtime()->profilingScripts) {
                IonSpew(IonSpew_Abort,
                        "Script too large for main thread, skipping (%u bytes) (%u locals/args)",
                        script->length(), numLocalsAndArgs);
                return Method_Skipped;
            }
        } else {
            IonSpew(IonSpew_Abort, "Script too large (%u bytes) (%u locals/args)",
                    script->length(), numLocalsAndArgs);
            return Method_CantCompile;
        }
    }

    return Method_Compiled;
}

bool
CanIonCompileScript(JSContext *cx, HandleScript script, bool osr)
{
    if (!script->canIonCompile() || !CheckScript(cx, script, osr))
        return false;

    return CheckScriptSize(cx, script) == Method_Compiled;
}

static OptimizationLevel
GetOptimizationLevel(HandleScript script, jsbytecode *pc, ExecutionMode executionMode)
{
    if (executionMode == ParallelExecution)
        return Optimization_Normal;

    JS_ASSERT(executionMode == SequentialExecution);

    return js_IonOptimizations.levelForScript(script, pc);
}

static MethodStatus
Compile(JSContext *cx, HandleScript script, BaselineFrame *osrFrame, jsbytecode *osrPc,
        bool constructing, ExecutionMode executionMode)
{
    JS_ASSERT(jit::IsIonEnabled(cx));
    JS_ASSERT(jit::IsBaselineEnabled(cx));
    JS_ASSERT_IF(osrPc != nullptr, (JSOp)*osrPc == JSOP_LOOPENTRY);
    JS_ASSERT_IF(executionMode == ParallelExecution, !osrFrame && !osrPc);
    JS_ASSERT_IF(executionMode == ParallelExecution, !HasIonScript(script, executionMode));

    if (!script->hasBaselineScript())
        return Method_Skipped;

    if (cx->compartment()->debugMode()) {
        IonSpew(IonSpew_Abort, "debugging");
        return Method_CantCompile;
    }

    if (!CheckScript(cx, script, bool(osrPc))) {
        IonSpew(IonSpew_Abort, "Aborted compilation of %s:%d", script->filename(), script->lineno());
        return Method_CantCompile;
    }

    MethodStatus status = CheckScriptSize(cx, script);
    if (status != Method_Compiled) {
        IonSpew(IonSpew_Abort, "Aborted compilation of %s:%d", script->filename(), script->lineno());
        return status;
    }

    bool recompile = false;
    OptimizationLevel optimizationLevel = GetOptimizationLevel(script, osrPc, executionMode);
    if (optimizationLevel == Optimization_DontCompile)
        return Method_Skipped;

    IonScript *scriptIon = GetIonScript(script, executionMode);
    if (scriptIon) {
        if (!scriptIon->method())
            return Method_CantCompile;

        MethodStatus failedState = Method_Compiled;

        
        
        if (osrPc && script->ionScript()->osrPc() != osrPc) {
            uint32_t count = script->ionScript()->incrOsrPcMismatchCounter();
            if (count <= js_JitOptions.osrPcMismatchesBeforeRecompile)
                return Method_Skipped;

            failedState = Method_Skipped;
        }

        
        
        if (optimizationLevel < scriptIon->optimizationLevel())
            return failedState;

        if (optimizationLevel == scriptIon->optimizationLevel() &&
            (!osrPc || script->ionScript()->osrPc() == osrPc))
        {
            return failedState;
        }

        
        if (scriptIon->isRecompiling())
            return failedState;

        if (osrPc)
            script->ionScript()->resetOsrPcMismatchCounter();

        recompile = true;
    }

    AbortReason reason = IonCompile(cx, script, osrFrame, osrPc, constructing, executionMode,
                                    recompile, optimizationLevel);
    if (reason == AbortReason_Error)
        return Method_Error;

    if (reason == AbortReason_Disable)
        return Method_CantCompile;

    if (reason == AbortReason_Alloc) {
        js_ReportOutOfMemory(cx);
        return Method_Error;
    }

    
    if (HasIonScript(script, executionMode)) {
        if (osrPc && script->ionScript()->osrPc() != osrPc)
            return Method_Skipped;
        return Method_Compiled;
    }
    return Method_Skipped;
}

} 
} 



MethodStatus
jit::CanEnterAtBranch(JSContext *cx, JSScript *script, BaselineFrame *osrFrame,
                      jsbytecode *pc, bool isConstructing)
{
    JS_ASSERT(jit::IsIonEnabled(cx));
    JS_ASSERT((JSOp)*pc == JSOP_LOOPENTRY);

    
    if (!script->canIonCompile())
        return Method_Skipped;

    
    if (script->isIonCompilingOffThread())
        return Method_Skipped;

    
    if (script->hasIonScript() && script->ionScript()->bailoutExpected())
        return Method_Skipped;

    
    if (!js_JitOptions.osr)
        return Method_Skipped;

    
    if (!CheckFrame(osrFrame)) {
        ForbidCompilation(cx, script);
        return Method_CantCompile;
    }

    
    
    
    
    
    RootedScript rscript(cx, script);
    MethodStatus status = Compile(cx, rscript, osrFrame, pc, isConstructing, SequentialExecution);
    if (status != Method_Compiled) {
        if (status == Method_CantCompile)
            ForbidCompilation(cx, script);
        return status;
    }

    return Method_Compiled;
}

MethodStatus
jit::CanEnter(JSContext *cx, RunState &state)
{
    JS_ASSERT(jit::IsIonEnabled(cx));

    JSScript *script = state.script();

    
    if (!script->canIonCompile())
        return Method_Skipped;

    
    if (script->isIonCompilingOffThread())
        return Method_Skipped;

    
    if (script->hasIonScript() && script->ionScript()->bailoutExpected())
        return Method_Skipped;

    
    
    
    if (state.isInvoke()) {
        InvokeState &invoke = *state.asInvoke();

        if (TooManyArguments(invoke.args().length())) {
            IonSpew(IonSpew_Abort, "too many actual args");
            ForbidCompilation(cx, script);
            return Method_CantCompile;
        }

        if (TooManyArguments(invoke.args().callee().as<JSFunction>().nargs())) {
            IonSpew(IonSpew_Abort, "too many args");
            ForbidCompilation(cx, script);
            return Method_CantCompile;
        }

        if (invoke.constructing() && invoke.args().thisv().isPrimitive()) {
            RootedScript scriptRoot(cx, script);
            RootedObject callee(cx, &invoke.args().callee());
            RootedObject obj(cx, CreateThisForFunction(cx, callee,
                                                       invoke.useNewType()
                                                       ? SingletonObject
                                                       : GenericObject));
            if (!obj || !jit::IsIonEnabled(cx)) 
                return Method_Skipped;
            invoke.args().setThis(ObjectValue(*obj));
            script = scriptRoot;
        }
    } else if (state.isGenerator()) {
        IonSpew(IonSpew_Abort, "generator frame");
        ForbidCompilation(cx, script);
        return Method_CantCompile;
    }

    
    
    RootedScript rscript(cx, script);
    if (js_JitOptions.eagerCompilation && !rscript->hasBaselineScript()) {
        MethodStatus status = CanEnterBaselineMethod(cx, state);
        if (status != Method_Compiled)
            return status;
    }

    
    bool constructing = state.isInvoke() && state.asInvoke()->constructing();
    MethodStatus status =
        Compile(cx, rscript, nullptr, nullptr, constructing, SequentialExecution);
    if (status != Method_Compiled) {
        if (status == Method_CantCompile)
            ForbidCompilation(cx, rscript);
        return status;
    }

    return Method_Compiled;
}

MethodStatus
jit::CompileFunctionForBaseline(JSContext *cx, HandleScript script, BaselineFrame *frame,
                                bool isConstructing)
{
    JS_ASSERT(jit::IsIonEnabled(cx));
    JS_ASSERT(frame->fun()->nonLazyScript()->canIonCompile());
    JS_ASSERT(!frame->fun()->nonLazyScript()->isIonCompilingOffThread());
    JS_ASSERT(!frame->fun()->nonLazyScript()->hasIonScript());
    JS_ASSERT(frame->isFunctionFrame());

    
    if (!CheckFrame(frame)) {
        ForbidCompilation(cx, script);
        return Method_CantCompile;
    }

    
    MethodStatus status =
        Compile(cx, script, frame, nullptr, isConstructing, SequentialExecution);
    if (status != Method_Compiled) {
        if (status == Method_CantCompile)
            ForbidCompilation(cx, script);
        return status;
    }

    return Method_Compiled;
}

MethodStatus
jit::Recompile(JSContext *cx, HandleScript script, BaselineFrame *osrFrame, jsbytecode *osrPc,
               bool constructing)
{
    JS_ASSERT(script->hasIonScript());
    if (script->ionScript()->isRecompiling())
        return Method_Compiled;

    MethodStatus status =
        Compile(cx, script, osrFrame, osrPc, constructing, SequentialExecution);
    if (status != Method_Compiled) {
        if (status == Method_CantCompile)
            ForbidCompilation(cx, script);
        return status;
    }

    return Method_Compiled;
}

MethodStatus
jit::CanEnterInParallel(JSContext *cx, HandleScript script)
{
    
    
    
    
    
    if (!script->canParallelIonCompile())
        return Method_Skipped;

    
    if (script->isParallelIonCompilingOffThread())
        return Method_Skipped;

    MethodStatus status = Compile(cx, script, nullptr, nullptr, false, ParallelExecution);
    if (status != Method_Compiled) {
        if (status == Method_CantCompile)
            ForbidCompilation(cx, script, ParallelExecution);
        return status;
    }

    
    
    if (!cx->runtime()->jitRuntime()->enterIon())
        return Method_Error;

    
    
    
    
    
    if (!script->hasParallelIonScript()) {
        parallel::Spew(
            parallel::SpewCompile,
            "Script %p:%s:%u was garbage-collected or invalidated",
            script.get(), script->filename(), script->lineno());
        return Method_Skipped;
    }

    return Method_Compiled;
}

MethodStatus
jit::CanEnterUsingFastInvoke(JSContext *cx, HandleScript script, uint32_t numActualArgs)
{
    JS_ASSERT(jit::IsIonEnabled(cx));

    
    if (!script->hasIonScript() || script->ionScript()->bailoutExpected())
        return Method_Skipped;

    
    
    if (numActualArgs < script->functionNonDelazifying()->nargs())
        return Method_Skipped;

    if (!cx->compartment()->ensureJitCompartmentExists(cx))
        return Method_Error;

    
    if (!cx->runtime()->jitRuntime()->enterIon())
        return Method_Error;

    if (!script->hasIonScript())
        return Method_Skipped;

    return Method_Compiled;
}

static IonExecStatus
EnterIon(JSContext *cx, EnterJitData &data)
{
    JS_CHECK_RECURSION(cx, return IonExec_Aborted);
    JS_ASSERT(jit::IsIonEnabled(cx));
    JS_ASSERT(!data.osrFrame);

    EnterJitCode enter = cx->runtime()->jitRuntime()->enterIon();

    
    JS_ASSERT_IF(data.constructing, data.maxArgv[0].isObject());

    data.result.setInt32(data.numActualArgs);
    {
        AssertCompartmentUnchanged pcc(cx);
        IonContext ictx(cx, nullptr);
        JitActivation activation(cx, data.constructing);
        JSAutoResolveFlags rf(cx, RESOLVE_INFER);
        AutoFlushInhibitor afi(cx->runtime()->jitRuntime());

        CALL_GENERATED_CODE(enter, data.jitcode, data.maxArgc, data.maxArgv, nullptr, data.calleeToken,
                             nullptr, 0, data.result.address());
    }

    JS_ASSERT(!cx->runtime()->hasIonReturnOverride());

    
    if (!data.result.isMagic() && data.constructing && data.result.isPrimitive())
        data.result = data.maxArgv[0];

    
    cx->runtime()->getJitRuntime(cx)->freeOsrTempData();

    JS_ASSERT_IF(data.result.isMagic(), data.result.isMagic(JS_ION_ERROR));
    return data.result.isMagic() ? IonExec_Error : IonExec_Ok;
}

bool
jit::SetEnterJitData(JSContext *cx, EnterJitData &data, RunState &state, AutoValueVector &vals)
{
    data.osrFrame = nullptr;

    if (state.isInvoke()) {
        CallArgs &args = state.asInvoke()->args();
        unsigned numFormals = state.script()->functionNonDelazifying()->nargs();
        data.constructing = state.asInvoke()->constructing();
        data.numActualArgs = args.length();
        data.maxArgc = Max(args.length(), numFormals) + 1;
        data.scopeChain = nullptr;
        data.calleeToken = CalleeToToken(&args.callee().as<JSFunction>());

        if (data.numActualArgs >= numFormals) {
            data.maxArgv = args.base() + 1;
        } else {
            
            for (size_t i = 1; i < args.length() + 2; i++) {
                if (!vals.append(args.base()[i]))
                    return false;
            }

            while (vals.length() < numFormals + 1) {
                if (!vals.append(UndefinedValue()))
                    return false;
            }

            JS_ASSERT(vals.length() >= numFormals + 1);
            data.maxArgv = vals.begin();
        }
    } else {
        data.constructing = false;
        data.numActualArgs = 0;
        data.maxArgc = 1;
        data.maxArgv = state.asExecute()->addressOfThisv();
        data.scopeChain = state.asExecute()->scopeChain();

        data.calleeToken = CalleeToToken(state.script());

        if (state.script()->isForEval() &&
            !(state.asExecute()->type() & StackFrame::GLOBAL))
        {
            ScriptFrameIter iter(cx);
            if (iter.isFunctionFrame())
                data.calleeToken = CalleeToToken(iter.callee());
        }
    }

    return true;
}

IonExecStatus
jit::IonCannon(JSContext *cx, RunState &state)
{
    IonScript *ion = state.script()->ionScript();

    EnterJitData data(cx);
    data.jitcode = ion->method()->raw();

    AutoValueVector vals(cx);
    if (!SetEnterJitData(cx, data, state, vals))
        return IonExec_Error;

    IonExecStatus status = EnterIon(cx, data);

    if (status == IonExec_Ok)
        state.setReturnValue(data.result);

    return status;
}

IonExecStatus
jit::FastInvoke(JSContext *cx, HandleFunction fun, CallArgs &args)
{
    JS_CHECK_RECURSION(cx, return IonExec_Error);

    IonScript *ion = fun->nonLazyScript()->ionScript();
    JitCode *code = ion->method();
    void *jitcode = code->raw();

    JS_ASSERT(jit::IsIonEnabled(cx));
    JS_ASSERT(!ion->bailoutExpected());

    JitActivation activation(cx, false);

    EnterJitCode enter = cx->runtime()->jitRuntime()->enterIon();
    void *calleeToken = CalleeToToken(fun);

    RootedValue result(cx, Int32Value(args.length()));
    JS_ASSERT(args.length() >= fun->nargs());

    JSAutoResolveFlags rf(cx, RESOLVE_INFER);

    CALL_GENERATED_CODE(enter, jitcode, args.length() + 1, args.array() - 1, nullptr,
                        calleeToken,  nullptr, 0, result.address());

    JS_ASSERT(!cx->runtime()->hasIonReturnOverride());

    args.rval().set(result);

    JS_ASSERT_IF(result.isMagic(), result.isMagic(JS_ION_ERROR));
    return result.isMagic() ? IonExec_Error : IonExec_Ok;
}

static void
InvalidateActivation(FreeOp *fop, uint8_t *ionTop, bool invalidateAll)
{
    IonSpew(IonSpew_Invalidate, "BEGIN invalidating activation");

    size_t frameno = 1;

    for (IonFrameIterator it(ionTop, SequentialExecution); !it.done(); ++it, ++frameno) {
        JS_ASSERT_IF(frameno == 1, it.type() == IonFrame_Exit);

#ifdef DEBUG
        switch (it.type()) {
          case IonFrame_Exit:
            IonSpew(IonSpew_Invalidate, "#%d exit frame @ %p", frameno, it.fp());
            break;
          case IonFrame_BaselineJS:
          case IonFrame_OptimizedJS:
          {
            JS_ASSERT(it.isScripted());
            const char *type = it.isOptimizedJS() ? "Optimized" : "Baseline";
            IonSpew(IonSpew_Invalidate, "#%d %s JS frame @ %p, %s:%d (fun: %p, script: %p, pc %p)",
                    frameno, type, it.fp(), it.script()->filename(), it.script()->lineno(),
                    it.maybeCallee(), (JSScript *)it.script(), it.returnAddressToFp());
            break;
          }
          case IonFrame_BaselineStub:
            IonSpew(IonSpew_Invalidate, "#%d baseline stub frame @ %p", frameno, it.fp());
            break;
          case IonFrame_Rectifier:
            IonSpew(IonSpew_Invalidate, "#%d rectifier frame @ %p", frameno, it.fp());
            break;
          case IonFrame_Unwound_OptimizedJS:
          case IonFrame_Unwound_BaselineStub:
            MOZ_ASSUME_UNREACHABLE("invalid");
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

        if (!it.isOptimizedJS())
            continue;

        
        if (it.checkInvalidation())
            continue;

        JSScript *script = it.script();
        if (!script->hasIonScript())
            continue;

        if (!invalidateAll && !script->ionScript()->invalidated())
            continue;

        IonScript *ionScript = script->ionScript();

        
        
        
        ionScript->purgeCaches(script->zone());

        
        
        ionScript->unlinkFromRuntime(fop);

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        ionScript->incref();

        const SafepointIndex *si = ionScript->getSafepointIndex(it.returnAddressToFp());
        JitCode *ionCode = ionScript->method();

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
jit::StopAllOffThreadCompilations(JSCompartment *comp)
{
    if (!comp->jitCompartment())
        return;
    CancelOffThreadIonCompile(comp, nullptr);
    FinishAllOffThreadCompilations(comp->jitCompartment());
}

void
jit::InvalidateAll(FreeOp *fop, Zone *zone)
{
    for (CompartmentsInZoneIter comp(zone); !comp.done(); comp.next())
        StopAllOffThreadCompilations(comp);

    for (JitActivationIterator iter(fop->runtime()); !iter.done(); ++iter) {
        if (iter.activation()->compartment()->zone() == zone) {
            IonContext ictx(CompileRuntime::get(fop->runtime()));
            AutoFlushCache afc("InvalidateAll", fop->runtime()->jitRuntime());
            IonSpew(IonSpew_Invalidate, "Invalidating all frames for GC");
            InvalidateActivation(fop, iter.jitTop(), true);
        }
    }
}


void
jit::Invalidate(types::TypeZone &types, FreeOp *fop,
                const Vector<types::RecompileInfo> &invalid, bool resetUses,
                bool cancelOffThread)
{
    IonSpew(IonSpew_Invalidate, "Start invalidation.");
    AutoFlushCache afc ("Invalidate", fop->runtime()->jitRuntime());

    
    
    size_t numInvalidations = 0;
    for (size_t i = 0; i < invalid.length(); i++) {
        const types::CompilerOutput &co = *invalid[i].compilerOutput(types);
        if (!co.isValid())
            continue;

        if (cancelOffThread)
            CancelOffThreadIonCompile(co.script()->compartment(), co.script());

        if (!co.ion())
            continue;

        IonSpew(IonSpew_Invalidate, " Invalidate %s:%u, IonScript %p",
                co.script()->filename(), co.script()->lineno(), co.ion());

        
        
        
        co.ion()->incref();
        numInvalidations++;
    }

    if (!numInvalidations) {
        IonSpew(IonSpew_Invalidate, " No IonScript invalidation.");
        return;
    }

    for (JitActivationIterator iter(fop->runtime()); !iter.done(); ++iter)
        InvalidateActivation(fop, iter.jitTop(), false);

    
    
    
    for (size_t i = 0; i < invalid.length(); i++) {
        types::CompilerOutput &co = *invalid[i].compilerOutput(types);
        if (!co.isValid())
            continue;

        ExecutionMode executionMode = co.mode();
        JSScript *script = co.script();
        IonScript *ionScript = co.ion();
        if (!ionScript)
            continue;

        SetIonScript(script, executionMode, nullptr);
        ionScript->decref(fop);
        co.invalidate();
        numInvalidations--;

        
        
        
        
        
        
        
        
        
        
        
        if (resetUses && executionMode != ParallelExecution)
            script->resetUseCount();
    }

    
    
    JS_ASSERT(!numInvalidations);
}

void
jit::Invalidate(JSContext *cx, const Vector<types::RecompileInfo> &invalid, bool resetUses,
                bool cancelOffThread)
{
    jit::Invalidate(cx->zone()->types, cx->runtime()->defaultFreeOp(), invalid, resetUses,
                    cancelOffThread);
}

bool
jit::Invalidate(JSContext *cx, JSScript *script, ExecutionMode mode, bool resetUses,
                bool cancelOffThread)
{
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
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }

    Invalidate(cx, scripts, resetUses, cancelOffThread);
    return true;
}

bool
jit::Invalidate(JSContext *cx, JSScript *script, bool resetUses, bool cancelOffThread)
{
    return Invalidate(cx, script, SequentialExecution, resetUses, cancelOffThread);
}

static void
FinishInvalidationOf(FreeOp *fop, JSScript *script, IonScript *ionScript, bool parallel)
{
    
    
    if (parallel)
        script->setParallelIonScript(nullptr);
    else
        script->setIonScript(nullptr);

    types::TypeZone &types = script->zone()->types;
    ionScript->recompileInfo().compilerOutput(types)->invalidate();

    
    
    if (!ionScript->invalidated())
        jit::IonScript::Destroy(fop, ionScript);
}

void
jit::FinishInvalidation(FreeOp *fop, JSScript *script)
{
    if (script->hasIonScript())
        FinishInvalidationOf(fop, script, script->ionScript(), false);

    if (script->hasParallelIonScript())
        FinishInvalidationOf(fop, script, script->parallelIonScript(), true);
}

void
jit::FinishDiscardJitCode(FreeOp *fop, JSCompartment *comp)
{
    
    if (comp->jitCompartment())
        comp->jitCompartment()->optimizedStubSpace()->free();
}

void
jit::MarkValueFromIon(JSRuntime *rt, Value *vp)
{
    gc::MarkValueUnbarriered(&rt->gcMarker, vp, "write barrier");
}

void
jit::MarkShapeFromIon(JSRuntime *rt, Shape **shapep)
{
    gc::MarkShapeUnbarriered(&rt->gcMarker, shapep, "write barrier");
}

void
jit::ForbidCompilation(JSContext *cx, JSScript *script)
{
    ForbidCompilation(cx, script, SequentialExecution);
}

void
jit::ForbidCompilation(JSContext *cx, JSScript *script, ExecutionMode mode)
{
    IonSpew(IonSpew_Abort, "Disabling Ion mode %d compilation of script %s:%d",
            mode, script->filename(), script->lineno());

    CancelOffThreadIonCompile(cx->compartment(), script);

    switch (mode) {
      case SequentialExecution:
        if (script->hasIonScript()) {
            
            
            
            
            
            if (!Invalidate(cx, script, mode, false))
                return;
        }

        script->setIonScript(ION_DISABLED_SCRIPT);
        return;

      case ParallelExecution:
        if (script->hasParallelIonScript()) {
            if (!Invalidate(cx, script, mode, false))
                return;
        }

        script->setParallelIonScript(ION_DISABLED_SCRIPT);
        return;

      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }

    MOZ_ASSUME_UNREACHABLE("No such execution mode");
}

void
AutoFlushCache::updateTop(uintptr_t p, size_t len)
{
    IonContext *ictx = MaybeGetIonContext();
    JitRuntime *jrt = (ictx != nullptr) ? const_cast<JitRuntime *>(ictx->runtime->jitRuntime()) : nullptr;
    if (!jrt || !jrt->flusher())
        JSC::ExecutableAllocator::cacheFlush((void*)p, len);
    else
        jrt->flusher()->update(p, len);
}

AutoFlushCache::AutoFlushCache(const char *nonce, JitRuntime *rt)
  : start_(0),
    stop_(0),
    name_(nonce),
    runtime_(rt),
    used_(false)
{
    if (rt->flusher())
        IonSpew(IonSpew_CacheFlush, "<%s ", nonce);
    else
        IonSpewCont(IonSpew_CacheFlush, "<%s ", nonce);

    rt->setFlusher(this);
}

AutoFlushInhibitor::AutoFlushInhibitor(JitRuntime *rt)
  : runtime_(rt),
    afc(nullptr)
{
    afc = rt->flusher();

    
    rt->setFlusher(nullptr);

    
    if (afc) {
        afc->flushAnyway();
        IonSpewCont(IonSpew_CacheFlush, "}");
    }
}
AutoFlushInhibitor::~AutoFlushInhibitor()
{
    JS_ASSERT(runtime_->flusher() == nullptr);

    
    runtime_->setFlusher(afc);

    if (afc)
        IonSpewCont(IonSpew_CacheFlush, "{");
}

void
jit::PurgeCaches(JSScript *script, Zone *zone)
{
    if (script->hasIonScript())
        script->ionScript()->purgeCaches(zone);

    if (script->hasParallelIonScript())
        script->parallelIonScript()->purgeCaches(zone);
}

size_t
jit::SizeOfIonData(JSScript *script, mozilla::MallocSizeOf mallocSizeOf)
{
    size_t result = 0;

    if (script->hasIonScript())
        result += script->ionScript()->sizeOfIncludingThis(mallocSizeOf);

    if (script->hasParallelIonScript())
        result += script->parallelIonScript()->sizeOfIncludingThis(mallocSizeOf);

    return result;
}

void
jit::DestroyIonScripts(FreeOp *fop, JSScript *script)
{
    if (script->hasIonScript())
        jit::IonScript::Destroy(fop, script->ionScript());

    if (script->hasParallelIonScript())
        jit::IonScript::Destroy(fop, script->parallelIonScript());

    if (script->hasBaselineScript())
        jit::BaselineScript::Destroy(fop, script->baselineScript());
}

void
jit::TraceIonScripts(JSTracer* trc, JSScript *script)
{
    if (script->hasIonScript())
        jit::IonScript::Trace(trc, script->ionScript());

    if (script->hasParallelIonScript())
        jit::IonScript::Trace(trc, script->parallelIonScript());

    if (script->hasBaselineScript())
        jit::BaselineScript::Trace(trc, script->baselineScript());
}

AutoDebugModeInvalidation::~AutoDebugModeInvalidation()
{
    MOZ_ASSERT(!!comp_ != !!zone_);

    if (needInvalidation_ == NoNeed)
        return;

    
    
    bool invalidateStack = needInvalidation_ == ToggledOff;
    Zone *zone = zone_ ? zone_ : comp_->zone();
    JSRuntime *rt = zone->runtimeFromMainThread();
    FreeOp *fop = rt->defaultFreeOp();

    if (comp_) {
        StopAllOffThreadCompilations(comp_);
    } else {
        for (CompartmentsInZoneIter comp(zone_); !comp.done(); comp.next())
            StopAllOffThreadCompilations(comp);
    }

    if (invalidateStack) {
        jit::MarkActiveBaselineScripts(zone);

        for (JitActivationIterator iter(rt); !iter.done(); ++iter) {
            JSCompartment *comp = iter.activation()->compartment();
            if ((comp_ && comp_ == comp) ||
                (zone_ && zone_ == comp->zone() && comp->principals))
            {
                IonContext ictx(CompileRuntime::get(rt));
                AutoFlushCache afc("AutoDebugModeInvalidation", rt->jitRuntime());
                IonSpew(IonSpew_Invalidate, "Invalidating frames for debug mode toggle");
                InvalidateActivation(fop, iter.jitTop(), true);
            }
        }
    }

    for (gc::CellIter i(zone, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if ((comp_ && script->compartment() == comp_) ||
            (zone_ && script->compartment()->principals))
        {
            FinishInvalidation(fop, script);
            FinishDiscardBaselineScript(fop, script);
            
            script->resetUseCount();
        } else if (script->hasBaselineScript()) {
            script->baselineScript()->resetActive();
        }
    }

    if (comp_) {
        FinishDiscardJitCode(fop, comp_);
    } else {
        for (CompartmentsInZoneIter comp(zone_); !comp.done(); comp.next())
            FinishDiscardJitCode(fop, comp);
    }
}
