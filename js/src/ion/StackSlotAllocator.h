






#ifndef jsion_stack_slot_allocator_h_
#define jsion_stack_slot_allocator_h_

#include "Registers.h"

namespace js {
namespace ion {

class StackSlotAllocator
{
    js::Vector<uint32_t, 4, SystemAllocPolicy> normalSlots;
    js::Vector<uint32_t, 4, SystemAllocPolicy> doubleSlots;
    uint32_t height_;

  public:
    StackSlotAllocator() : height_(0)
    { }

    void freeSlot(uint32_t index) {
        normalSlots.append(index);
    }
    void freeDoubleSlot(uint32_t index) {
        doubleSlots.append(index);
    }
    void freeValueSlot(uint32_t index) {
        freeDoubleSlot(index);
    }

    uint32_t allocateDoubleSlot() {
        if (!doubleSlots.empty())
            return doubleSlots.popCopy();
        if (ComputeByteAlignment(height_, DOUBLE_STACK_ALIGNMENT))
            normalSlots.append(++height_);
        height_ += (sizeof(double) / STACK_SLOT_SIZE);
        return height_;
    }
    uint32_t allocateSlot() {
        if (!normalSlots.empty())
            return normalSlots.popCopy();
        if (!doubleSlots.empty()) {
            uint32_t index = doubleSlots.popCopy();
            normalSlots.append(index - 1);
            return index;
        }
        return ++height_;
    }
    uint32_t allocateValueSlot() {
        return allocateDoubleSlot();
    }
    uint32_t stackHeight() const {
        return height_;
    }
};

} 
} 

#endif 

