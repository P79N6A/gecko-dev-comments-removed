





#ifndef jit_IonFrameIterator_inl_h
#define jit_IonFrameIterator_inl_h

#ifdef JS_ION

#include "jit/IonFrameIterator.h"

#include "jit/Bailouts.h"
#include "jit/BaselineFrame.h"

namespace js {
namespace jit {

template <AllowGC allowGC>
inline
InlineFrameIteratorMaybeGC<allowGC>::InlineFrameIteratorMaybeGC(
                                                JSContext *cx, const IonBailoutIterator *iter)
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
IonFrameIterator::baselineFrame() const
{
    JS_ASSERT(isBaselineJS());
    return (BaselineFrame *)(fp() - BaselineFrame::FramePointerOffset - BaselineFrame::Size());
}

} 
} 

#endif 

#endif 
