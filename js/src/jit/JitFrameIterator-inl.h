





#ifndef jit_JitFrameIterator_inl_h
#define jit_JitFrameIterator_inl_h

#ifdef JS_ION

#include "jit/JitFrameIterator.h"

#include "jit/Bailouts.h"
#include "jit/BaselineFrame.h"
#include "jit/IonFrames.h"

namespace js {
namespace jit {

template <AllowGC allowGC>
inline
InlineFrameIteratorMaybeGC<allowGC>::InlineFrameIteratorMaybeGC(ThreadSafeContext *cx,
                                                                const IonBailoutIterator *iter)
  : frame_(iter),
    framesRead_(0),
    frameCount_(UINT32_MAX),
    callee_(cx),
    script_(cx)
{
    if (iter) {
        start_ = SnapshotIterator(*iter);
        findNextFrame();
    }
}

inline BaselineFrame *
JitFrameIterator::baselineFrame() const
{
    JS_ASSERT(isBaselineJS());
    return (BaselineFrame *)(fp() - BaselineFrame::FramePointerOffset - BaselineFrame::Size());
}

template <typename T>
bool
JitFrameIterator::isExitFrameLayout() const
{
    if (type_ != JitFrame_Exit || isFakeExitFrame())
        return false;
    return exitFrame()->is<T>();
}

} 
} 

#endif 

#endif 
