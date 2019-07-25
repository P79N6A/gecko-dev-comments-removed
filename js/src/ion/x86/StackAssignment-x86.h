








































#ifndef jsion_cpu_x86_stack_assignment_h__
#define jsion_cpu_x86_stack_assignment_h__

namespace js {
namespace ion {

class StackAssignment
{
    js::Vector<uint32, 4, IonAllocPolicy> normalSlots;
    js::Vector<uint32, 4, IonAllocPolicy> doubleSlots;
    uint32 height_;

  public:
    StackAssignment() : height_(0)
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
            JS_ASSERT(*index <= height_);
            return true;
        }
        if (ComputeByteAlignment(height_, DOUBLE_STACK_ALIGNMENT)) {
            normalSlots.append(++height_);
            JS_ASSERT(!ComputeByteAlignment(height_, DOUBLE_STACK_ALIGNMENT));
        }
        height_ += 2;
        *index = height_;
        return true;
    }

    bool allocateSlot(uint32 *index) {
        if (!normalSlots.empty()) {
            *index = normalSlots.popCopy();
            JS_ASSERT(*index <= height_);
            return true;
        }
        if (!doubleSlots.empty()) {
            *index = doubleSlots.popCopy();
            JS_ASSERT(*index <= height_);
            return normalSlots.append(*index - 1);
        }
        *index = ++height_;
        return true;
    }

    uint32 stackHeight() const {
        return height_;
    }
};

} 
} 

#endif 

