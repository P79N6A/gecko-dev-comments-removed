








































#ifndef jsion_cpu_x64_stack_assignment_h__
#define jsion_cpu_x64_stack_assignment_h__

namespace js {
namespace ion {

class StackAssignment
{
    js::Vector<uint32, 4, IonAllocPolicy> slots;
    uint32 height_;

  public:
    StackAssignment() : height_(0)
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

