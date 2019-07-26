





#ifndef jit_SnapshotReader_h
#define jit_SnapshotReader_h

#include "jit/CompactBuffer.h"
#include "jit/IonCode.h"
#include "jit/IonTypes.h"
#include "jit/Registers.h"
#include "jit/Slot.h"

namespace js {
namespace jit {

#ifdef TRACK_SNAPSHOTS
class LInstruction;
#endif




class SnapshotReader
{
    CompactBufferReader reader_;

    uint32_t pcOffset_;           
    uint32_t slotCount_;          
    uint32_t frameCount_;
    BailoutKind bailoutKind_;
    uint32_t framesRead_;         
    uint32_t slotsRead_;          
    bool resumeAfter_;

#ifdef TRACK_SNAPSHOTS
  private:
    uint32_t pcOpcode_;
    uint32_t mirOpcode_;
    uint32_t mirId_;
    uint32_t lirOpcode_;
    uint32_t lirId_;
  public:
    void spewBailingFrom() const;
#endif

  private:

    void readSnapshotHeader();
    void readFrameHeader();

  public:
    SnapshotReader(const uint8_t *buffer, const uint8_t *end);

    uint32_t pcOffset() const {
        return pcOffset_;
    }
    uint32_t slots() const {
        return slotCount_;
    }
    BailoutKind bailoutKind() const {
        return bailoutKind_;
    }
    bool resumeAfter() const {
        if (moreFrames())
            return false;
        return resumeAfter_;
    }
    bool moreFrames() const {
        return framesRead_ < frameCount_;
    }
    void nextFrame() {
        readFrameHeader();
    }
    Slot readSlot();

    Value skip() {
        readSlot();
        return UndefinedValue();
    }

    bool moreSlots() const {
        return slotsRead_ < slotCount_;
    }
    uint32_t frameCount() const {
        return frameCount_;
    }
};

}
}

#endif 
