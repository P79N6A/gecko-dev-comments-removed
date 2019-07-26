





#include "jscntxt.h"
#include "jscompartment.h"

#include "jit/Bailouts.h"
#include "jit/JitCompartment.h"

using namespace js;
using namespace js::jit;

namespace js {
namespace jit {

class BailoutStack
{
    uintptr_t frameClassId_;
    
    
    
  public:
    union {
        uintptr_t frameSize_;
        uintptr_t tableOffset_;
    };

  protected: 
    mozilla::Array<double, FloatRegisters::Total> fpregs_;
    mozilla::Array<uintptr_t, Registers::Total> regs_;

    uintptr_t snapshotOffset_;
    uintptr_t padding_;

  public:
    FrameSizeClass frameClass() const {
        return FrameSizeClass::FromClass(frameClassId_);
    }
    uintptr_t tableOffset() const {
        JS_ASSERT(frameClass() != FrameSizeClass::None());
        return tableOffset_;
    }
    uint32_t frameSize() const {
        if (frameClass() == FrameSizeClass::None())
            return frameSize_;
        return frameClass().frameSize();
    }
    MachineState machine() {
        return MachineState::FromBailout(regs_, fpregs_);
    }
    SnapshotOffset snapshotOffset() const {
        JS_ASSERT(frameClass() == FrameSizeClass::None());
        return snapshotOffset_;
    }
    uint8_t *parentStackPointer() const {
        if (frameClass() == FrameSizeClass::None())
            return (uint8_t *)this + sizeof(BailoutStack);
        return (uint8_t *)this + offsetof(BailoutStack, snapshotOffset_);
    }
};


static_assert((sizeof(BailoutStack) % 8) == 0, "BailoutStack should be 8-byte aligned.");

} 
} 

IonBailoutIterator::IonBailoutIterator(const JitActivationIterator &activations,
                                       BailoutStack *bailout)
  : JitFrameIterator(activations),
    machine_(bailout->machine())
{
    uint8_t *sp = bailout->parentStackPointer();
    uint8_t *fp = sp + bailout->frameSize();

    current_ = fp;
    type_ = JitFrame_IonJS;
    topFrameSize_ = current_ - sp;
    topIonScript_ = script()->ionScript();

    if (bailout->frameClass() == FrameSizeClass::None()) {
        snapshotOffset_ = bailout->snapshotOffset();
        return;
    }

    
    JitActivation *activation = activations.activation()->asJit();
    JSRuntime *rt = activation->compartment()->runtimeFromMainThread();
    JitCode *code = rt->jitRuntime()->getBailoutTable(bailout->frameClass());
    uintptr_t tableOffset = bailout->tableOffset();
    uintptr_t tableStart = reinterpret_cast<uintptr_t>(code->raw());

    JS_ASSERT(tableOffset >= tableStart &&
              tableOffset < tableStart + code->instructionsSize());
    JS_ASSERT((tableOffset - tableStart) % BAILOUT_TABLE_ENTRY_SIZE == 0);

    uint32_t bailoutId = ((tableOffset - tableStart) / BAILOUT_TABLE_ENTRY_SIZE) - 1;
    JS_ASSERT(bailoutId < BAILOUT_TABLE_SIZE);

    snapshotOffset_ = topIonScript_->bailoutToSnapshot(bailoutId);
}

IonBailoutIterator::IonBailoutIterator(const JitActivationIterator &activations,
                                       InvalidationBailoutStack *bailout)
  : JitFrameIterator(activations),
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
