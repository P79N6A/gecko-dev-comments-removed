








































#include "jscntxt.h"
#include "jscompartment.h"
#include "ion/Bailouts.h"
#include "ion/IonCompartment.h"

using namespace js;
using namespace js::ion;

#if defined(_WIN32)
# pragma pack(push, 1)
#endif

namespace js {
namespace ion {

class BailoutStack
{
    uintptr_t frameClassId_;
    double    fpregs_[FloatRegisters::Total];
    uintptr_t regs_[Registers::Total];
    union {
        uintptr_t frameSize_;
        uintptr_t tableOffset_;
    };
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

