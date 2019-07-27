





#ifndef jit_JitFrameIterator_inl_h
#define jit_JitFrameIterator_inl_h

#include "jit/JitFrameIterator.h"

#include "jit/Bailouts.h"
#include "jit/BaselineFrame.h"
#include "jit/JitFrames.h"

namespace js {
namespace jit {

inline JitFrameLayout *
JitProfilingFrameIterator::framePtr()
{
    MOZ_ASSERT(!done());
    return (JitFrameLayout *) fp_;
}

inline JSScript *
JitProfilingFrameIterator::frameScript()
{
    return ScriptFromCalleeToken(framePtr()->calleeToken());
}

inline BaselineFrame *
JitFrameIterator::baselineFrame() const
{
    MOZ_ASSERT(isBaselineJS());
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
