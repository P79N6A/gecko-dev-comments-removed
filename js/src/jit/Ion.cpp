





#include "jit/Ion.h"

#include "mozilla/MemoryReporting.h"
#include "mozilla/ThreadLocal.h"

#include "jscompartment.h"
#include "jsprf.h"

#include "asmjs/AsmJSModule.h"
#include "gc/Marking.h"
#include "jit/AliasAnalysis.h"
#include "jit/BacktrackingAllocator.h"
#include "jit/BaselineDebugModeOSR.h"
#include "jit/BaselineFrame.h"
#include "jit/BaselineInspector.h"
#include "jit/BaselineJIT.h"
#include "jit/CodeGenerator.h"
#include "jit/EdgeCaseAnalysis.h"
#include "jit/EffectiveAddressAnalysis.h"
#include "jit/IonAnalysis.h"
#include "jit/IonBuilder.h"
#include "jit/IonOptimizationLevels.h"
#include "jit/JitcodeMap.h"
#include "jit/JitCommon.h"
#include "jit/JitCompartment.h"
#include "jit/JitSpewer.h"
#include "jit/LICM.h"
#include "jit/LinearScan.h"
#include "jit/LIR.h"
#include "jit/LoopUnroller.h"
#include "jit/Lowering.h"
#include "jit/ParallelSafetyAnalysis.h"
#include "jit/PerfSpewer.h"
#include "jit/RangeAnalysis.h"
#include "jit/ScalarReplacement.h"
#include "jit/StupidAllocator.h"
#include "jit/UnreachableCodeElimination.h"
#include "jit/ValueNumbering.h"
#include "vm/ForkJoin.h"
#include "vm/HelperThreads.h"
#include "vm/TraceLogging.h"

#include "jscompartmentinlines.h"
#include "jsgcinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"

#include "jit/ExecutionMode-inl.h"

using namespace js;
using namespace js::jit;

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
#if defined(JS_CODEGEN_ARM)
    InitARMFlags();
#endif
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
    forkJoinGetSliceStub_(nullptr),
    baselineDebugModeOSRHandler_(nullptr),
    functionWrappers_(nullptr),
    osrTempData_(nullptr),
    ionCodeProtected_(false),
    ionReturnOverride_(MagicValue(JS_ARG_POISON)),
    jitcodeGlobalTable_(nullptr)
{
}

JitRuntime::~JitRuntime()
{
    js_delete(functionWrappers_);
    freeOsrTempData();

    
    
    js_delete(ionAlloc_);

    
    JS_ASSERT_IF(jitcodeGlobalTable_, jitcodeGlobalTable_->empty());
    js_delete(jitcodeGlobalTable_);
}

