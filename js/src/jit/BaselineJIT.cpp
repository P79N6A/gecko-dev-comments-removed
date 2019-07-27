





#include "jit/BaselineJIT.h"

#include "mozilla/MemoryReporting.h"

#include "asmjs/AsmJSModule.h"
#include "jit/BaselineCompiler.h"
#include "jit/BaselineIC.h"
#include "jit/CompileInfo.h"
#include "jit/JitCommon.h"
#include "jit/JitSpewer.h"
#include "vm/Debugger.h"
#include "vm/Interpreter.h"
#include "vm/TraceLogging.h"

#include "jsobjinlines.h"
#include "jsopcodeinlines.h"
#include "jsscriptinlines.h"

#include "jit/JitFrames-inl.h"
#include "vm/Stack-inl.h"

using namespace js;
using namespace js::jit;

 PCMappingSlotInfo::SlotLocation
PCMappingSlotInfo::ToSlotLocation(const StackValue *stackVal)
{
    if (stackVal->kind() == StackValue::Register) {
        if (stackVal->reg() == R0)
            return SlotInR0;
        MOZ_ASSERT(stackVal->reg() == R1);
        return SlotInR1;
    }
    MOZ_ASSERT(stackVal->kind() != StackValue::Stack);
    return SlotIgnore;
}

BaselineScript::BaselineScript(uint32_t prologueOffset, uint32_t epilogueOffset,
                               uint32_t profilerEnterToggleOffset,
                               uint32_t profilerExitToggleOffset,
                               uint32_t traceLoggerEnterToggleOffset,
                               uint32_t traceLoggerExitToggleOffset,
                               uint32_t postDebugPrologueOffset)
  : method_(nullptr),
    templateScope_(nullptr),
    fallbackStubSpace_(),
    dependentAsmJSModules_(nullptr),
    prologueOffset_(prologueOffset),
    epilogueOffset_(epilogueOffset),
    profilerEnterToggleOffset_(profilerEnterToggleOffset),
    profilerExitToggleOffset_(profilerExitToggleOffset),
#ifdef JS_TRACE_LOGGING
# ifdef DEBUG
    traceLoggerScriptsEnabled_(false),
    traceLoggerEngineEnabled_(false),
# endif
    traceLoggerEnterToggleOffset_(traceLoggerEnterToggleOffset),
    traceLoggerExitToggleOffset_(traceLoggerExitToggleOffset),
    traceLoggerScriptEvent_(),
#endif
    postDebugPrologueOffset_(postDebugPrologueOffset),
    flags_(0),
    maxInliningDepth_(UINT8_MAX)
{ }

static const unsigned BASELINE_MAX_ARGS_LENGTH = 20000;

static bool
CheckFrame(InterpreterFrame *fp)
{
    if (fp->isDebuggerEvalFrame()) {
        
        
        JitSpew(JitSpew_BaselineAbort, "debugger frame");
        return false;
    }

    if (fp->isNonEvalFunctionFrame() && fp->numActualArgs() > BASELINE_MAX_ARGS_LENGTH) {
        
        JitSpew(JitSpew_BaselineAbort, "Too many arguments (%u)", fp->numActualArgs());
        return false;
    }

    return true;
}

static JitExecStatus
EnterBaseline(JSContext *cx, EnterJitData &data)
{
    if (data.osrFrame) {
        
        uint8_t spDummy;
        uint32_t extra = BaselineFrame::Size() + (data.osrNumStackValues * sizeof(Value));
        uint8_t *checkSp = (&spDummy) - extra;
        JS_CHECK_RECURSION_WITH_SP(cx, checkSp, return JitExec_Aborted);
    } else {
        JS_CHECK_RECURSION(cx, return JitExec_Aborted);
    }

    MOZ_ASSERT(jit::IsBaselineEnabled(cx));
    MOZ_ASSERT_IF(data.osrFrame, CheckFrame(data.osrFrame));

    EnterJitCode enter = cx->runtime()->jitRuntime()->enterBaseline();

    
    MOZ_ASSERT_IF(data.constructing, data.maxArgv[0].isObject());

    data.result.setInt32(data.numActualArgs);
    {
        AssertCompartmentUnchanged pcc(cx);
        JitActivation activation(cx);

        if (data.osrFrame)
            data.osrFrame->setRunningInJit();

        
        CALL_GENERATED_CODE(enter, data.jitcode, data.maxArgc, data.maxArgv, data.osrFrame, data.calleeToken,
                            data.scopeChain.get(), data.osrNumStackValues, data.result.address());

        if (data.osrFrame)
            data.osrFrame->clearRunningInJit();
    }

    MOZ_ASSERT(!cx->runtime()->jitRuntime()->hasIonReturnOverride());

    
    if (!data.result.isMagic() && data.constructing && data.result.isPrimitive())
        data.result = data.maxArgv[0];

    
    cx->runtime()->getJitRuntime(cx)->freeOsrTempData();

    MOZ_ASSERT_IF(data.result.isMagic(), data.result.isMagic(JS_ION_ERROR));
    return data.result.isMagic() ? JitExec_Error : JitExec_Ok;
}

