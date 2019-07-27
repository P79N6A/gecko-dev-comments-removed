





#include "jit/BaselineDebugModeOSR.h"

#include "mozilla/DebugOnly.h"

#include "jit/IonLinker.h"

#include "jit/JitcodeMap.h"
#include "jit/PerfSpewer.h"

#include "jit/IonFrames-inl.h"
#include "vm/Stack-inl.h"

using namespace js;
using namespace js::jit;

struct DebugModeOSREntry
{
    JSScript *script;
    BaselineScript *oldBaselineScript;
    ICStub *oldStub;
    ICStub *newStub;
    BaselineDebugModeOSRInfo *recompInfo;
    uint32_t pcOffset;
    ICEntry::Kind frameKind;

    explicit DebugModeOSREntry(JSScript *script)
      : script(script),
        oldBaselineScript(script->baselineScript()),
        oldStub(nullptr),
        newStub(nullptr),
        recompInfo(nullptr),
        pcOffset(uint32_t(-1)),
        frameKind(ICEntry::Kind_NonOp)
    { }

    DebugModeOSREntry(JSScript *script, const ICEntry &icEntry)
      : script(script),
        oldBaselineScript(script->baselineScript()),
        oldStub(nullptr),
        newStub(nullptr),
        recompInfo(nullptr),
        pcOffset(icEntry.pcOffset()),
        frameKind(icEntry.kind())
    {
#ifdef DEBUG
        MOZ_ASSERT(pcOffset == icEntry.pcOffset());
        MOZ_ASSERT(frameKind == icEntry.kind());
#endif
    }

    DebugModeOSREntry(JSScript *script, BaselineDebugModeOSRInfo *info)
      : script(script),
        oldBaselineScript(script->baselineScript()),
        oldStub(nullptr),
        newStub(nullptr),
        recompInfo(nullptr),
        pcOffset(script->pcToOffset(info->pc)),
        frameKind(info->frameKind)
    {
#ifdef DEBUG
        MOZ_ASSERT(pcOffset == script->pcToOffset(info->pc));
        MOZ_ASSERT(frameKind == info->frameKind);
#endif
    }

    DebugModeOSREntry(DebugModeOSREntry &&other)
      : script(other.script),
        oldBaselineScript(other.oldBaselineScript),
        oldStub(other.oldStub),
        newStub(other.newStub),
        recompInfo(other.recompInfo ? other.takeRecompInfo() : nullptr),
        pcOffset(other.pcOffset),
        frameKind(other.frameKind)
    { }

    ~DebugModeOSREntry() {
        
        
        
        js_delete(recompInfo);
    }

    bool needsRecompileInfo() const {
        return frameKind == ICEntry::Kind_CallVM ||
               frameKind == ICEntry::Kind_DebugTrap ||
               frameKind == ICEntry::Kind_DebugPrologue ||
               frameKind == ICEntry::Kind_DebugEpilogue;
    }

    bool recompiled() const {
        return oldBaselineScript != script->baselineScript();
    }

    BaselineDebugModeOSRInfo *takeRecompInfo() {
        MOZ_ASSERT(needsRecompileInfo() && recompInfo);
        BaselineDebugModeOSRInfo *tmp = recompInfo;
        recompInfo = nullptr;
        return tmp;
    }

    bool allocateRecompileInfo(JSContext *cx) {
        MOZ_ASSERT(needsRecompileInfo());

        
        
        
        jsbytecode *pc = script->offsetToPC(pcOffset);

        
        
        ICEntry::Kind kind = frameKind;
        recompInfo = cx->new_<BaselineDebugModeOSRInfo>(pc, kind);
        return !!recompInfo;
    }

    ICFallbackStub *fallbackStub() const {
        MOZ_ASSERT(oldStub);
        return script->baselineScript()->icEntryFromPCOffset(pcOffset).fallbackStub();
    }
};

typedef Vector<DebugModeOSREntry> DebugModeOSREntryVector;

class UniqueScriptOSREntryIter
{
    const DebugModeOSREntryVector &entries_;
    size_t index_;

  public:
    explicit UniqueScriptOSREntryIter(const DebugModeOSREntryVector &entries)
      : entries_(entries),
        index_(0)
    { }

    bool done() {
        return index_ == entries_.length();
    }

