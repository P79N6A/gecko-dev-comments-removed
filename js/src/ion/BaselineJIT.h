






#if !defined(jsion_baseline_jit_h__) && defined(JS_ION)
#define jsion_baseline_jit_h__

#include "jscntxt.h"
#include "jscompartment.h"

#include "IonCode.h"
#include "IonMacroAssembler.h"
#include "Bailouts.h"

#include "ds/LifoAlloc.h"

namespace js {
namespace ion {

class StackValue;
struct ICEntry;
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

  private:
    
    HeapPtr<IonCode> method_;

    
    FallbackICStubSpace fallbackStubSpace_;

    
    uint32_t prologueOffset_;

  public:
    enum Flag {
        
        
        NEEDS_ARGS_OBJ = 1 << 0,

        
        
        ACTIVE         = 1 << 1
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

  public:
    
    BaselineScript(uint32_t prologueOffset);

    static BaselineScript *New(JSContext *cx, uint32_t prologueOffset, size_t icEntries,
                               size_t pcMappingIndexEntries, size_t pcMappingSize);
    static void Trace(JSTracer *trc, BaselineScript *script);
    static void Destroy(FreeOp *fop, BaselineScript *script);

    void purgeOptimizedStubs(Zone *zone);

    static inline size_t offsetOfMethod() {
        return offsetof(BaselineScript, method_);
    }

    void sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf, size_t *data,
                             size_t *fallbackStubs) const {
        *data = mallocSizeOf(this);

        
        
        *fallbackStubs = fallbackStubSpace_.sizeOfExcludingThis(mallocSizeOf);
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

    uint32_t prologueOffset() const {
        return prologueOffset_;
    }
    uint8_t *prologueEntryAddr() const {
        return method_->raw() + prologueOffset_;
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

    IonCode *method() const {
        return method_;
    }
    void setMethod(IonCode *code) {
        JS_ASSERT(!method_);
        method_ = code;
    }

    void toggleBarriers(bool enabled) {
        method()->togglePreBarriers(enabled);
    }

    ICEntry &icEntry(size_t index);
    ICEntry &icEntryFromReturnOffset(CodeOffsetLabel returnOffset);
    ICEntry &icEntryFromPCOffset(uint32_t pcOffset);
    ICEntry &icEntryFromPCOffset(uint32_t pcOffset, ICEntry *prevLookedUpEntry);
    ICEntry &icEntryFromReturnAddress(uint8_t *returnAddr);
    uint8_t *returnAddressForIC(const ICEntry &ent);

    size_t numICEntries() const {
        return icEntries_;
    }

    void copyICEntries(HandleScript script, const ICEntry *entries, MacroAssembler &masm);
    void adoptFallbackStubs(FallbackICStubSpace *stubSpace);

    PCMappingIndexEntry &pcMappingIndexEntry(size_t index);
    CompactBufferReader pcMappingReader(size_t indexEntry);

    size_t numPCMappingIndexEntries() const {
        return pcMappingIndexEntries_;
    }

    void copyPCMappingIndexEntries(const PCMappingIndexEntry *entries);

    void copyPCMappingEntries(const CompactBufferWriter &entries);
    uint8_t *nativeCodeForPC(JSScript *script, jsbytecode *pc, PCMappingSlotInfo *slotInfo = NULL);

    
    
    
    void toggleDebugTraps(RawScript script, jsbytecode *pc);

    static size_t offsetOfFlags() {
        return offsetof(BaselineScript, flags_);
    }
};

inline bool IsBaselineEnabled(JSContext *cx)
{
    return cx->hasOption(JSOPTION_BASELINE);
}

MethodStatus
CanEnterBaselineJIT(JSContext *cx, JSScript *scriptArg, StackFrame *fp, bool newType);

IonExecStatus
EnterBaselineMethod(JSContext *cx, StackFrame *fp);

IonExecStatus
EnterBaselineAtBranch(JSContext *cx, StackFrame *fp, jsbytecode *pc);

void
FinishDiscardBaselineScript(FreeOp *fop, RawScript script);

void
SizeOfBaselineData(JSScript *script, JSMallocSizeOfFun mallocSizeOf, size_t *data,
                   size_t *fallbackStubs);

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
BailoutIonToBaseline(JSContext *cx, IonActivation *activation, IonBailoutIterator &iter,
                     bool invalidate, BaselineBailoutInfo **bailoutInfo);



void
MarkActiveBaselineScripts(Zone *zone);

} 
} 

#endif