JitExecStatus
jit::EnterBaselineMethod(JSContext *cx, RunState &state)
{
    BaselineScript *baseline = state.script()->baselineScript();

    EnterJitData data(cx);
    data.jitcode = baseline->method()->raw();

    AutoValueVector vals(cx);
    if (!SetEnterJitData(cx, data, state, vals))
        return JitExec_Error;

    JitExecStatus status = EnterBaseline(cx, data);
    if (status != JitExec_Ok)
        return status;

    state.setReturnValue(data.result);
    return JitExec_Ok;
}

JitExecStatus
jit::EnterBaselineAtBranch(JSContext *cx, InterpreterFrame *fp, jsbytecode *pc)
{
    MOZ_ASSERT(JSOp(*pc) == JSOP_LOOPENTRY);

    BaselineScript *baseline = fp->script()->baselineScript();

    EnterJitData data(cx);
    data.jitcode = baseline->nativeCodeForPC(fp->script(), pc);

    
    
    if (fp->isDebuggee()) {
        MOZ_RELEASE_ASSERT(baseline->hasDebugInstrumentation());
        data.jitcode += MacroAssembler::ToggledCallSize(data.jitcode);
    }

    data.osrFrame = fp;
    data.osrNumStackValues = fp->script()->nfixed() + cx->interpreterRegs().stackDepth();

    RootedValue thisv(cx);

    if (fp->isNonEvalFunctionFrame()) {
        data.constructing = fp->isConstructing();
        data.numActualArgs = fp->numActualArgs();
        data.maxArgc = Max(fp->numActualArgs(), fp->numFormalArgs()) + 1; 
        data.maxArgv = fp->argv() - 1; 
        data.scopeChain = nullptr;
        data.calleeToken = CalleeToToken(&fp->callee(), data.constructing);
    } else {
        thisv = fp->thisValue();
        data.constructing = false;
        data.numActualArgs = 0;
        data.maxArgc = 1;
        data.maxArgv = thisv.address();
        data.scopeChain = fp->scopeChain();

        
        if (fp->isFunctionFrame())
            data.calleeToken = CalleeToToken(&fp->callee(),  false);
        else
            data.calleeToken = CalleeToToken(fp->script());
    }

    TraceLoggerThread *logger = TraceLoggerForMainThread(cx->runtime());
    TraceLogStopEvent(logger, TraceLogger_Interpreter);
    TraceLogStartEvent(logger, TraceLogger_Baseline);

    JitExecStatus status = EnterBaseline(cx, data);
    if (status != JitExec_Ok)
        return status;

    fp->setReturnValue(data.result);
    return JitExec_Ok;
}

MethodStatus
jit::BaselineCompile(JSContext *cx, JSScript *script, bool forceDebugInstrumentation)
{
    MOZ_ASSERT(!script->hasBaselineScript());
    MOZ_ASSERT(script->canBaselineCompile());
    MOZ_ASSERT(IsBaselineEnabled(cx));

    script->ensureNonLazyCanonicalFunction(cx);

    LifoAlloc alloc(TempAllocator::PreferredLifoChunkSize);
    TempAllocator *temp = alloc.new_<TempAllocator>(&alloc);
    if (!temp)
        return Method_Error;

    JitContext jctx(cx, temp);

    BaselineCompiler compiler(cx, *temp, script);
    if (!compiler.init())
        return Method_Error;
    if (forceDebugInstrumentation)
        compiler.setCompileDebugInstrumentation();

    MethodStatus status = compiler.compile();

    MOZ_ASSERT_IF(status == Method_Compiled, script->hasBaselineScript());
    MOZ_ASSERT_IF(status != Method_Compiled, !script->hasBaselineScript());

    if (status == Method_CantCompile)
        script->setBaselineScript(cx, BASELINE_DISABLED_SCRIPT);

    return status;
}

static MethodStatus
CanEnterBaselineJIT(JSContext *cx, HandleScript script, InterpreterFrame *osrFrame)
{
    MOZ_ASSERT(jit::IsBaselineEnabled(cx));

    
    if (!script->canBaselineCompile())
        return Method_Skipped;

    if (script->length() > BaselineScript::MAX_JSSCRIPT_LENGTH)
        return Method_CantCompile;

    if (script->nslots() > BaselineScript::MAX_JSSCRIPT_SLOTS)
        return Method_CantCompile;

    if (!cx->compartment()->ensureJitCompartmentExists(cx))
        return Method_Error;

    if (script->hasBaselineScript())
        return Method_Compiled;

    
    if (script->incWarmUpCounter() <= js_JitOptions.baselineWarmUpThreshold)
        return Method_Skipped;

    
    
    
    return BaselineCompile(cx, script, osrFrame && osrFrame->isDebuggee());
}