bool
JitRuntime::initialize(JSContext *cx)
{
    JS_ASSERT(cx->runtime()->currentThreadHasExclusiveAccess());
    JS_ASSERT(cx->runtime()->currentThreadOwnsInterruptLock());

    AutoCompartment ac(cx, cx->atomsCompartment());

    IonContext ictx(cx, nullptr);

    execAlloc_ = cx->runtime()->getExecAlloc(cx);
    if (!execAlloc_)
        return false;

    if (!cx->compartment()->ensureJitCompartmentExists(cx))
        return false;

    functionWrappers_ = cx->new_<VMWrapperMap>(cx);
    if (!functionWrappers_ || !functionWrappers_->init())
        return false;

    JitSpew(JitSpew_Codegen, "# Emitting exception tail stub");
    exceptionTail_ = generateExceptionTailStub(cx);
    if (!exceptionTail_)
        return false;

    JitSpew(JitSpew_Codegen, "# Emitting bailout tail stub");
    bailoutTail_ = generateBailoutTailStub(cx);
    if (!bailoutTail_)
        return false;

    if (cx->runtime()->jitSupportsFloatingPoint) {
        JitSpew(JitSpew_Codegen, "# Emitting bailout tables");

        
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

        JitSpew(JitSpew_Codegen, "# Emitting bailout handler");
        bailoutHandler_ = generateBailoutHandler(cx, SequentialExecution);
        if (!bailoutHandler_)
            return false;

        JitSpew(JitSpew_Codegen, "# Emitting parallel bailout handler");
        parallelBailoutHandler_ = generateBailoutHandler(cx, ParallelExecution);
        if (!parallelBailoutHandler_)
            return false;

        JitSpew(JitSpew_Codegen, "# Emitting invalidator");
        invalidator_ = generateInvalidator(cx);
        if (!invalidator_)
            return false;
    }

    JitSpew(JitSpew_Codegen, "# Emitting sequential arguments rectifier");
    argumentsRectifier_ = generateArgumentsRectifier(cx, SequentialExecution, &argumentsRectifierReturnAddr_);
    if (!argumentsRectifier_)
        return false;

    JitSpew(JitSpew_Codegen, "# Emitting parallel arguments rectifier");
    parallelArgumentsRectifier_ = generateArgumentsRectifier(cx, ParallelExecution, nullptr);
    if (!parallelArgumentsRectifier_)
        return false;

    JitSpew(JitSpew_Codegen, "# Emitting EnterJIT sequence");
    enterJIT_ = generateEnterJIT(cx, EnterJitOptimized);
    if (!enterJIT_)
        return false;

    JitSpew(JitSpew_Codegen, "# Emitting EnterBaselineJIT sequence");
    enterBaselineJIT_ = generateEnterJIT(cx, EnterJitBaseline);
    if (!enterBaselineJIT_)
        return false;

    JitSpew(JitSpew_Codegen, "# Emitting Pre Barrier for Value");
    valuePreBarrier_ = generatePreBarrier(cx, MIRType_Value);
    if (!valuePreBarrier_)
        return false;

    JitSpew(JitSpew_Codegen, "# Emitting Pre Barrier for Shape");
    shapePreBarrier_ = generatePreBarrier(cx, MIRType_Shape);
    if (!shapePreBarrier_)
        return false;

    JitSpew(JitSpew_Codegen, "# Emitting Pre Barrier for TypeObject");
    typeObjectPreBarrier_ = generatePreBarrier(cx, MIRType_TypeObject);
    if (!typeObjectPreBarrier_)
        return false;

    JitSpew(JitSpew_Codegen, "# Emitting malloc stub");
    mallocStub_ = generateMallocStub(cx);
    if (!mallocStub_)
        return false;

    JitSpew(JitSpew_Codegen, "# Emitting free stub");
    freeStub_ = generateFreeStub(cx);
    if (!freeStub_)
        return false;

    JitSpew(JitSpew_Codegen, "# Emitting VM function wrappers");
    for (VMFunction *fun = VMFunction::functions; fun; fun = fun->next) {
        if (!generateVMWrapper(cx, *fun))
            return false;
    }

    JitSpew(JitSpew_Codegen, "# Emitting lazy link stub");
    lazyLinkStub_ = generateLazyLinkStub(cx);
    if (!lazyLinkStub_)
        return false;

    jitcodeGlobalTable_ = cx->new_<JitcodeGlobalTable>();
    if (!jitcodeGlobalTable_)
        return false;

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

bool
JitRuntime::ensureForkJoinGetSliceStubExists(JSContext *cx)
{
    if (!forkJoinGetSliceStub_) {
        JitSpew(JitSpew_Codegen, "# Emitting ForkJoinGetSlice stub");
        AutoLockForExclusiveAccess lock(cx);
        AutoCompartment ac(cx, cx->runtime()->atomsCompartment());
        forkJoinGetSliceStub_ = generateForkJoinGetSliceStub(cx);
    }
    return !!forkJoinGetSliceStub_;
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

ExecutableAllocator *
JitRuntime::createIonAlloc(JSContext *cx)
{
    JS_ASSERT(cx->runtime()->currentThreadOwnsInterruptLock());

    ionAlloc_ = js_new<ExecutableAllocator>();
    if (!ionAlloc_)
        js_ReportOutOfMemory(cx);
    return ionAlloc_;
}

void
JitRuntime::ensureIonCodeProtected(JSRuntime *rt)
{
    JS_ASSERT(rt->currentThreadOwnsInterruptLock());

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

    
    
    
    
    JS_ASSERT(!rt->currentThreadOwnsInterruptLock());

    
    
    
    JSRuntime::AutoLockForInterrupt lock(rt);

    
    
    
    ensureIonCodeAccessible(rt);
    return true;
}

void
JitRuntime::ensureIonCodeAccessible(JSRuntime *rt)
{
    JS_ASSERT(rt->currentThreadOwnsInterruptLock());

    
    
#ifndef XP_MACOSX
    JS_ASSERT(CurrentThreadCanAccessRuntime(rt));
#endif

    if (ionCodeProtected_) {
        ionAlloc_->toggleAllCodeAsAccessible(true);
        ionCodeProtected_ = false;
    }

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
        if (target == BackedgeLoopHeader)
            PatchBackedge(patchableBackedge->backedge, patchableBackedge->loopHeader, target);
        else
            PatchBackedge(patchableBackedge->backedge, patchableBackedge->interruptCheck, target);
    }
}

void
jit::RequestInterruptForIonCode(JSRuntime *rt, JSRuntime::InterruptMode mode)
{
    JitRuntime *jitRuntime = rt->jitRuntime();
    if (!jitRuntime)
        return;

    JS_ASSERT(rt->currentThreadOwnsInterruptLock());

    
    
    switch (mode) {
      case JSRuntime::RequestInterruptMainThread:
        
        
        
        
        JS_ASSERT(CurrentThreadCanAccessRuntime(rt));
        jitRuntime->ensureIonCodeAccessible(rt);
        break;

      case JSRuntime::RequestInterruptAnyThread:
        
        
        
        
        
        jitRuntime->ensureIonCodeProtected(rt);
        break;

      case JSRuntime::RequestInterruptAnyThreadDontStopIon:
      case JSRuntime::RequestInterruptAnyThreadForkJoin:
        
        
        break;

      default:
        MOZ_CRASH("Bad interrupt mode");
    }
}

JitCompartment::JitCompartment()
  : stubCodes_(nullptr),
    baselineCallReturnAddr_(nullptr),
    baselineGetPropReturnAddr_(nullptr),
    baselineSetPropReturnAddr_(nullptr),
    stringConcatStub_(nullptr),
    parallelStringConcatStub_(nullptr),
    activeParallelEntryScripts_(nullptr)
{
}

JitCompartment::~JitCompartment()
{
    js_delete(stubCodes_);
    js_delete(activeParallelEntryScripts_);
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

    if (!parallelStringConcatStub_) {
        parallelStringConcatStub_ = generateStringConcatStub(cx, ParallelExecution);
        if (!parallelStringConcatStub_)
            return false;
    }

    return true;
}

bool
JitCompartment::notifyOfActiveParallelEntryScript(JSContext *cx, HandleScript script)
{
    
    
    if (script->parallelIonScript()->isParallelEntryScript()) {
        MOZ_ASSERT(activeParallelEntryScripts_ && activeParallelEntryScripts_->has(script));
        script->parallelIonScript()->resetParallelAge();
        return true;
    }

    if (!activeParallelEntryScripts_) {
        activeParallelEntryScripts_ = cx->new_<ScriptSet>(cx);
        if (!activeParallelEntryScripts_ || !activeParallelEntryScripts_->init())
            return false;
    }

    script->parallelIonScript()->setIsParallelEntryScript();
    ScriptSet::AddPtr p = activeParallelEntryScripts_->lookupForAdd(script);
    return p || activeParallelEntryScripts_->add(p, script);
}

bool
JitCompartment::hasRecentParallelActivity() const
{
    return activeParallelEntryScripts_ && !activeParallelEntryScripts_->empty();
}

void
jit::FinishOffThreadBuilder(JSContext *cx, IonBuilder *builder)
{
    ExecutionMode executionMode = builder->info().executionMode();

    
    if (builder->script()->hasIonScript() && builder->script()->pendingIonBuilder() == builder)
        builder->script()->setPendingIonBuilder(cx, nullptr);
    if (builder->isInList())
        builder->remove();

    
    
    if (executionMode == SequentialExecution && builder->script()->hasIonScript())
        builder->script()->ionScript()->clearRecompiling();

    
    if (CompilingOffThread(builder->script(), executionMode)) {
        SetIonScript(cx, builder->script(), executionMode,
                     builder->abortReason() == AbortReason_Disable
                     ? ION_DISABLED_SCRIPT
                     : nullptr);
    }

    
    
    
    
    js_delete(builder->backgroundCodegen());
    js_delete(builder->alloc().lifoAlloc());
}

static inline void
FinishAllOffThreadCompilations(JSCompartment *comp)
{
    AutoLockHelperThreadState lock;
    GlobalHelperThreadState::IonBuilderVector &finished = HelperThreadState().ionFinishedList();

    for (size_t i = 0; i < finished.length(); i++) {
        IonBuilder *builder = finished[i];
        if (builder->compartment == CompileCompartment::get(comp)) {
            FinishOffThreadBuilder(nullptr, builder);
            HelperThreadState().remove(finished, &i);
        }
    }
}

uint8_t *
jit::LazyLinkTopActivation(JSContext *cx)
{
    JitActivationIterator iter(cx->runtime());

    
    JitFrameIterator it(iter.jitTop(), SequentialExecution);
    MOZ_ASSERT(it.type() == JitFrame_Exit);

    
    ++it;
    MOZ_ASSERT(it.type() == JitFrame_IonJS);

    
    IonBuilder *builder = it.script()->ionScript()->pendingBuilder();
    it.script()->setPendingIonBuilder(cx, nullptr);

    types::AutoEnterAnalysis enterTypes(cx);
    RootedScript script(cx, builder->script());

    
    builder->remove();

    if (CodeGenerator *codegen = builder->backgroundCodegen()) {
        js::TraceLogger *logger = TraceLoggerForMainThread(cx->runtime());
        AutoTraceLog logScript(logger, TraceLogCreateTextId(logger, script));
        AutoTraceLog logLink(logger, TraceLogger::IonLinking);

        IonContext ictx(cx, &builder->alloc());

        
        
        
        codegen->masm.constructRoot(cx);

        if (!codegen->link(cx, builder->constraints())) {
            
            
            
            cx->clearPendingException();
        }
    }

    FinishOffThreadBuilder(cx, builder);

    MOZ_ASSERT(script->hasBaselineScript());
    MOZ_ASSERT(script->baselineOrIonRawPointer());

    return script->baselineOrIonRawPointer();
}
 void
JitRuntime::Mark(JSTracer *trc)
{
    JS_ASSERT(!trc->runtime()->isHeapMinorCollecting());
    Zone *zone = trc->runtime()->atomsCompartment()->zone();
    for (gc::ZoneCellIterUnderGC i(zone, gc::FINALIZE_JITCODE); !i.done(); i.next()) {
        JitCode *code = i.get<JitCode>();
        MarkJitCodeRoot(trc, &code, "wrapper");
    }
}

void
JitCompartment::mark(JSTracer *trc, JSCompartment *compartment)
{
    
    trc->runtime()->jitRuntime()->freeOsrTempData();

    
    if (activeParallelEntryScripts_) {
        for (ScriptSet::Enum e(*activeParallelEntryScripts_); !e.empty(); e.popFront()) {
            JSScript *script = e.front();

            
            
            
            if (!script->hasParallelIonScript() ||
                !script->parallelIonScript()->isParallelEntryScript() ||
                trc->runtime()->gc.shouldCleanUpEverything())
            {
                e.removeFront();
                continue;
            }

            
            
            
            
            
            
            if (script->parallelIonScript()->shouldPreserveParallelCode(IonScript::IncreaseAge)) {
                MarkScript(trc, const_cast<PreBarrieredScript *>(&e.front()), "par-script");
                MOZ_ASSERT(script == e.front());
            } else {
                script->parallelIonScript()->clearIsParallelEntryScript();
                e.removeFront();
            }
        }
    }
}

void
JitCompartment::sweep(FreeOp *fop, JSCompartment *compartment)
{
    
    
    
    JS_ASSERT(!fop->runtime()->isHeapMinorCollecting());
    CancelOffThreadIonCompile(compartment, nullptr);
    FinishAllOffThreadCompilations(compartment);

    stubCodes_->sweep(fop);

    
    if (!stubCodes_->lookup(static_cast<uint32_t>(ICStub::Call_Fallback)))
        baselineCallReturnAddr_ = nullptr;
    
    if (!stubCodes_->lookup(static_cast<uint32_t>(ICStub::GetProp_Fallback)))
        baselineGetPropReturnAddr_ = nullptr;
    if (!stubCodes_->lookup(static_cast<uint32_t>(ICStub::SetProp_Fallback)))
        baselineSetPropReturnAddr_ = nullptr;

    if (stringConcatStub_ && !IsJitCodeMarked(&stringConcatStub_))
        stringConcatStub_ = nullptr;

    if (parallelStringConcatStub_ && !IsJitCodeMarked(&parallelStringConcatStub_))
        parallelStringConcatStub_ = nullptr;

    if (activeParallelEntryScripts_) {
        for (ScriptSet::Enum e(*activeParallelEntryScripts_); !e.empty(); e.popFront()) {
            JSScript *script = e.front();
            if (!IsScriptMarked(&script))
                e.removeFront();
            else
                MOZ_ASSERT(script == e.front());
        }
    }
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
JitCode::New(JSContext *cx, uint8_t *code, uint32_t bufferSize, uint32_t headerSize,
             ExecutablePool *pool, CodeKind kind)
{
    JitCode *codeObj = js::NewJitCode<allowGC>(cx);
    if (!codeObj) {
        pool->release(headerSize + bufferSize, kind);
        return nullptr;
    }

    new (codeObj) JitCode(code, bufferSize, headerSize, pool, kind);
    return codeObj;
}

template
JitCode *
JitCode::New<CanGC>(JSContext *cx, uint8_t *code, uint32_t bufferSize, uint32_t headerSize,
                    ExecutablePool *pool, CodeKind kind);

template
JitCode *
JitCode::New<NoGC>(JSContext *cx, uint8_t *code, uint32_t bufferSize, uint32_t headerSize,
                   ExecutablePool *pool, CodeKind kind);

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
    
    
    JS_ASSERT(fop->runtime()->currentThreadOwnsInterruptLock());

    
    if (hasBytecodeMap_) {
        JS_ASSERT(fop->runtime()->jitRuntime()->hasJitcodeGlobalTable());
        fop->runtime()->jitRuntime()->getJitcodeGlobalTable()->removeEntry(raw());
    }

    
    
    
    if (fop->runtime()->jitRuntime() && !fop->runtime()->jitRuntime()->ionCodeProtected())
        memset(code_, JS_SWEPT_CODE_PATTERN, bufferSize_);
    code_ = nullptr;

    
    
    if (pool_) {
        
        
        if (!PerfEnabled())
            pool_->release(headerSize_ + bufferSize_, CodeKind(kind_));
        pool_ = nullptr;
    }
}

void
JitCode::togglePreBarriers(bool enabled)
{
    uint8_t *start = code_ + preBarrierTableOffset();
    CompactBufferReader reader(start, start + preBarrierTableBytes_);

    while (reader.more()) {
        size_t offset = reader.readUnsigned();
        CodeLocationLabel loc(this, CodeOffsetLabel(offset));
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
    isParallelEntryScript_(false),
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
    snapshotsListSize_(0),
    snapshotsRVATableSize_(0),
    constantTable_(0),
    constantEntries_(0),
    callTargetList_(0),
    callTargetEntries_(0),
    backedgeList_(0),
    backedgeEntries_(0),
    refcount_(0),
    parallelAge_(0),
    recompileInfo_(),
    osrPcMismatchCounter_(0),
    dependentAsmJSModules(nullptr),
    pendingBuilder_(nullptr)
{
}

IonScript *
IonScript::New(JSContext *cx, types::RecompileInfo recompileInfo,
               uint32_t frameSlots, uint32_t frameSize,
               size_t snapshotsListSize, size_t snapshotsRVATableSize,
               size_t recoversSize, size_t bailoutEntries,
               size_t constants, size_t safepointIndices,
               size_t osiIndices, size_t cacheEntries,
               size_t runtimeSize,  size_t safepointsSize,
               size_t callTargetEntries, size_t backedgeEntries,
               OptimizationLevel optimizationLevel)
{
    static const int DataAlignment = sizeof(void *);

    if (snapshotsListSize >= MAX_BUFFER_SIZE ||
        (bailoutEntries >= MAX_BUFFER_SIZE / sizeof(uint32_t)))
    {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }

    
    
    
    size_t paddedSnapshotsSize = AlignBytes(snapshotsListSize + snapshotsRVATableSize, DataAlignment);
    size_t paddedRecoversSize = AlignBytes(recoversSize, DataAlignment);
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
                   paddedRecoversSize +
                   paddedBailoutSize +
                   paddedConstantsSize +
                   paddedSafepointIndicesSize+
                   paddedOsiIndicesSize +
                   paddedCacheEntriesSize +
                   paddedRuntimeSize +
                   paddedSafepointSize +
                   paddedCallTargetSize +
                   paddedBackedgeSize;
    IonScript *script = cx->zone()->pod_malloc_with_extra<IonScript, uint8_t>(bytes);
    if (!script)
        return nullptr;
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
    script->snapshotsListSize_ = snapshotsListSize;
    script->snapshotsRVATableSize_ = snapshotsRVATableSize;
    offsetCursor += paddedSnapshotsSize;

    script->recovers_ = offsetCursor;
    script->recoversSize_ = recoversSize;
    offsetCursor += paddedRecoversSize;

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
    if (zone->needsIncrementalBarrier())
        ionScript->trace(zone->barrierTracer());
#endif
}

void
IonScript::copySnapshots(const SnapshotWriter *writer)
{
    MOZ_ASSERT(writer->listSize() == snapshotsListSize_);
    memcpy((uint8_t *)this + snapshots_,
           writer->listBuffer(), snapshotsListSize_);

    MOZ_ASSERT(snapshotsRVATableSize_);
    MOZ_ASSERT(writer->RVATableSize() == snapshotsRVATableSize_);
    memcpy((uint8_t *)this + snapshots_ + snapshotsListSize_,
           writer->RVATableBuffer(), snapshotsRVATableSize_);
}

void
IonScript::copyRecovers(const RecoverWriter *writer)
{
    MOZ_ASSERT(writer->size() == recoversSize_);
    memcpy((uint8_t *)this + recovers_, writer->buffer(), recoversSize_);
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
                                  PatchableBackedgeInfo *backedges,
                                  MacroAssembler &masm)
{
    for (size_t i = 0; i < backedgeEntries_; i++) {
        PatchableBackedgeInfo &info = backedges[i];
        PatchableBackedge *patchableBackedge = &backedgeList()[i];

        
        info.backedge.fixup(&masm);
        uint32_t loopHeaderOffset = masm.actualOffset(info.loopHeader->offset());
        uint32_t interruptCheckOffset = masm.actualOffset(info.interruptCheck->offset());

        CodeLocationJump backedge(code, info.backedge);
        CodeLocationLabel loopHeader(code, CodeOffsetLabel(loopHeaderOffset));
        CodeLocationLabel interruptCheck(code, CodeOffsetLabel(interruptCheckOffset));
        new(patchableBackedge) PatchableBackedge(backedge, loopHeader, interruptCheck);

        
        
        
        
        if (cx->runtime()->interrupt)
            PatchBackedge(backedge, interruptCheck, JitRuntime::BackedgeInterruptCheck);
        else
            PatchBackedge(backedge, loopHeader, JitRuntime::BackedgeLoopHeader);

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

    MOZ_CRASH("displacement not found.");
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

    MOZ_CRASH("Failed to find OSI point return address");
}

const OsiIndex *
IonScript::getOsiIndex(uint8_t *retAddr) const
{
    JitSpew(JitSpew_IonInvalidate, "IonScript %p has method %p raw %p", (void *) this, (void *)
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
    if (script->pendingBuilder())
        jit::FinishOffThreadBuilder(nullptr, script->pendingBuilder());

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
IonScript::purgeCaches()
{
    
    
    
    
    
    if (invalidated())
        return;

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

    for (gc::ZoneCellIterUnderGC i(zone, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
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
    TraceLogger *logger;
    if (GetIonContext()->runtime->onMainThread())
        logger = TraceLoggerForMainThread(GetIonContext()->runtime);
    else
        logger = TraceLoggerForCurrentThread();

    if (!mir->compilingAsmJS()) {
        if (!MakeMRegExpHoistable(graph))
            return false;
    }

    IonSpewPass("BuildSSA");
    AssertBasicGraphCoherency(graph);

    if (mir->shouldCancel("Start"))
        return false;

    if (!mir->compilingAsmJS()) {
        AutoTraceLog log(logger, TraceLogger::FoldTests);
        FoldTests(graph);
        IonSpewPass("Fold Tests");
        AssertBasicGraphCoherency(graph);

        if (mir->shouldCancel("Fold Tests"))
            return false;
    }

    {
        AutoTraceLog log(logger, TraceLogger::SplitCriticalEdges);
        if (!SplitCriticalEdges(graph))
            return false;
        IonSpewPass("Split Critical Edges");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("Split Critical Edges"))
            return false;
    }

    {
        AutoTraceLog log(logger, TraceLogger::RenumberBlocks);
        if (!RenumberBlocks(graph))
            return false;
        IonSpewPass("Renumber Blocks");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("Renumber Blocks"))
            return false;
    }

    {
        AutoTraceLog log(logger, TraceLogger::DominatorTree);
        if (!BuildDominatorTree(graph))
            return false;
        

        if (mir->shouldCancel("Dominator Tree"))
            return false;
    }

    {
        AutoTraceLog log(logger, TraceLogger::PhiAnalysis);
        
        
        
        
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
    }

    if (mir->optimizationInfo().scalarReplacementEnabled()) {
        AutoTraceLog log(logger, TraceLogger::ScalarReplacement);
        if (!ScalarReplacement(mir, graph))
            return false;
        IonSpewPass("Scalar Replacement");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("Scalar Replacement"))
            return false;
    }

    if (!mir->compilingAsmJS()) {
        AutoTraceLog log(logger, TraceLogger::ApplyTypes);
        if (!ApplyTypeInformation(mir, graph))
            return false;
        IonSpewPass("Apply types");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("Apply types"))
            return false;
    }

    if (graph.entryBlock()->info().executionMode() == ParallelExecution) {
        AutoTraceLog log(logger, TraceLogger::ParallelSafetyAnalysis);
        ParallelSafetyAnalysis analysis(mir, graph);
        if (!analysis.analyze())
            return false;
        IonSpewPass("Parallel Safety Analysis");
        AssertExtendedGraphCoherency(graph);
        if (mir->shouldCancel("Parallel Safety Analysis"))
            return false;
    }

    
    
    if (mir->optimizationInfo().licmEnabled() ||
        mir->optimizationInfo().gvnEnabled())
    {
        AutoTraceLog log(logger, TraceLogger::AliasAnalysis);
        AliasAnalysis analysis(mir, graph);
        if (!analysis.analyze())
            return false;
        IonSpewPass("Alias analysis");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("Alias analysis"))
            return false;

        if (!mir->compilingAsmJS()) {
            
            
            
            if (!EliminateDeadResumePointOperands(mir, graph))
                return false;

            if (mir->shouldCancel("Eliminate dead resume point operands"))
                return false;
        }
    }

    if (mir->optimizationInfo().gvnEnabled()) {
        AutoTraceLog log(logger, TraceLogger::GVN);
        ValueNumberer gvn(mir, graph);
        if (!gvn.run(ValueNumberer::UpdateAliasAnalysis))
            return false;
        IonSpewPass("GVN");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("GVN"))
            return false;
    }

    if (mir->optimizationInfo().uceEnabled()) {
        AutoTraceLog log(logger, TraceLogger::UCE);
        UnreachableCodeElimination uce(mir, graph);
        if (!uce.analyze())
            return false;
        IonSpewPass("UCE");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("UCE"))
            return false;
    }

    if (mir->optimizationInfo().licmEnabled()) {
        AutoTraceLog log(logger, TraceLogger::LICM);
        
        
        
        JSScript *script = mir->info().script();
        if (!script || !script->hadFrequentBailouts()) {
            if (!LICM(mir, graph))
                return false;
            IonSpewPass("LICM");
            AssertExtendedGraphCoherency(graph);

            if (mir->shouldCancel("LICM"))
                return false;
        }
    }

    if (mir->optimizationInfo().rangeAnalysisEnabled()) {
        AutoTraceLog log(logger, TraceLogger::RangeAnalysis);
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

        if (mir->optimizationInfo().autoTruncateEnabled()) {
            if (!r.truncate())
                return false;
            IonSpewPass("Truncate Doubles");
            AssertExtendedGraphCoherency(graph);

            if (mir->shouldCancel("Truncate Doubles"))
                return false;
        }

        if (mir->optimizationInfo().loopUnrollingEnabled()) {
            AutoTraceLog log(logger, TraceLogger::LoopUnrolling);

            if (!UnrollLoops(graph, r.loopIterationBounds))
                return false;

            IonSpewPass("Unroll Loops");
            AssertExtendedGraphCoherency(graph);
        }
    }

    if (mir->optimizationInfo().eaaEnabled()) {
        AutoTraceLog log(logger, TraceLogger::EffectiveAddressAnalysis);
        EffectiveAddressAnalysis eaa(graph);
        if (!eaa.analyze())
            return false;
        IonSpewPass("Effective Address Analysis");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("Effective Address Analysis"))
            return false;
    }

    {
        AutoTraceLog log(logger, TraceLogger::EliminateDeadCode);
        if (!EliminateDeadCode(mir, graph))
            return false;
        IonSpewPass("DCE");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("DCE"))
            return false;
    }

    
    
    {
        AutoTraceLog log(logger, TraceLogger::MakeLoopsContiguous);
        if (!MakeLoopsContiguous(graph))
            return false;
        IonSpewPass("Make loops contiguous");
        AssertExtendedGraphCoherency(graph);

        if (mir->shouldCancel("Make loops contiguous"))
            return false;
    }

    
    

    if (mir->optimizationInfo().edgeCaseAnalysisEnabled()) {
        AutoTraceLog log(logger, TraceLogger::EdgeCaseAnalysis);
        EdgeCaseAnalysis edgeCaseAnalysis(mir, graph);
        if (!edgeCaseAnalysis.analyzeLate())
            return false;
        IonSpewPass("Edge Case Analysis (Late)");
        AssertGraphCoherency(graph);

        if (mir->shouldCancel("Edge Case Analysis (Late)"))
            return false;
    }

    if (mir->optimizationInfo().eliminateRedundantChecksEnabled()) {
        AutoTraceLog log(logger, TraceLogger::EliminateRedundantChecks);
        
        
        
        
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

    TraceLogger *logger;
    if (GetIonContext()->runtime->onMainThread())
        logger = TraceLoggerForMainThread(GetIonContext()->runtime);
    else
        logger = TraceLoggerForCurrentThread();

    LIRGraph *lir = mir->alloc().lifoAlloc()->new_<LIRGraph>(&graph);
    if (!lir || !lir->init())
        return nullptr;

    LIRGenerator lirgen(mir, graph, *lir);
    {
        AutoTraceLog log(logger, TraceLogger::GenerateLIR);
        if (!lirgen.generate())
            return nullptr;
        IonSpewPass("Generate LIR");

        if (mir->shouldCancel("Generate LIR"))
            return nullptr;
    }

    AllocationIntegrityState integrity(*lir);

    {
        AutoTraceLog log(logger, TraceLogger::RegisterAllocation);

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
            MOZ_CRASH("Bad regalloc");
        }

        if (mir->shouldCancel("Allocate Registers"))
            return nullptr;
    }

    return lir;
}

