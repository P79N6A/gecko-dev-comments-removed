





#include "mozilla/MemoryReporting.h"

#include "ion/BaselineCompiler.h"
#include "ion/BaselineIC.h"
#include "ion/BaselineJIT.h"
#include "ion/CompileInfo.h"
#include "ion/IonSpewer.h"
#include "ion/IonFrames-inl.h"

#include "vm/Stack-inl.h"

#include "jsopcodeinlines.h"

using namespace js;
using namespace js::ion;

 PCMappingSlotInfo::SlotLocation
PCMappingSlotInfo::ToSlotLocation(const StackValue *stackVal)
{
    if (stackVal->kind() == StackValue::Register) {
        if (stackVal->reg() == R0)
            return SlotInR0;
        JS_ASSERT(stackVal->reg() == R1);
        return SlotInR1;
    }
    JS_ASSERT(stackVal->kind() != StackValue::Stack);
    return SlotIgnore;
}

BaselineScript::BaselineScript(uint32_t prologueOffset, uint32_t spsPushToggleOffset)
  : method_(NULL),
    fallbackStubSpace_(),
    prologueOffset_(prologueOffset),
#ifdef DEBUG
    spsOn_(false),
#endif
    spsPushToggleOffset_(spsPushToggleOffset),
    flags_(0)
{ }

static const size_t BASELINE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 4096;
static const unsigned BASELINE_MAX_ARGS_LENGTH = 20000;

static bool
CheckFrame(StackFrame *fp)
{
    if (fp->isGeneratorFrame()) {
        IonSpew(IonSpew_BaselineAbort, "generator frame");
        return false;
    }

    if (fp->isDebuggerFrame()) {
        
        
        IonSpew(IonSpew_BaselineAbort, "debugger frame");
        return false;
    }

    if (fp->isNonEvalFunctionFrame() && fp->numActualArgs() > BASELINE_MAX_ARGS_LENGTH) {
        
        IonSpew(IonSpew_BaselineAbort, "Too many arguments (%u)", fp->numActualArgs());
        return false;
    }

    return true;
}

static bool
IsJSDEnabled(JSContext *cx)
{
    return cx->compartment()->debugMode() && cx->runtime()->debugHooks.callHook;
}

static IonExecStatus
EnterBaseline(JSContext *cx, EnterJitData &data)
{
    JS_CHECK_RECURSION(cx, return IonExec_Aborted);
    JS_ASSERT(ion::IsBaselineEnabled(cx));
    JS_ASSERT_IF(data.osrFrame, CheckFrame(data.osrFrame));

    EnterIonCode enter = cx->compartment()->ionCompartment()->enterBaselineJIT();

    
    JS_ASSERT_IF(data.constructing, data.maxArgv[0].isObject());

    data.result.setInt32(data.numActualArgs);
    {
        AssertCompartmentUnchanged pcc(cx);
        IonContext ictx(cx, NULL);
        JitActivation activation(cx, data.constructing);
        JSAutoResolveFlags rf(cx, RESOLVE_INFER);
        AutoFlushInhibitor afi(cx->compartment()->ionCompartment());

        if (data.osrFrame)
            data.osrFrame->setRunningInJit();

        JS_ASSERT_IF(data.osrFrame, !IsJSDEnabled(cx));

        
        enter(data.jitcode, data.maxArgc, data.maxArgv, data.osrFrame, data.calleeToken,
              data.scopeChain, data.osrNumStackValues, data.result.address());

        if (data.osrFrame)
            data.osrFrame->clearRunningInJit();
    }

    JS_ASSERT(!cx->runtime()->hasIonReturnOverride());

    
    if (!data.result.isMagic() && data.constructing && data.result.isPrimitive())
        data.result = data.maxArgv[0];

    
    cx->runtime()->getIonRuntime(cx)->freeOsrTempData();

    JS_ASSERT_IF(data.result.isMagic(), data.result.isMagic(JS_ION_ERROR));
    return data.result.isMagic() ? IonExec_Error : IonExec_Ok;
}

