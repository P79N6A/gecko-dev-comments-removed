





#ifndef ion_IonFrameIterator_inl_h
#define ion_IonFrameIterator_inl_h

#ifdef JS_ION

#include "ion/IonFrameIterator.h"

#include "ion/Bailouts.h"
#include "ion/BaselineFrame.h"

namespace js {
namespace ion {

template <AllowGC allowGC>
inline
InlineFrameIteratorMaybeGC<allowGC>::InlineFrameIteratorMaybeGC(
                                                JSContext *cx, const IonBailoutIterator *iter)
  : frame_(iter),
    framesRead_(0),
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
