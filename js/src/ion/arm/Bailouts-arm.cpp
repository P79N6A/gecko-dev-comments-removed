








































#include "jscntxt.h"
#include "jscompartment.h"
#include "ion/Bailouts.h"
#include "ion/IonCompartment.h"
#include "ion/IonFrames-inl.h"

using namespace js;
using namespace js::ion;

#if 0

JS_STATIC_ASSERT(sizeof(BailoutStack) ==
                 sizeof(uintptr_t) +
                 sizeof(double) * 8 +
                 sizeof(uintptr_t) * 8 +
                 sizeof(uintptr_t));

JS_STATIC_ASSERT(sizeof(ExtendedBailoutStack) ==
                 sizeof(BailoutStack) +
                 sizeof(uintptr_t));

#endif
#if 0
BailoutEnvironment::BailoutEnvironment(IonCompartment *ion, void **sp)
  : sp_(sp)
{
    bailout_ = reinterpret_cast<ExtendedBailoutStack *>(sp);

    if (bailout_->frameClass() != FrameSizeClass::None()) {
        frameSize_ = bailout_->frameSize();
        frame_ = &sp_[sizeof(BailoutStack) / STACK_SLOT_SIZE];

        
        IonCode *code = ion->getBailoutTable(bailout_->frameClass());
        uintptr_t tableOffset = bailout_->tableOffset();
        uintptr_t tableStart = reinterpret_cast<uintptr_t>(code->raw());

        JS_ASSERT(tableOffset >= tableStart &&
                  tableOffset < tableStart + code->instructionsSize());
        JS_ASSERT((tableOffset - tableStart) % BAILOUT_TABLE_ENTRY_SIZE == 0);

        bailoutId_ = ((tableOffset - tableStart) / BAILOUT_TABLE_ENTRY_SIZE) - 1;
        JS_ASSERT(bailoutId_ < BAILOUT_TABLE_SIZE);
    } else {
        frameSize_ = bailout_->frameSize();
        frame_ = &sp_[sizeof(ExtendedBailoutStack) / STACK_SLOT_SIZE];
    }
}

IonFramePrefix *
BailoutEnvironment::top() const
{
    return (IonFramePrefix *)&frame_[frameSize_ / STACK_SLOT_SIZE];
}

#endif

namespace js {
namespace ion {

class BailoutStack
{
    uintptr_t frameClassId_;
    
    
    
  public:
    union {
        uintptr_t frameSize_;
        uintptr_t tableOffset_;
    };

  private:
    double    fpregs_[FloatRegisters::Total];
    uintptr_t regs_[Registers::Total];

    uintptr_t snapshotOffset_;

  public:
    FrameSizeClass frameClass() const {
        return FrameSizeClass::FromClass(frameClassId_);
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
    MachineState machine() {
        return MachineState(regs_, fpregs_);
    }
    SnapshotOffset snapshotOffset() const {
        JS_ASSERT(frameClass() == FrameSizeClass::None());
        return snapshotOffset_;
    }
    uint8 *parentStackPointer() const {
        if (frameClass() == FrameSizeClass::None())
            return (uint8 *)this + sizeof(BailoutStack);
        return (uint8 *)this + offsetof(BailoutStack, snapshotOffset_);
    }
};

class InvalidationBailoutStack
{
    double fpregs_[FloatRegisters::Total];
    uintptr_t regs_[Registers::Total];
    uintptr_t pad[2];
    uintptr_t frameDescriptor_;

    size_t frameSize() const {
        return frameDescriptor_ >> FRAMETYPE_BITS;
    }
    size_t frameDescriptorOffset() const {
        return offsetof(InvalidationBailoutStack, frameDescriptor_);
    }

  public:
    uint8 *sp() const {
        return (uint8 *) this + frameDescriptorOffset() + sizeof(size_t) + sizeof(size_t);
    }
    uint8 *fp() const {
        return sp() + frameSize();
    }
    MachineState machine() {
        return MachineState(regs_, fpregs_);
    }
  public:
};

} 
} 

FrameRecovery
ion::FrameRecoveryFromBailout(IonCompartment *ion, BailoutStack *bailout)
{
    uint8 *sp = bailout->parentStackPointer();
    uint8 *fp = sp + bailout->frameSize();

    if (bailout->frameClass() == FrameSizeClass::None())
        return FrameRecovery::FromSnapshot(fp, sp, bailout->machine(), bailout->snapshotOffset());

    
    IonCode *code = ion->getBailoutTable(bailout->frameClass());
    uintptr_t tableOffset = bailout->tableOffset();
    uintptr_t tableStart = reinterpret_cast<uintptr_t>(code->raw());
    
    JS_ASSERT(tableOffset >= tableStart &&
              tableOffset < tableStart + code->instructionsSize());
    JS_ASSERT((tableOffset - tableStart) % BAILOUT_TABLE_ENTRY_SIZE == 0);

    uint32 bailoutId = ((tableOffset - tableStart) / BAILOUT_TABLE_ENTRY_SIZE) - 1;
    JS_ASSERT(bailoutId < BAILOUT_TABLE_SIZE);

    return FrameRecovery::FromBailoutId(fp, sp, bailout->machine(), bailoutId);
}

FrameRecovery
ion::FrameRecoveryFromInvalidation(IonCompartment *ion, InvalidationBailoutStack *bailout)
{
    IonJSFrameLayout *fp = (IonJSFrameLayout *) bailout->fp();
    InvalidationRecord *record = CalleeTokenToInvalidationRecord(fp->calleeToken());
    const IonFrameInfo *exitInfo = record->ionScript->getFrameInfo(record->returnAddress);
    SnapshotOffset snapshotOffset = exitInfo->snapshotOffset();

    return FrameRecovery::FromSnapshot(bailout->fp(), bailout->sp(), bailout->machine(),
                                       snapshotOffset);
}
