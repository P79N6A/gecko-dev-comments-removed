








































#include "jscntxt.h"
#include "jscompartment.h"
#include "ion/Bailouts.h"
#include "ion/IonCompartment.h"

using namespace js;
using namespace js::ion;

JS_STATIC_ASSERT(sizeof(BailoutStack) ==
                 sizeof(double) * 16 +
                 sizeof(uintptr_t) * 16 +
                 sizeof(uintptr_t) +
                 sizeof(uintptr_t));

BailoutEnvironment::BailoutEnvironment(IonCompartment *ion, void **rsp)
  : rsp_(rsp)
{
    bailout_ = reinterpret_cast<BailoutStack *>(rsp);
    frame_ = &rsp_[sizeof(BailoutStack) / STACK_SLOT_SIZE];
}

IonFramePrefix *
BailoutEnvironment::top() const
{
    return (IonFramePrefix *)&frame_[bailout_->frameSize() / STACK_SLOT_SIZE];
}

