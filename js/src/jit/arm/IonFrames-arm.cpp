





#include "jit/Ion.h"
#include "jit/IonFrames.h"

using namespace js;
using namespace js::jit;

IonJSFrameLayout *
InvalidationBailoutStack::fp() const
{
    return (IonJSFrameLayout *) (sp() + ionScript_->frameSize());
}

void
InvalidationBailoutStack::checkInvariants() const
{
#ifdef DEBUG
    IonJSFrameLayout *frame = fp();
    CalleeToken token = frame->calleeToken();
    JS_ASSERT(token);

    uint8_t *rawBase = ionScript()->method()->raw();
    uint8_t *rawLimit = rawBase + ionScript()->method()->instructionsSize();
    uint8_t *osiPoint = osiPointReturnAddress();
    JS_ASSERT(rawBase <= osiPoint && osiPoint <= rawLimit);
#endif
}
