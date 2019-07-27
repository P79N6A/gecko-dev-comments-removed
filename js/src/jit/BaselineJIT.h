





#ifndef jit_BaselineJIT_h
#define jit_BaselineJIT_h

#include "mozilla/MemoryReporting.h"

#include "jscntxt.h"
#include "jscompartment.h"

#include "ds/LifoAlloc.h"
#include "jit/Bailouts.h"
#include "jit/IonCode.h"
#include "jit/MacroAssembler.h"
#include "vm/TraceLogging.h"

namespace js {
namespace jit {

class StackValue;
class ICEntry;
class ICStub;

class PCMappingSlotInfo
{
    uint8_t slotInfo_;

  public:
    
    
    
    
    enum SlotLocation { SlotInR0 = 0, SlotInR1 = 1, SlotIgnore = 3 };

    PCMappingSlotInfo()
      : slotInfo_(0)
    { }

    explicit PCMappingSlotInfo(uint8_t slotInfo)
      : slotInfo_(slotInfo)
    { }

    static inline bool ValidSlotLocation(SlotLocation loc) {
        return (loc == SlotInR0) || (loc == SlotInR1) || (loc == SlotIgnore);
    }

    static SlotLocation ToSlotLocation(const StackValue* stackVal);

    inline static PCMappingSlotInfo MakeSlotInfo() { return PCMappingSlotInfo(0); }

    inline static PCMappingSlotInfo MakeSlotInfo(SlotLocation topSlotLoc) {
        MOZ_ASSERT(ValidSlotLocation(topSlotLoc));
        return PCMappingSlotInfo(1 | (topSlotLoc << 2));
    }

    inline static PCMappingSlotInfo MakeSlotInfo(SlotLocation topSlotLoc, SlotLocation nextSlotLoc) {
        MOZ_ASSERT(ValidSlotLocation(topSlotLoc));
        MOZ_ASSERT(ValidSlotLocation(nextSlotLoc));
        return PCMappingSlotInfo(2 | (topSlotLoc << 2) | (nextSlotLoc) << 4);
    }

    inline unsigned numUnsynced() const {
        return slotInfo_ & 0x3;
    }
    inline SlotLocation topSlotLocation() const {
        return static_cast<SlotLocation>((slotInfo_ >> 2) & 0x3);
    }
    inline SlotLocation nextSlotLocation() const {
        return static_cast<SlotLocation>((slotInfo_ >> 4) & 0x3);
    }
    inline uint8_t toByte() const {
        return slotInfo_;
    }
};






struct PCMappingIndexEntry
{
    
    uint32_t pcOffset;

    
    uint32_t nativeOffset;

    
    uint32_t bufferOffset;
};



struct DependentAsmJSModuleExit
{
    const AsmJSModule* module;
    size_t exitIndex;

    DependentAsmJSModuleExit(const AsmJSModule* module, size_t exitIndex)
      : module(module),
        exitIndex(exitIndex)
    { }
};

struct BaselineScript
{
  public:
    static const uint32_t MAX_JSSCRIPT_LENGTH = 0x0fffffffu;

    
    
    
    static const uint32_t MAX_JSSCRIPT_SLOTS = 0xffffu;

  private:
    
    HeapPtrJitCode method_;

    
    
    HeapPtrObject templateScope_;

    
    FallbackICStubSpace fallbackStubSpace_;

    
    
    Vector<DependentAsmJSModuleExit>* dependentAsmJSModules_;

    
    uint32_t prologueOffset_;

    
    
    uint32_t epilogueOffset_;

    
    uint32_t profilerEnterToggleOffset_;
    uint32_t profilerExitToggleOffset_;

    
#ifdef JS_TRACE_LOGGING
# ifdef DEBUG
    bool traceLoggerScriptsEnabled_;
    bool traceLoggerEngineEnabled_;
# endif
    uint32_t traceLoggerEnterToggleOffset_;
    uint32_t traceLoggerExitToggleOffset_;
    TraceLoggerEvent traceLoggerScriptEvent_;
#endif

    
    
    
    
    
    
