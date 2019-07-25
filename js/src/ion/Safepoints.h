








































#ifndef jsion_include_safepoints_h_
#define jsion_include_safepoints_h_

#include "IonRegisters.h"
#include "CompactBuffer.h"
#include "BitSet.h"

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

  private:
    void advanceFromGcRegs();
    void advanceFromGcSlots();
    void advanceFromValueSlots();
    bool getSlotFromBitmap(uint32 *slot);

  public:
    SafepointReader(IonScript *script, const IonFrameInfo *fi);

    
    void getGcRegs(GeneralRegisterSet *actual, GeneralRegisterSet *spilled);

    
    bool getGcSlot(uint32 *slot);

    
    bool getValueSlot(uint32 *slot);
};

} 
} 

#endif 

