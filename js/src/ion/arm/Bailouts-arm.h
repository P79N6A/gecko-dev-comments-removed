








































#ifndef jsion_bailouts_arm_h__
#define jsion_bailouts_arm_h__

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
    uintptr_t frameClassId_;
    union {
        uintptr_t frameSize_;
        uintptr_t tableOffset_;
    };
    double    fpregs_[FloatRegisters::Total];
    uintptr_t regs_[Registers::Total];


  public:
    FrameSizeClass frameClass() const {
        return FrameSizeClass::FromClass(frameClassId_);
    }
    double readFloatReg(const FloatRegister &reg) const {
        return fpregs_[reg.code()];
    }
    uintptr_t readReg(const Register &reg) const {
        return regs_[reg.code()];
    }
    uintptr_t tableOffset() const {
        JS_ASSERT(frameClass() != FrameSizeClass::None());
        return tableOffset_;
    }
    uint32 frameSize() const {
        if (frameClass() == FrameSizeClass::None())
            return frameSize_;
        return frameClass().frameSize();
    }
};

class ExtendedBailoutStack : public BailoutStack
{
    uintptr_t snapshotOffset_;

  public:
    SnapshotOffset snapshotOffset() const {
        JS_ASSERT(frameClass() == FrameSizeClass::None());
        return snapshotOffset_;
    }
};

class BailoutEnvironment
{
    void **sp_;
    void **frame_;
    const ExtendedBailoutStack *bailout_;
    uint32 frameSize_;
    uint32 bailoutId_;

  public:
    BailoutEnvironment(IonCompartment *ion, void **esp);

    IonFramePrefix *top() const;
    FrameSizeClass frameClass() const {
        return bailout_->frameClass();
    }
    uint32 bailoutId() const {
        JS_ASSERT(frameClass() != FrameSizeClass::None());
        return bailoutId_;
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