MethodStatus
jit::CanEnterBaselineAtBranch(JSContext *cx, InterpreterFrame *fp, bool newType)
{
   
   if (fp->isConstructing() && fp->functionThis().isPrimitive()) {
       RootedObject callee(cx, &fp->callee());
       RootedObject obj(cx, CreateThisForFunction(cx, callee, newType ? SingletonObject : GenericObject));
       if (!obj)
           return Method_Skipped;
       fp->functionThis().setObject(*obj);
   }

   if (!CheckFrame(fp))
       return Method_CantCompile;

   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   if (fp->isDebuggee() && !Debugger::ensureExecutionObservabilityOfOsrFrame(cx, fp))
       return Method_Error;

   RootedScript script(cx, fp->script());
   return CanEnterBaselineJIT(cx, script, fp);
}

MethodStatus
jit::CanEnterBaselineMethod(JSContext *cx, RunState &state)
{
    if (state.isInvoke()) {
        InvokeState &invoke = *state.asInvoke();

        if (invoke.args().length() > BASELINE_MAX_ARGS_LENGTH) {
            JitSpew(JitSpew_BaselineAbort, "Too many arguments (%u)", invoke.args().length());
            return Method_CantCompile;
        }

        if (!state.maybeCreateThisForConstructor(cx))
            return Method_Skipped;
    } else {
        MOZ_ASSERT(state.isExecute());
        ExecuteType type = state.asExecute()->type();
        if (type == EXECUTE_DEBUG || type == EXECUTE_DEBUG_GLOBAL) {
            JitSpew(JitSpew_BaselineAbort, "debugger frame");
            return Method_CantCompile;
        }
    }

    RootedScript script(cx, state.script());
    return CanEnterBaselineJIT(cx, script,  nullptr);
};

BaselineScript *
BaselineScript::New(JSScript *jsscript, uint32_t prologueOffset, uint32_t epilogueOffset,
                    uint32_t profilerEnterToggleOffset, uint32_t profilerExitToggleOffset,
                    uint32_t traceLoggerEnterToggleOffset, uint32_t traceLoggerExitToggleOffset,
                    uint32_t postDebugPrologueOffset,
                    size_t icEntries, size_t pcMappingIndexEntries, size_t pcMappingSize,
                    size_t bytecodeTypeMapEntries, size_t yieldEntries)
{
    static const unsigned DataAlignment = sizeof(uintptr_t);

    size_t icEntriesSize = icEntries * sizeof(ICEntry);
    size_t pcMappingIndexEntriesSize = pcMappingIndexEntries * sizeof(PCMappingIndexEntry);
    size_t bytecodeTypeMapSize = bytecodeTypeMapEntries * sizeof(uint32_t);
    size_t yieldEntriesSize = yieldEntries * sizeof(uintptr_t);

    size_t paddedICEntriesSize = AlignBytes(icEntriesSize, DataAlignment);
    size_t paddedPCMappingIndexEntriesSize = AlignBytes(pcMappingIndexEntriesSize, DataAlignment);
    size_t paddedPCMappingSize = AlignBytes(pcMappingSize, DataAlignment);
    size_t paddedBytecodeTypesMapSize = AlignBytes(bytecodeTypeMapSize, DataAlignment);
    size_t paddedYieldEntriesSize = AlignBytes(yieldEntriesSize, DataAlignment);

    size_t allocBytes = paddedICEntriesSize +
                        paddedPCMappingIndexEntriesSize +
                        paddedPCMappingSize +
                        paddedBytecodeTypesMapSize +
                        paddedYieldEntriesSize;

    BaselineScript *script = jsscript->zone()->pod_malloc_with_extra<BaselineScript, uint8_t>(allocBytes);
    if (!script)
        return nullptr;
    new (script) BaselineScript(prologueOffset, epilogueOffset,
                                profilerEnterToggleOffset, profilerExitToggleOffset,
                                traceLoggerEnterToggleOffset, traceLoggerExitToggleOffset,
                                postDebugPrologueOffset);

    size_t offsetCursor = sizeof(BaselineScript);
    MOZ_ASSERT(offsetCursor == AlignBytes(sizeof(BaselineScript), DataAlignment));

    script->icEntriesOffset_ = offsetCursor;
    script->icEntries_ = icEntries;
    offsetCursor += paddedICEntriesSize;

    script->pcMappingIndexOffset_ = offsetCursor;
    script->pcMappingIndexEntries_ = pcMappingIndexEntries;
    offsetCursor += paddedPCMappingIndexEntriesSize;

    script->pcMappingOffset_ = offsetCursor;
    script->pcMappingSize_ = pcMappingSize;
    offsetCursor += paddedPCMappingSize;

    script->bytecodeTypeMapOffset_ = bytecodeTypeMapEntries ? offsetCursor : 0;
    offsetCursor += paddedBytecodeTypesMapSize;

    script->yieldEntriesOffset_ = yieldEntries ? offsetCursor : 0;
    offsetCursor += paddedYieldEntriesSize;

    MOZ_ASSERT(offsetCursor == sizeof(BaselineScript) + allocBytes);
    return script;
}

