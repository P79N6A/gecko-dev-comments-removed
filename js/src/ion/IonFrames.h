








































#ifndef jsion_frames_h__
#define jsion_frames_h__

#include "jstypes.h"
#include "jsutil.h"

struct JSFunction;
struct JSScript;

namespace js {
namespace ion {













class IonFrameData
{
  protected:
    void *returnAddress_;
    uintptr_t sizeDescriptor_;
    void *calleeToken_;
};

class IonFramePrefix : public IonFrameData
{
  public:
    
    bool isEntryFrame() const {
        return !(sizeDescriptor_ & 1);
    }
    
    size_t prevFrameDepth() const {
        JS_ASSERT(!isEntryFrame());
        return sizeDescriptor_ >> 1;
    }
    IonFramePrefix *prev() const {
        JS_ASSERT(!isEntryFrame());
        return (IonFramePrefix *)((uint8 *)this - prevFrameDepth());
    }
    void *calleeToken() const {
        return calleeToken_;
    }
    void setReturnAddress(void *address) {
        returnAddress_ = address;
    }
};

static const uint32 ION_FRAME_PREFIX_SIZE = sizeof(IonFramePrefix);






















static const uint32 NO_FRAME_SIZE_CLASS_ID = uint32(-1);

class FrameSizeClass
{
    uint32 class_;

    explicit FrameSizeClass(uint32 class_) : class_(class_)
    { }
  
  public:
    FrameSizeClass()
    { }

    static FrameSizeClass None() {
        return FrameSizeClass(NO_FRAME_SIZE_CLASS_ID);
    }
    static FrameSizeClass FromClass(uint32 class_) {
        return FrameSizeClass(class_);
    }

    
    static FrameSizeClass FromDepth(uint32 frameDepth);
    uint32 frameSize() const;

    uint32 classId() const {
        JS_ASSERT(class_ != NO_FRAME_SIZE_CLASS_ID);
        return class_;
    }

    bool operator ==(const FrameSizeClass &other) const {
        return class_ == other.class_;
    }
    bool operator !=(const FrameSizeClass &other) const {
        return class_ != other.class_;
    }
};

typedef void * CalleeToken;

static inline CalleeToken
CalleeToToken(JSObject *fun)
{
    return (CalleeToken *)fun;
}
static inline CalleeToken
CalleeToToken(JSScript *script)
{
    return (CalleeToken *)(uintptr_t(script) | 1);
}
static inline bool
IsCalleeTokenFunction(CalleeToken token)
{
    return (uintptr_t(token) & 1) == 0;
}
static inline JSObject *
CalleeTokenToFunction(CalleeToken token)
{
    JS_ASSERT(IsCalleeTokenFunction(token));
    return (JSObject *)token;
}
static inline JSScript *
CalleeTokenToScript(CalleeToken token)
{
    JS_ASSERT(!IsCalleeTokenFunction(token));
    return (JSScript*)(uintptr_t(token) & ~uintptr_t(1));
}

}
}

#endif 