    const DebugModeOSREntry &entry() {
        MOZ_ASSERT(!done());
        return entries_[index_];
    }

    UniqueScriptOSREntryIter &operator++() {
        MOZ_ASSERT(!done());
        while (++index_ < entries_.length()) {
            bool unique = true;
            for (size_t i = 0; i < index_; i++) {
                if (entries_[i].script == entries_[index_].script) {
                    unique = false;
                    break;
                }
            }
            if (unique)
                break;
        }
        return *this;
    }
};

static bool
CollectOnStackScripts(JSContext *cx, const JitActivationIterator &activation,
                      DebugModeOSREntryVector &entries)
{
    ICStub *prevFrameStubPtr = nullptr;
    bool needsRecompileHandler = false;
    for (JitFrameIterator iter(activation); !iter.done(); ++iter) {
        switch (iter.type()) {
          case JitFrame_BaselineJS: {
            JSScript *script = iter.script();

            if (BaselineDebugModeOSRInfo *info = iter.baselineFrame()->getDebugModeOSRInfo()) {
                
                
                
                
                
                
                
                if (!entries.append(DebugModeOSREntry(script, info)))
                    return false;
            } else {
                
                uint8_t *retAddr = iter.returnAddressToFp();
                ICEntry &entry = script->baselineScript()->icEntryFromReturnAddress(retAddr);
                if (!entries.append(DebugModeOSREntry(script, entry)))
                    return false;
            }

            if (entries.back().needsRecompileInfo()) {
                if (!entries.back().allocateRecompileInfo(cx))
                    return false;

                needsRecompileHandler |= true;
            }

            entries.back().oldStub = prevFrameStubPtr;
            prevFrameStubPtr = nullptr;
            break;
          }

          case JitFrame_BaselineStub:
            prevFrameStubPtr =
                reinterpret_cast<IonBaselineStubFrameLayout *>(iter.fp())->maybeStubPtr();
            break;

          case JitFrame_IonJS: {
            JSScript *script = iter.script();
            if (!entries.append(DebugModeOSREntry(script)))
                return false;
            for (InlineFrameIterator inlineIter(cx, &iter); inlineIter.more(); ++inlineIter) {
                if (!entries.append(DebugModeOSREntry(inlineIter.script())))
                    return false;
            }
            break;
          }

          default:;
        }
    }

    
    
    if (needsRecompileHandler) {
        JitRuntime *rt = cx->runtime()->jitRuntime();
        if (!rt->getBaselineDebugModeOSRHandlerAddress(cx, true))
            return false;
    }

    return true;
}

static const char *
ICEntryKindToString(ICEntry::Kind kind)
{
    switch (kind) {
      case ICEntry::Kind_Op:
        return "IC";
      case ICEntry::Kind_NonOp:
        return "non-op IC";
      case ICEntry::Kind_CallVM:
        return "callVM";
      case ICEntry::Kind_DebugTrap:
        return "debug trap";
      case ICEntry::Kind_DebugPrologue:
        return "debug prologue";
      case ICEntry::Kind_DebugEpilogue:
        return "debug epilogue";
      default:
        MOZ_CRASH("bad ICEntry kind");
    }
}

static void
SpewPatchBaselineFrame(uint8_t *oldReturnAddress, uint8_t *newReturnAddress,
                       JSScript *script, ICEntry::Kind frameKind, jsbytecode *pc)
{
    JitSpew(JitSpew_BaselineDebugModeOSR,
            "Patch return %p -> %p on BaselineJS frame (%s:%d) from %s at %s",
            oldReturnAddress, newReturnAddress, script->filename(), script->lineno(),
            ICEntryKindToString(frameKind), js_CodeName[(JSOp)*pc]);
}

static void
SpewPatchStubFrame(ICStub *oldStub, ICStub *newStub)
{
    JitSpew(JitSpew_BaselineDebugModeOSR,
            "Patch   stub %p -> %p on BaselineStub frame (%s)",
            oldStub, newStub, ICStub::KindString(newStub->kind()));
}