IonExecStatus
ion::EnterBaselineMethod(JSContext *cx, RunState &state)
{
    BaselineScript *baseline = state.script()->baselineScript();

    EnterJitData data(cx);
    data.jitcode = baseline->method()->raw();

    AutoValueVector vals(cx);
    if (!SetEnterJitData(cx, data, state, vals))
        return IonExec_Error;

    IonExecStatus status = EnterBaseline(cx, data);
    if (status != IonExec_Ok)
        return status;

    state.setReturnValue(data.result);
    return IonExec_Ok;
}

IonExecStatus
ion::EnterBaselineAtBranch(JSContext *cx, StackFrame *fp, jsbytecode *pc)
{
    JS_ASSERT(JSOp(*pc) == JSOP_LOOPENTRY);

    BaselineScript *baseline = fp->script()->baselineScript();

    EnterJitData data(cx);
    data.jitcode = baseline->nativeCodeForPC(fp->script(), pc);

    
    
    if (cx->compartment()->debugMode())
        data.jitcode += MacroAssembler::ToggledCallSize();

    data.osrFrame = fp;
    data.osrNumStackValues = fp->script()->nfixed + cx->interpreterRegs().stackDepth();

    RootedValue thisv(cx);

    if (fp->isNonEvalFunctionFrame()) {
        data.constructing = fp->isConstructing();
        data.numActualArgs = fp->numActualArgs();
        data.maxArgc = Max(fp->numActualArgs(), fp->numFormalArgs()) + 1; 
        data.maxArgv = fp->argv() - 1; 
        data.scopeChain = NULL;
        data.calleeToken = CalleeToToken(&fp->callee());
    } else {
        thisv = fp->thisValue();
        data.constructing = false;
        data.numActualArgs = 0;
        data.maxArgc = 1;
        data.maxArgv = thisv.address();
        data.scopeChain = fp->scopeChain();

        
        if (fp->isFunctionFrame())
            data.calleeToken = CalleeToToken(&fp->callee());
        else
            data.calleeToken = CalleeToToken(fp->script());
    }

#if JS_TRACE_LOGGING
    TraceLogging::defaultLogger()->log(TraceLogging::INFO_ENGINE_BASELINE);
#endif

    IonExecStatus status = EnterBaseline(cx, data);
    if (status != IonExec_Ok)
        return status;

    fp->setReturnValue(data.result);
    return IonExec_Ok;
}

static MethodStatus
BaselineCompile(JSContext *cx, HandleScript script)
{
    JS_ASSERT(!script->hasBaselineScript());
    JS_ASSERT(script->canBaselineCompile());

    LifoAlloc alloc(BASELINE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE);

    TempAllocator *temp = alloc.new_<TempAllocator>(&alloc);
    if (!temp)
        return Method_Error;

    IonContext ictx(cx, temp);

    BaselineCompiler compiler(cx, script);
    if (!compiler.init())
        return Method_Error;

    AutoFlushCache afc("BaselineJIT", cx->runtime()->ionRuntime());
    MethodStatus status = compiler.compile();

    JS_ASSERT_IF(status == Method_Compiled, script->hasBaselineScript());
    JS_ASSERT_IF(status != Method_Compiled, !script->hasBaselineScript());

    if (status == Method_CantCompile)
        script->setBaselineScript(BASELINE_DISABLED_SCRIPT);

    return status;
}

static MethodStatus
CanEnterBaselineJIT(JSContext *cx, HandleScript script, bool osr)
{
    JS_ASSERT(ion::IsBaselineEnabled(cx));

    
    if (!script->canBaselineCompile())
        return Method_Skipped;

    if (script->length > BaselineScript::MAX_JSSCRIPT_LENGTH)
        return Method_CantCompile;

    if (!cx->compartment()->ensureIonCompartmentExists(cx))
        return Method_Error;

    if (script->hasBaselineScript())
        return Method_Compiled;

    
    
    
    
    
    
    
    
    
    if (IsJSDEnabled(cx) || cx->runtime()->parallelWarmup > 0) {
        if (osr)
            return Method_Skipped;
    } else if (script->incUseCount() <= js_IonOptions.baselineUsesBeforeCompile) {
        return Method_Skipped;
    }

    if (script->isCallsiteClone) {
        
        
        RootedScript original(cx, script->originalFunction()->nonLazyScript());
        JS_ASSERT(original != script);

        if (!original->canBaselineCompile())
            return Method_CantCompile;

        if (!original->hasBaselineScript()) {
            MethodStatus status = BaselineCompile(cx, original);
            if (status != Method_Compiled)
                return status;
        }
    }

    return BaselineCompile(cx, script);
}