CodeGenerator *
GenerateCode(MIRGenerator *mir, LIRGraph *lir)
{
    TraceLogger *logger;
    if (GetIonContext()->runtime->onMainThread())
        logger = TraceLoggerForMainThread(GetIonContext()->runtime);
    else
        logger = TraceLoggerForCurrentThread();
    AutoTraceLog log(logger, TraceLogger::GenerateCode);

    CodeGenerator *codegen = js_new<CodeGenerator>(mir, lir);
    if (!codegen)
        return nullptr;

    if (!codegen->generate()) {
        js_delete(codegen);
        return nullptr;
    }

    return codegen;
}

CodeGenerator *
CompileBackEnd(MIRGenerator *mir)
{
    
    AutoEnterIonCompilation enter;

    if (!OptimizeMIR(mir))
        return nullptr;

    LIRGraph *lir = GenerateLIR(mir);
    if (!lir)
        return nullptr;

    return GenerateCode(mir, lir);
}

void
AttachFinishedCompilations(JSContext *cx)
{
    JitCompartment *ion = cx->compartment()->jitCompartment();
    if (!ion)
        return;

    types::AutoEnterAnalysis enterTypes(cx);
    AutoLockHelperThreadState lock;

    GlobalHelperThreadState::IonBuilderVector &finished = HelperThreadState().ionFinishedList();

    TraceLogger *logger = TraceLoggerForMainThread(cx->runtime());

    
    
    while (true) {
        IonBuilder *builder = nullptr;

        
        for (size_t i = 0; i < finished.length(); i++) {
            IonBuilder *testBuilder = finished[i];
            if (testBuilder->compartment == CompileCompartment::get(cx->compartment())) {
                builder = testBuilder;
                HelperThreadState().remove(finished, &i);
                break;
            }
        }
        if (!builder)
            break;

        
        
        if (builder->info().executionMode() == SequentialExecution &&
            builder->script()->hasIonScript())
        {
            bool onStack = false;
            for (JitActivationIterator iter(cx->runtime()); !iter.done(); ++iter) {
                for (JitFrameIterator it(iter.jitTop(), SequentialExecution); !it.done(); ++it) {
                    if (!it.isIonJS())
                        continue;
                    if (it.checkInvalidation())
                        continue;

                    JSScript *script = it.script();
                    if (builder->script() == script) {
                        onStack = true;
                        break;
                    }
                }
                if (onStack)
                    break;
            }

            if (onStack) {
                builder->script()->setPendingIonBuilder(cx, builder);
                HelperThreadState().ionLazyLinkList().insertFront(builder);
                continue;
            }
        }

        if (CodeGenerator *codegen = builder->backgroundCodegen()) {
            RootedScript script(cx, builder->script());
            IonContext ictx(cx, &builder->alloc());
            AutoTraceLog logScript(logger, TraceLogCreateTextId(logger, script));
            AutoTraceLog logLink(logger, TraceLogger::IonLinking);

            
            
            
            codegen->masm.constructRoot(cx);

            bool success;
            {
                AutoUnlockHelperThreadState unlock;
                success = codegen->link(cx, builder->constraints());
            }

            if (!success) {
                
                
                
                
                cx->clearPendingException();
            }
        }

        FinishOffThreadBuilder(cx, builder);
    }
}

