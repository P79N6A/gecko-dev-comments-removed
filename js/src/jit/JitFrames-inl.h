





#ifndef jit_JitFrames_inl_h
#define jit_JitFrames_inl_h

#include "jit/JitFrames.h"

#include "jit/JitFrameIterator.h"
#include "jit/LIR.h"

#include "jit/JitFrameIterator-inl.h"

namespace js {
namespace jit {

inline void
SafepointIndex::resolve()
{
    MOZ_ASSERT(!resolved);
    safepointOffset_ = safepoint_->offset();
#ifdef DEBUG
    resolved = true;
#endif
}

inline uint8_t *
JitFrameIterator::returnAddress() const
{
    CommonFrameLayout *current = (CommonFrameLayout *) current_;
    return current->returnAddress();
}

inline size_t
JitFrameIterator::prevFrameLocalSize() const
{
    CommonFrameLayout *current = (CommonFrameLayout *) current_;
    return current->prevFrameLocalSize();
}

inline FrameType
JitFrameIterator::prevType() const
{
    CommonFrameLayout *current = (CommonFrameLayout *) current_;
    return current->prevType();
}

inline bool
JitFrameIterator::isFakeExitFrame() const
{
    bool res = (prevType() == JitFrame_Unwound_Rectifier ||
                prevType() == JitFrame_Unwound_IonJS ||
                prevType() == JitFrame_Unwound_BaselineJS ||
                prevType() == JitFrame_Unwound_BaselineStub ||
                prevType() == JitFrame_Unwound_IonAccessorIC ||
                (prevType() == JitFrame_Entry && type() == JitFrame_Exit));
    MOZ_ASSERT_IF(res, type() == JitFrame_Exit || type() == JitFrame_BaselineJS);
    return res;
}

inline ExitFrameLayout *
JitFrameIterator::exitFrame() const
{
    MOZ_ASSERT(type() == JitFrame_Exit);
    MOZ_ASSERT(!isFakeExitFrame());
    return (ExitFrameLayout *) fp();
}

inline BaselineFrame *
GetTopBaselineFrame(JSContext *cx)
{
    JitFrameIterator iter(cx);
    MOZ_ASSERT(iter.type() == JitFrame_Exit);
    ++iter;
    if (iter.isBaselineStub())
        ++iter;
    MOZ_ASSERT(iter.isBaselineJS());
    return iter.baselineFrame();
}

} 
} 

#endif 
