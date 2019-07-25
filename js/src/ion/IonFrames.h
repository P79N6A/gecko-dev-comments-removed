








































#ifndef jsion_frames_h__
#define jsion_frames_h__

#include "jsfun.h"
#include "jstypes.h"
#include "jsutil.h"
#include "Registers.h"
#include "IonCode.h"
#include "IonFrameIterator.h"

struct JSFunction;
struct JSScript;

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
CalleeToToken(JSScript *script)
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
static inline JSScript *
CalleeTokenToScript(CalleeToken token)
{
    JS_ASSERT(GetCalleeTokenTag(token) == CalleeToken_Script);
    return (JSScript *)(uintptr_t(token) & ~uintptr_t(0x3));
}
JSScript *
MaybeScriptFromCalleeToken(CalleeToken token);













class LSafepoint;



class SafepointIndex
{
    
    
    uint32 displacement_;

    union {
        LSafepoint *safepoint_;

        
        uint32 safepointOffset_;
    };

    DebugOnly<bool> resolved;

  public:
    SafepointIndex(uint32 displacement, LSafepoint *safepoint)
      : displacement_(displacement),
        safepoint_(safepoint),
        resolved(false)
    { }

    void resolve();

    LSafepoint *safepoint() {
        JS_ASSERT(!resolved);
        return safepoint_;
    }
    uint32 displacement() const {
        return displacement_;
    }
    uint32 safepointOffset() const {
        return safepointOffset_;
    }
    void adjustDisplacement(uint32 offset) {
        JS_ASSERT(offset >= displacement_);
        displacement_ = offset;
    }
    inline SnapshotOffset snapshotOffset() const;
    inline bool hasSnapshotOffset() const;
};

class MacroAssembler;




class OsiIndex
{
    uint32 callPointDisplacement_;
    uint32 snapshotOffset_;

  public:
    OsiIndex(uint32 callPointDisplacement, uint32 snapshotOffset)
      : callPointDisplacement_(callPointDisplacement),
        snapshotOffset_(snapshotOffset)
    { }

    uint32 returnPointDisplacement() const;
    uint32 callPointDisplacement() const {
        return callPointDisplacement_;
    }
    uint32 snapshotOffset() const {
        return snapshotOffset_;
    }
    void fixUpOffset(MacroAssembler &masm);
};














static const uintptr_t FRAMESIZE_SHIFT = 3;
static const uintptr_t FRAMETYPE_BITS = 3;
static const uintptr_t FRAMETYPE_MASK = (1 << FRAMETYPE_BITS) - 1;






















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

class IonJSFrameLayout;
class IonFrameIterator;


class FrameRecovery
{
    IonJSFrameLayout *fp_;
    uint8 *sp_;             

    MachineState machine_;
    uint32 snapshotOffset_;

    JSFunction *callee_;
    JSScript *script_;
    IonScript *ionScript_;

  private:
    FrameRecovery(uint8 *fp, uint8 *sp, const MachineState &machine);

    void setSnapshotOffset(uint32 snapshotOffset) {
        snapshotOffset_ = snapshotOffset;
    }
    void setBailoutId(BailoutId bailoutId);

    void unpackCalleeToken(CalleeToken token);

  public:
    static FrameRecovery FromBailoutId(uint8 *fp, uint8 *sp, const MachineState &machine,
                                       BailoutId bailoutId);
    static FrameRecovery FromSnapshot(uint8 *fp, uint8 *sp, const MachineState &machine,
                                      SnapshotOffset offset);

    
    void setIonScript(IonScript *ionScript);

    MachineState &machine() {
        return machine_;
    }
    const MachineState &machine() const {
        return machine_;
    }
    JSFunction *callee() const {
        return callee_;
    }
    JSScript *script() const {
        return script_;
    }
    IonScript *ionScript() const;
    uint32 snapshotOffset() const {
        return snapshotOffset_;
    }
    uint32 frameSize() const {
        return ((uint8 *) fp_) - sp_;
    }
    IonJSFrameLayout *fp() {
        return fp_;
    }
};


struct ResumeFromException
{
    void *stackPointer;
};

void HandleException(ResumeFromException *rfe);

void MarkIonActivations(JSRuntime *rt, JSTracer *trc);

static inline uint32
MakeFrameDescriptor(uint32 frameSize, FrameType type)
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

inline JSScript *
GetTopIonJSScript(JSContext *cx);

void
GetPcScript(JSContext *cx, JSScript **scriptRes, jsbytecode **pcRes);




static inline int32
OffsetOfFrameSlot(int32 slot)
{
    if (slot <= 0)
        return sizeof(IonJSFrameLayout) + -slot;
    return -(slot * STACK_SLOT_SIZE);
}

static inline uintptr_t
ReadFrameSlot(IonJSFrameLayout *fp, int32 slot)
{
    return *(uintptr_t *)((char *)fp + OffsetOfFrameSlot(slot));
}

static inline double
ReadFrameDoubleSlot(IonJSFrameLayout *fp, int32 slot)
{
    return *(double *)((char *)fp + OffsetOfFrameSlot(slot));
}

} 
} 

#endif 