static const size_t BUILDER_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 1 << 12;

static inline bool
OffThreadCompilationAvailable(JSContext *cx)
{
    
    
    
    
    
    return cx->runtime()->canUseOffthreadIonCompilation()
        && HelperThreadState().cpuCount > 1
        && CanUseExtraThreads();
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
    TraceLogger *logger = TraceLoggerForMainThread(cx->runtime());
    AutoTraceLog logScript(logger, TraceLogCreateTextId(logger, script));
    AutoTraceLog logCompile(logger, TraceLogger::IonCompilation);

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

    if (executionMode == ParallelExecution &&
        LIRGenerator::allowInlineForkJoinGetSlice() &&
        !cx->runtime()->jitRuntime()->ensureForkJoinGetSliceStubExists(cx))
    {
        return AbortReason_Alloc;
    }

    MIRGraph *graph = alloc->new_<MIRGraph>(temp);
    if (!graph)
        return AbortReason_Alloc;

    InlineScriptTree *inlineScriptTree = InlineScriptTree::New(temp, nullptr, nullptr, script);
    if (!inlineScriptTree)
        return AbortReason_Alloc;

    CompileInfo *info = alloc->new_<CompileInfo>(script, script->functionNonDelazifying(), osrPc,
                                                 constructing, executionMode,
                                                 script->needsArgsObj(), inlineScriptTree);
    if (!info)
        return AbortReason_Alloc;

    BaselineInspector *inspector = alloc->new_<BaselineInspector>(script);
    if (!inspector)
        return AbortReason_Alloc;

    BaselineFrameInspector *baselineFrameInspector = nullptr;
    if (baselineFrame) {
        baselineFrameInspector = NewBaselineFrameInspector(temp, baselineFrame, info);
        if (!baselineFrameInspector)
            return AbortReason_Alloc;
    }

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

#ifdef DEBUG
    IonSpewFunction ionSpewFunction(graph, builderScript);
#endif

    bool succeeded = builder->build();
    builder->clearForBackEnd();

    if (!succeeded) {
        AbortReason reason = builder->abortReason();
        if (reason == AbortReason_NewScriptProperties) {
            
            
            
            const MIRGenerator::TypeObjectVector &types = builder->abortedNewScriptPropertiesTypes();
            for (size_t i = 0; i < types.length(); i++) {
                if (!types[i]->newScript()->maybeAnalyze(cx, types[i], nullptr,  true))
                    return AbortReason_Alloc;
            }
        }
        return reason;
    }

    
    if (OffThreadCompilationAvailable(cx)) {
        if (!recompile)
            SetIonScript(cx, builderScript, executionMode, ION_COMPILING_SCRIPT);

        JitSpew(JitSpew_IonLogs, "Can't log script %s:%d. (Compiled on background thread.)",
                builderScript->filename(), builderScript->lineno());

        if (!StartOffThreadIonCompile(cx, builder)) {
            JitSpew(JitSpew_IonAbort, "Unable to start off-thread ion compilation.");
            return AbortReason_Alloc;
        }

        
        
        autoDelete.forget();

        return AbortReason_NoAbort;
    }

    ScopedJSDeletePtr<CodeGenerator> codegen(CompileBackEnd(builder));
    if (!codegen) {
        JitSpew(JitSpew_IonAbort, "Failed during back-end compilation.");
        return AbortReason_Disable;
    }

    bool success = codegen->link(cx, builder->constraints());

    return success ? AbortReason_NoAbort : AbortReason_Disable;
}

