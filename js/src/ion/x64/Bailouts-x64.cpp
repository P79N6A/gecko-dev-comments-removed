








































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

IonBailoutIterator::IonBailoutIterator(const IonActivationIterator &activations,
                                       BailoutStack *bailout)
  : IonFrameIterator(activations),
    machine_(bailout->machineState())
{
    uint8 *sp = bailout->parentStackPointer();
    uint8 *fp = sp + bailout->frameSize();

    current_ = fp;
    type_ = IonFrame_JS;
    topFrameSize_ = current_ - sp;
    topIonScript_ = script()->ion;
    snapshotOffset_ = bailout->snapshotOffset();
}

IonBailoutIterator::IonBailoutIterator(const IonActivationIterator &activations,
                                       InvalidationBailoutStack *bailout)
  : IonFrameIterator(activations),
    machine_(bailout->machine())
{
    returnAddressToFp_ = bailout->osiPointReturnAddress();
    topIonScript_ = bailout->ionScript();
    const OsiIndex *osiIndex = topIonScript_->getOsiIndex(returnAddressToFp_);

    current_ = (uint8*) bailout->fp();
    type_ = IonFrame_JS;
    topFrameSize_ = current_ - bailout->sp();
    snapshotOffset_ = osiIndex->snapshotOffset();
}