static void
PatchBaselineFramesForDebugMode(JSContext *cx, const JitActivationIterator &activation,
                                DebugModeOSREntryVector &entries, size_t *start)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    IonCommonFrameLayout *prev = nullptr;
    size_t entryIndex = *start;
    bool expectedDebugMode = cx->compartment()->debugMode();

    for (JitFrameIterator iter(activation); !iter.done(); ++iter) {
        DebugModeOSREntry &entry = entries[entryIndex];

        switch (iter.type()) {
          case JitFrame_BaselineJS: {
            
            if (!entry.recompiled()) {
                entryIndex++;
                break;
            }

            JSScript *script = entry.script;
            uint32_t pcOffset = entry.pcOffset;
            jsbytecode *pc = script->offsetToPC(pcOffset);

            MOZ_ASSERT(script == iter.script());
            MOZ_ASSERT(pcOffset < script->length());
            MOZ_ASSERT(script->baselineScript()->debugMode() == expectedDebugMode);

            BaselineScript *bl = script->baselineScript();
            ICEntry::Kind kind = entry.frameKind;

            if (kind == ICEntry::Kind_Op) {
                
                
                
                
                
                
                
                
                
                uint8_t *retAddr = bl->returnAddressForIC(bl->icEntryFromPCOffset(pcOffset));
                SpewPatchBaselineFrame(prev->returnAddress(), retAddr, script, kind, pc);
                prev->setReturnAddress(retAddr);
                entryIndex++;
                break;
            }

            
            
            
            
            
            if (BaselineDebugModeOSRInfo *info = iter.baselineFrame()->getDebugModeOSRInfo()) {
                MOZ_ASSERT(info->pc == pc);
                MOZ_ASSERT(info->frameKind == kind);

                
                MOZ_ASSERT_IF(expectedDebugMode, (kind == ICEntry::Kind_CallVM ||
                                                  kind == ICEntry::Kind_DebugTrap ||
                                                  kind == ICEntry::Kind_DebugPrologue ||
                                                  kind == ICEntry::Kind_DebugEpilogue));
                
                MOZ_ASSERT_IF(!expectedDebugMode, kind == ICEntry::Kind_CallVM);

                
                
                iter.baselineFrame()->deleteDebugModeOSRInfo();
            }

            
            
            BaselineDebugModeOSRInfo *recompInfo = entry.takeRecompInfo();

            bool popFrameReg;
            switch (kind) {
              case ICEntry::Kind_CallVM:
                
                
                
                
                
                pc += GetBytecodeLength(pc);
                recompInfo->resumeAddr = bl->nativeCodeForPC(script, pc, &recompInfo->slotInfo);
                popFrameReg = true;
                break;

              case ICEntry::Kind_DebugTrap:
                
                
                
                
                
                
                
                recompInfo->resumeAddr = bl->nativeCodeForPC(script, pc, &recompInfo->slotInfo);
                popFrameReg = false;
                break;

              case ICEntry::Kind_DebugPrologue:
                
                
                
                
                recompInfo->resumeAddr = bl->postDebugPrologueAddr();
                popFrameReg = true;
                break;

              default:
                
                
                
                
                MOZ_ASSERT(kind == ICEntry::Kind_DebugEpilogue);
                recompInfo->resumeAddr = bl->epilogueEntryAddr();
                popFrameReg = true;
                break;
            }

            SpewPatchBaselineFrame(prev->returnAddress(), recompInfo->resumeAddr,
                                   script, kind, recompInfo->pc);

            
            
            JitRuntime *rt = cx->runtime()->jitRuntime();
            void *handlerAddr = rt->getBaselineDebugModeOSRHandlerAddress(cx, popFrameReg);
            MOZ_ASSERT(handlerAddr);

            prev->setReturnAddress(reinterpret_cast<uint8_t *>(handlerAddr));
            iter.baselineFrame()->setDebugModeOSRInfo(recompInfo);

            entryIndex++;
            break;
          }

          case JitFrame_BaselineStub: {
            
            if (!entry.recompiled())
                break;

            IonBaselineStubFrameLayout *layout =
                reinterpret_cast<IonBaselineStubFrameLayout *>(iter.fp());
            MOZ_ASSERT(entry.script->baselineScript()->debugMode() == expectedDebugMode);
            MOZ_ASSERT(layout->maybeStubPtr() == entry.oldStub);

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (layout->maybeStubPtr()) {
                MOZ_ASSERT(entry.newStub);
                SpewPatchStubFrame(entry.oldStub, entry.newStub);
                layout->setStubPtr(entry.newStub);
            }

            break;
          }

          case JitFrame_IonJS:
            
            entryIndex++;
            for (InlineFrameIterator inlineIter(cx, &iter); inlineIter.more(); ++inlineIter)
                entryIndex++;
            break;

          default:;
        }

        prev = iter.current();
    }

    *start = entryIndex;
}