    uint32_t postDebugPrologueOffset_;

  public:
    enum Flag {
        
        
        NEEDS_ARGS_OBJ = 1 << 0,

        
        
        ACTIVE = 1 << 1,

        
        
        MODIFIES_ARGUMENTS = 1 << 2,

        
        
        HAS_DEBUG_INSTRUMENTATION = 1 << 3,

        
        
        
        ION_COMPILED_OR_INLINED = 1 << 4,

        
        PROFILER_INSTRUMENTATION_ON = 1 << 5
    };

  private:
    uint32_t flags_;

  private:
    void trace(JSTracer* trc);

    uint32_t icEntriesOffset_;
    uint32_t icEntries_;

    uint32_t pcMappingIndexOffset_;
    uint32_t pcMappingIndexEntries_;

    uint32_t pcMappingOffset_;
    uint32_t pcMappingSize_;

    
    
    uint32_t bytecodeTypeMapOffset_;

    
    
    uint32_t yieldEntriesOffset_;

    
    
    
    uint16_t inlinedBytecodeLength_;

    
    
    
    
    
    uint8_t maxInliningDepth_;

  public:
    
    BaselineScript(uint32_t prologueOffset, uint32_t epilogueOffset,
                   uint32_t profilerEnterToggleOffset,
                   uint32_t profilerExitToggleOffset,
                   uint32_t traceLoggerEnterToggleOffset,
                   uint32_t traceLoggerExitToggleOffset,
                   uint32_t postDebugPrologueOffset);

    static BaselineScript* New(JSScript* jsscript, uint32_t prologueOffset,
                               uint32_t epilogueOffset, uint32_t postDebugPrologueOffset,
                               uint32_t profilerEnterToggleOffset,
                               uint32_t profilerExitToggleOffset,
                               uint32_t traceLoggerEnterToggleOffset,
                               uint32_t traceLoggerExitToggleOffset,
                               size_t icEntries, size_t pcMappingIndexEntries,
                               size_t pcMappingSize,
                               size_t bytecodeTypeMapEntries, size_t yieldEntries);

    static void Trace(JSTracer* trc, BaselineScript* script);
    static void Destroy(FreeOp* fop, BaselineScript* script);

    void purgeOptimizedStubs(Zone* zone);

    static inline size_t offsetOfMethod() {
        return offsetof(BaselineScript, method_);
    }

    void addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf, size_t* data,
                                size_t* fallbackStubs) const {
        *data += mallocSizeOf(this);

        
        
        *fallbackStubs += fallbackStubSpace_.sizeOfExcludingThis(mallocSizeOf);
    }

    bool active() const {
        return flags_ & ACTIVE;
    }
    void setActive() {
        flags_ |= ACTIVE;
    }
    void resetActive() {
        flags_ &= ~ACTIVE;
    }

    void setNeedsArgsObj() {
        flags_ |= NEEDS_ARGS_OBJ;
    }

    void setModifiesArguments() {
        flags_ |= MODIFIES_ARGUMENTS;
    }
    bool modifiesArguments() {
        return flags_ & MODIFIES_ARGUMENTS;
    }

    void setHasDebugInstrumentation() {
        flags_ |= HAS_DEBUG_INSTRUMENTATION;
    }
    bool hasDebugInstrumentation() const {
        return flags_ & HAS_DEBUG_INSTRUMENTATION;
    }

    void setIonCompiledOrInlined() {
        flags_ |= ION_COMPILED_OR_INLINED;
    }
    void clearIonCompiledOrInlined() {
        flags_ &= ~ION_COMPILED_OR_INLINED;
    }
    bool ionCompiledOrInlined() const {
        return flags_ & ION_COMPILED_OR_INLINED;
    }

    uint32_t prologueOffset() const {
        return prologueOffset_;
    }
    uint8_t* prologueEntryAddr() const {
        return method_->raw() + prologueOffset_;
    }

