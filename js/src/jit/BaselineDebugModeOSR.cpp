





#include "jit/BaselineDebugModeOSR.h"

#include "mozilla/DebugOnly.h"

#include "jit/JitcodeMap.h"
#include "jit/Linker.h"
#include "jit/PerfSpewer.h"

#include "jit/JitFrames-inl.h"
#include "vm/Stack-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;

struct DebugModeOSREntry
{
    JSScript* script;
    BaselineScript* oldBaselineScript;
    ICStub* oldStub;
    ICStub* newStub;
    BaselineDebugModeOSRInfo* recompInfo;
    uint32_t pcOffset;
    ICEntry::Kind frameKind;

    explicit DebugModeOSREntry(JSScript* script)
      : script(script),
        oldBaselineScript(script->baselineScript()),
        oldStub(nullptr),
        newStub(nullptr),
        recompInfo(nullptr),
        pcOffset(uint32_t(-1)),
        frameKind(ICEntry::Kind_Invalid)
    { }

    DebugModeOSREntry(JSScript* script, uint32_t pcOffset)
      : script(script),
        oldBaselineScript(script->baselineScript()),
        oldStub(nullptr),
        newStub(nullptr),
        recompInfo(nullptr),
        pcOffset(pcOffset),
        frameKind(ICEntry::Kind_Invalid)
    { }

    DebugModeOSREntry(JSScript* script, const ICEntry& icEntry)
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

    DebugModeOSREntry(JSScript* script, BaselineDebugModeOSRInfo* info)
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

    DebugModeOSREntry(DebugModeOSREntry&& other)
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
               frameKind == ICEntry::Kind_StackCheck ||
               frameKind == ICEntry::Kind_EarlyStackCheck ||
               frameKind == ICEntry::Kind_DebugTrap ||
               frameKind == ICEntry::Kind_DebugPrologue ||
               frameKind == ICEntry::Kind_DebugEpilogue;
    }

    bool recompiled() const {
        return oldBaselineScript != script->baselineScript();
    }

    BaselineDebugModeOSRInfo* takeRecompInfo() {
        MOZ_ASSERT(needsRecompileInfo() && recompInfo);
        BaselineDebugModeOSRInfo* tmp = recompInfo;
        recompInfo = nullptr;
        return tmp;
    }

    bool allocateRecompileInfo(JSContext* cx) {
        MOZ_ASSERT(script);
        MOZ_ASSERT(needsRecompileInfo());

        
        
        
        jsbytecode* pc = script->offsetToPC(pcOffset);

        
        
        ICEntry::Kind kind = frameKind;
        recompInfo = cx->new_<BaselineDebugModeOSRInfo>(pc, kind);
        return !!recompInfo;
    }

    ICFallbackStub* fallbackStub() const {
        MOZ_ASSERT(script);
        MOZ_ASSERT(oldStub);
        return script->baselineScript()->icEntryFromPCOffset(pcOffset).fallbackStub();
    }
};

typedef Vector<DebugModeOSREntry> DebugModeOSREntryVector;

class UniqueScriptOSREntryIter
{
    const DebugModeOSREntryVector& entries_;
    size_t index_;

  public:
    explicit UniqueScriptOSREntryIter(const DebugModeOSREntryVector& entries)
      : entries_(entries),
        index_(0)
    { }

    bool done() {
        return index_ == entries_.length();
    }

    const DebugModeOSREntry& entry() {
        MOZ_ASSERT(!done());
        return entries_[index_];
    }

