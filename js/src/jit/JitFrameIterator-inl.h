





#ifndef jit_JitFrameIterator_inl_h
#define jit_JitFrameIterator_inl_h

#ifdef JS_ION

#include "jit/JitFrameIterator.h"

#include "jit/Bailouts.h"
#include "jit/BaselineFrame.h"
#include "jit/IonFrames.h"

namespace js {
namespace jit {

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