MethodStatus
ion::CanEnterBaselineAtBranch(JSContext *cx, StackFrame *fp, bool newType)
{
   
   if (fp->isConstructing() && fp->functionThis().isPrimitive()) {
       RootedObject callee(cx, &fp->callee());
       RootedObject obj(cx, CreateThisForFunction(cx, callee, newType));
       if (!obj)
           return Method_Skipped;
       fp->functionThis().setObject(*obj);
   }

   if (!CheckFrame(fp))
       return Method_CantCompile;

   RootedScript script(cx, fp->script());
   return CanEnterBaselineJIT(cx, script, true);
}

MethodStatus
ion::CanEnterBaselineMethod(JSContext *cx, RunState &state)
{
    if (state.isInvoke()) {
        InvokeState &invoke = *state.asInvoke();

        if (invoke.args().length() > BASELINE_MAX_ARGS_LENGTH) {
            IonSpew(IonSpew_BaselineAbort, "Too many arguments (%u)", invoke.args().length());
            return Method_CantCompile;
        }

        
        if (invoke.constructing() && invoke.args().thisv().isPrimitive()) {
            RootedObject callee(cx, &invoke.args().callee());
            RootedObject obj(cx, CreateThisForFunction(cx, callee, invoke.useNewType()));
            if (!obj)
                return Method_Skipped;
            invoke.args().setThis(ObjectValue(*obj));
        }
    } else if (state.isExecute()) {
        ExecuteType type = state.asExecute()->type();
        if (type == EXECUTE_DEBUG || type == EXECUTE_DEBUG_GLOBAL) {
            IonSpew(IonSpew_BaselineAbort, "debugger frame");
            return Method_CantCompile;
        }
    } else {
        JS_ASSERT(state.isGenerator());
        IonSpew(IonSpew_BaselineAbort, "generator frame");
        return Method_CantCompile;
    }

    RootedScript script(cx, state.script());
    return CanEnterBaselineJIT(cx, script, false);
};


static const unsigned DataAlignment = sizeof(uintptr_t);

BaselineScript *
BaselineScript::New(JSContext *cx, uint32_t prologueOffset,
                    uint32_t spsPushToggleOffset, size_t icEntries,
                    size_t pcMappingIndexEntries, size_t pcMappingSize)
{
    size_t paddedBaselineScriptSize = AlignBytes(sizeof(BaselineScript), DataAlignment);

    size_t icEntriesSize = icEntries * sizeof(ICEntry);
    size_t pcMappingIndexEntriesSize = pcMappingIndexEntries * sizeof(PCMappingIndexEntry);

    size_t paddedICEntriesSize = AlignBytes(icEntriesSize, DataAlignment);
    size_t paddedPCMappingIndexEntriesSize = AlignBytes(pcMappingIndexEntriesSize, DataAlignment);
    size_t paddedPCMappingSize = AlignBytes(pcMappingSize, DataAlignment);

    size_t allocBytes = paddedBaselineScriptSize +
        paddedICEntriesSize +
        paddedPCMappingIndexEntriesSize +
        paddedPCMappingSize;

    uint8_t *buffer = (uint8_t *)cx->malloc_(allocBytes);
    if (!buffer)
        return NULL;

    BaselineScript *script = reinterpret_cast<BaselineScript *>(buffer);
    new (script) BaselineScript(prologueOffset, spsPushToggleOffset);

    size_t offsetCursor = paddedBaselineScriptSize;

    script->icEntriesOffset_ = offsetCursor;
    script->icEntries_ = icEntries;
    offsetCursor += paddedICEntriesSize;

    script->pcMappingIndexOffset_ = offsetCursor;
    script->pcMappingIndexEntries_ = pcMappingIndexEntries;
    offsetCursor += paddedPCMappingIndexEntriesSize;

    script->pcMappingOffset_ = offsetCursor;
    script->pcMappingSize_ = pcMappingSize;
    offsetCursor += paddedPCMappingSize;

    return script;
}

