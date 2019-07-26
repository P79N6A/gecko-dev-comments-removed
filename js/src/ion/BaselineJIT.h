






#if !defined(jsion_baseline_jit_h__) && defined(JS_ION)
#define jsion_baseline_jit_h__

#include "jscntxt.h"
#include "jscompartment.h"

#include "IonCode.h"
#include "IonMacroAssembler.h"

#include "ds/LifoAlloc.h"

namespace js {
namespace ion {

class StackValue;
struct ICEntry;


struct ICStubSpace
{
  private:
    const static size_t STUB_DEFAULT_CHUNK_SIZE = 256;
    LifoAlloc allocator_;

    inline void *alloc_(size_t size) {
        return allocator_.alloc(size);
    }

  public:
    inline ICStubSpace()
      : allocator_(STUB_DEFAULT_CHUNK_SIZE) {}

    JS_DECLARE_NEW_METHODS(allocate, alloc_, inline)

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

    inline static int SlotInfoNumUnsynced(uint8_t slotInfo) {
        return static_cast<int>(slotInfo & 0x3);
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
  private:
    
    HeapPtr<IonCode> method_;

    
    ICStubSpace fallbackStubSpace_;

    
    ICStubSpace optimizedStubSpace_;

    
    
    bool active_;

  private:
    void trace(JSTracer *trc);

    uint32_t icEntriesOffset_;
    uint32_t icEntries_;

    uint32_t pcMappingOffset_;
    uint32_t pcMappingEntries_;

  public:
    
    BaselineScript();

    static BaselineScript *New(JSContext *cx, size_t icEntries, size_t pcMappingEntries);
    static void Trace(JSTracer *trc, BaselineScript *script);
    static void Destroy(FreeOp *fop, BaselineScript *script);

    static inline size_t offsetOfMethod() {
        return offsetof(BaselineScript, method_);
    }

    bool active() const {
        return active_;
    }
    void setActive() {
        active_ = true;
    }
    void resetActive() {
        active_ = false;
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
        
        method_ = code;
    }

    ICEntry &icEntry(size_t index);
    ICEntry &icEntryFromReturnOffset(CodeOffsetLabel returnOffset);
    ICEntry &icEntryFromReturnAddress(uint8_t *returnAddr);

    size_t numICEntries() const {
        return icEntries_;
    }

    void copyICEntries(const ICEntry *entries, MacroAssembler &masm);
    void adoptFallbackStubs(ICStubSpace *stubSpace);

    size_t numPCMappingEntries() const {
        return pcMappingEntries_;
    }

    PCMappingEntry &pcMappingEntry(size_t index);
    void copyPCMappingEntries(const PCMappingEntry *entries, MacroAssembler &masm);
    uint8_t *nativeCodeForPC(HandleScript script, jsbytecode *pc);

    
    
    
    void toggleDebugTraps(UnrootedScript script, jsbytecode *pc);
};

inline bool IsBaselineEnabled(JSContext *cx)
{
    return cx->hasRunOption(JSOPTION_BASELINE);
}

MethodStatus
CanEnterBaselineJIT(JSContext *cx, JSScript *scriptArg, StackFrame *fp, bool newType);

IonExecStatus
EnterBaselineMethod(JSContext *cx, StackFrame *fp);

void
FinishDiscardBaselineScript(FreeOp *fop, UnrootedScript script);

} 
} 

#endif