    UniqueScriptOSREntryIter& operator++() {
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
CollectJitStackScripts(JSContext* cx, const Debugger::ExecutionObservableSet& obs,
                       const ActivationIterator& activation, DebugModeOSREntryVector& entries)
{
    ICStub* prevFrameStubPtr = nullptr;
    bool needsRecompileHandler = false;
    for (JitFrameIterator iter(activation); !iter.done(); ++iter) {
        switch (iter.type()) {
          case JitFrame_BaselineJS: {
            JSScript* script = iter.script();

            if (!obs.shouldRecompileOrInvalidate(script)) {
                prevFrameStubPtr = nullptr;
                break;
            }

            BaselineFrame* frame = iter.baselineFrame();

            if (BaselineDebugModeOSRInfo* info = frame->getDebugModeOSRInfo()) {
                
                
                
                
                
                
                
                if (!entries.append(DebugModeOSREntry(script, info)))
                    return false;
            } else if (frame->isHandlingException()) {
                
                
                uint32_t offset = script->pcToOffset(frame->overridePc());
                if (!entries.append(DebugModeOSREntry(script, offset)))
                    return false;
            } else {
                
                uint8_t* retAddr = iter.returnAddressToFp();
                ICEntry& icEntry = script->baselineScript()->icEntryFromReturnAddress(retAddr);
                if (!entries.append(DebugModeOSREntry(script, icEntry)))
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
                reinterpret_cast<BaselineStubFrameLayout*>(iter.fp())->maybeStubPtr();
            break;

          case JitFrame_IonJS: {
            InlineFrameIterator inlineIter(cx, &iter);
            while (true) {
                if (obs.shouldRecompileOrInvalidate(inlineIter.script())) {
                    if (!entries.append(DebugModeOSREntry(inlineIter.script())))
                        return false;
                }
                if (!inlineIter.more())
                    break;
                ++inlineIter;
            }
            break;
          }

          default:;
        }
    }

    
    
    if (needsRecompileHandler) {
        JitRuntime* rt = cx->runtime()->jitRuntime();
        if (!rt->getBaselineDebugModeOSRHandlerAddress(cx, true))
            return false;
    }

    return true;
}

static bool
CollectInterpreterStackScripts(JSContext* cx, const Debugger::ExecutionObservableSet& obs,
                               const ActivationIterator& activation,
                               DebugModeOSREntryVector& entries)
{
    
    
    
    InterpreterActivation* act = activation.activation()->asInterpreter();
    for (InterpreterFrameIterator iter(act); !iter.done(); ++iter) {
        JSScript* script = iter.frame()->script();
        if (obs.shouldRecompileOrInvalidate(script)) {
            if (!entries.append(DebugModeOSREntry(iter.frame()->script())))
                return false;
        }
    }
    return true;
}

static const char*
ICEntryKindToString(ICEntry::Kind kind)
{
    switch (kind) {
      case ICEntry::Kind_Op:
        return "IC";
      case ICEntry::Kind_NonOp:
        return "non-op IC";
      case ICEntry::Kind_CallVM:
        return "callVM";
      case ICEntry::Kind_StackCheck:
        return "stack check";
      case ICEntry::Kind_EarlyStackCheck:
        return "early stack check";
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
SpewPatchBaselineFrame(uint8_t* oldReturnAddress, uint8_t* newReturnAddress,
                       JSScript* script, ICEntry::Kind frameKind, jsbytecode* pc)
{
    JitSpew(JitSpew_BaselineDebugModeOSR,
            "Patch return %p -> %p on BaselineJS frame (%s:%d) from %s at %s",
            oldReturnAddress, newReturnAddress, script->filename(), script->lineno(),
            ICEntryKindToString(frameKind), js_CodeName[(JSOp)*pc]);
}

static void
SpewPatchBaselineFrameFromExceptionHandler(uint8_t* oldReturnAddress, uint8_t* newReturnAddress,
                                           JSScript* script, jsbytecode* pc)
{
    JitSpew(JitSpew_BaselineDebugModeOSR,
            "Patch return %p -> %p on BaselineJS frame (%s:%d) from exception handler at %s",
            oldReturnAddress, newReturnAddress, script->filename(), script->lineno(),
            js_CodeName[(JSOp)*pc]);
}

static void
SpewPatchStubFrame(ICStub* oldStub, ICStub* newStub)
{
    JitSpew(JitSpew_BaselineDebugModeOSR,
            "Patch   stub %p -> %p on BaselineStub frame (%s)",
            oldStub, newStub, newStub ? ICStub::KindString(newStub->kind()) : "exception handler");
}

static void
PatchBaselineFramesForDebugMode(JSContext* cx, const Debugger::ExecutionObservableSet& obs,
                                const ActivationIterator& activation,
                                DebugModeOSREntryVector& entries, size_t* start)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    CommonFrameLayout* prev = nullptr;
    size_t entryIndex = *start;

    for (JitFrameIterator iter(activation); !iter.done(); ++iter) {
        switch (iter.type()) {
          case JitFrame_BaselineJS: {
            
            
            if (!obs.shouldRecompileOrInvalidate(iter.script()))
                break;

            DebugModeOSREntry& entry = entries[entryIndex];

            if (!entry.recompiled()) {
                entryIndex++;
                break;
            }

            JSScript* script = entry.script;
            uint32_t pcOffset = entry.pcOffset;
            jsbytecode* pc = script->offsetToPC(pcOffset);

            MOZ_ASSERT(script == iter.script());
            MOZ_ASSERT(pcOffset < script->length());

            BaselineScript* bl = script->baselineScript();
            ICEntry::Kind kind = entry.frameKind;

            if (kind == ICEntry::Kind_Op) {
                
                
                
                
                
                
                
                
                
                uint8_t* retAddr = bl->returnAddressForIC(bl->icEntryFromPCOffset(pcOffset));
                SpewPatchBaselineFrame(prev->returnAddress(), retAddr, script, kind, pc);
                DebugModeOSRVolatileJitFrameIterator::forwardLiveIterators(
                    cx, prev->returnAddress(), retAddr);
                prev->setReturnAddress(retAddr);
                entryIndex++;
                break;
            }

            if (kind == ICEntry::Kind_Invalid) {
                
                
                
                
                
                
                
                
                
                MOZ_ASSERT(iter.baselineFrame()->isHandlingException());
                MOZ_ASSERT(iter.baselineFrame()->overridePc() == pc);
                uint8_t* retAddr = nullptr;
                SpewPatchBaselineFrameFromExceptionHandler(prev->returnAddress(), retAddr,
                                                           script, pc);
                DebugModeOSRVolatileJitFrameIterator::forwardLiveIterators(
                    cx, prev->returnAddress(), retAddr);
                prev->setReturnAddress(retAddr);
                entryIndex++;
                break;
            }

            
            
            
            
            
            BaselineDebugModeOSRInfo* info = iter.baselineFrame()->getDebugModeOSRInfo();
            if (info) {
                MOZ_ASSERT(info->pc == pc);
                MOZ_ASSERT(info->frameKind == kind);

                
                MOZ_ASSERT_IF(script->baselineScript()->hasDebugInstrumentation(),
                              kind == ICEntry::Kind_CallVM ||
                              kind == ICEntry::Kind_StackCheck ||
                              kind == ICEntry::Kind_EarlyStackCheck ||
                              kind == ICEntry::Kind_DebugTrap ||
                              kind == ICEntry::Kind_DebugPrologue ||
                              kind == ICEntry::Kind_DebugEpilogue);
                
                MOZ_ASSERT_IF(!script->baselineScript()->hasDebugInstrumentation(),
                              kind == ICEntry::Kind_CallVM ||
                              kind == ICEntry::Kind_StackCheck ||
                              kind == ICEntry::Kind_EarlyStackCheck);

                
                
                iter.baselineFrame()->deleteDebugModeOSRInfo();
            }

            
            
            BaselineDebugModeOSRInfo* recompInfo = entry.takeRecompInfo();

            bool popFrameReg;
            switch (kind) {
              case ICEntry::Kind_CallVM: {
                
                
                
                
                
                
                
                
                
                ICEntry& callVMEntry = bl->callVMEntryFromPCOffset(pcOffset);
                recompInfo->resumeAddr = bl->returnAddressForIC(callVMEntry);
                popFrameReg = false;
                break;
              }

              case ICEntry::Kind_StackCheck:
              case ICEntry::Kind_EarlyStackCheck: {
                
                
                
                
                
                bool earlyCheck = kind == ICEntry::Kind_EarlyStackCheck;
                ICEntry& stackCheckEntry = bl->stackCheckICEntry(earlyCheck);
                recompInfo->resumeAddr = bl->returnAddressForIC(stackCheckEntry);
                popFrameReg = false;
                break;
              }

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

            
            
            JitRuntime* rt = cx->runtime()->jitRuntime();
            void* handlerAddr = rt->getBaselineDebugModeOSRHandlerAddress(cx, popFrameReg);
            MOZ_ASSERT(handlerAddr);

            prev->setReturnAddress(reinterpret_cast<uint8_t*>(handlerAddr));
            iter.baselineFrame()->setDebugModeOSRInfo(recompInfo);
            iter.baselineFrame()->setOverridePc(recompInfo->pc);

            entryIndex++;
            break;
          }

          case JitFrame_BaselineStub: {
            JitFrameIterator prev(iter);
            ++prev;
            BaselineFrame* prevFrame = prev.baselineFrame();
            if (!obs.shouldRecompileOrInvalidate(prevFrame->script()))
                break;

            DebugModeOSREntry& entry = entries[entryIndex];

            
            if (!entry.recompiled())
                break;

            BaselineStubFrameLayout* layout =
                reinterpret_cast<BaselineStubFrameLayout*>(iter.fp());
            MOZ_ASSERT(layout->maybeStubPtr() == entry.oldStub);

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (layout->maybeStubPtr()) {
                MOZ_ASSERT(entry.newStub || prevFrame->isHandlingException());
                SpewPatchStubFrame(entry.oldStub, entry.newStub);
                layout->setStubPtr(entry.newStub);
            }

            break;
          }

          case JitFrame_IonJS: {
            
            InlineFrameIterator inlineIter(cx, &iter);
            while (true) {
                if (obs.shouldRecompileOrInvalidate(inlineIter.script()))
                    entryIndex++;
                if (!inlineIter.more())
                    break;
                ++inlineIter;
            }
            break;
          }

          default:;
        }

        prev = iter.current();
    }

    *start = entryIndex;
}

static void
SkipInterpreterFrameEntries(const Debugger::ExecutionObservableSet& obs,
                            const ActivationIterator& activation,
                            DebugModeOSREntryVector& entries, size_t* start)
{
    size_t entryIndex = *start;

    
    InterpreterActivation* act = activation.activation()->asInterpreter();
    for (InterpreterFrameIterator iter(act); !iter.done(); ++iter) {
        if (obs.shouldRecompileOrInvalidate(iter.frame()->script()))
            entryIndex++;
    }

    *start = entryIndex;
}

static bool
RecompileBaselineScriptForDebugMode(JSContext* cx, JSScript* script,
                                    Debugger::IsObserving observing)
{
    BaselineScript* oldBaselineScript = script->baselineScript();

    
    
    if (oldBaselineScript->hasDebugInstrumentation() == observing)
        return true;

    JitSpew(JitSpew_BaselineDebugModeOSR, "Recompiling (%s:%d) for %s",
            script->filename(), script->lineno(), observing ? "DEBUGGING" : "NORMAL EXECUTION");

    script->setBaselineScript(cx, nullptr);

    MethodStatus status = BaselineCompile(cx, script,  observing);
    if (status != Method_Compiled) {
        
        
        
        MOZ_ASSERT(status == Method_Error);
        script->setBaselineScript(cx, oldBaselineScript);
        return false;
    }

    
    
    MOZ_ASSERT(script->baselineScript()->hasDebugInstrumentation() == observing);
    return true;
}

#define PATCHABLE_ICSTUB_KIND_LIST(_)           \
    _(Call_Scripted)                            \
    _(Call_AnyScripted)                         \
    _(Call_Native)                              \
    _(Call_ClassHook)                           \
    _(Call_ScriptedApplyArray)                  \
    _(Call_ScriptedApplyArguments)              \
    _(Call_ScriptedFunCall)                     \
    _(GetElem_NativePrototypeCallNative)        \
    _(GetElem_NativePrototypeCallScripted)      \
    _(GetProp_CallScripted)                     \
    _(GetProp_CallNative)                       \
    _(GetProp_CallDOMProxyNative)               \
    _(GetProp_CallDOMProxyWithGenerationNative) \
    _(GetProp_DOMProxyShadowed)                 \
    _(GetProp_Generic)                          \
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
CloneOldBaselineStub(JSContext* cx, DebugModeOSREntryVector& entries, size_t entryIndex)
{
    DebugModeOSREntry& entry = entries[entryIndex];
    if (!entry.oldStub)
        return true;

    ICStub* oldStub = entry.oldStub;
    MOZ_ASSERT(ICStub::CanMakeCalls(oldStub->kind()));

    if (entry.frameKind == ICEntry::Kind_Invalid) {
        
        
        
        
        
        entry.newStub = nullptr;
        return true;
    }

    
    ICFallbackStub* fallbackStub = entry.fallbackStub();

    
    
    
    if (oldStub->isFallback()) {
        MOZ_ASSERT(oldStub->jitCode() == fallbackStub->jitCode());
        entry.newStub = fallbackStub;
        return true;
    }

    
    
    
    for (size_t i = 0; i < entryIndex; i++) {
        if (oldStub == entries[i].oldStub && entries[i].frameKind != ICEntry::Kind_Invalid) {
            MOZ_ASSERT(entries[i].newStub);
            entry.newStub = entries[i].newStub;
            return true;
        }
    }

    
    
    ICStub* firstMonitorStub;
    if (fallbackStub->isMonitoredFallback()) {
        ICMonitoredFallbackStub* monitored = fallbackStub->toMonitoredFallbackStub();
        firstMonitorStub = monitored->fallbackMonitorStub()->firstMonitorStub();
    } else {
        firstMonitorStub = nullptr;
    }
    ICStubSpace* stubSpace = ICStubCompiler::StubSpaceForKind(oldStub->kind(), entry.script);

    
    
    
    
    switch (oldStub->kind()) {
#define CASE_KIND(kindName)                                                  \
      case ICStub::kindName:                                                 \
        entry.newStub = IC##kindName::Clone(stubSpace, firstMonitorStub,     \
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

static bool
InvalidateScriptsInZone(JSContext* cx, Zone* zone, const Vector<DebugModeOSREntry>& entries)
{
    RecompileInfoVector invalid;
    for (UniqueScriptOSREntryIter iter(entries); !iter.done(); ++iter) {
        JSScript* script = iter.entry().script;
        if (script->compartment()->zone() != zone)
            continue;

        if (script->hasIonScript()) {
            if (!invalid.append(script->ionScript()->recompileInfo()))
                return false;
        }

        
        
        
        
        if (script->hasBaselineScript())
            CancelOffThreadIonCompile(script->compartment(), script);
    }

    
    
    Invalidate(zone->types, cx->runtime()->defaultFreeOp(), invalid,
                true,  false);
    return true;
}

static void
UndoRecompileBaselineScriptsForDebugMode(JSContext* cx,
                                         const DebugModeOSREntryVector& entries)
{
    
    
    for (UniqueScriptOSREntryIter iter(entries); !iter.done(); ++iter) {
        const DebugModeOSREntry& entry = iter.entry();
        JSScript* script = entry.script;
        BaselineScript* baselineScript = script->baselineScript();
        if (entry.recompiled()) {
            script->setBaselineScript(cx, entry.oldBaselineScript);
            BaselineScript::Destroy(cx->runtime()->defaultFreeOp(), baselineScript);
        }
    }
}

bool
jit::RecompileOnStackBaselineScriptsForDebugMode(JSContext* cx,
                                                 const Debugger::ExecutionObservableSet& obs,
                                                 Debugger::IsObserving observing)
{
    
    
    Vector<DebugModeOSREntry> entries(cx);

    for (ActivationIterator iter(cx->runtime()); !iter.done(); ++iter) {
        if (iter->isJit()) {
            if (!CollectJitStackScripts(cx, obs, iter, entries))
                return false;
        } else if (iter->isInterpreter()) {
            if (!CollectInterpreterStackScripts(cx, obs, iter, entries))
                return false;
        }
    }

    if (entries.empty())
        return true;

    
    cx->runtime()->gc.evictNursery();

    
    
    MOZ_ASSERT(!cx->runtime()->isProfilerSamplingEnabled());

    
    if (Zone* zone = obs.singleZone()) {
        if (!InvalidateScriptsInZone(cx, zone, entries))
            return false;
    } else {
        typedef Debugger::ExecutionObservableSet::ZoneRange ZoneRange;
        for (ZoneRange r = obs.zones()->all(); !r.empty(); r.popFront()) {
            if (!InvalidateScriptsInZone(cx, r.front(), entries))
                return false;
        }
    }

    
    
    
    for (size_t i = 0; i < entries.length(); i++) {
        JSScript* script = entries[i].script;
        AutoCompartment ac(cx, script->compartment());
        if (!RecompileBaselineScriptForDebugMode(cx, script, observing) ||
            !CloneOldBaselineStub(cx, entries, i))
        {
            UndoRecompileBaselineScriptsForDebugMode(cx, entries);
            return false;
        }
    }

    
    
    
    

    for (UniqueScriptOSREntryIter iter(entries); !iter.done(); ++iter) {
        const DebugModeOSREntry& entry = iter.entry();
        if (entry.recompiled())
            BaselineScript::Destroy(cx->runtime()->defaultFreeOp(), entry.oldBaselineScript);
    }

    size_t processed = 0;
    for (ActivationIterator iter(cx->runtime()); !iter.done(); ++iter) {
        if (iter->isJit())
            PatchBaselineFramesForDebugMode(cx, obs, iter, entries, &processed);
        else if (iter->isInterpreter())
            SkipInterpreterFrameEntries(obs, iter, entries, &processed);
    }
    MOZ_ASSERT(processed == entries.length());

    return true;
}

void
BaselineDebugModeOSRInfo::popValueInto(PCMappingSlotInfo::SlotLocation loc, Value* vp)
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
HasForcedReturn(BaselineDebugModeOSRInfo* info, bool rv)
{
    ICEntry::Kind kind = info->frameKind;

    
    
    if (kind == ICEntry::Kind_DebugEpilogue)
        return true;

    
    
    if (kind == ICEntry::Kind_DebugPrologue)
        return rv;

    
    
    return false;
}

static inline bool
IsReturningFromCallVM(BaselineDebugModeOSRInfo* info)
{
    
    
    
    
    return info->frameKind == ICEntry::Kind_CallVM ||
           info->frameKind == ICEntry::Kind_StackCheck ||
           info->frameKind == ICEntry::Kind_EarlyStackCheck;
}

static void
EmitBranchICEntryKind(MacroAssembler& masm, Register entry, ICEntry::Kind kind, Label* label)
{
    masm.branch32(MacroAssembler::Equal,
                  Address(entry, offsetof(BaselineDebugModeOSRInfo, frameKind)),
                  Imm32(kind), label);
}

static void
EmitBranchIsReturningFromCallVM(MacroAssembler& masm, Register entry, Label* label)
{
    
    EmitBranchICEntryKind(masm, entry, ICEntry::Kind_CallVM, label);
    EmitBranchICEntryKind(masm, entry, ICEntry::Kind_StackCheck, label);
    EmitBranchICEntryKind(masm, entry, ICEntry::Kind_EarlyStackCheck, label);
}

static void
SyncBaselineDebugModeOSRInfo(BaselineFrame* frame, Value* vp, bool rv)
{
    BaselineDebugModeOSRInfo* info = frame->debugModeOSRInfo();
    MOZ_ASSERT(info);
    MOZ_ASSERT(frame->script()->baselineScript()->containsCodeAddress(info->resumeAddr));

    if (HasForcedReturn(info, rv)) {
        
        
        MOZ_ASSERT(R0 == JSReturnOperand);
        info->valueR0 = frame->returnValue();
        info->resumeAddr = frame->script()->baselineScript()->epilogueEntryAddr();
        return;
    }

    
    
    
    
    
    if (!IsReturningFromCallVM(info)) {
        unsigned numUnsynced = info->slotInfo.numUnsynced();
        MOZ_ASSERT(numUnsynced <= 2);
        if (numUnsynced > 0)
            info->popValueInto(info->slotInfo.topSlotLocation(), vp);
        if (numUnsynced > 1)
            info->popValueInto(info->slotInfo.nextSlotLocation(), vp);
    }

    
    info->stackAdjust *= sizeof(Value);
}

static void
FinishBaselineDebugModeOSR(BaselineFrame* frame)
{
    frame->deleteDebugModeOSRInfo();

    
    frame->clearOverridePc();
}

void
BaselineFrame::deleteDebugModeOSRInfo()
{
    js_delete(getDebugModeOSRInfo());
    flags_ &= ~HAS_DEBUG_MODE_OSR_INFO;
}

JitCode*
JitRuntime::getBaselineDebugModeOSRHandler(JSContext* cx)
{
    if (!baselineDebugModeOSRHandler_) {
        AutoLockForExclusiveAccess lock(cx);
        AutoCompartment ac(cx, cx->runtime()->atomsCompartment());
        uint32_t offset;
        if (JitCode* code = generateBaselineDebugModeOSRHandler(cx, &offset)) {
            baselineDebugModeOSRHandler_ = code;
            baselineDebugModeOSRHandlerNoFrameRegPopAddr_ = code->raw() + offset;
        }
    }

    return baselineDebugModeOSRHandler_;
}

void*
JitRuntime::getBaselineDebugModeOSRHandlerAddress(JSContext* cx, bool popFrameReg)
{
    if (!getBaselineDebugModeOSRHandler(cx))
        return nullptr;
    return popFrameReg
           ? baselineDebugModeOSRHandler_->raw()
           : baselineDebugModeOSRHandlerNoFrameRegPopAddr_;
}

static void
EmitBaselineDebugModeOSRHandlerTail(MacroAssembler& masm, Register temp, bool returnFromCallVM)
{
    
    
    
    
    
    
    if (returnFromCallVM) {
        masm.push(ReturnReg);
    } else {
        masm.pushValue(Address(temp, offsetof(BaselineDebugModeOSRInfo, valueR0)));
        masm.pushValue(Address(temp, offsetof(BaselineDebugModeOSRInfo, valueR1)));
    }
    masm.push(BaselineFrameReg);
    masm.push(Address(temp, offsetof(BaselineDebugModeOSRInfo, resumeAddr)));

    
    masm.setupUnalignedABICall(1, temp);
    masm.loadBaselineFramePtr(BaselineFrameReg, temp);
    masm.passABIArg(temp);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, FinishBaselineDebugModeOSR));

    
    AllocatableGeneralRegisterSet jumpRegs(GeneralRegisterSet::All());
    if (returnFromCallVM) {
        jumpRegs.take(ReturnReg);
    } else {
        jumpRegs.take(R0);
        jumpRegs.take(R1);
    }
    jumpRegs.take(BaselineFrameReg);
    Register target = jumpRegs.takeAny();

    masm.pop(target);
    masm.pop(BaselineFrameReg);
    if (returnFromCallVM) {
        masm.pop(ReturnReg);
    } else {
        masm.popValue(R1);
        masm.popValue(R0);
    }

    masm.jump(target);
}

JitCode*
JitRuntime::generateBaselineDebugModeOSRHandler(JSContext* cx, uint32_t* noFrameRegPopOffsetOut)
{
    MacroAssembler masm(cx);

    AllocatableGeneralRegisterSet regs(GeneralRegisterSet::All());
    regs.take(BaselineFrameReg);
    regs.take(ReturnReg);
    Register temp = regs.takeAny();
    Register syncedStackStart = regs.takeAny();

    
    masm.pop(BaselineFrameReg);

    
    
    CodeOffsetLabel noFrameRegPopOffset(masm.currentOffset());

    
    masm.moveStackPtrTo(syncedStackStart);
    masm.push(ReturnReg);
    masm.push(BaselineFrameReg);

    
    masm.setupUnalignedABICall(3, temp);
    masm.loadBaselineFramePtr(BaselineFrameReg, temp);
    masm.passABIArg(temp);
    masm.passABIArg(syncedStackStart);
    masm.passABIArg(ReturnReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, SyncBaselineDebugModeOSRInfo));

    
    
    
    
    masm.pop(BaselineFrameReg);
    masm.pop(ReturnReg);
    masm.loadPtr(Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfScratchValue()), temp);
    masm.addToStackPtr(Address(temp, offsetof(BaselineDebugModeOSRInfo, stackAdjust)));

    
    
    Label returnFromCallVM, end;
    EmitBranchIsReturningFromCallVM(masm, temp, &returnFromCallVM);

    EmitBaselineDebugModeOSRHandlerTail(masm, temp,  false);
    masm.jump(&end);
    masm.bind(&returnFromCallVM);
    EmitBaselineDebugModeOSRHandlerTail(masm, temp,  true);
    masm.bind(&end);

    Linker linker(masm);
    AutoFlushICache afc("BaselineDebugModeOSRHandler");
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);
    if (!code)
        return nullptr;

    noFrameRegPopOffset.fixup(&masm);
    *noFrameRegPopOffsetOut = noFrameRegPopOffset.offset();

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BaselineDebugModeOSRHandler");
#endif

    return code;
}

 void
DebugModeOSRVolatileJitFrameIterator::forwardLiveIterators(JSContext* cx,
                                                           uint8_t* oldAddr, uint8_t* newAddr)
{
    DebugModeOSRVolatileJitFrameIterator* iter;
    for (iter = cx->liveVolatileJitFrameIterators_; iter; iter = iter->prev) {
        if (iter->returnAddressToFp_ == oldAddr)
            iter->returnAddressToFp_ = newAddr;
    }
}
