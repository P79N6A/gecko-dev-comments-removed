








































#include "jscntxt.h"
#include "jscompartment.h"
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
        return MachineState::FromBailout(regs_, fpregs_);
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
    IonScript *ionScript = bailout->ionScript();
    const OsiIndex *osiIndex = ionScript->getOsiIndex(bailout->osiPointReturnAddress());
    FrameRecovery fr = FrameRecovery::FromSnapshot((uint8 *) bailout->fp(), bailout->sp(),
                                                   bailout->machine(), osiIndex->snapshotOffset());
    fr.setIonScript(ionScript);
    return fr;
}