void
BaselineScript::trace(JSTracer *trc)
{
    MarkJitCode(trc, &method_, "baseline-method");
    if (templateScope_)
        MarkObject(trc, &templateScope_, "baseline-template-scope");

    
    for (size_t i = 0; i < numICEntries(); i++) {
        ICEntry &ent = icEntry(i);
        if (!ent.hasStub())
            continue;
        for (ICStub *stub = ent.firstStub(); stub; stub = stub->next())
            stub->trace(trc);
    }
}


void
BaselineScript::writeBarrierPre(Zone *zone, BaselineScript *script)
{
    if (zone->needsIncrementalBarrier())
        script->trace(zone->barrierTracer());
}

void
BaselineScript::Trace(JSTracer *trc, BaselineScript *script)
{
    script->trace(trc);
}

void
BaselineScript::Destroy(FreeOp *fop, BaselineScript *script)
{
    






    MOZ_ASSERT(fop->runtime()->gc.nursery.isEmpty());

    script->unlinkDependentAsmJSModules(fop);

    fop->delete_(script);
}

void
BaselineScript::unlinkDependentAsmJSModules(FreeOp *fop)
{
    
    
    if (dependentAsmJSModules_) {
        for (size_t i = 0; i < dependentAsmJSModules_->length(); i++) {
            DependentAsmJSModuleExit exit = (*dependentAsmJSModules_)[i];
            exit.module->detachJitCompilation(exit.exitIndex);
        }

        fop->delete_(dependentAsmJSModules_);
        dependentAsmJSModules_ = nullptr;
    }
}

bool
BaselineScript::addDependentAsmJSModule(JSContext *cx, DependentAsmJSModuleExit exit)
{
    if (!dependentAsmJSModules_) {
        dependentAsmJSModules_ = cx->new_<Vector<DependentAsmJSModuleExit> >(cx);
        if (!dependentAsmJSModules_)
            return false;
    }
    return dependentAsmJSModules_->append(exit);
}

void
BaselineScript::removeDependentAsmJSModule(DependentAsmJSModuleExit exit)
{
    if (!dependentAsmJSModules_)
        return;

    for (size_t i = 0; i < dependentAsmJSModules_->length(); i++) {
        if ((*dependentAsmJSModules_)[i].module == exit.module &&
            (*dependentAsmJSModules_)[i].exitIndex == exit.exitIndex)
        {
            dependentAsmJSModules_->erase(dependentAsmJSModules_->begin() + i);
            break;
        }
    }
}

ICEntry &
BaselineScript::icEntry(size_t index)
{
    MOZ_ASSERT(index < numICEntries());
    return icEntryList()[index];
}

PCMappingIndexEntry &
BaselineScript::pcMappingIndexEntry(size_t index)
{
    MOZ_ASSERT(index < numPCMappingIndexEntries());
    return pcMappingIndexEntryList()[index];
}

CompactBufferReader
BaselineScript::pcMappingReader(size_t indexEntry)
{
    PCMappingIndexEntry &entry = pcMappingIndexEntry(indexEntry);

    uint8_t *dataStart = pcMappingData() + entry.bufferOffset;
    uint8_t *dataEnd = (indexEntry == numPCMappingIndexEntries() - 1)
        ? pcMappingData() + pcMappingSize_
        : pcMappingData() + pcMappingIndexEntry(indexEntry + 1).bufferOffset;

    return CompactBufferReader(dataStart, dataEnd);
}

ICEntry &
BaselineScript::icEntryFromReturnOffset(CodeOffsetLabel returnOffset)
{
    size_t bottom = 0;
    size_t top = numICEntries();
    size_t mid = bottom + (top - bottom) / 2;
    while (mid < top) {
        ICEntry &midEntry = icEntry(mid);
        if (midEntry.returnOffset().offset() < returnOffset.offset())
            bottom = mid + 1;
        else
            top = mid;
        mid = bottom + (top - bottom) / 2;
    }

    MOZ_ASSERT(mid < numICEntries());
    MOZ_ASSERT(icEntry(mid).returnOffset().offset() == returnOffset.offset());

    return icEntry(mid);
}

uint8_t *
BaselineScript::returnAddressForIC(const ICEntry &ent)
{
    return method()->raw() + ent.returnOffset().offset();
}

