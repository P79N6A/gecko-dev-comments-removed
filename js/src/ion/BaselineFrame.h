






#if !defined(jsion_baseline_frame_h__) && defined(JS_ION)
#define jsion_baseline_frame_h__

#include "jscntxt.h"
#include "jscompartment.h"

#include "IonFrames.h"

namespace js {
namespace ion {









class BaselineFrame
{
  public:
    enum Flags {
        
        HAS_RVAL         = 1 << 0,

        
        PREV_UP_TO_DATE  = 1 << 1
    };

  protected: 
    
    
    uint32_t loScratchValue_;
    uint32_t hiScratchValue_;
    uint32_t loReturnValue_;
    uint32_t hiReturnValue_;
    size_t frameSize_;
    JSObject *scopeChain_;
    uint32_t flags_;

  public:
    
    
    static const uint32_t FramePointerOffset = sizeof(void *);

    inline size_t frameSize() const {
        return frameSize_;
    }
    inline UnrootedObject scopeChain() const {
        return scopeChain_;
    }
    CalleeToken calleeToken() const {
        uint8_t *pointer = (uint8_t *)this + Size() + offsetOfCalleeToken();
        return *(CalleeToken *)pointer;
    }
    inline UnrootedScript script() const {
        return ScriptFromCalleeToken(calleeToken());
    }
    inline UnrootedFunction fun() const {
        return CalleeTokenToFunction(calleeToken());
    }
    inline UnrootedFunction callee() const {
        return CalleeTokenToFunction(calleeToken());
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

    unsigned numFormalArgs() const {
        return script()->function()->nargs;
    }
    Value *formals() const {
        return (Value *)(reinterpret_cast<const uint8_t *>(this) +
                         BaselineFrame::Size() +
                         offsetOfArg(0));
    }

    bool copyRawFrameSlots(AutoValueVector *vec) const;

    inline bool hasReturnValue() const {
        return flags_ & HAS_RVAL;
    }
    inline Value *returnValue() {
        return reinterpret_cast<Value *>(&loReturnValue_);
    }
    inline void setReturnValue(const Value &v) {
        flags_ |= HAS_RVAL;
        *returnValue() = v;
    }

    bool prevUpToDate() const {
        return flags_ & PREV_UP_TO_DATE;
    }
    void setPrevUpToDate() {
        flags_ |= PREV_UP_TO_DATE;
    }

    void *maybeHookData() const {
        return NULL;
    }

    void trace(JSTracer *trc);

    bool isGlobalFrame() const {
        return !script()->function();
    }
    bool isEvalFrame() const {
        return false;
    }
    bool isNonEvalFunctionFrame() const {
        return !!script()->function();
    }

    
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
    static inline size_t reverseOffsetOfFlags() {
        return -BaselineFrame::Size() + offsetof(BaselineFrame, flags_);
    }
    static inline size_t reverseOffsetOfReturnValue() {
        return -BaselineFrame::Size() + offsetof(BaselineFrame, loReturnValue_);
    }
    static inline size_t reverseOffsetOfLocal(size_t index) {
        return -BaselineFrame::Size() - (index + 1) * sizeof(Value);
    }
};


JS_STATIC_ASSERT(((sizeof(BaselineFrame) + BaselineFrame::FramePointerOffset) % 8) == 0);

} 
} 

#endif