static bool
CheckFrame(BaselineFrame *frame)
{
    JS_ASSERT(!frame->isGeneratorFrame());
    JS_ASSERT(!frame->isDebuggerFrame());

    
    if (frame->isFunctionFrame()) {
        if (TooManyActualArguments(frame->numActualArgs())) {
            JitSpew(JitSpew_IonAbort, "too many actual args");
            return false;
        }

        if (TooManyFormalArguments(frame->numFormalArgs())) {
            JitSpew(JitSpew_IonAbort, "too many args");
            return false;
        }
    }

    return true;
}

static bool
CheckScript(JSContext *cx, JSScript *script, bool osr)
{
    if (script->isForEval()) {
        
        
        
        JitSpew(JitSpew_IonAbort, "eval script");
        return false;
    }

    if (!script->compileAndGo() && !script->functionNonDelazifying()) {
        
        
        
        JitSpew(JitSpew_IonAbort, "not compile-and-go");
        return false;
    }

    return true;
}

static MethodStatus
CheckScriptSize(JSContext *cx, JSScript* script)
{
    if (!js_JitOptions.limitScriptSize)
        return Method_Compiled;

    uint32_t numLocalsAndArgs = NumLocalsAndArgs(script);

    if (script->length() > MAX_MAIN_THREAD_SCRIPT_SIZE ||
        numLocalsAndArgs > MAX_MAIN_THREAD_LOCALS_AND_ARGS)
    {
        if (!OffThreadCompilationAvailable(cx)) {
            JitSpew(JitSpew_IonAbort, "Script too large (%u bytes) (%u locals/args)",
                    script->length(), numLocalsAndArgs);
            return Method_CantCompile;
        }
    }

    return Method_Compiled;
}

