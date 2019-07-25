








































#ifndef jsion_frames_h__
#define jsion_frames_h__

#include "jstypes.h"
#include "jsutil.h"
#include "IonRegisters.h"
#include "IonCode.h"

struct JSFunction;
struct JSScript;

namespace js {
namespace ion {
















struct IonFrameInfo
{
    
    
    ptrdiff_t displacement;
    SnapshotOffset snapshotOffset;
};












enum FrameType
{
    IonFrame_JS,
    IonFrame_Entry,
    IonFrame_Rectifier,
    IonFrame_Exit
};
static const uint32 FRAMETYPE_BITS = 3;






















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

class IonFrameIterator
{
    uint8 *current_;
    FrameType type_;

    
    
    
    mutable uint8 *prevCache_;

  public:
    IonFrameIterator(uint8 *top)
      : current_(top),
        type_(IonFrame_Exit),
        prevCache_(top)
    { }

    
    FrameType type() const {
        return type_;
    }
    uint8 *fp() const {
        return current_;
    }
    uint8 *returnAddress() const;

    
    size_t prevFrameLocalSize() const;
    FrameType prevType() const;
    uint8 *prevFp() const;

    
    
    bool more() const {
        return prevType() != IonFrame_Entry;
    }
    void prev();
};


class MachineState
{
    uintptr_t *regs_;
    double *fpregs_;

  public:
    MachineState()
      : regs_(NULL), fpregs_(NULL)
    { }
    MachineState(uintptr_t *regs, double *fpregs)
      : regs_(regs), fpregs_(fpregs)
    { }

    double readFloatReg(FloatRegister reg) const {
        return fpregs_[reg.code()];
    }
    uintptr_t readReg(Register reg) const {
        return regs_[reg.code()];
    }
};


typedef uint32 BailoutId;
typedef uint32 SnapshotOffset;

class IonJSFrameLayout;


class FrameRecovery
{
    IonJSFrameLayout *fp_;
    uint8 *sp_;             

    MachineState machine_;
    uint32 snapshotOffset_;

    JSObject *callee_;
    JSFunction *fun_;
    JSScript *script_;

  private:
    FrameRecovery(uint8 *fp, uint8 *sp, const MachineState &machine);

    void setSnapshotOffset(uint32 snapshotOffset) {
        snapshotOffset_ = snapshotOffset;
    }
    void setBailoutId(BailoutId bailoutId);

  public:
    static FrameRecovery FromBailoutId(uint8 *fp, uint8 *sp, const MachineState &machine,
                                       BailoutId bailoutId);
    static FrameRecovery FromSnapshot(uint8 *fp, uint8 *sp, const MachineState &machine,
                                      SnapshotOffset offset);
    static FrameRecovery FromFrameIterator(const IonFrameIterator& it);

    MachineState &machine() {
        return machine_;
    }
    const MachineState &machine() const {
        return machine_;
    }
    uintptr_t readSlot(uint32 offset) const {
        JS_ASSERT((offset % STACK_SLOT_SIZE) == 0);
        return *(uintptr_t *)(sp_ + offset);
    }
    double readDoubleSlot(uint32 offset) const {
        JS_ASSERT((offset % STACK_SLOT_SIZE) == 0);
        return *(double *)(sp_ + offset);
    }
    JSObject *callee() const {
        return callee_;
    }
    JSFunction *fun() const {
        return fun_;
    }
    JSScript *script() const {
        return script_;
    }
    IonScript *ionScript() const;
    uint32 snapshotOffset() const {
        return snapshotOffset_;
    }
};


struct ResumeFromException
{
    void *stackPointer;
};

void HandleException(ResumeFromException *rfe);

static inline uint32
MakeFrameDescriptor(uint32 frameSize, FrameType type)
{
    return (frameSize << FRAMETYPE_BITS) | type;
}

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

#if defined(JS_CPU_X86) || defined (JS_CPU_X64)
# include "ion/shared/IonFrames-x86-shared.h"
#elif defined (JS_CPU_ARM)
# include "ion/arm/IonFrames-arm.h"
#else
# error "unsupported architecture"
#endif

#endif 

