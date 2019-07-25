








































#include "Ion.h"
#include "IonFrames.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jsfun.h"

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
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    JS_ASSERT(type_ != IonFrame_Entry);

    if (prevCache_ != current_)
        return prevCache_;

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
    currentSize += current->prevFrameLocalSize();
    prevCache_ = current_ + currentSize;
    return prevCache_;
}

void
IonFrameIterator::prev()
{
    JS_ASSERT(type_ != IonFrame_Entry);

    IonCommonFrameLayout *current = (IonCommonFrameLayout *)current_;

    
    
    if (current->prevType() == IonFrame_Entry) {
        type_ = IonFrame_Entry;
        return;
    }

    type_ = current->prevType();
    current_ = prevFp();
}

void
ion::HandleException(ResumeFromException *rfe)
{
    JSContext *cx = GetIonContext()->cx;

    IonFrameIterator iter(JS_THREAD_DATA(cx)->ionTop);
    while (iter.type() != IonFrame_Entry)
        iter.prev();

    rfe->stackPointer = iter.fp();
}

