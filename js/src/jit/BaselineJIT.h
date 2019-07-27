





#ifndef jit_BaselineJIT_h
#define jit_BaselineJIT_h

#include "mozilla/MemoryReporting.h"

#include "jscntxt.h"
#include "jscompartment.h"

#include "ds/LifoAlloc.h"
#include "jit/Bailouts.h"
#include "jit/IonCode.h"
#include "jit/IonMacroAssembler.h"

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

    static SlotLocation ToSlotLocation(const StackValue *stackVal);

    inline static PCMappingSlotInfo MakeSlotInfo() { return PCMappingSlotInfo(0); }

    inline static PCMappingSlotInfo MakeSlotInfo(SlotLocation topSlotLoc) {
        JS_ASSERT(ValidSlotLocation(topSlotLoc));
        return PCMappingSlotInfo(1 | (topSlotLoc << 2));
    }

    inline static PCMappingSlotInfo MakeSlotInfo(SlotLocation topSlotLoc, SlotLocation nextSlotLoc) {
        JS_ASSERT(ValidSlotLocation(topSlotLoc));
        JS_ASSERT(ValidSlotLocation(nextSlotLoc));
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

struct BaselineScript
{
  public:
    static const uint32_t MAX_JSSCRIPT_LENGTH = 0x0fffffffu;

    
    
    
    static const uint32_t MAX_JSSCRIPT_SLOTS = 0xffffu;

  private:
    
    HeapPtrJitCode method_;

    
    
    HeapPtrObject templateScope_;

    
    FallbackICStubSpace fallbackStubSpace_;

    
    uint32_t prologueOffset_;

    
    
    uint32_t epilogueOffset_;

    
#ifdef DEBUG
    mozilla::DebugOnly<bool> spsOn_;
#endif
    uint32_t spsPushToggleOffset_;

    
    
    
    
    
    
    uint32_t postDebugPrologueOffset_;

  public:
    enum Flag {
        
        
        NEEDS_ARGS_OBJ = 1 << 0,

        
        
        ACTIVE = 1 << 1,

        
        
        MODIFIES_ARGUMENTS = 1 << 2,

        
        
        DEBUG_MODE = 1 << 3,

        
        
        
        ION_COMPILED_OR_INLINED = 1 << 4
    };

  private:
    uint32_t flags_;

  private:
    void trace(JSTracer *trc);

    uint32_t icEntriesOffset_;
    uint32_t icEntries_;

    uint32_t pcMappingIndexOffset_;
    uint32_t pcMappingIndexEntries_;

    uint32_t pcMappingOffset_;
    uint32_t pcMappingSize_;

    
    
    uint32_t bytecodeTypeMapOffset_;

  public:
    
    BaselineScript(uint32_t prologueOffset, uint32_t epilogueOffset,
                   uint32_t spsPushToggleOffset, uint32_t postDebugPrologueOffset);

    static BaselineScript *New(JSScript *script, uint32_t prologueOffset,
                               uint32_t epilogueOffset, uint32_t postDebugPrologueOffset,
                               uint32_t spsPushToggleOffset, size_t icEntries,
                               size_t pcMappingIndexEntries, size_t pcMappingSize,
                               size_t bytecodeTypeMapEntries);
    static void Trace(JSTracer *trc, BaselineScript *script);
    static void Destroy(FreeOp *fop, BaselineScript *script);

    void purgeOptimizedStubs(Zone *zone);

    static inline size_t offsetOfMethod() {
        return offsetof(BaselineScript, method_);
    }

    void addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf, size_t *data,
                                size_t *fallbackStubs) const {
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

    void setDebugMode() {
        flags_ |= DEBUG_MODE;
    }
    bool debugMode() const {
        return flags_ & DEBUG_MODE;
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
    uint8_t *prologueEntryAddr() const {
        return method_->raw() + prologueOffset_;
    }

    uint32_t epilogueOffset() const {
        return epilogueOffset_;
    }
    uint8_t *epilogueEntryAddr() const {
        return method_->raw() + epilogueOffset_;
    }

    uint32_t postDebugPrologueOffset() const {
        return postDebugPrologueOffset_;
    }
    uint8_t *postDebugPrologueAddr() const {
        return method_->raw() + postDebugPrologueOffset_;
    }

    ICEntry *icEntryList() {
        return (ICEntry *)(reinterpret_cast<uint8_t *>(this) + icEntriesOffset_);
    }
    PCMappingIndexEntry *pcMappingIndexEntryList() {
        return (PCMappingIndexEntry *)(reinterpret_cast<uint8_t *>(this) + pcMappingIndexOffset_);
    }
    uint8_t *pcMappingData() {
        return reinterpret_cast<uint8_t *>(this) + pcMappingOffset_;
    }
    FallbackICStubSpace *fallbackStubSpace() {
        return &fallbackStubSpace_;
    }

    JitCode *method() const {
        return method_;
    }
    void setMethod(JitCode *code) {
        JS_ASSERT(!method_);
        method_ = code;
    }

    JSObject *templateScope() const {
        return templateScope_;
    }
    void setTemplateScope(JSObject *templateScope) {
        JS_ASSERT(!templateScope_);
        templateScope_ = templateScope;
    }

    void toggleBarriers(bool enabled) {
        method()->togglePreBarriers(enabled);
    }

    bool containsCodeAddress(uint8_t *addr) const {
        return method()->raw() <= addr && addr <= method()->raw() + method()->instructionsSize();
    }

    ICEntry &icEntry(size_t index);
    ICEntry *maybeICEntryFromReturnOffset(CodeOffsetLabel returnOffset);
    ICEntry &icEntryFromReturnOffset(CodeOffsetLabel returnOffset);
    ICEntry &icEntryFromPCOffset(uint32_t pcOffset);
    ICEntry &icEntryForDebugModeRecompileFromPCOffset(uint32_t pcOffset);
    ICEntry &icEntryFromPCOffset(uint32_t pcOffset, ICEntry *prevLookedUpEntry);
    ICEntry *maybeICEntryFromReturnAddress(uint8_t *returnAddr);
    ICEntry &icEntryFromReturnAddress(uint8_t *returnAddr);
    uint8_t *returnAddressForIC(const ICEntry &ent);

    size_t numICEntries() const {
        return icEntries_;
    }

    void copyICEntries(JSScript *script, const ICEntry *entries, MacroAssembler &masm);
    void adoptFallbackStubs(FallbackICStubSpace *stubSpace);

    PCMappingIndexEntry &pcMappingIndexEntry(size_t index);
    CompactBufferReader pcMappingReader(size_t indexEntry);

    size_t numPCMappingIndexEntries() const {
        return pcMappingIndexEntries_;
    }

    void copyPCMappingIndexEntries(const PCMappingIndexEntry *entries);

    void copyPCMappingEntries(const CompactBufferWriter &entries);
    uint8_t *nativeCodeForPC(JSScript *script, jsbytecode *pc, PCMappingSlotInfo *slotInfo = nullptr);

    jsbytecode *pcForReturnOffset(JSScript *script, uint32_t nativeOffset);
    jsbytecode *pcForReturnAddress(JSScript *script, uint8_t *nativeAddress);

    jsbytecode *pcForNativeAddress(JSScript *script, uint8_t *nativeAddress);
    jsbytecode *pcForNativeOffset(JSScript *script, uint32_t nativeOffset);

  private:
    jsbytecode *pcForNativeOffset(JSScript *script, uint32_t nativeOffset, bool isReturn);

  public:
    
    
    
    void toggleDebugTraps(JSScript *script, jsbytecode *pc);

    void toggleSPS(bool enable);

    void noteAccessedGetter(uint32_t pcOffset);
    void noteArrayWriteHole(uint32_t pcOffset);

    static size_t offsetOfFlags() {
        return offsetof(BaselineScript, flags_);
    }

    static void writeBarrierPre(Zone *zone, BaselineScript *script);

    uint32_t *bytecodeTypeMap() {
        JS_ASSERT(bytecodeTypeMapOffset_);
        return reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(this) + bytecodeTypeMapOffset_);
    }
};
static_assert(sizeof(BaselineScript) % sizeof(uintptr_t) == 0,
              "The data attached to the script must be aligned for fast JIT access.");

inline bool
IsBaselineEnabled(JSContext *cx)
{
#ifdef JS_CODEGEN_NONE
    return false;
#else
    return cx->runtime()->options().baseline();
#endif
}

MethodStatus
CanEnterBaselineMethod(JSContext *cx, RunState &state);

MethodStatus
CanEnterBaselineAtBranch(JSContext *cx, InterpreterFrame *fp, bool newType);

IonExecStatus
EnterBaselineMethod(JSContext *cx, RunState &state);

IonExecStatus
EnterBaselineAtBranch(JSContext *cx, InterpreterFrame *fp, jsbytecode *pc);

void
FinishDiscardBaselineScript(FreeOp *fop, JSScript *script);

void
AddSizeOfBaselineData(JSScript *script, mozilla::MallocSizeOf mallocSizeOf, size_t *data,
                      size_t *fallbackStubs);

void
ToggleBaselineSPS(JSRuntime *runtime, bool enable);

struct BaselineBailoutInfo
{
    
    uint8_t *incomingStack;

    
    
    uint8_t *copyStackTop;
    uint8_t *copyStackBottom;

    
    
    
    uint32_t setR0;
    Value valueR0;
    uint32_t setR1;
    Value valueR1;

    
    void *resumeFramePtr;

    
    void *resumeAddr;

    
    
    
    
    
    
    ICStub *monitorStub;

    
    uint32_t numFrames;

    
    BailoutKind bailoutKind;
};

uint32_t
BailoutIonToBaseline(JSContext *cx, JitActivation *activation, IonBailoutIterator &iter,
                     bool invalidate, BaselineBailoutInfo **bailoutInfo,
                     const ExceptionBailoutInfo *exceptionInfo = nullptr);



void
MarkActiveBaselineScripts(Zone *zone);

MethodStatus
BaselineCompile(JSContext *cx, JSScript *script);

} 
} 

#endif 