static inline size_t
ComputeBinarySearchMid(BaselineScript *baseline, uint32_t pcOffset)
{
    size_t bottom = 0;
    size_t top = baseline->numICEntries();
    size_t mid = bottom + (top - bottom) / 2;
    while (mid < top) {
        ICEntry &midEntry = baseline->icEntry(mid);
        if (midEntry.pcOffset() < pcOffset)
            bottom = mid + 1;
        else if (midEntry.pcOffset() > pcOffset)
            top = mid;
        else
            break;
        mid = bottom + (top - bottom) / 2;
    }
    return mid;
}

ICEntry &
BaselineScript::icEntryFromPCOffset(uint32_t pcOffset)
{
    
    
    size_t mid = ComputeBinarySearchMid(this, pcOffset);

    
    
    
    for (size_t i = mid; i < numICEntries() && icEntry(i).pcOffset() == pcOffset; i--) {
        if (icEntry(i).isForOp())
            return icEntry(i);
    }
    for (size_t i = mid+1; i < numICEntries() && icEntry(i).pcOffset() == pcOffset; i++) {
        if (icEntry(i).isForOp())
            return icEntry(i);
    }
    MOZ_CRASH("Invalid PC offset for IC entry.");
}

ICEntry &
BaselineScript::icEntryFromPCOffset(uint32_t pcOffset, ICEntry *prevLookedUpEntry)
{
    
    
    if (prevLookedUpEntry && pcOffset >= prevLookedUpEntry->pcOffset() &&
        (pcOffset - prevLookedUpEntry->pcOffset()) <= 10)
    {
        ICEntry *firstEntry = &icEntry(0);
        ICEntry *lastEntry = &icEntry(numICEntries() - 1);
        ICEntry *curEntry = prevLookedUpEntry;
        while (curEntry >= firstEntry && curEntry <= lastEntry) {
            if (curEntry->pcOffset() == pcOffset && curEntry->isForOp())
                break;
            curEntry++;
        }
        MOZ_ASSERT(curEntry->pcOffset() == pcOffset && curEntry->isForOp());
        return *curEntry;
    }

    return icEntryFromPCOffset(pcOffset);
}

ICEntry &
BaselineScript::callVMEntryFromPCOffset(uint32_t pcOffset)
{
    
    
    size_t mid = ComputeBinarySearchMid(this, pcOffset);

    for (size_t i = mid; i < numICEntries() && icEntry(i).pcOffset() == pcOffset; i--) {
        if (icEntry(i).kind() == ICEntry::Kind_CallVM)
            return icEntry(i);
    }
    for (size_t i = mid+1; i < numICEntries() && icEntry(i).pcOffset() == pcOffset; i++) {
        if (icEntry(i).kind() == ICEntry::Kind_CallVM)
            return icEntry(i);
    }
    MOZ_CRASH("Invalid PC offset for callVM entry.");
}

ICEntry &
BaselineScript::stackCheckICEntry(bool earlyCheck)
{
    
    
    
    
    ICEntry::Kind kind = earlyCheck ? ICEntry::Kind_EarlyStackCheck : ICEntry::Kind_StackCheck;
    for (size_t i = 0; i < numICEntries() && icEntry(i).pcOffset() == 0; i++) {
        if (icEntry(i).kind() == kind)
            return icEntry(i);
    }
    MOZ_CRASH("No stack check ICEntry found.");
}

ICEntry &
BaselineScript::icEntryFromReturnAddress(uint8_t *returnAddr)
{
    MOZ_ASSERT(returnAddr > method_->raw());
    MOZ_ASSERT(returnAddr < method_->raw() + method_->instructionsSize());
    CodeOffsetLabel offset(returnAddr - method_->raw());
    return icEntryFromReturnOffset(offset);
}

void
BaselineScript::copyYieldEntries(JSScript *script, Vector<uint32_t> &yieldOffsets)
{
    uint8_t **entries = yieldEntryList();

    for (size_t i = 0; i < yieldOffsets.length(); i++) {
        uint32_t offset = yieldOffsets[i];
        entries[i] = nativeCodeForPC(script, script->offsetToPC(offset));
    }
}

void
BaselineScript::copyICEntries(JSScript *script, const ICEntry *entries, MacroAssembler &masm)
{
    
    
    for (uint32_t i = 0; i < numICEntries(); i++) {
        ICEntry &realEntry = icEntry(i);
        realEntry = entries[i];
        realEntry.fixupReturnOffset(masm);

        if (!realEntry.hasStub()) {
            
            continue;
        }

        
        
        if (realEntry.firstStub()->isFallback())
            realEntry.firstStub()->toFallbackStub()->fixupICEntry(&realEntry);

        if (realEntry.firstStub()->isTypeMonitor_Fallback()) {
            ICTypeMonitor_Fallback *stub = realEntry.firstStub()->toTypeMonitor_Fallback();
            stub->fixupICEntry(&realEntry);
        }

        if (realEntry.firstStub()->isTableSwitch()) {
            ICTableSwitch *stub = realEntry.firstStub()->toTableSwitch();
            stub->fixupJumpTable(script, this);
        }
    }
}