bool
CanIonCompileScript(JSContext *cx, JSScript *script, bool osr)
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
    JS_ASSERT_IF(osrPc != nullptr, LoopEntryCanIonOsr(osrPc));
    JS_ASSERT_IF(executionMode == ParallelExecution, !osrFrame && !osrPc);
    JS_ASSERT_IF(executionMode == ParallelExecution, !HasIonScript(script, executionMode));

    if (!script->hasBaselineScript())
        return Method_Skipped;

    if (cx->compartment()->debugMode()) {
        JitSpew(JitSpew_IonAbort, "debugging");
        return Method_CantCompile;
    }

    if (!CheckScript(cx, script, bool(osrPc))) {
        JitSpew(JitSpew_IonAbort, "Aborted compilation of %s:%d", script->filename(), script->lineno());
        return Method_CantCompile;
    }

    MethodStatus status = CheckScriptSize(cx, script);
    if (status != Method_Compiled) {
        JitSpew(JitSpew_IonAbort, "Aborted compilation of %s:%d", script->filename(), script->lineno());
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
jit::CanEnterAtBranch(JSContext *cx, JSScript *script, BaselineFrame *osrFrame, jsbytecode *pc)
{
    JS_ASSERT(jit::IsIonEnabled(cx));
    JS_ASSERT((JSOp)*pc == JSOP_LOOPENTRY);
    JS_ASSERT(LoopEntryCanIonOsr(pc));

    
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
    MethodStatus status = Compile(cx, rscript, osrFrame, pc, osrFrame->isConstructing(),
                                  SequentialExecution);
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

    RootedScript rscript(cx, script);

    
    
    
    if (state.isInvoke()) {
        InvokeState &invoke = *state.asInvoke();

        if (TooManyActualArguments(invoke.args().length())) {
            JitSpew(JitSpew_IonAbort, "too many actual args");
            ForbidCompilation(cx, script);
            return Method_CantCompile;
        }

        if (TooManyFormalArguments(invoke.args().callee().as<JSFunction>().nargs())) {
            JitSpew(JitSpew_IonAbort, "too many args");
            ForbidCompilation(cx, script);
            return Method_CantCompile;
        }

        if (!state.maybeCreateThisForConstructor(cx))
            return Method_Skipped;
    } else if (state.isGenerator()) {
        JitSpew(JitSpew_IonAbort, "generator frame");
        ForbidCompilation(cx, script);
        return Method_CantCompile;
    }

    
    
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
jit::CompileFunctionForBaseline(JSContext *cx, HandleScript script, BaselineFrame *frame)
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
        Compile(cx, script, frame, nullptr, frame->isConstructing(), SequentialExecution);
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
        JitActivation activation(cx);

        CALL_GENERATED_CODE(enter, data.jitcode, data.maxArgc, data.maxArgv, nullptr, data.calleeToken,
                             nullptr, 0, data.result.address());
    }

    JS_ASSERT(!cx->runtime()->jitRuntime()->hasIonReturnOverride());

    
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
        data.calleeToken = CalleeToToken(&args.callee().as<JSFunction>(), data.constructing);

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
            !(state.asExecute()->type() & InterpreterFrame::GLOBAL))
        {
            ScriptFrameIter iter(cx);
            if (iter.isFunctionFrame())
                data.calleeToken = CalleeToToken(iter.callee(),  false);
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

    JitActivation activation(cx);

    EnterJitCode enter = cx->runtime()->jitRuntime()->enterIon();
    void *calleeToken = CalleeToToken(fun,  false);

    RootedValue result(cx, Int32Value(args.length()));
    JS_ASSERT(args.length() >= fun->nargs());

    CALL_GENERATED_CODE(enter, jitcode, args.length() + 1, args.array() - 1, nullptr,
                        calleeToken,  nullptr, 0, result.address());

    JS_ASSERT(!cx->runtime()->jitRuntime()->hasIonReturnOverride());

    args.rval().set(result);

    JS_ASSERT_IF(result.isMagic(), result.isMagic(JS_ION_ERROR));
    return result.isMagic() ? IonExec_Error : IonExec_Ok;
}