static bool
RecompileBaselineScriptForDebugMode(JSContext *cx, JSScript *script)
{
    BaselineScript *oldBaselineScript = script->baselineScript();

    
    
    bool expectedDebugMode = cx->compartment()->debugMode();
    if (oldBaselineScript->debugMode() == expectedDebugMode)
        return true;

    JitSpew(JitSpew_BaselineDebugModeOSR, "Recompiling (%s:%d) for debug mode %s",
            script->filename(), script->lineno(), expectedDebugMode ? "ON" : "OFF");

    CancelOffThreadIonCompile(cx->compartment(), script);

    if (script->hasIonScript())
        Invalidate(cx, script,  false);

    script->setBaselineScript(cx, nullptr);

    MethodStatus status = BaselineCompile(cx, script);
    if (status != Method_Compiled) {
        
        
        
        MOZ_ASSERT(status == Method_Error);
        script->setBaselineScript(cx, oldBaselineScript);
        return false;
    }

    
    
    MOZ_ASSERT(script->baselineScript()->debugMode() == expectedDebugMode);
    return true;
}

#define PATCHABLE_ICSTUB_KIND_LIST(_)           \
    _(Call_Scripted)                            \
    _(Call_AnyScripted)                         \
    _(Call_Native)                              \
    _(Call_ScriptedApplyArray)                  \
    _(Call_ScriptedApplyArguments)              \
    _(Call_ScriptedFunCall)                     \
    _(GetElem_NativePrototypeCallNative)        \
    _(GetElem_NativePrototypeCallScripted)      \
    _(GetProp_CallScripted)                     \
    _(GetProp_CallNative)                       \
    _(GetProp_CallNativePrototype)              \
    _(GetProp_CallDOMProxyNative)               \
    _(GetProp_CallDOMProxyWithGenerationNative) \
    _(GetProp_DOMProxyShadowed)                 \
    _(SetProp_CallScripted)                     \
    _(SetProp_CallNative)

#if JS_HAS_NO_SUCH_METHOD
#define PATCHABLE_NSM_ICSTUB_KIND_LIST(_)       \
    _(GetElem_Dense)                            \
    _(GetElem_Arguments)                        \
    _(GetProp_NativePrototype)                  \
    _(GetProp_Native)
#endif

static bool
CloneOldBaselineStub(JSContext *cx, DebugModeOSREntryVector &entries, size_t entryIndex)
{
    DebugModeOSREntry &entry = entries[entryIndex];
    if (!entry.oldStub)
        return true;

    ICStub *oldStub = entry.oldStub;
    MOZ_ASSERT(ICStub::CanMakeCalls(oldStub->kind()));

    
    ICFallbackStub *fallbackStub = entry.fallbackStub();

    
    
    
    if (oldStub->isFallback()) {
        MOZ_ASSERT(oldStub->jitCode() == fallbackStub->jitCode());
        entry.newStub = fallbackStub;
        return true;
    }

    
    for (size_t i = 0; i < entryIndex; i++) {
        if (oldStub == entries[i].oldStub) {
            MOZ_ASSERT(entries[i].newStub);
            entry.newStub = entries[i].newStub;
            return true;
        }
    }

    
    
    ICStub *firstMonitorStub;
    if (fallbackStub->isMonitoredFallback()) {
        ICMonitoredFallbackStub *monitored = fallbackStub->toMonitoredFallbackStub();
        firstMonitorStub = monitored->fallbackMonitorStub()->firstMonitorStub();
    } else {
        firstMonitorStub = nullptr;
    }
    ICStubSpace *stubSpace = ICStubCompiler::StubSpaceForKind(oldStub->kind(), entry.script);

    
    
    
    
    switch (oldStub->kind()) {
#define CASE_KIND(kindName)                                                  \
      case ICStub::kindName:                                                 \
        entry.newStub = IC##kindName::Clone(cx, stubSpace, firstMonitorStub, \
                                            *oldStub->to##kindName());       \
        break;
        PATCHABLE_ICSTUB_KIND_LIST(CASE_KIND)
#if JS_HAS_NO_SUCH_METHOD
        PATCHABLE_NSM_ICSTUB_KIND_LIST(CASE_KIND)
#endif
#undef CASE_KIND

      default:
        MOZ_CRASH("Bad stub kind");
    }

    if (!entry.newStub)
        return false;

    fallbackStub->addNewStub(entry.newStub);
    return true;
}