void
BaselineScript::adoptFallbackStubs(FallbackICStubSpace *stubSpace)
{
    fallbackStubSpace_.adoptFrom(stubSpace);
}

void
BaselineScript::copyPCMappingEntries(const CompactBufferWriter &entries)
{
    MOZ_ASSERT(entries.length() > 0);
    MOZ_ASSERT(entries.length() == pcMappingSize_);

    memcpy(pcMappingData(), entries.buffer(), entries.length());
}

void
BaselineScript::copyPCMappingIndexEntries(const PCMappingIndexEntry *entries)
{
    for (uint32_t i = 0; i < numPCMappingIndexEntries(); i++)
        pcMappingIndexEntry(i) = entries[i];
}

uint8_t *
BaselineScript::nativeCodeForPC(JSScript *script, jsbytecode *pc, PCMappingSlotInfo *slotInfo)
{
    MOZ_ASSERT_IF(script->hasBaselineScript(), script->baselineScript() == this);

    uint32_t pcOffset = script->pcToOffset(pc);

    
    
    uint32_t i = 1;
    for (; i < numPCMappingIndexEntries(); i++) {
        if (pcMappingIndexEntry(i).pcOffset > pcOffset)
            break;
    }

    
    MOZ_ASSERT(i > 0);
    i--;

    PCMappingIndexEntry &entry = pcMappingIndexEntry(i);
    MOZ_ASSERT(pcOffset >= entry.pcOffset);

    CompactBufferReader reader(pcMappingReader(i));
    jsbytecode *curPC = script->offsetToPC(entry.pcOffset);
    uint32_t nativeOffset = entry.nativeOffset;

    MOZ_ASSERT(script->containsPC(curPC));
    MOZ_ASSERT(curPC <= pc);

    while (reader.more()) {
        
        
        uint8_t b = reader.readByte();
        if (b & 0x80)
            nativeOffset += reader.readUnsigned();

        if (curPC == pc) {
            if (slotInfo)
                *slotInfo = PCMappingSlotInfo(b & ~0x80);
            return method_->raw() + nativeOffset;
        }

        curPC += GetBytecodeLength(curPC);
    }

    MOZ_CRASH("No native code for this pc");
}

jsbytecode *
BaselineScript::approximatePcForNativeAddress(JSScript *script, uint8_t *nativeAddress)
{
    MOZ_ASSERT(script->baselineScript() == this);
    MOZ_ASSERT(nativeAddress >= method_->raw());
    MOZ_ASSERT(nativeAddress < method_->raw() + method_->instructionsSize());

    uint32_t nativeOffset = nativeAddress - method_->raw();
    MOZ_ASSERT(nativeOffset < method_->instructionsSize());

    
    
    uint32_t i = 1;
    for (; i < numPCMappingIndexEntries(); i++) {
        if (pcMappingIndexEntry(i).nativeOffset > nativeOffset)
            break;
    }

    
    MOZ_ASSERT(i > 0);
    i--;

    PCMappingIndexEntry &entry = pcMappingIndexEntry(i);

    CompactBufferReader reader(pcMappingReader(i));
    jsbytecode *curPC = script->offsetToPC(entry.pcOffset);
    uint32_t curNativeOffset = entry.nativeOffset;

    MOZ_ASSERT(script->containsPC(curPC));

    
    
    if (curNativeOffset > nativeOffset)
        return script->code();

    jsbytecode *lastPC = curPC;
    while (true) {
        
        
        uint8_t b = reader.readByte();
        if (b & 0x80)
            curNativeOffset += reader.readUnsigned();

        
        
        
        
        if (curNativeOffset > nativeOffset)
            return lastPC;

        
        
        if (!reader.more())
            return curPC;

        lastPC = curPC;
        curPC += GetBytecodeLength(curPC);
    }
}

void
BaselineScript::toggleDebugTraps(JSScript *script, jsbytecode *pc)
{
    MOZ_ASSERT(script->baselineScript() == this);

    
    if (!hasDebugInstrumentation())
        return;

    SrcNoteLineScanner scanner(script->notes(), script->lineno());

    for (uint32_t i = 0; i < numPCMappingIndexEntries(); i++) {
        PCMappingIndexEntry &entry = pcMappingIndexEntry(i);

        CompactBufferReader reader(pcMappingReader(i));
        jsbytecode *curPC = script->offsetToPC(entry.pcOffset);
        uint32_t nativeOffset = entry.nativeOffset;

        MOZ_ASSERT(script->containsPC(curPC));

        while (reader.more()) {
            uint8_t b = reader.readByte();
            if (b & 0x80)
                nativeOffset += reader.readUnsigned();

            scanner.advanceTo(script->pcToOffset(curPC));

            if (!pc || pc == curPC) {
                bool enabled = (script->stepModeEnabled() && scanner.isLineHeader()) ||
                    script->hasBreakpointsAt(curPC);

                
                CodeLocationLabel label(method(), CodeOffsetLabel(nativeOffset));
                Assembler::ToggleCall(label, enabled);
            }

            curPC += GetBytecodeLength(curPC);
        }
    }
}

