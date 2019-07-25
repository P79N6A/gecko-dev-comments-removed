








































#ifndef jsion_cpu_x64_stack_assignment_h__
#define jsion_cpu_x64_stack_assignment_h__

namespace js {
namespace ion {

class StackAssignmentX64
{
    js::Vector<uint32, 4, IonAllocPolicy> slots;
    uint32 height_;

  public:
    StackAssignmentX64() : height_(0)
    { }

    void freeSlot(uint32 index) {
        slots.append(index);
    }

    void freeDoubleSlot(uint32 index) {
        freeSlot(index);
    }

    bool allocateDoubleSlot(uint32 *index) {
        return allocateSlot(index);
    }

    bool allocateSlot(uint32 *index) {
        if (!slots.empty()) {
            *index = slots.popCopy();
            return true;
        }
        *index = height_++;
        return height_ < MAX_STACK_SLOTS;
    }

    uint32 stackHeight() const {
        return height_;
    }
};

typedef StackAssignmentX64 StackAssignment;

} 
} 

#endif 

