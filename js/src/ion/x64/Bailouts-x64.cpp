








































#include "jscntxt.h"
#include "jscompartment.h"
#include "ion/shared/Bailouts-x86-shared.h"
#include "ion/Bailouts.h"
#include "ion/IonCompartment.h"
#include "ion/IonFrames-inl.h"

using namespace js;
using namespace js::ion;

#if defined(_WIN32)
# pragma pack(push, 1)
#endif

namespace js {
namespace ion {

class BailoutStack
{
    double    fpregs_[FloatRegisters::Total];
    uintptr_t regs_[Registers::Total];
    uintptr_t frameSize_;
    uintptr_t snapshotOffset_;

  public:
    MachineState machineState() {
        return MachineState(regs_, fpregs_);
    }
    uint32 snapshotOffset() const {
        return snapshotOffset_;
    }
    uint32 frameSize() const {
        return frameSize_;
    }
    uint8 *parentStackPointer() {
        return (uint8 *)this + sizeof(BailoutStack);
    }
};

} 
} 

#if defined(_WIN32)
# pragma pack(pop)
#endif

FrameRecovery
ion::FrameRecoveryFromBailout(IonCompartment *ion, BailoutStack *bailout)
{
    uint8 *sp = bailout->parentStackPointer();
    uint8 *fp = sp + bailout->frameSize();

    return FrameRecovery::FromSnapshot(fp, sp, bailout->machineState(), bailout->snapshotOffset());
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