#ifdef JS_TRACE_LOGGING
void
BaselineScript::initTraceLogger(JSRuntime *runtime, JSScript *script)
{
#ifdef DEBUG
    traceLoggerScriptsEnabled_ = TraceLogTextIdEnabled(TraceLogger_Scripts);
    traceLoggerEngineEnabled_ = TraceLogTextIdEnabled(TraceLogger_Engine);
#endif

    TraceLoggerThread *logger = TraceLoggerForMainThread(runtime);
    if (TraceLogTextIdEnabled(TraceLogger_Scripts))
        traceLoggerScriptEvent_ = TraceLoggerEvent(logger, TraceLogger_Scripts, script);
    else
        traceLoggerScriptEvent_ = TraceLoggerEvent(logger, TraceLogger_Scripts);

    if (TraceLogTextIdEnabled(TraceLogger_Engine) || TraceLogTextIdEnabled(TraceLogger_Scripts)) {
        CodeLocationLabel enter(method_, CodeOffsetLabel(traceLoggerEnterToggleOffset_));
        CodeLocationLabel exit(method_, CodeOffsetLabel(traceLoggerExitToggleOffset_));
        Assembler::ToggleToCmp(enter);
        Assembler::ToggleToCmp(exit);
    }
}

void
BaselineScript::toggleTraceLoggerScripts(JSRuntime *runtime, JSScript *script, bool enable)
{
    bool engineEnabled = TraceLogTextIdEnabled(TraceLogger_Engine);

    MOZ_ASSERT(enable == !traceLoggerScriptsEnabled_);
    MOZ_ASSERT(engineEnabled == traceLoggerEngineEnabled_);

    
    
    TraceLoggerThread *logger = TraceLoggerForMainThread(runtime);
    if (enable)
        traceLoggerScriptEvent_ = TraceLoggerEvent(logger, TraceLogger_Scripts, script);
    else
        traceLoggerScriptEvent_ = TraceLoggerEvent(logger, TraceLogger_Scripts);

    
    CodeLocationLabel enter(method_, CodeOffsetLabel(traceLoggerEnterToggleOffset_));
    CodeLocationLabel exit(method_, CodeOffsetLabel(traceLoggerExitToggleOffset_));
    if (!engineEnabled) {
        if (enable) {
            Assembler::ToggleToCmp(enter);
            Assembler::ToggleToCmp(exit);
        } else {
            Assembler::ToggleToJmp(enter);
            Assembler::ToggleToJmp(exit);
        }
    }

#if DEBUG
    traceLoggerScriptsEnabled_ = enable;
#endif
}

void
BaselineScript::toggleTraceLoggerEngine(bool enable)
{
    bool scriptsEnabled = TraceLogTextIdEnabled(TraceLogger_Scripts);

    MOZ_ASSERT(enable == !traceLoggerEngineEnabled_);
    MOZ_ASSERT(scriptsEnabled == traceLoggerScriptsEnabled_);

    
    CodeLocationLabel enter(method_, CodeOffsetLabel(traceLoggerEnterToggleOffset_));
    CodeLocationLabel exit(method_, CodeOffsetLabel(traceLoggerExitToggleOffset_));
    if (!scriptsEnabled) {
        if (enable) {
            Assembler::ToggleToCmp(enter);
            Assembler::ToggleToCmp(exit);
        } else {
            Assembler::ToggleToJmp(enter);
            Assembler::ToggleToJmp(exit);
        }
    }

#if DEBUG
    traceLoggerEngineEnabled_ = enable;
#endif
}
#endif

void
BaselineScript::toggleProfilerInstrumentation(bool enable)
{
    if (enable == isProfilerInstrumentationOn())
        return;

    JitSpew(JitSpew_BaselineIC, "  toggling profiling %s for BaselineScript %p",
            enable ? "on" : "off", this);

    
    CodeLocationLabel enterToggleLocation(method_, CodeOffsetLabel(profilerEnterToggleOffset_));
    CodeLocationLabel exitToggleLocation(method_, CodeOffsetLabel(profilerExitToggleOffset_));
    if (enable) {
        Assembler::ToggleToCmp(enterToggleLocation);
        Assembler::ToggleToCmp(exitToggleLocation);
        flags_ |= uint32_t(PROFILER_INSTRUMENTATION_ON);
    } else {
        Assembler::ToggleToJmp(enterToggleLocation);
        Assembler::ToggleToJmp(exitToggleLocation);
        flags_ &= ~uint32_t(PROFILER_INSTRUMENTATION_ON);
    }
}

