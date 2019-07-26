






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


struct ICStubSpace
{
  private:
    const static size_t STUB_DEFAULT_CHUNK_SIZE = 256;
    LifoAlloc allocator_;

  public:
    inline ICStubSpace()
      : allocator_(STUB_DEFAULT_CHUNK_SIZE) {}

    inline void *alloc(size_t size) {
        return allocator_.alloc(size);
    }

    JS_DECLARE_NEW_METHODS(allocate, alloc, inline)

    inline void adoptFrom(ICStubSpace *other) {
        allocator_.transferFrom(&(other->allocator_));
    }

    static ICStubSpace *FallbackStubSpaceFor(JSScript *script);
    static ICStubSpace *StubSpaceFor(JSScript *script);
};


struct PCMappingEntry
{
    enum SlotLocation { SlotInR0 = 0, SlotInR1 = 1, SlotIgnore = 3 };

    static inline bool ValidSlotLocation(SlotLocation loc) {
        return (loc == SlotInR0) || (loc == SlotInR1) || (loc == SlotIgnore);
    }

    uint32_t pcOffset;
    uint32_t nativeOffset;
    
    
    
    
    uint8_t slotInfo;

    void fixupNativeOffset(MacroAssembler &masm) {
        CodeOffsetLabel offset(nativeOffset);
        offset.fixup(&masm);
        JS_ASSERT(offset.offset() <= UINT32_MAX);
        nativeOffset = (uint32_t) offset.offset();
    }

    static SlotLocation ToSlotLocation(const StackValue *stackVal);
    inline static uint8_t MakeSlotInfo() { return static_cast<uint8_t>(0); }
    inline static uint8_t MakeSlotInfo(SlotLocation topSlotLoc) {
        JS_ASSERT(ValidSlotLocation(topSlotLoc));
        return static_cast<uint8_t>(1) | (static_cast<uint8_t>(topSlotLoc)) << 2;
    }
    inline static uint8_t MakeSlotInfo(SlotLocation topSlotLoc, SlotLocation nextSlotLoc) {
        JS_ASSERT(ValidSlotLocation(topSlotLoc));
        JS_ASSERT(ValidSlotLocation(nextSlotLoc));
        return static_cast<uint8_t>(2) | (static_cast<uint8_t>(topSlotLoc) << 2)
                                       | (static_cast<uint8_t>(nextSlotLoc) << 4);
    }

    inline static unsigned SlotInfoNumUnsynced(uint8_t slotInfo) {
        return static_cast<unsigned>(slotInfo & 0x3);
    }
    inline static SlotLocation SlotInfoTopSlotLocation(uint8_t slotInfo) {
        return static_cast<SlotLocation>((slotInfo >> 2) & 0x3);
    }
    inline static SlotLocation SlotInfoNextSlotLocation(uint8_t slotInfo) {
        return static_cast<SlotLocation>((slotInfo >> 4) & 0x3);
    }
};

struct BaselineScript
{
  public:
    static const uint32_t MAX_JSSCRIPT_LENGTH = 0x0fffffffu;

  private:
    
    HeapPtr<IonCode> method_;

    
    ICStubSpace fallbackStubSpace_;

    
    ICStubSpace optimizedStubSpace_;

    
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

    uint32_t pcMappingOffset_;
    uint32_t pcMappingEntries_;

  public:
    
    BaselineScript(uint32_t prologueOffset);

    static BaselineScript *New(JSContext *cx, uint32_t prologueOffset, size_t icEntries,
                               size_t pcMappingEntries);
    static void Trace(JSTracer *trc, BaselineScript *script);
    static void Destroy(FreeOp *fop, BaselineScript *script);

    static inline size_t offsetOfMethod() {
        return offsetof(BaselineScript, method_);
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
    PCMappingEntry *pcMappingEntryList() {
        return (PCMappingEntry *)(reinterpret_cast<uint8_t *>(this) + pcMappingOffset_);
    }

    ICStubSpace *fallbackStubSpace() {
        return &fallbackStubSpace_;
    }

    ICStubSpace *optimizedStubSpace() {
        return &optimizedStubSpace_;
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
    ICEntry &icEntryFromReturnAddress(uint8_t *returnAddr);
    uint8_t *returnAddressForIC(const ICEntry &ent);

    size_t numICEntries() const {
        return icEntries_;
    }

    void copyICEntries(HandleScript script, const ICEntry *entries, MacroAssembler &masm);
    void adoptFallbackStubs(ICStubSpace *stubSpace);

    size_t numPCMappingEntries() const {
        return pcMappingEntries_;
    }

    PCMappingEntry &pcMappingEntry(size_t index);
    void copyPCMappingEntries(const PCMappingEntry *entries, MacroAssembler &masm);
    uint8_t *nativeCodeForPC(HandleScript script, jsbytecode *pc);
    uint8_t slotInfoForPC(HandleScript script, jsbytecode *pc);

    
    
    
    void toggleDebugTraps(UnrootedScript script, jsbytecode *pc);

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

void
FinishDiscardBaselineScript(FreeOp *fop, UnrootedScript script);

struct BaselineBailoutInfo
{
    
    void *incomingStack;

    
    
    uint8_t *copyStackTop;
    uint8_t *copyStackBottom;

    
    
    
    uint32_t setR0;
    Value valueR0;
    uint32_t setR1;
    Value valueR1;

    
    void *resumeFramePtr;

    
    void *resumeAddr;

    
    
    
    
    
    
    ICStub *monitorStub;
};

uint32_t
BailoutIonToBaseline(JSContext *cx, IonActivation *activation, IonBailoutIterator &iter,
                     bool invalidate, BaselineBailoutInfo **bailoutInfo);

} 
} 

#endif

