








































#ifndef jsion_frames_inl_h__
#define jsion_frames_inl_h__

#include "ion/IonFrames.h"
#include "ion/Snapshots.h"
#include "ion/IonFrameIterator.h"
#include "ion/LIR.h"

namespace js {
namespace ion {

inline void
SafepointIndex::resolve()
{
    JS_ASSERT(!resolved);
    safepointOffset_ = safepoint_->offset();
    resolved = true;
}

static inline size_t
SizeOfFramePrefix(FrameType type)
{
    switch (type) {
      case IonFrame_Entry:
        return IonEntryFrameLayout::Size();
      case IonFrame_JS:
        return IonJSFrameLayout::Size();
      case IonFrame_Rectifier:
        return IonRectifierFrameLayout::Size();
      case IonFrame_Bailed_Rectifier:
        return IonBailedRectifierFrameLayout::Size();
      case IonFrame_Exit:
        return IonExitFrameLayout::Size();
      default:
        JS_NOT_REACHED("unknown frame type");
    }
    return 0;
}

inline IonCommonFrameLayout *
IonFrameIterator::current() const
{
    return (IonCommonFrameLayout *)current_;
}

inline uint8 *
IonFrameIterator::returnAddress() const
{
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    return current->returnAddress();
}

inline size_t
IonFrameIterator::prevFrameLocalSize() const
{
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    return current->prevFrameLocalSize();
}

inline FrameType
IonFrameIterator::prevType() const
{
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    return current->prevType();
}

size_t
IonFrameIterator::frameSize() const
{
    JS_ASSERT(type_ != IonFrame_Exit);
    return frameSize_;
}


inline JSScript *
GetTopIonJSScript(JSContext *cx)
{
    IonFrameIterator iter(cx->runtime->ionTop);
    JS_ASSERT(iter.type() == IonFrame_Exit);
    ++iter;
    JS_ASSERT(iter.type() == IonFrame_JS);
    IonJSFrameLayout *frame = static_cast<IonJSFrameLayout*>(iter.current());
    switch (GetCalleeTokenTag(frame->calleeToken())) {
      case CalleeToken_Function: {
        JSFunction *fun = CalleeTokenToFunction(frame->calleeToken());
        return fun->script();
      }
      case CalleeToken_Script:
        return CalleeTokenToScript(frame->calleeToken());
      default:
        JS_NOT_REACHED("unexpected callee token kind");
        return NULL;
    }
}

} 
} 

#endif 