static void
InvalidateActivation(FreeOp *fop, uint8_t *jitTop, bool invalidateAll)
{
    JitSpew(JitSpew_IonInvalidate, "BEGIN invalidating activation");

    size_t frameno = 1;

    for (JitFrameIterator it(jitTop, SequentialExecution); !it.done(); ++it, ++frameno) {
        JS_ASSERT_IF(frameno == 1, it.type() == JitFrame_Exit);

#ifdef DEBUG
        switch (it.type()) {
          case JitFrame_Exit:
            JitSpew(JitSpew_IonInvalidate, "#%d exit frame @ %p", frameno, it.fp());
            break;
          case JitFrame_BaselineJS:
          case JitFrame_IonJS:
          {
            JS_ASSERT(it.isScripted());
            const char *type = it.isIonJS() ? "Optimized" : "Baseline";
            JitSpew(JitSpew_IonInvalidate, "#%d %s JS frame @ %p, %s:%d (fun: %p, script: %p, pc %p)",
                    frameno, type, it.fp(), it.script()->filename(), it.script()->lineno(),
                    it.maybeCallee(), (JSScript *)it.script(), it.returnAddressToFp());
            break;
          }
          case JitFrame_BaselineStub:
            JitSpew(JitSpew_IonInvalidate, "#%d baseline stub frame @ %p", frameno, it.fp());
            break;
          case JitFrame_Rectifier:
            JitSpew(JitSpew_IonInvalidate, "#%d rectifier frame @ %p", frameno, it.fp());
            break;
          case JitFrame_Unwound_IonJS:
          case JitFrame_Unwound_BaselineStub:
            MOZ_CRASH("invalid");
          case JitFrame_Unwound_Rectifier:
            JitSpew(JitSpew_IonInvalidate, "#%d unwound rectifier frame @ %p", frameno, it.fp());
            break;
          case JitFrame_Entry:
            JitSpew(JitSpew_IonInvalidate, "#%d entry frame @ %p", frameno, it.fp());
            break;
        }
#endif

        if (!it.isIonJS())
            continue;

        bool calledFromLinkStub = false;
        JitCode *lazyLinkStub = fop->runtime()->jitRuntime()->lazyLinkStub();
        if (it.returnAddressToFp() >= lazyLinkStub->raw() &&
            it.returnAddressToFp() < lazyLinkStub->rawEnd())
        {
            calledFromLinkStub = true;
        }

        
        if (!calledFromLinkStub && it.checkInvalidation())
            continue;

        JSScript *script = it.script();
        if (!script->hasIonScript())
            continue;

        if (!invalidateAll && !script->ionScript()->invalidated())
            continue;

        IonScript *ionScript = script->ionScript();

        
        
        
        ionScript->purgeCaches();

        
        
        ionScript->unlinkFromRuntime(fop);

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        ionScript->incref();

        JitCode *ionCode = ionScript->method();

        JS::Zone *zone = script->zone();
        if (zone->needsIncrementalBarrier()) {
            
            
            
            
            ionCode->trace(zone->barrierTracer());
        }
        ionCode->setInvalidated();

        
        if (calledFromLinkStub)
            continue;

        
        
        
        
        
        
        const SafepointIndex *si = ionScript->getSafepointIndex(it.returnAddressToFp());
        CodeLocationLabel dataLabelToMunge(it.returnAddressToFp());
        ptrdiff_t delta = ionScript->invalidateEpilogueDataOffset() -
                          (it.returnAddressToFp() - ionCode->raw());
        Assembler::PatchWrite_Imm32(dataLabelToMunge, Imm32(delta));

        CodeLocationLabel osiPatchPoint = SafepointReader::InvalidationPatchPoint(ionScript, si);
        CodeLocationLabel invalidateEpilogue(ionCode, CodeOffsetLabel(ionScript->invalidateEpilogueOffset()));

        JitSpew(JitSpew_IonInvalidate, "   ! Invalidate ionScript %p (ref %u) -> patching osipoint %p",
                ionScript, ionScript->refcount(), (void *) osiPatchPoint.raw());
        Assembler::PatchWrite_NearCall(osiPatchPoint, invalidateEpilogue);
    }

    JitSpew(JitSpew_IonInvalidate, "END invalidating activation");
}

void
jit::StopAllOffThreadCompilations(JSCompartment *comp)
{
    if (!comp->jitCompartment())
        return;
    CancelOffThreadIonCompile(comp, nullptr);
    FinishAllOffThreadCompilations(comp);
}

void
jit::InvalidateAll(FreeOp *fop, Zone *zone)
{
    for (CompartmentsInZoneIter comp(zone); !comp.done(); comp.next())
        StopAllOffThreadCompilations(comp);

    for (JitActivationIterator iter(fop->runtime()); !iter.done(); ++iter) {
        if (iter->compartment()->zone() == zone) {
            JitSpew(JitSpew_IonInvalidate, "Invalidating all frames for GC");
            InvalidateActivation(fop, iter.jitTop(), true);
        }
    }
}