static void
UndoRecompileBaselineScriptsForDebugMode(JSContext *cx,
                                         const DebugModeOSREntryVector &entries)
{
    
    
    for (UniqueScriptOSREntryIter iter(entries); !iter.done(); ++iter) {
        const DebugModeOSREntry &entry = iter.entry();
        JSScript *script = entry.script;
        BaselineScript *baselineScript = script->baselineScript();
        if (entry.recompiled()) {
            script->setBaselineScript(cx, entry.oldBaselineScript);
            BaselineScript::Destroy(cx->runtime()->defaultFreeOp(), baselineScript);
        }
    }
}

bool
jit::RecompileOnStackBaselineScriptsForDebugMode(JSContext *cx, JSCompartment *comp)
{
    AutoCompartment ac(cx, comp);

    
    
    Vector<DebugModeOSREntry> entries(cx);

    for (JitActivationIterator iter(cx->runtime()); !iter.done(); ++iter) {
        if (iter.activation()->compartment() == comp) {
            if (!CollectOnStackScripts(cx, iter, entries))
                return false;
        }
    }

#ifdef JSGC_GENERATIONAL
    
    if (!entries.empty())
        cx->runtime()->gc.evictNursery();
#endif

    
    
    
    AutoSuppressProfilerSampling suppressProfilerSampling(cx);

    
    
    
    for (size_t i = 0; i < entries.length(); i++) {
        JSScript *script = entries[i].script;

        if (!RecompileBaselineScriptForDebugMode(cx, script) ||
            !CloneOldBaselineStub(cx, entries, i))
        {
            UndoRecompileBaselineScriptsForDebugMode(cx, entries);
            return false;
        }
    }

    
    
    
    

    for (UniqueScriptOSREntryIter iter(entries); !iter.done(); ++iter) {
        const DebugModeOSREntry &entry = iter.entry();
        if (entry.recompiled())
            BaselineScript::Destroy(cx->runtime()->defaultFreeOp(), entry.oldBaselineScript);
    }

    size_t processed = 0;
    for (JitActivationIterator iter(cx->runtime()); !iter.done(); ++iter) {
        if (iter.activation()->compartment() == comp)
            PatchBaselineFramesForDebugMode(cx, iter, entries, &processed);
    }
    MOZ_ASSERT(processed == entries.length());

    return true;
}

void
BaselineDebugModeOSRInfo::popValueInto(PCMappingSlotInfo::SlotLocation loc, Value *vp)
{
    switch (loc) {
      case PCMappingSlotInfo::SlotInR0:
        valueR0 = vp[stackAdjust];
        break;
      case PCMappingSlotInfo::SlotInR1:
        valueR1 = vp[stackAdjust];
        break;
      case PCMappingSlotInfo::SlotIgnore:
        break;
      default:
        MOZ_CRASH("Bad slot location");
    }

    stackAdjust++;
}

static inline bool
HasForcedReturn(BaselineDebugModeOSRInfo *info, bool rv)
{
    ICEntry::Kind kind = info->frameKind;

    
    
    if (kind == ICEntry::Kind_DebugEpilogue)
        return true;

    
    
    if (kind == ICEntry::Kind_DebugPrologue ||
        (kind == ICEntry::Kind_CallVM && JSOp(*info->pc) == JSOP_DEBUGGER))
    {
        return rv;
    }

    
    
    return false;
}

static void
SyncBaselineDebugModeOSRInfo(BaselineFrame *frame, Value *vp, bool rv)
{
    BaselineDebugModeOSRInfo *info = frame->debugModeOSRInfo();
    MOZ_ASSERT(info);
    MOZ_ASSERT(frame->script()->baselineScript()->containsCodeAddress(info->resumeAddr));

    if (HasForcedReturn(info, rv)) {
        
        
        MOZ_ASSERT(R0 == JSReturnOperand);
        info->valueR0 = frame->returnValue();
        info->resumeAddr = frame->script()->baselineScript()->epilogueEntryAddr();
        return;
    }

    
    unsigned numUnsynced = info->slotInfo.numUnsynced();
    MOZ_ASSERT(numUnsynced <= 2);
    if (numUnsynced > 0)
        info->popValueInto(info->slotInfo.topSlotLocation(), vp);
    if (numUnsynced > 1)
        info->popValueInto(info->slotInfo.nextSlotLocation(), vp);

    
    info->stackAdjust *= sizeof(Value);
}

