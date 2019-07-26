





#ifndef jit_Safepoints_h
#define jit_Safepoints_h

#include "jit/CompactBuffer.h"
#include "jit/shared/Assembler-shared.h"

namespace js {
namespace jit {

class BitSet;
struct SafepointNunboxEntry;
class LAllocation;
class LSafepoint;

static const uint32_t INVALID_SAFEPOINT_OFFSET = uint32_t(-1);

class SafepointWriter
{
    CompactBufferWriter stream_;
    BitSet *frameSlots_;

  public:
    bool init(uint32_t slotCount);

  private:
    
    uint32_t startEntry();

    void writeOsiCallPointOffset(uint32_t osiPointOffset);
    void writeGcRegs(LSafepoint *safepoint);
    void writeGcSlots(LSafepoint *safepoint);
    void writeValueSlots(LSafepoint *safepoint);

    void writeSlotsOrElementsSlots(LSafepoint *safepoint);

#ifdef JS_NUNBOX32
    void writeNunboxParts(LSafepoint *safepoint);
#endif

    void endEntry();

  public:
    void encode(LSafepoint *safepoint);

    size_t size() const {
        return stream_.length();
    }
    const uint8_t *buffer() const {
        return stream_.buffer();
    }
};

class SafepointReader
{
    CompactBufferReader stream_;
    uint32_t frameSlots_;
    uint32_t currentSlotChunk_;
    uint32_t currentSlotChunkNumber_;
    uint32_t osiCallPointOffset_;
    GeneralRegisterSet gcSpills_;
    GeneralRegisterSet valueSpills_;
    GeneralRegisterSet slotsOrElementsSpills_;
    GeneralRegisterSet allGprSpills_;
    FloatRegisterSet allFloatSpills_;
    uint32_t nunboxSlotsRemaining_;
    uint32_t slotsOrElementsSlotsRemaining_;

  private:
    void advanceFromGcRegs();
    void advanceFromGcSlots();
    void advanceFromValueSlots();
    void advanceFromNunboxSlots();
    bool getSlotFromBitmap(uint32_t *slot);

  public:
    SafepointReader(IonScript *script, const SafepointIndex *si);

    static CodeLocationLabel InvalidationPatchPoint(IonScript *script, const SafepointIndex *si);

    uint32_t osiCallPointOffset() const {
        return osiCallPointOffset_;
    }
    GeneralRegisterSet gcSpills() const {
        return gcSpills_;
    }
    GeneralRegisterSet slotsOrElementsSpills() const {
        return slotsOrElementsSpills_;
    }
    GeneralRegisterSet valueSpills() const {
        return valueSpills_;
    }
    GeneralRegisterSet allGprSpills() const {
        return allGprSpills_;
    }
    FloatRegisterSet allFloatSpills() const {
        return allFloatSpills_;
    }
    uint32_t osiReturnPointOffset() const;

    
    bool getGcSlot(uint32_t *slot);

    
    bool getValueSlot(uint32_t *slot);

    
    
    bool getNunboxSlot(LAllocation *type, LAllocation *payload);

    
    bool getSlotsOrElementsSlot(uint32_t *slot);
};

} 
} 

#endif 
