








































#ifndef jsion_cpu_x86_stack_assignment_h__
#define jsion_cpu_x86_stack_assignment_h__

namespace js {
namespace ion {

class StackAssignmentX86
{
    js::Vector<uint32, 4, IonAllocPolicy> normalSlots;
    js::Vector<uint32, 4, IonAllocPolicy> doubleSlots;
    uint32 height_;

  public:
    StackAssignmentX86() : height_(0)
    { }

    void freeSlot(uint32 index) {
        normalSlots.append(index);
    }
    void freeDoubleSlot(uint32 index) {
        doubleSlots.append(index);
    }

    bool allocateDoubleSlot(uint32 *index) {
        if (!doubleSlots.empty()) {
            *index = doubleSlots.popCopy();
            return false;
        }
        if (ComputeByteAlignment(height_, DOUBLE_STACK_ALIGNMENT)) {
            normalSlots.append(height_++);
            JS_ASSERT(!ComputeByteAlignment(height_, DOUBLE_STACK_ALIGNMENT));
        }
        *index = height_;
        height_ += 2;
        return height_ < MAX_STACK_SLOTS;
    }

    bool allocateSlot(uint32 *index) {
        if (!normalSlots.empty()) {
            *index = normalSlots.popCopy();
            return true;
        }
        if (!doubleSlots.empty()) {
            *index = doubleSlots.popCopy();
            return normalSlots.append(*index + 1);
        }
        *index = height_++;
        return height_ < MAX_STACK_SLOTS;
    }

    uint32 stackHeight() const {
        return height_;
    }
};

typedef StackAssignmentX86 StackAssignment;

} 
} 

#endif 