static void
FinishBaselineDebugModeOSR(BaselineFrame *frame)
{
    frame->deleteDebugModeOSRInfo();
}

void
BaselineFrame::deleteDebugModeOSRInfo()
{
    js_delete(getDebugModeOSRInfo());
    flags_ &= ~HAS_DEBUG_MODE_OSR_INFO;
}

JitCode *
JitRuntime::getBaselineDebugModeOSRHandler(JSContext *cx)
{
    if (!baselineDebugModeOSRHandler_) {
        AutoLockForExclusiveAccess lock(cx);
        AutoCompartment ac(cx, cx->runtime()->atomsCompartment());
        uint32_t offset;
        if (JitCode *code = generateBaselineDebugModeOSRHandler(cx, &offset)) {
            baselineDebugModeOSRHandler_ = code;
            baselineDebugModeOSRHandlerNoFrameRegPopAddr_ = code->raw() + offset;
        }
    }

    return baselineDebugModeOSRHandler_;
}

void *
JitRuntime::getBaselineDebugModeOSRHandlerAddress(JSContext *cx, bool popFrameReg)
{
    if (!getBaselineDebugModeOSRHandler(cx))
        return nullptr;
    return popFrameReg
           ? baselineDebugModeOSRHandler_->raw()
           : baselineDebugModeOSRHandlerNoFrameRegPopAddr_;
}

JitCode *
JitRuntime::generateBaselineDebugModeOSRHandler(JSContext *cx, uint32_t *noFrameRegPopOffsetOut)
{
    MacroAssembler masm(cx);

    GeneralRegisterSet regs(GeneralRegisterSet::All());
    regs.take(BaselineFrameReg);
    regs.take(ReturnReg);
    Register temp = regs.takeAny();
    Register syncedStackStart = regs.takeAny();

    
    masm.pop(BaselineFrameReg);

    
    
    CodeOffsetLabel noFrameRegPopOffset(masm.currentOffset());

    
    masm.movePtr(StackPointer, syncedStackStart);
    masm.push(BaselineFrameReg);

    
    masm.setupUnalignedABICall(3, temp);
    masm.loadBaselineFramePtr(BaselineFrameReg, temp);
    masm.passABIArg(temp);
    masm.passABIArg(syncedStackStart);
    masm.passABIArg(ReturnReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, SyncBaselineDebugModeOSRInfo));

    
    
    
    masm.pop(BaselineFrameReg);
    masm.loadPtr(Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfScratchValue()), temp);
    masm.addPtr(Address(temp, offsetof(BaselineDebugModeOSRInfo, stackAdjust)), StackPointer);

    
    masm.pushValue(Address(temp, offsetof(BaselineDebugModeOSRInfo, valueR0)));
    masm.pushValue(Address(temp, offsetof(BaselineDebugModeOSRInfo, valueR1)));
    masm.push(BaselineFrameReg);
    masm.push(Address(temp, offsetof(BaselineDebugModeOSRInfo, resumeAddr)));

    
    masm.setupUnalignedABICall(1, temp);
    masm.loadBaselineFramePtr(BaselineFrameReg, temp);
    masm.passABIArg(temp);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, FinishBaselineDebugModeOSR));

    
    GeneralRegisterSet jumpRegs(GeneralRegisterSet::All());
    jumpRegs.take(R0);
    jumpRegs.take(R1);
    jumpRegs.take(BaselineFrameReg);
    Register target = jumpRegs.takeAny();

    masm.pop(target);
    masm.pop(BaselineFrameReg);
    masm.popValue(R1);
    masm.popValue(R0);

    masm.jump(target);

    Linker linker(masm);
    AutoFlushICache afc("BaselineDebugModeOSRHandler");
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);
    if (!code)
        return nullptr;

    noFrameRegPopOffset.fixup(&masm);
    *noFrameRegPopOffsetOut = noFrameRegPopOffset.offset();

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BaselineDebugModeOSRHandler");
#endif

    return code;
}