void
jit::Invalidate(types::TypeZone &types, FreeOp *fop,
                const Vector<types::RecompileInfo> &invalid, bool resetUses,
                bool cancelOffThread)
{
    JitSpew(JitSpew_IonInvalidate, "Start invalidation.");

    
    
    size_t numInvalidations = 0;
    for (size_t i = 0; i < invalid.length(); i++) {
        const types::CompilerOutput &co = *invalid[i].compilerOutput(types);
        if (!co.isValid())
            continue;

        if (cancelOffThread)
            CancelOffThreadIonCompile(co.script()->compartment(), co.script());

        if (!co.ion())
            continue;

        JitSpew(JitSpew_IonInvalidate, " Invalidate %s:%u, IonScript %p",
                co.script()->filename(), co.script()->lineno(), co.ion());

        
        
        
        co.ion()->incref();
        numInvalidations++;
    }

    if (!numInvalidations) {
        JitSpew(JitSpew_IonInvalidate, " No IonScript invalidation.");
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

        SetIonScript(nullptr, script, executionMode, nullptr);
        ionScript->decref(fop);
        co.invalidate();
        numInvalidations--;

        
        
        
        
        
        
        
        
        
        
        
        if (resetUses && executionMode != ParallelExecution)
            script->resetWarmUpCounter();
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

    if (cx->runtime()->spsProfiler.enabled()) {
        
        
        

        
        const char *filename = script->filename();
        if (filename == nullptr)
            filename = "<unknown>";

        size_t len = strlen(filename) + 20;
        char *buf = js_pod_malloc<char>(len);
        if (!buf)
            return false;

        
        JS_snprintf(buf, len, "Invalidate %s:%u", filename, (unsigned int)script->lineno());
        cx->runtime()->spsProfiler.markEvent(buf);
        js_free(buf);
    }

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
        MOZ_CRASH("No such execution mode");
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
FinishInvalidationOf(FreeOp *fop, JSScript *script, IonScript *ionScript)
{
    types::TypeZone &types = script->zone()->types;

    
    
    if (types::CompilerOutput *output = ionScript->recompileInfo().compilerOutput(types))
        output->invalidate();

    
    
    if (!ionScript->invalidated())
        jit::IonScript::Destroy(fop, ionScript);
}

template <ExecutionMode mode>
void
jit::FinishInvalidation(FreeOp *fop, JSScript *script)
{
    
    
    switch (mode) {
      case SequentialExecution:
        if (script->hasIonScript()) {
            IonScript *ion = script->ionScript();
            script->setIonScript(nullptr, nullptr);
            FinishInvalidationOf(fop, script, ion);
        }
        return;

      case ParallelExecution:
        if (script->hasParallelIonScript()) {
            IonScript *parallelIon = script->parallelIonScript();
            script->setParallelIonScript(nullptr);
            FinishInvalidationOf(fop, script, parallelIon);
        }
        return;

      default:
        MOZ_CRASH("bad execution mode");
    }
}

template void
jit::FinishInvalidation<SequentialExecution>(FreeOp *fop, JSScript *script);

template void
jit::FinishInvalidation<ParallelExecution>(FreeOp *fop, JSScript *script);

void
jit::ForbidCompilation(JSContext *cx, JSScript *script)
{
    ForbidCompilation(cx, script, SequentialExecution);
}

void
jit::ForbidCompilation(JSContext *cx, JSScript *script, ExecutionMode mode)
{
    JitSpew(JitSpew_IonAbort, "Disabling Ion mode %d compilation of script %s:%d",
            mode, script->filename(), script->lineno());

    CancelOffThreadIonCompile(cx->compartment(), script);

    switch (mode) {
      case SequentialExecution:
        if (script->hasIonScript()) {
            
            
            
            
            
            if (!Invalidate(cx, script, mode, false))
                return;
        }

        script->setIonScript(cx, ION_DISABLED_SCRIPT);
        return;

      case ParallelExecution:
        if (script->hasParallelIonScript()) {
            if (!Invalidate(cx, script, mode, false))
                return;
        }

        script->setParallelIonScript(ION_DISABLED_SCRIPT);
        return;

      default:
        MOZ_CRASH("No such execution mode");
    }

    MOZ_CRASH("No such execution mode");
}

AutoFlushICache *
PerThreadData::autoFlushICache() const
{
    return autoFlushICache_;
}

void
PerThreadData::setAutoFlushICache(AutoFlushICache *afc)
{
    autoFlushICache_ = afc;
}






void
AutoFlushICache::setRange(uintptr_t start, size_t len)
{
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
    AutoFlushICache *afc = TlsPerThreadData.get()->PerThreadData::autoFlushICache();
    JS_ASSERT(afc);
    JS_ASSERT(!afc->start_);
    JitSpewCont(JitSpew_CacheFlush, "(%x %x):", start, len);

    uintptr_t stop = start + len;
    afc->start_ = start;
    afc->stop_ = stop;
#endif
}



















void
AutoFlushICache::flush(uintptr_t start, size_t len)
{
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
    PerThreadData *pt = TlsPerThreadData.get();
    AutoFlushICache *afc = pt ? pt->PerThreadData::autoFlushICache() : nullptr;
    if (!afc) {
        JitSpewCont(JitSpew_CacheFlush, "#");
        ExecutableAllocator::cacheFlush((void*)start, len);
        JS_ASSERT(len <= 16);
        return;
    }

    uintptr_t stop = start + len;
    if (start >= afc->start_ && stop <= afc->stop_) {
        
        JitSpewCont(JitSpew_CacheFlush, afc->inhibit_ ? "-" : "=");
        return;
    }

    JitSpewCont(JitSpew_CacheFlush, afc->inhibit_ ? "x" : "*");
    ExecutableAllocator::cacheFlush((void *)start, len);
#endif
}



void
AutoFlushICache::setInhibit()
{
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
    AutoFlushICache *afc = TlsPerThreadData.get()->PerThreadData::autoFlushICache();
    JS_ASSERT(afc);
    JS_ASSERT(afc->start_);
    JitSpewCont(JitSpew_CacheFlush, "I");
    afc->inhibit_ = true;
#endif
}


















AutoFlushICache::AutoFlushICache(const char *nonce, bool inhibit)
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
  : start_(0),
    stop_(0),
    name_(nonce),
    inhibit_(inhibit)
#endif
{
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
    PerThreadData *pt = TlsPerThreadData.get();
    AutoFlushICache *afc = pt->PerThreadData::autoFlushICache();
    if (afc)
        JitSpew(JitSpew_CacheFlush, "<%s,%s%s ", nonce, afc->name_, inhibit ? " I" : "");
    else
        JitSpewCont(JitSpew_CacheFlush, "<%s%s ", nonce, inhibit ? " I" : "");

    prev_ = afc;
    pt->PerThreadData::setAutoFlushICache(this);
#endif
}

AutoFlushICache::~AutoFlushICache()
{
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
    PerThreadData *pt = TlsPerThreadData.get();
    JS_ASSERT(pt->PerThreadData::autoFlushICache() == this);

    if (!inhibit_ && start_)
        ExecutableAllocator::cacheFlush((void *)start_, size_t(stop_ - start_));

    JitSpewCont(JitSpew_CacheFlush, "%s%s>", name_, start_ ? "" : " U");
    JitSpewFin(JitSpew_CacheFlush);
    pt->PerThreadData::setAutoFlushICache(prev_);
#endif
}

void
jit::PurgeCaches(JSScript *script)
{
    if (script->hasIonScript())
        script->ionScript()->purgeCaches();

    if (script->hasParallelIonScript())
        script->parallelIonScript()->purgeCaches();
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

bool
jit::RematerializeAllFrames(JSContext *cx, JSCompartment *comp)
{
    for (JitActivationIterator iter(comp->runtimeFromMainThread()); !iter.done(); ++iter) {
        if (iter.activation()->compartment() == comp) {
            for (JitFrameIterator frameIter(iter); !frameIter.done(); ++frameIter) {
                if (!frameIter.isIonJS())
                    continue;
                if (!iter.activation()->asJit()->getRematerializedFrame(cx, frameIter))
                    return false;
            }
        }
    }
    return true;
}

bool
jit::UpdateForDebugMode(JSContext *maybecx, JSCompartment *comp,
                     AutoDebugModeInvalidation &invalidate)
{
    MOZ_ASSERT(invalidate.isFor(comp));

    
    
    invalidate.scheduleInvalidation(comp->debugMode());

    
    if (maybecx) {
        IonContext ictx(maybecx, nullptr);
        if (!RecompileOnStackBaselineScriptsForDebugMode(maybecx, comp)) {
            js_ReportOutOfMemory(maybecx);
            return false;
        }
    }

    return true;
}

AutoDebugModeInvalidation::~AutoDebugModeInvalidation()
{
    MOZ_ASSERT(!!comp_ != !!zone_);

    if (needInvalidation_ == NoNeed)
        return;

    Zone *zone = zone_ ? zone_ : comp_->zone();
    JSRuntime *rt = zone->runtimeFromMainThread();
    FreeOp *fop = rt->defaultFreeOp();

    if (comp_) {
        StopAllOffThreadCompilations(comp_);
    } else {
        for (CompartmentsInZoneIter comp(zone_); !comp.done(); comp.next())
            StopAllOffThreadCompilations(comp);
    }

    
    
    jit::MarkActiveBaselineScripts(zone);

    for (JitActivationIterator iter(rt); !iter.done(); ++iter) {
        JSCompartment *comp = iter->compartment();
        if (comp_ == comp || zone_ == comp->zone()) {
            IonContext ictx(CompileRuntime::get(rt));
            JitSpew(JitSpew_IonInvalidate, "Invalidating frames for debug mode toggle");
            InvalidateActivation(fop, iter.jitTop(), true);
        }
    }

    for (gc::ZoneCellIter i(zone, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->compartment() == comp_ || zone_) {
            FinishInvalidation<SequentialExecution>(fop, script);
            FinishInvalidation<ParallelExecution>(fop, script);
            FinishDiscardBaselineScript(fop, script);
            script->resetWarmUpCounter();
        } else if (script->hasBaselineScript()) {
            script->baselineScript()->resetActive();
        }
    }
}

bool
jit::JitSupportsFloatingPoint()
{
    return js::jit::MacroAssembler::SupportsFloatingPoint();
}

bool
jit::JitSupportsSimd()
{
    return js::jit::MacroAssembler::SupportsSimd();
}