void
BaselineScript::trace(JSTracer *trc)
{
    MarkIonCode(trc, &method_, "baseline-method");

    
    for (size_t i = 0; i < numICEntries(); i++) {
        ICEntry &ent = icEntry(i);
        if (!ent.hasStub())
            continue;
        for (ICStub *stub = ent.firstStub(); stub; stub = stub->next())
            stub->trace(trc);
    }
}

void
BaselineScript::Trace(JSTracer *trc, BaselineScript *script)
{
    script->trace(trc);
}

void
BaselineScript::Destroy(FreeOp *fop, BaselineScript *script)
{
    fop->delete_(script);
}

ICEntry &
BaselineScript::icEntry(size_t index)
{
    JS_ASSERT(index < numICEntries());
    return icEntryList()[index];
}

PCMappingIndexEntry &
BaselineScript::pcMappingIndexEntry(size_t index)
{
    JS_ASSERT(index < numPCMappingIndexEntries());
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

ICEntry *
BaselineScript::maybeICEntryFromReturnOffset(CodeOffsetLabel returnOffset)
{
    size_t bottom = 0;
    size_t top = numICEntries();
    size_t mid = (bottom + top) / 2;
    while (mid < top) {
        ICEntry &midEntry = icEntry(mid);
        if (midEntry.returnOffset().offset() < returnOffset.offset())
            bottom = mid + 1;
        else 
            top = mid;
        mid = (bottom + top) / 2;
    }
    if (mid >= numICEntries())
        return NULL;

    if (icEntry(mid).returnOffset().offset() != returnOffset.offset())
        return NULL;

    return &icEntry(mid);
}

ICEntry &
BaselineScript::icEntryFromReturnOffset(CodeOffsetLabel returnOffset)
{
    ICEntry *result = maybeICEntryFromReturnOffset(returnOffset);
    JS_ASSERT(result);
    return *result;
}

uint8_t *
BaselineScript::returnAddressForIC(const ICEntry &ent)
{
    return method()->raw() + ent.returnOffset().offset();
}

ICEntry &
BaselineScript::icEntryFromPCOffset(uint32_t pcOffset)
{
    
    
    size_t bottom = 0;
    size_t top = numICEntries();
    size_t mid = (bottom + top) / 2;
    while (mid < top) {
        ICEntry &midEntry = icEntry(mid);
        if (midEntry.pcOffset() < pcOffset)
            bottom = mid + 1;
        else if (midEntry.pcOffset() > pcOffset)
            top = mid;
        else
            break;
        mid = (bottom + top) / 2;
    }
    
    
    
    for (size_t i = mid; i < numICEntries() && icEntry(i).pcOffset() == pcOffset; i--) {
        if (icEntry(i).isForOp())
            return icEntry(i);
    }
    for (size_t i = mid+1; i < numICEntries() && icEntry(i).pcOffset() == pcOffset; i++) {
        if (icEntry(i).isForOp())
            return icEntry(i);
    }
    MOZ_ASSUME_UNREACHABLE("Invalid PC offset for IC entry.");
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
        JS_ASSERT(curEntry->pcOffset() == pcOffset && curEntry->isForOp());
        return *curEntry;
    }

    return icEntryFromPCOffset(pcOffset);
}

ICEntry *
BaselineScript::maybeICEntryFromReturnAddress(uint8_t *returnAddr)
{
    JS_ASSERT(returnAddr > method_->raw());
    JS_ASSERT(returnAddr < method_->raw() + method_->instructionsSize());
    CodeOffsetLabel offset(returnAddr - method_->raw());
    return maybeICEntryFromReturnOffset(offset);
}

ICEntry &
BaselineScript::icEntryFromReturnAddress(uint8_t *returnAddr)
{
    JS_ASSERT(returnAddr > method_->raw());
    JS_ASSERT(returnAddr < method_->raw() + method_->instructionsSize());
    CodeOffsetLabel offset(returnAddr - method_->raw());
    return icEntryFromReturnOffset(offset);
}

