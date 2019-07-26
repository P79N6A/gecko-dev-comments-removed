






#ifndef jsion_frames_h__
#define jsion_frames_h__

#include "mozilla/DebugOnly.h"

#include "jsfun.h"
#include "jstypes.h"
#include "jsutil.h"
#include "Registers.h"
#include "IonCode.h"
#include "IonFrameIterator.h"

class JSFunction;
class JSScript;

namespace js {
namespace ion {

typedef void * CalleeToken;

enum CalleeTokenTag
{
    CalleeToken_Function = 0x0, 
    CalleeToken_Script = 0x1
};

static inline CalleeTokenTag
GetCalleeTokenTag(CalleeToken token)
{
    CalleeTokenTag tag = CalleeTokenTag(uintptr_t(token) & 0x3);
    JS_ASSERT(tag <= CalleeToken_Script);
    return tag;
}
static inline CalleeToken
CalleeToToken(JSFunction *fun)
{
    return CalleeToken(uintptr_t(fun) | uintptr_t(CalleeToken_Function));
}
static inline CalleeToken
CalleeToToken(RawScript script)
{
    return CalleeToken(uintptr_t(script) | uintptr_t(CalleeToken_Script));
}
static inline bool
CalleeTokenIsFunction(CalleeToken token)
{
    return GetCalleeTokenTag(token) == CalleeToken_Function;
}
static inline JSFunction *
CalleeTokenToFunction(CalleeToken token)
{
    JS_ASSERT(CalleeTokenIsFunction(token));
    return (JSFunction *)token;
}
static inline RawScript
CalleeTokenToScript(CalleeToken token)
{
    JS_ASSERT(GetCalleeTokenTag(token) == CalleeToken_Script);
    return (RawScript)(uintptr_t(token) & ~uintptr_t(0x3));
}

static inline RawScript
ScriptFromCalleeToken(CalleeToken token)
{
    AutoAssertNoGC nogc;
    switch (GetCalleeTokenTag(token)) {
      case CalleeToken_Script:
        return CalleeTokenToScript(token);
      case CalleeToken_Function:
        return CalleeTokenToFunction(token)->nonLazyScript();
    }
    JS_NOT_REACHED("invalid callee token tag");
    return NULL;
}













class LSafepoint;



class SafepointIndex
{
    
    
    uint32_t displacement_;

    union {
        LSafepoint *safepoint_;

        
        uint32_t safepointOffset_;
    };

    mozilla::DebugOnly<bool> resolved;

  public:
    SafepointIndex(uint32_t displacement, LSafepoint *safepoint)
      : displacement_(displacement),
        safepoint_(safepoint),
        resolved(false)
    { }

    void resolve();

    LSafepoint *safepoint() {
        JS_ASSERT(!resolved);
        return safepoint_;
    }
    uint32_t displacement() const {
        return displacement_;
    }
    uint32_t safepointOffset() const {
        return safepointOffset_;
    }
    void adjustDisplacement(uint32_t offset) {
        JS_ASSERT(offset >= displacement_);
        displacement_ = offset;
    }
    inline SnapshotOffset snapshotOffset() const;
    inline bool hasSnapshotOffset() const;
};

class MacroAssembler;




class OsiIndex
{
    uint32_t callPointDisplacement_;
    uint32_t snapshotOffset_;

  public:
    OsiIndex(uint32_t callPointDisplacement, uint32_t snapshotOffset)
      : callPointDisplacement_(callPointDisplacement),
        snapshotOffset_(snapshotOffset)
    { }

    uint32_t returnPointDisplacement() const;
    uint32_t callPointDisplacement() const {
        return callPointDisplacement_;
    }
    uint32_t snapshotOffset() const {
        return snapshotOffset_;
    }
    void fixUpOffset(MacroAssembler &masm);
};














static const uintptr_t FRAMESIZE_SHIFT = 3;
static const uintptr_t FRAMETYPE_BITS = 3;
static const uintptr_t FRAMETYPE_MASK = (1 << FRAMETYPE_BITS) - 1;






















static const uint32_t NO_FRAME_SIZE_CLASS_ID = uint32_t(-1);

class FrameSizeClass
{
    uint32_t class_;

    explicit FrameSizeClass(uint32_t class_) : class_(class_)
    { }
  
  public:
    FrameSizeClass()
    { }

    static FrameSizeClass None() {
        return FrameSizeClass(NO_FRAME_SIZE_CLASS_ID);
    }
    static FrameSizeClass FromClass(uint32_t class_) {
        return FrameSizeClass(class_);
    }

    
    static FrameSizeClass FromDepth(uint32_t frameDepth);
    static FrameSizeClass ClassLimit();
    uint32_t frameSize() const;

    uint32_t classId() const {
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


struct ResumeFromException
{
    void *stackPointer;
};

void HandleException(ResumeFromException *rfe);

void EnsureExitFrame(IonCommonFrameLayout *frame);

void MarkIonActivations(JSRuntime *rt, JSTracer *trc);
void MarkIonCompilerRoots(JSTracer *trc);

static inline uint32_t
MakeFrameDescriptor(uint32_t frameSize, FrameType type)
{
    return (frameSize << FRAMESIZE_SHIFT) | type;
}

} 
} 

#if defined(JS_CPU_X86) || defined (JS_CPU_X64)
# include "ion/shared/IonFrames-x86-shared.h"
#elif defined (JS_CPU_ARM)
# include "ion/arm/IonFrames-arm.h"
#else
# error "unsupported architecture"
#endif

namespace js {
namespace ion {

RawScript
GetTopIonJSScript(JSContext *cx,
                  const SafepointIndex **safepointIndexOut = NULL,
                  void **returnAddrOut = NULL);

void
GetPcScript(JSContext *cx, JSScript **scriptRes, jsbytecode **pcRes);




static inline int32_t
OffsetOfFrameSlot(int32_t slot)
{
    if (slot <= 0)
        return -slot;
    return -(slot * STACK_SLOT_SIZE);
}

static inline uintptr_t
ReadFrameSlot(IonJSFrameLayout *fp, int32_t slot)
{
    return *(uintptr_t *)((char *)fp + OffsetOfFrameSlot(slot));
}

static inline double
ReadFrameDoubleSlot(IonJSFrameLayout *fp, int32_t slot)
{
    return *(double *)((char *)fp + OffsetOfFrameSlot(slot));
}

} 
} 

#endif 

