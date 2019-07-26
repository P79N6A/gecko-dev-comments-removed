





#ifndef jit_IonFrames_inl_h
#define jit_IonFrames_inl_h

#ifdef JS_ION

#include "jit/IonFrames.h"

#include "jit/JitFrameIterator.h"
#include "jit/LIR.h"
#include "vm/ForkJoin.h"

#include "jit/JitFrameIterator-inl.h"

namespace js {
namespace jit {

inline void
SafepointIndex::resolve()
{
    JS_ASSERT(!resolved);
    safepointOffset_ = safepoint_->offset();
#ifdef DEBUG
    resolved = true;
#endif
}

inline uint8_t *
JitFrameIterator::returnAddress() const
{
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    return current->returnAddress();
}

inline size_t
JitFrameIterator::prevFrameLocalSize() const
{
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    return current->prevFrameLocalSize();
}

inline FrameType
JitFrameIterator::prevType() const
{
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    return current->prevType();
}

inline bool
JitFrameIterator::isFakeExitFrame() const
{
    bool res = (prevType() == JitFrame_Unwound_Rectifier ||
                prevType() == JitFrame_Unwound_IonJS ||
                prevType() == JitFrame_Unwound_BaselineStub ||
                (prevType() == JitFrame_Entry && type() == JitFrame_Exit));
    JS_ASSERT_IF(res, type() == JitFrame_Exit || type() == JitFrame_BaselineJS);
    return res;
}

inline IonExitFrameLayout *
JitFrameIterator::exitFrame() const
{
    JS_ASSERT(type() == JitFrame_Exit);
    JS_ASSERT(!isFakeExitFrame());
    return (IonExitFrameLayout *) fp();
}

inline BaselineFrame *
GetTopBaselineFrame(JSContext *cx)
{
    JitFrameIterator iter(cx);
    JS_ASSERT(iter.type() == JitFrame_Exit);
    ++iter;
    if (iter.isBaselineStub())
        ++iter;
    JS_ASSERT(iter.isBaselineJS());
    return iter.baselineFrame();
}

inline JSScript *
GetTopIonJSScript(JSContext *cx, void **returnAddrOut = nullptr)
{
    return GetTopIonJSScript(cx->mainThread().jitTop, returnAddrOut, SequentialExecution);
}

inline JSScript *
GetTopIonJSScript(ForkJoinContext *cx, void **returnAddrOut = nullptr)
{
    return GetTopIonJSScript(cx->perThreadData->jitTop, returnAddrOut, ParallelExecution);
}

} 
} 

#endif 

#endif 
