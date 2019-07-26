






#ifndef jsion_frames_inl_h__
#define jsion_frames_inl_h__

#include "ion/IonFrames.h"
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
      case IonFrame_OptimizedJS:
      case IonFrame_Unwound_OptimizedJS:
        return IonJSFrameLayout::Size();
      case IonFrame_Rectifier:
        return IonRectifierFrameLayout::Size();
      case IonFrame_Unwound_Rectifier:
        return IonUnwoundRectifierFrameLayout::Size();
      case IonFrame_Exit:
        return IonExitFrameLayout::Size();
      case IonFrame_Osr:
        return IonOsrFrameLayout::Size();
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

inline uint8_t *
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


inline RawScript
GetTopIonJSScript(JSContext *cx, const SafepointIndex **safepointIndexOut, void **returnAddrOut)
{
    AutoAssertNoGC nogc;
    IonFrameIterator iter(cx->mainThread().ionTop);
    JS_ASSERT(iter.type() == IonFrame_Exit);
    ++iter;

    
    if (safepointIndexOut)
        *safepointIndexOut = iter.safepoint();

    JS_ASSERT(iter.returnAddressToFp() != NULL);
    if (returnAddrOut)
        *returnAddrOut = (void *) iter.returnAddressToFp();

    JS_ASSERT(iter.isScripted());
    return iter.script();
}

} 
} 

#endif 

