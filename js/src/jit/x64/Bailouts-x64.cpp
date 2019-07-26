





#include "jit/Bailouts.h"

using namespace js;
using namespace js::jit;

#if defined(_WIN32)
# pragma pack(push, 1)
#endif

namespace js {
namespace jit {

class BailoutStack
{
    mozilla::Array<double, FloatRegisters::Total> fpregs_;
    mozilla::Array<uintptr_t, Registers::Total> regs_;
    uintptr_t frameSize_;
    uintptr_t snapshotOffset_;

  public:
    MachineState machineState() {
        return MachineState::FromBailout(regs_, fpregs_);
    }
    uint32_t snapshotOffset() const {
        return snapshotOffset_;
    }
    uint32_t frameSize() const {
        return frameSize_;
    }
    uint8_t *parentStackPointer() {
        return (uint8_t *)this + sizeof(BailoutStack);
    }
};

} 
} 

#if defined(_WIN32)
# pragma pack(pop)
#endif

IonBailoutIterator::IonBailoutIterator(const JitActivationIterator &activations,
                                       BailoutStack *bailout)
  : IonFrameIterator(activations),
    machine_(bailout->machineState())
{
    uint8_t *sp = bailout->parentStackPointer();
    uint8_t *fp = sp + bailout->frameSize();

    current_ = fp;
    type_ = JitFrame_IonJS;
    topFrameSize_ = current_ - sp;
    topIonScript_ = script()->ionScript();
    snapshotOffset_ = bailout->snapshotOffset();
}

IonBailoutIterator::IonBailoutIterator(const JitActivationIterator &activations,
                                       InvalidationBailoutStack *bailout)
  : IonFrameIterator(activations),
    machine_(bailout->machine())
{
    returnAddressToFp_ = bailout->osiPointReturnAddress();
    topIonScript_ = bailout->ionScript();
    const OsiIndex *osiIndex = topIonScript_->getOsiIndex(returnAddressToFp_);

    current_ = (uint8_t*) bailout->fp();
    type_ = JitFrame_IonJS;
    topFrameSize_ = current_ - bailout->sp();
    snapshotOffset_ = osiIndex->snapshotOffset();
}
