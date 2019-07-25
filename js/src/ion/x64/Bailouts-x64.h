








































#ifndef jsion_bailouts_x64_h__
#define jsion_bailouts_x64_h__

#include "ion/IonFrames.h"
#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {

class IonCompartment;

#if defined(_WIN32)
# pragma pack(push, 1)
#endif

class BailoutStack
{
    double    fpregs_[FloatRegisters::Total];
    uintptr_t regs_[Registers::Total];
    uintptr_t frameSize_;
    uintptr_t snapshotOffset_;

  public:
    double readFloatReg(const FloatRegister &reg) const {
        return fpregs_[reg.code()];
    }
    uintptr_t readReg(const Register &reg) const {
        return regs_[reg.code()];
    }
    uint32 snapshotOffset() const {
        return snapshotOffset_;
    }
    uint32 frameSize() const {
        return frameSize_;
    }
};

#if defined(_WIN32)
# pragma pack(pop)
#endif

class BailoutEnvironment
{
    void **rsp_;
    void **frame_;
    const BailoutStack *bailout_;

  public:
    BailoutEnvironment(IonCompartment *ion, void **rsp);

    IonFramePrefix *top() const;
    FrameSizeClass frameClass() const {
        return FrameSizeClass::None();
    }
    uint32 bailoutId() const {
        JS_NOT_REACHED("x64 does not have bailout IDs");
        return uint32(-1);
    }
    uint32 snapshotOffset() const {
        return bailout_->snapshotOffset();
    }
    uintptr_t readSlot(uint32 offset) const {
        JS_ASSERT(offset % STACK_SLOT_SIZE == 0);
        return *(uintptr_t *)((uint8 *)frame_ + offset);
    }
    double readDoubleSlot(uint32 offset) const {
        JS_ASSERT(offset % STACK_SLOT_SIZE == 0);
        return *(double *)((uint8 *)frame_ + offset);
    }
    uintptr_t readReg(const Register &reg) const {
        return bailout_->readReg(reg);
    }
    double readFloatReg(const FloatRegister &reg) const {
        return bailout_->readFloatReg(reg);
    }
};

} 
} 

#endif 

