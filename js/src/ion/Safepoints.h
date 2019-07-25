








































#ifndef jsion_include_safepoints_h_
#define jsion_include_safepoints_h_

#include "Registers.h"
#include "CompactBuffer.h"
#include "BitSet.h"

#include "shared/Assembler-shared.h"

namespace js {
namespace ion {

struct SafepointNunboxEntry;

static const uint32 INVALID_SAFEPOINT_OFFSET = uint32(-1);

class SafepointWriter
{
    CompactBufferWriter stream_;
    BitSet *frameSlots_;

  public:
    bool init(uint32 localSlotCount);

    
    uint32 startEntry();
    void writeOsiCallPointOffset(uint32 osiPointOffset);
    void writeGcRegs(GeneralRegisterSet actual, GeneralRegisterSet spilled);
    void writeGcSlots(uint32 nslots, uint32 *slots);
    void writeValueSlots(uint32 nslots, uint32 *slots);
    void writeNunboxParts(uint32 nentries, SafepointNunboxEntry *entries);
    void endEntry();

    size_t size() const {
        return stream_.length();
    }
    const uint8 *buffer() const {
        return stream_.buffer();
    }
};

class SafepointReader
{
    CompactBufferReader stream_;
    uint32 localSlotCount_;
    uint32 currentSlotChunk_;
    uint32 currentSlotChunkNumber_;
    uint32 osiCallPointOffset_;
    GeneralRegisterSet gcSpills_;
    GeneralRegisterSet allSpills_;

  private:
    void advanceFromGcRegs();
    void advanceFromGcSlots();
    void advanceFromValueSlots();
    bool getSlotFromBitmap(uint32 *slot);

  public:
    SafepointReader(IonScript *script, const SafepointIndex *si);

    static CodeLocationLabel InvalidationPatchPoint(IonScript *script, const SafepointIndex *si);

    uint32 osiCallPointOffset() const {
        return osiCallPointOffset_;
    }
    GeneralRegisterSet gcSpills() const {
        return gcSpills_;
    }
    GeneralRegisterSet allSpills() const {
        return allSpills_;
    }
    uint32 osiReturnPointOffset() const;

    
    bool getGcSlot(uint32 *slot);

    
    bool getValueSlot(uint32 *slot);
};

} 
} 

#endif 

