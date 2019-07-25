








































#include "jscntxt.h"
#include "jscompartment.h"
#include "ion/Bailouts.h"
#include "ion/IonCompartment.h"

using namespace js;
using namespace js::ion;

static const uintptr_t BAILOUT_TABLE_ENTRY_SIZE = 5;

JS_STATIC_ASSERT(sizeof(BailoutStack) ==
                 sizeof(uintptr_t) +
                 sizeof(double) * 8 +
                 sizeof(uintptr_t) * 8 +
                 sizeof(uintptr_t));

JS_STATIC_ASSERT(sizeof(ExtendedBailoutStack) ==
                 sizeof(BailoutStack) +
                 sizeof(uintptr_t));

BailoutEnvironment::BailoutEnvironment(IonCompartment *ion, void **esp)
  : esp_(esp)
{
    bailout_ = reinterpret_cast<ExtendedBailoutStack *>(esp);

    if (bailout_->frameClass() != FrameSizeClass::None()) {
        frameSize_ = bailout_->frameSize();
        frame_ = &esp_[sizeof(BailoutStack) / STACK_SLOT_SIZE];

        
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
        frame_ = &esp_[sizeof(ExtendedBailoutStack) / STACK_SLOT_SIZE];
    }
}

IonFramePrefix *
BailoutEnvironment::top() const
{
    return (IonFramePrefix *)&frame_[frameSize_ / STACK_SLOT_SIZE];
}

