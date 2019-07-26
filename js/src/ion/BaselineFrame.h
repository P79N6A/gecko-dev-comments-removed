






#if !defined(jsion_baseline_frame_h__) && defined(JS_ION)
#define jsion_baseline_frame_h__

#include "jscntxt.h"
#include "jscompartment.h"

#include "IonFrames.h"

namespace js {
namespace ion {









class BaselineFrame
{
  protected: 
    
    
    uint32_t loScratchValue_;
    uint32_t hiScratchValue_;
    size_t frameSize_;
    JSObject *scopeChain_;
    uint32_t dummy; 

  public:
    
    
    static const uint32_t FramePointerOffset = sizeof(void *);

    inline size_t frameSize() const {
        return frameSize_;
    }
    inline UnrootedObject scopeChain() const {
        return scopeChain_;
    }
    inline size_t numValueSlots() const {
        size_t size = frameSize();

        JS_ASSERT(size >= BaselineFrame::FramePointerOffset + BaselineFrame::Size());
        size -= BaselineFrame::FramePointerOffset + BaselineFrame::Size();

        JS_ASSERT((size % sizeof(Value)) == 0);
        return size / sizeof(Value);
    }
    inline Value *valueSlot(size_t slot) const {
        JS_ASSERT(slot < numValueSlots());
        return (Value *)this - (slot + 1);
    }

    void trace(JSTracer *trc);

    
    static size_t offsetOfCalleeToken() {
        return FramePointerOffset + js::ion::IonJSFrameLayout::offsetOfCalleeToken();
    }
    static inline size_t offsetOfThis() {
        return FramePointerOffset + js::ion::IonJSFrameLayout::offsetOfThis();
    }
    static inline size_t offsetOfArg(size_t index) {
        return FramePointerOffset + js::ion::IonJSFrameLayout::offsetOfActualArg(index);
    }
    static size_t Size() {
        return sizeof(BaselineFrame);
    }

    
    
    
    static inline size_t reverseOffsetOfFrameSize() {
        return -BaselineFrame::Size() + offsetof(BaselineFrame, frameSize_);
    }
    static inline size_t reverseOffsetOfScratchValue() {
        return -BaselineFrame::Size() + offsetof(BaselineFrame, loScratchValue_);
    }
    static inline size_t reverseOffsetOfScopeChain() {
        return -BaselineFrame::Size() + offsetof(BaselineFrame, scopeChain_);
    }
    static inline size_t reverseOffsetOfLocal(size_t index) {
        return -BaselineFrame::Size() - (index + 1) * sizeof(Value);
    }
};


JS_STATIC_ASSERT(((sizeof(BaselineFrame) + BaselineFrame::FramePointerOffset) % 8) == 0);

} 
} 

#endif

