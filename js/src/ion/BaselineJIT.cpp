






#include "BaselineCompiler.h"
#include "BaselineIC.h"
#include "BaselineJIT.h"
#include "CompileInfo.h"
#include "IonSpewer.h"
#include "IonFrames-inl.h"

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

BaselineScript::BaselineScript(uint32_t prologueOffset)
  : method_(NULL),
    fallbackStubSpace_(),
    optimizedStubSpace_(),
    prologueOffset_(prologueOffset),
    flags_(0)
{ }

static const size_t BUILDER_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 1 << 12; 


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

    static const unsigned MAX_ARGS_LENGTH = 20000;

    if (fp->isNonEvalFunctionFrame() && fp->numActualArgs() > MAX_ARGS_LENGTH) {
        
        IonSpew(IonSpew_BaselineAbort, "Too many arguments (%u)", fp->numActualArgs());
        return false;
    }

    return true;
}

static bool
IsJSDEnabled(JSContext *cx)
{
    return cx->compartment->debugMode() && cx->runtime->debugHooks.callHook;
}

static IonExecStatus
EnterBaseline(JSContext *cx, StackFrame *fp, void *jitcode, bool osr)
{
    JS_CHECK_RECURSION(cx, return IonExec_Aborted);
    JS_ASSERT(ion::IsBaselineEnabled(cx));
    JS_ASSERT(CheckFrame(fp));

    EnterIonCode enter = cx->compartment->ionCompartment()->enterBaselineJIT();

    
    
    int maxArgc = 0;
    Value *maxArgv = NULL;
    int numActualArgs = 0;
    RootedValue thisv(cx);

    void *calleeToken;
    if (fp->isNonEvalFunctionFrame()) {
        
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
        
        if (fp->isFunctionFrame())
            calleeToken = CalleeToToken(&fp->callee());
        else
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

        
        JSObject *scopeChain = NULL;
        if (!fp->isNonEvalFunctionFrame())
            scopeChain = fp->scopeChain();

        
        uint32_t numStackValues = osr ? fp->script()->nfixed + cx->regs().stackDepth() : 0;
        JS_ASSERT_IF(osr, !IsJSDEnabled(cx));

        
        enter(jitcode, maxArgc, maxArgv, osr ? fp : NULL, calleeToken, scopeChain, numStackValues,
              result.address());
    }

    JS_ASSERT(fp == cx->fp());
    JS_ASSERT(!cx->runtime->hasIonReturnOverride());

    
    fp->setReturnValue(result);

    
    if (!result.isMagic() && fp->isConstructing() && fp->returnValue().isPrimitive())
        fp->setReturnValue(ObjectValue(fp->constructorThis()));

    
    cx->runtime->getIonRuntime(cx)->freeOsrTempData();

    JS_ASSERT_IF(result.isMagic(), result.isMagic(JS_ION_ERROR));
    return result.isMagic() ? IonExec_Error : IonExec_Ok;
}

IonExecStatus
ion::EnterBaselineMethod(JSContext *cx, StackFrame *fp)
{
    RootedScript script(cx, fp->script());
    BaselineScript *baseline = script->baseline;
    IonCode *code = baseline->method();
    void *jitcode = code->raw();

    return EnterBaseline(cx, fp, jitcode, false);
}

IonExecStatus
ion::EnterBaselineAtBranch(JSContext *cx, StackFrame *fp, jsbytecode *pc)
{
    JS_ASSERT(JSOp(*pc) == JSOP_LOOPENTRY);

    RootedScript script(cx, fp->script());
    BaselineScript *baseline = script->baseline;
    uint8_t *jitcode = baseline->nativeCodeForPC(script, pc);

    
    
    if (cx->compartment->debugMode())
        jitcode += MacroAssembler::ToggledCallSize();

    return EnterBaseline(cx, fp, jitcode, true);
}