void
BaselineScript::copyICEntries(HandleScript script, const ICEntry *entries, MacroAssembler &masm)
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
    JS_ASSERT(entries.length() > 0);
    JS_ASSERT(entries.length() == pcMappingSize_);

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
    JS_ASSERT(script->baselineScript() == this);
    JS_ASSERT(pc >= script->code);
    JS_ASSERT(pc < script->code + script->length);

    uint32_t pcOffset = pc - script->code;

    
    
    uint32_t i = 1;
    for (; i < numPCMappingIndexEntries(); i++) {
        if (pcMappingIndexEntry(i).pcOffset > pcOffset)
            break;
    }

    
    JS_ASSERT(i > 0);
    i--;

    PCMappingIndexEntry &entry = pcMappingIndexEntry(i);
    JS_ASSERT(pcOffset >= entry.pcOffset);

    CompactBufferReader reader(pcMappingReader(i));
    jsbytecode *curPC = script->code + entry.pcOffset;
    uint32_t nativeOffset = entry.nativeOffset;

    JS_ASSERT(curPC >= script->code);
    JS_ASSERT(curPC <= pc);

    while (true) {
        
        
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

    MOZ_ASSUME_UNREACHABLE("Invalid pc");
}

jsbytecode *
BaselineScript::pcForReturnOffset(JSScript *script, uint32_t nativeOffset)
{
    JS_ASSERT(script->baselineScript() == this);
    JS_ASSERT(nativeOffset < method_->instructionsSize());

    
    
    uint32_t i = 1;
    for (; i < numPCMappingIndexEntries(); i++) {
        if (pcMappingIndexEntry(i).nativeOffset > nativeOffset)
            break;
    }

    
    JS_ASSERT(i > 0);
    i--;

    PCMappingIndexEntry &entry = pcMappingIndexEntry(i);
    JS_ASSERT(nativeOffset >= entry.nativeOffset);

    CompactBufferReader reader(pcMappingReader(i));
    jsbytecode *curPC = script->code + entry.pcOffset;
    uint32_t curNativeOffset = entry.nativeOffset;

    JS_ASSERT(curPC >= script->code);
    JS_ASSERT(curNativeOffset <= nativeOffset);

    while (true) {
        
        
        uint8_t b = reader.readByte();
        if (b & 0x80)
            curNativeOffset += reader.readUnsigned();

        if (curNativeOffset == nativeOffset)
            return curPC;

        curPC += GetBytecodeLength(curPC);
    }

    MOZ_ASSUME_UNREACHABLE("Invalid pc");
}

jsbytecode *
BaselineScript::pcForReturnAddress(JSScript *script, uint8_t *nativeAddress)
{
    JS_ASSERT(script->baselineScript() == this);
    JS_ASSERT(nativeAddress >= method_->raw());
    JS_ASSERT(nativeAddress < method_->raw() + method_->instructionsSize());
    return pcForReturnOffset(script, uint32_t(nativeAddress - method_->raw()));
}

void
BaselineScript::toggleDebugTraps(JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(script->baselineScript() == this);

    SrcNoteLineScanner scanner(script->notes(), script->lineno);

    IonContext ictx(script->compartment(), NULL);
    AutoFlushCache afc("DebugTraps");

    for (uint32_t i = 0; i < numPCMappingIndexEntries(); i++) {
        PCMappingIndexEntry &entry = pcMappingIndexEntry(i);

        CompactBufferReader reader(pcMappingReader(i));
        jsbytecode *curPC = script->code + entry.pcOffset;
        uint32_t nativeOffset = entry.nativeOffset;

        JS_ASSERT(curPC >= script->code);
        JS_ASSERT(curPC < script->code + script->length);

        while (reader.more()) {
            uint8_t b = reader.readByte();
            if (b & 0x80)
                nativeOffset += reader.readUnsigned();

            scanner.advanceTo(curPC - script->code);

            if (!pc || pc == curPC) {
                bool enabled = (script->stepModeEnabled() && scanner.isLineHeader()) ||
                    script->hasBreakpointsAt(curPC);

                
                CodeLocationLabel label(method(), nativeOffset);
                Assembler::ToggleCall(label, enabled);
            }

            curPC += GetBytecodeLength(curPC);
        }
    }
}

