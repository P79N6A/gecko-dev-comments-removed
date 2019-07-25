








































#include "Ion.h"
#include "IonFrames.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jsfun.h"
#include "IonCompartment.h"

using namespace js;
using namespace js::ion;

FrameRecovery::FrameRecovery(uint8 *fp, uint8 *sp, const MachineState &machine)
  : fp_((IonJSFrameLayout *)fp),
    sp_(sp),
    machine_(machine)
{
    if (IsCalleeTokenFunction(fp_->calleeToken())) {
        callee_ = CalleeTokenToFunction(fp_->calleeToken());
        fun_ = callee_->getFunctionPrivate();
        script_ = fun_->script();
    } else {
        script_ = CalleeTokenToScript(fp_->calleeToken());
    }
}

void
FrameRecovery::setBailoutId(BailoutId bailoutId)
{
    snapshotOffset_ = ionScript()->bailoutToSnapshot(bailoutId);
}

FrameRecovery
FrameRecovery::FromBailoutId(uint8 *fp, uint8 *sp, const MachineState &machine,
                             BailoutId bailoutId)
{
    FrameRecovery frame(fp, sp, machine);
    frame.setBailoutId(bailoutId);
    return frame;
}

FrameRecovery
FrameRecovery::FromSnapshot(uint8 *fp, uint8 *sp, const MachineState &machine,
                            SnapshotOffset snapshotOffset)
{
    FrameRecovery frame(fp, sp, machine);
    frame.setSnapshotOffset(snapshotOffset);
    return frame;
}

FrameRecovery
FrameRecovery::FromFrameIterator(const IonFrameIterator& it)
{
    MachineState noRegs;
    FrameRecovery frame(it.prevFp(), it.prevFp() - it.prevFrameLocalSize(), noRegs);
    const IonFrameInfo *info = frame.ionScript()->getFrameInfo(it.returnAddress());
    frame.setSnapshotOffset(info->snapshotOffset);
    return frame;
}

IonScript *
FrameRecovery::ionScript() const
{
    return script_->ion;
}

IonCommonFrameLayout *
IonFrameIterator::current() const
{
    return (IonCommonFrameLayout *)current_;
}

uint8 *
IonFrameIterator::returnAddress() const
{
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    return current->returnAddress();
}

size_t
IonFrameIterator::prevFrameLocalSize() const
{
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    return current->prevFrameLocalSize();
}

FrameType
IonFrameIterator::prevType() const
{
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    return current->prevType();
}

uint8 *
IonFrameIterator::prevFp() const
{
    JS_ASSERT(type_ != IonFrame_Entry);

    size_t currentSize;
    switch (type_) {
      case IonFrame_JS:
        currentSize = sizeof(IonJSFrameLayout);
        break;
      case IonFrame_Rectifier:
        currentSize = sizeof(IonRectifierFrameLayout);
        break;
      case IonFrame_Exit:
        currentSize = sizeof(IonExitFrameLayout);
        break;
      default:
        JS_NOT_REACHED("unexpected frame type");
        return NULL;
    }
    currentSize += current()->prevFrameLocalSize();

    return current_ + currentSize;
}

IonFrameIterator &
IonFrameIterator::operator++()
{
    JS_ASSERT(type_ != IonFrame_Entry);

    
    
    if (current()->prevType() == IonFrame_Entry) {
        type_ = IonFrame_Entry;
        return *this;
    }

    
    
    uint8 *prev = prevFp();
    type_ = current()->prevType();
    current_ = prev;
    return *this;
}

void
ion::HandleException(ResumeFromException *rfe)
{
    JSContext *cx = GetIonContext()->cx;

    IonFrameIterator iter(JS_THREAD_DATA(cx)->ionTop);
    while (iter.type() != IonFrame_Entry)
        ++iter;

    rfe->stackPointer = iter.fp();
}

IonActivationIterator::IonActivationIterator(JSContext *cx)
  : cx_(cx),
    top_(JS_THREAD_DATA(cx)->ionTop),
    activation_(JS_THREAD_DATA(cx)->ionActivation)
{
}

IonActivationIterator &
IonActivationIterator::operator++()
{
    JS_ASSERT(activation_);
    top_ = activation_->prevIonTop();
    activation_ = activation_->prev();
    return *this;
}

