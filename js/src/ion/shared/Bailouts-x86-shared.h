







































#ifndef jsion_bailouts_x86_shared__
#define jsion_bailouts_x86_shared__

#include "ion/Bailouts.h"

namespace js {
namespace ion {



class InvalidationBailoutStack
{
    double fpregs_[FloatRegisters::Total];
    uintptr_t regs_[Registers::Total];
    uintptr_t frameDescriptor_;

    size_t frameSize() const {
        return frameDescriptor_ >> FRAMETYPE_BITS;
    }
    size_t frameDescriptorOffset() const {
        return offsetof(InvalidationBailoutStack, frameDescriptor_);
    }

  public:
    uint8 *sp() const {
        return (uint8 *) this + frameDescriptorOffset();
    }
    uint8 *fp() const {
        return sp() + frameSize() + sizeof(uintptr_t);
    }
    MachineState machine() {
        return MachineState(regs_, fpregs_);
    }
};

} 
} 

#endif