void
BaselineScript::purgeOptimizedStubs(Zone *zone)
{
    JitSpew(JitSpew_BaselineIC, "Purging optimized stubs");

    for (size_t i = 0; i < numICEntries(); i++) {
        ICEntry &entry = icEntry(i);
        if (!entry.hasStub())
            continue;

        ICStub *lastStub = entry.firstStub();
        while (lastStub->next())
            lastStub = lastStub->next();

        if (lastStub->isFallback()) {
            
            ICStub *stub = entry.firstStub();
            ICStub *prev = nullptr;

            while (stub->next()) {
                if (!stub->allocatedInFallbackSpace()) {
                    lastStub->toFallbackStub()->unlinkStub(zone, prev, stub);
                    stub = stub->next();
                    continue;
                }

                prev = stub;
                stub = stub->next();
            }

            if (lastStub->isMonitoredFallback()) {
                
                
                ICTypeMonitor_Fallback *lastMonStub =
                    lastStub->toMonitoredFallbackStub()->fallbackMonitorStub();
                lastMonStub->resetMonitorStubChain(zone);
            }
        } else if (lastStub->isTypeMonitor_Fallback()) {
            lastStub->toTypeMonitor_Fallback()->resetMonitorStubChain(zone);
        } else {
            MOZ_ASSERT(lastStub->isTableSwitch());
        }
    }

#ifdef DEBUG
    
    for (size_t i = 0; i < numICEntries(); i++) {
        ICEntry &entry = icEntry(i);
        if (!entry.hasStub())
            continue;

        ICStub *stub = entry.firstStub();
        while (stub->next()) {
            MOZ_ASSERT(stub->allocatedInFallbackSpace());
            stub = stub->next();
        }
    }
#endif
}

void
jit::FinishDiscardBaselineScript(FreeOp *fop, JSScript *script)
{
    if (!script->hasBaselineScript())
        return;

    if (script->baselineScript()->active()) {
        
        
        script->baselineScript()->purgeOptimizedStubs(script->zone());

        
        
        script->baselineScript()->resetActive();

        
        
        script->baselineScript()->clearIonCompiledOrInlined();
        return;
    }

    BaselineScript *baseline = script->baselineScript();
    script->setBaselineScript(nullptr, nullptr);
    BaselineScript::Destroy(fop, baseline);
}

void
jit::AddSizeOfBaselineData(JSScript *script, mozilla::MallocSizeOf mallocSizeOf, size_t *data,
                           size_t *fallbackStubs)
{
    if (script->hasBaselineScript())
        script->baselineScript()->addSizeOfIncludingThis(mallocSizeOf, data, fallbackStubs);
}

void
jit::ToggleBaselineProfiling(JSRuntime *runtime, bool enable)
{
    for (ZonesIter zone(runtime, SkipAtoms); !zone.done(); zone.next()) {
        for (gc::ZoneCellIter i(zone, gc::AllocKind::SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            if (!script->hasBaselineScript())
                continue;
            script->baselineScript()->toggleProfilerInstrumentation(enable);
        }
    }
}

#ifdef JS_TRACE_LOGGING
void
jit::ToggleBaselineTraceLoggerScripts(JSRuntime *runtime, bool enable)
{
    for (ZonesIter zone(runtime, SkipAtoms); !zone.done(); zone.next()) {
        for (gc::ZoneCellIter i(zone, gc::AllocKind::SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            if (!script->hasBaselineScript())
                continue;
            script->baselineScript()->toggleTraceLoggerScripts(runtime, script, enable);
        }
    }
}

void
jit::ToggleBaselineTraceLoggerEngine(JSRuntime *runtime, bool enable)
{
    for (ZonesIter zone(runtime, SkipAtoms); !zone.done(); zone.next()) {
        for (gc::ZoneCellIter i(zone, gc::AllocKind::SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            if (!script->hasBaselineScript())
                continue;
            script->baselineScript()->toggleTraceLoggerEngine(enable);
        }
    }
}
#endif

static void
MarkActiveBaselineScripts(JSRuntime *rt, const JitActivationIterator &activation)
{
    for (jit::JitFrameIterator iter(activation); !iter.done(); ++iter) {
        switch (iter.type()) {
          case JitFrame_BaselineJS:
            iter.script()->baselineScript()->setActive();
            break;
          case JitFrame_Bailout:
          case JitFrame_IonJS: {
            
            
            iter.script()->baselineScript()->setActive();
            for (InlineFrameIterator inlineIter(rt, &iter); inlineIter.more(); ++inlineIter)
                inlineIter.script()->baselineScript()->setActive();
            break;
          }
          default:;
        }
    }
}

void
jit::MarkActiveBaselineScripts(Zone *zone)
{
    JSRuntime *rt = zone->runtimeFromMainThread();
    for (JitActivationIterator iter(rt); !iter.done(); ++iter) {
        if (iter->compartment()->zone() == zone)
            MarkActiveBaselineScripts(rt, iter);
    }
}
