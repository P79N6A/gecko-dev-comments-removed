





#include "jscntxt.h"
#include "jscompartment.h"

#include "jit/arm/Assembler-arm.h"
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
    RegisterDump::FPUArray fpregs_;
    RegisterDump::GPRArray regs_;

    uintptr_t snapshotOffset_;
    uintptr_t padding_;

  public:
    FrameSizeClass frameClass() const {
        return FrameSizeClass::FromClass(frameClassId_);
    }
    uintptr_t tableOffset() const {
        MOZ_ASSERT(frameClass() != FrameSizeClass::None());
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
        MOZ_ASSERT(frameClass() == FrameSizeClass::None());
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

BailoutFrameInfo::BailoutFrameInfo(const JitActivationIterator &activations,
                                   BailoutStack *bailout)
  : machine_(bailout->machine())
{
    uint8_t *sp = bailout->parentStackPointer();
    framePointer_ = sp + bailout->frameSize();
    topFrameSize_ = framePointer_ - sp;

    JSScript *script = ScriptFromCalleeToken(((JitFrameLayout *) framePointer_)->calleeToken());
    JitActivation *activation = activations.activation()->asJit();
    topIonScript_ = script->ionScript();

    attachOnJitActivation(activations);

    if (bailout->frameClass() == FrameSizeClass::None()) {
        snapshotOffset_ = bailout->snapshotOffset();
        return;
    }

    
    JSRuntime *rt = activation->compartment()->runtimeFromMainThread();
    JitCode *code = rt->jitRuntime()->getBailoutTable(bailout->frameClass());
    uintptr_t tableOffset = bailout->tableOffset();
    uintptr_t tableStart = reinterpret_cast<uintptr_t>(Assembler::BailoutTableStart(code->raw()));

    MOZ_ASSERT(tableOffset >= tableStart &&
               tableOffset < tableStart + code->instructionsSize());
    MOZ_ASSERT((tableOffset - tableStart) % BAILOUT_TABLE_ENTRY_SIZE == 0);

    uint32_t bailoutId = ((tableOffset - tableStart) / BAILOUT_TABLE_ENTRY_SIZE) - 1;
    MOZ_ASSERT(bailoutId < BAILOUT_TABLE_SIZE);

    snapshotOffset_ = topIonScript_->bailoutToSnapshot(bailoutId);
}

BailoutFrameInfo::BailoutFrameInfo(const JitActivationIterator &activations,
                                   InvalidationBailoutStack *bailout)
  : machine_(bailout->machine())
{
    framePointer_ = (uint8_t*) bailout->fp();
    topFrameSize_ = framePointer_ - bailout->sp();
    topIonScript_ = bailout->ionScript();
    attachOnJitActivation(activations);

    uint8_t *returnAddressToFp_ = bailout->osiPointReturnAddress();
    const OsiIndex *osiIndex = topIonScript_->getOsiIndex(returnAddressToFp_);
    snapshotOffset_ = osiIndex->snapshotOffset();
}