static MethodStatus
BaselineCompile(JSContext *cx, HandleScript script, StackFrame *fp)
{
    JS_ASSERT(!script->baseline);

    LifoAlloc *alloc = cx->new_<LifoAlloc>(BUILDER_LIFO_ALLOC_PRIMARY_CHUNK_SIZE);
    if (!alloc)
        return Method_Error;

    AutoDestroyAllocator autoDestroy(alloc);

    TempAllocator *temp = alloc->new_<TempAllocator>(alloc);
    if (!temp)
        return Method_Error;

    IonContext ictx(cx, cx->compartment, temp);

    BaselineCompiler compiler(cx, script);
    if (!compiler.init())
        return Method_Error;

    AutoFlushCache afc("BaselineJIT", cx->compartment->ionCompartment());
    MethodStatus status = compiler.compile();

    JS_ASSERT_IF(status == Method_Compiled, script->baseline);
    JS_ASSERT_IF(status != Method_Compiled, !script->baseline);

    if (status == Method_CantCompile)
        script->baseline = BASELINE_DISABLED_SCRIPT;

    return status;
}

MethodStatus
ion::CanEnterBaselineJIT(JSContext *cx, JSScript *scriptArg, StackFrame *fp, bool newType)
{
    
    JS_ASSERT(ion::IsBaselineEnabled(cx));

    
    if (scriptArg->baseline == BASELINE_DISABLED_SCRIPT)
        return Method_Skipped;

    if (scriptArg->length > BaselineScript::MAX_JSSCRIPT_LENGTH)
        return Method_CantCompile;

    RootedScript script(cx, scriptArg);

    
    if (fp->isConstructing() && fp->functionThis().isPrimitive()) {
        RootedObject callee(cx, &fp->callee());
        RootedObject obj(cx, CreateThisForFunction(cx, callee, newType));
        if (!obj)
            return Method_Skipped;
        fp->functionThis().setObject(*obj);
    }

    if (!CheckFrame(fp))
        return Method_CantCompile;

    if (!cx->compartment->ensureIonCompartmentExists(cx))
        return Method_Error;

    if (script->hasBaselineScript())
        return Method_Compiled;

    
    
    if (scriptArg->incUseCount() <= js_IonOptions.baselineUsesBeforeCompile && !IsJSDEnabled(cx))
        return Method_Skipped;

    return BaselineCompile(cx, script, fp);
}


static const unsigned DataAlignment = sizeof(uintptr_t);

BaselineScript *
BaselineScript::New(JSContext *cx, uint32_t prologueOffset, size_t icEntries,
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
    new (script) BaselineScript(prologueOffset);

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
    fop->free_(script);
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

ICEntry &
BaselineScript::icEntryFromReturnOffset(CodeOffsetLabel returnOffset)
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
    JS_ASSERT(icEntry(mid).returnOffset().offset() == returnOffset.offset());
    return icEntry(mid);
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
    JS_NOT_REACHED("Invalid PC offset for IC entry.");
    return icEntry(mid);
}

ICEntry &
BaselineScript::icEntryFromPCOffset(uint32_t pcOffset, ICEntry *prevLookedUpEntry)
{
    
    
    if (prevLookedUpEntry && pcOffset >= prevLookedUpEntry->pcOffset() &&
        (pcOffset - prevLookedUpEntry->pcOffset()) <= 10)
    {
        uint32_t diff = pcOffset - prevLookedUpEntry->pcOffset();
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

ICEntry &
BaselineScript::icEntryFromReturnAddress(uint8_t *returnAddr)
{
    JS_ASSERT(returnAddr > method_->raw());
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
BaselineScript::adoptFallbackStubs(ICStubSpace *stubSpace)
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
    JS_ASSERT(script->baseline == this);
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

    JS_NOT_REACHED("Invalid pc");
    return NULL;
}

void
BaselineScript::toggleDebugTraps(UnrootedScript script, jsbytecode *pc)
{
    JS_ASSERT(script->baseline == this);

    SrcNoteLineScanner scanner(script->notes(), script->lineno);

    IonContext ictx(NULL, script->compartment(), NULL);
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

    optimizedStubSpace_.free();
}

void
ion::FinishDiscardBaselineScript(FreeOp *fop, UnrootedScript script)
{
    if (!script->hasBaselineScript())
        return;

    if (script->baseline->active()) {
        
        
        script->baseline->purgeOptimizedStubs(script->zone());

        
        
        script->baseline->resetActive();
        return;
    }

    BaselineScript::Destroy(fop, script->baseline);
    script->baseline = NULL;
}

void
ion::IonCompartment::toggleBaselineStubBarriers(bool enabled)
{
    for (ICStubCodeMap::Enum e(*stubCodes_); !e.empty(); e.popFront()) {
        IonCode *code = *e.front().value.unsafeGet();
        code->togglePreBarriers(enabled);
    }
}