    uint32_t epilogueOffset() const {
        return epilogueOffset_;
    }
    uint8_t* epilogueEntryAddr() const {
        return method_->raw() + epilogueOffset_;
    }

    uint32_t postDebugPrologueOffset() const {
        return postDebugPrologueOffset_;
    }
    uint8_t* postDebugPrologueAddr() const {
        return method_->raw() + postDebugPrologueOffset_;
    }

    ICEntry* icEntryList() {
        return (ICEntry*)(reinterpret_cast<uint8_t*>(this) + icEntriesOffset_);
    }
    uint8_t** yieldEntryList() {
        return (uint8_t**)(reinterpret_cast<uint8_t*>(this) + yieldEntriesOffset_);
    }
    PCMappingIndexEntry* pcMappingIndexEntryList() {
        return (PCMappingIndexEntry*)(reinterpret_cast<uint8_t*>(this) + pcMappingIndexOffset_);
    }
    uint8_t* pcMappingData() {
        return reinterpret_cast<uint8_t*>(this) + pcMappingOffset_;
    }
    FallbackICStubSpace* fallbackStubSpace() {
        return &fallbackStubSpace_;
    }

    JitCode* method() const {
        return method_;
    }
    void setMethod(JitCode* code) {
        MOZ_ASSERT(!method_);
        method_ = code;
    }

    JSObject* templateScope() const {
        return templateScope_;
    }
    void setTemplateScope(JSObject* templateScope) {
        MOZ_ASSERT(!templateScope_);
        templateScope_ = templateScope;
    }

    void toggleBarriers(bool enabled) {
        method()->togglePreBarriers(enabled);
    }

    bool containsCodeAddress(uint8_t* addr) const {
        return method()->raw() <= addr && addr <= method()->raw() + method()->instructionsSize();
    }

    ICEntry& icEntry(size_t index);
    ICEntry& icEntryFromReturnOffset(CodeOffsetLabel returnOffset);
    ICEntry& icEntryFromPCOffset(uint32_t pcOffset);
    ICEntry& icEntryFromPCOffset(uint32_t pcOffset, ICEntry* prevLookedUpEntry);
    ICEntry& callVMEntryFromPCOffset(uint32_t pcOffset);
    ICEntry& stackCheckICEntry(bool earlyCheck);
    ICEntry& icEntryFromReturnAddress(uint8_t* returnAddr);
    uint8_t* returnAddressForIC(const ICEntry& ent);

    size_t numICEntries() const {
        return icEntries_;
    }

    void copyICEntries(JSScript* script, const ICEntry* entries, MacroAssembler& masm);
    void adoptFallbackStubs(FallbackICStubSpace* stubSpace);

    void copyYieldEntries(JSScript* script, Vector<uint32_t>& yieldOffsets);

    PCMappingIndexEntry& pcMappingIndexEntry(size_t index);
    CompactBufferReader pcMappingReader(size_t indexEntry);

    size_t numPCMappingIndexEntries() const {
        return pcMappingIndexEntries_;
    }

    void copyPCMappingIndexEntries(const PCMappingIndexEntry* entries);
    void copyPCMappingEntries(const CompactBufferWriter& entries);

    uint8_t* nativeCodeForPC(JSScript* script, jsbytecode* pc,
                             PCMappingSlotInfo* slotInfo = nullptr);

    
    
    
    jsbytecode* approximatePcForNativeAddress(JSScript* script, uint8_t* nativeAddress);

    bool addDependentAsmJSModule(JSContext* cx, DependentAsmJSModuleExit exit);
    void unlinkDependentAsmJSModules(FreeOp* fop);
    void removeDependentAsmJSModule(DependentAsmJSModuleExit exit);

    
    
    
    void toggleDebugTraps(JSScript* script, jsbytecode* pc);

