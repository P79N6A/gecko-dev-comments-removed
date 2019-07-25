








































#ifndef jsion_stack_slot_allocator_h_
#define jsion_stack_slot_allocator_h_

#include "IonRegisters.h"

namespace js {
namespace ion {

class StackSlotAllocator
{
    js::Vector<uint32, 4, SystemAllocPolicy> normalSlots;
    js::Vector<uint32, 4, SystemAllocPolicy> doubleSlots;
    uint32 height_;

  public:
    StackSlotAllocator() : height_(0)
    { }

    void freeSlot(uint32 index) {
        normalSlots.append(index);
    }
    void freeDoubleSlot(uint32 index) {
        doubleSlots.append(index);
    }

    uint32 allocateDoubleSlot() {
        if (!doubleSlots.empty())
            return doubleSlots.popCopy();
        if (ComputeByteAlignment(height_, DOUBLE_STACK_ALIGNMENT))
            normalSlots.append(++height_);
        height_ += (sizeof(double) / STACK_SLOT_SIZE);
        return height_;
    }
    uint32 allocateSlot() {
        if (!normalSlots.empty())
            return normalSlots.popCopy();
        if (!doubleSlots.empty()) {
            uint32 index = doubleSlots.popCopy();
            normalSlots.append(index - 1);
            return index;
        }
        return ++height_;
    }
    uint32 stackHeight() const {
        return height_;
    }
};

} 
} 

#endif 