void
BaselineScript::toggleSPS(bool enable)
{
    JS_ASSERT(enable == !(bool)spsOn_);

    IonSpew(IonSpew_BaselineIC, "  toggling SPS %s for BaselineScript %p",
            enable ? "on" : "off", this);

    
    CodeLocationLabel pushToggleLocation(method_, CodeOffsetLabel(spsPushToggleOffset_));
    if (enable)
        Assembler::ToggleToCmp(pushToggleLocation);
    else
        Assembler::ToggleToJmp(pushToggleLocation);
#ifdef DEBUG
    spsOn_ = enable;
#endif
}

void
BaselineScript::purgeOptimizedStubs(Zone *zone)
{
    IonSpew(IonSpew_BaselineIC, "Purging optimized stubs");

    for (size_t i = 0; i < numICEntries(); i++) {
        ICEntry &entry = icEntry(i);
        if (!entry.hasStub())
            continue;

        ICStub *lastStub = entry.firstStub();
        while (lastStub->next())
            lastStub = lastStub->next();

        if (lastStub->isFallback()) {
            
            ICStub *stub = entry.firstStub();
            ICStub *prev = NULL;

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
            JS_ASSERT(lastStub->isTableSwitch());
        }
    }

#ifdef DEBUG
    
    for (size_t i = 0; i < numICEntries(); i++) {
        ICEntry &entry = icEntry(i);
        if (!entry.hasStub())
            continue;

        ICStub *stub = entry.firstStub();
        while (stub->next()) {
            JS_ASSERT(stub->allocatedInFallbackSpace());
            stub = stub->next();
        }
    }
#endif
}

void
ion::FinishDiscardBaselineScript(FreeOp *fop, JSScript *script)
{
    if (!script->hasBaselineScript())
        return;

    if (script->baselineScript()->active()) {
        
        
        script->baselineScript()->purgeOptimizedStubs(script->zone());

        
        
        script->baselineScript()->resetActive();
        return;
    }

    BaselineScript::Destroy(fop, script->baselineScript());
    script->setBaselineScript(NULL);
}

void
ion::IonCompartment::toggleBaselineStubBarriers(bool enabled)
{
    for (ICStubCodeMap::Enum e(*stubCodes_); !e.empty(); e.popFront()) {
        IonCode *code = *e.front().value.unsafeGet();
        code->togglePreBarriers(enabled);
    }
}

void
ion::SizeOfBaselineData(JSScript *script, mozilla::MallocSizeOf mallocSizeOf, size_t *data,
                        size_t *fallbackStubs)
{
    *data = 0;
    *fallbackStubs = 0;

    if (script->hasBaselineScript())
        script->baselineScript()->sizeOfIncludingThis(mallocSizeOf, data, fallbackStubs);
}

void
ion::ToggleBaselineSPS(JSRuntime *runtime, bool enable)
{
    for (ZonesIter zone(runtime); !zone.done(); zone.next()) {
        for (gc::CellIter i(zone, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            if (!script->hasBaselineScript())
                continue;
            script->baselineScript()->toggleSPS(enable);
        }
    }
}

static void
MarkActiveBaselineScripts(JSContext *cx, const JitActivationIterator &activation)
{
    for (ion::IonFrameIterator iter(activation); !iter.done(); ++iter) {
        switch (iter.type()) {
          case IonFrame_BaselineJS:
            iter.script()->baselineScript()->setActive();
            break;
          case IonFrame_OptimizedJS: {
            
            
            iter.script()->baselineScript()->setActive();
            for (InlineFrameIterator inlineIter(cx, &iter); inlineIter.more(); ++inlineIter)
                inlineIter.script()->baselineScript()->setActive();
            break;
          }
          default:;
        }
    }
}

void
ion::MarkActiveBaselineScripts(Zone *zone)
{
    
    
    JitActivationIterator iter(zone->rt);
    if (iter.done())
        return;

    
    JSContext *cx = GetIonContext()->cx;
    if (!ion::IsBaselineEnabled(cx))
        return;

    for (; !iter.done(); ++iter) {
        if (iter.activation()->compartment()->zone() == zone)
            MarkActiveBaselineScripts(cx, iter);
    }
}