    void toggleProfilerInstrumentation(bool enable);
    bool isProfilerInstrumentationOn() const {
        return flags_ & PROFILER_INSTRUMENTATION_ON;
    }

#ifdef JS_TRACE_LOGGING
    void initTraceLogger(JSRuntime* runtime, JSScript* script);
    void toggleTraceLoggerScripts(JSRuntime* runtime, JSScript* script, bool enable);
    void toggleTraceLoggerEngine(bool enable);

    static size_t offsetOfTraceLoggerScriptEvent() {
        return offsetof(BaselineScript, traceLoggerScriptEvent_);
    }
#endif

    void noteAccessedGetter(uint32_t pcOffset);
    void noteArrayWriteHole(uint32_t pcOffset);

    static size_t offsetOfFlags() {
        return offsetof(BaselineScript, flags_);
    }
    static size_t offsetOfYieldEntriesOffset() {
        return offsetof(BaselineScript, yieldEntriesOffset_);
    }

    static void writeBarrierPre(Zone* zone, BaselineScript* script);

    uint32_t* bytecodeTypeMap() {
        MOZ_ASSERT(bytecodeTypeMapOffset_);
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(this) + bytecodeTypeMapOffset_);
    }

    uint8_t maxInliningDepth() const {
        return maxInliningDepth_;
    }
    void setMaxInliningDepth(uint32_t depth) {
        MOZ_ASSERT(depth <= UINT8_MAX);
        maxInliningDepth_ = depth;
    }
    void resetMaxInliningDepth() {
        maxInliningDepth_ = UINT8_MAX;
    }

    uint16_t inlinedBytecodeLength() const {
        return inlinedBytecodeLength_;
    }
    void setInlinedBytecodeLength(uint32_t len) {
        if (len > UINT16_MAX)
            len = UINT16_MAX;
        inlinedBytecodeLength_ = len;
    }
};
static_assert(sizeof(BaselineScript) % sizeof(uintptr_t) == 0,
              "The data attached to the script must be aligned for fast JIT access.");

inline bool
IsBaselineEnabled(JSContext* cx)
{
#ifdef JS_CODEGEN_NONE
    return false;
#else
    return cx->runtime()->options().baseline();
#endif
}

MethodStatus
CanEnterBaselineMethod(JSContext* cx, RunState& state);

MethodStatus
CanEnterBaselineAtBranch(JSContext* cx, InterpreterFrame* fp, bool newType);

JitExecStatus
EnterBaselineMethod(JSContext* cx, RunState& state);

JitExecStatus
EnterBaselineAtBranch(JSContext* cx, InterpreterFrame* fp, jsbytecode* pc);

void
FinishDiscardBaselineScript(FreeOp* fop, JSScript* script);

void
AddSizeOfBaselineData(JSScript* script, mozilla::MallocSizeOf mallocSizeOf, size_t* data,
                      size_t* fallbackStubs);

void
ToggleBaselineProfiling(JSRuntime* runtime, bool enable);

void
ToggleBaselineTraceLoggerScripts(JSRuntime* runtime, bool enable);
void
ToggleBaselineTraceLoggerEngine(JSRuntime* runtime, bool enable);

struct BaselineBailoutInfo
{
    
    uint8_t* incomingStack;

    
    
    uint8_t* copyStackTop;
    uint8_t* copyStackBottom;

    
    
    
    uint32_t setR0;
    Value valueR0;
    uint32_t setR1;
    Value valueR1;

    
    void* resumeFramePtr;

    
    void* resumeAddr;

    
    jsbytecode* resumePC;

    
    
    
    
    
    
    ICStub* monitorStub;

    
    uint32_t numFrames;

    
    BailoutKind bailoutKind;
};

uint32_t
BailoutIonToBaseline(JSContext* cx, JitActivation* activation, JitFrameIterator& iter,
                     bool invalidate, BaselineBailoutInfo** bailoutInfo,
                     const ExceptionBailoutInfo* exceptionInfo);



void
MarkActiveBaselineScripts(Zone* zone);

MethodStatus
BaselineCompile(JSContext* cx, JSScript* script, bool forceDebugInstrumentation = false);

} 
} 

#endif 
