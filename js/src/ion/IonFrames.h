








































#ifndef jsion_frames_h__
#define jsion_frames_h__

#include "jsutil.h"
#include "IonRegisters.h"
#include "IonCode.h"
#include "IonFrameIterator.h"

struct JSFunction;
struct JSScript;

namespace js {
namespace ion {
















struct IonFrameInfo
{
    
    
    ptrdiff_t displacement;
    SnapshotOffset snapshotOffset;
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

class IonJSFrameLayout;


class FrameRecovery
{
    IonJSFrameLayout *fp_;
    uint8 *sp_;             

    MachineState machine_;
    uint32 snapshotOffset_;

    JSFunction *callee_;
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
CalleeToToken(JSFunction *fun)
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
static inline JSFunction *
CalleeTokenToFunction(CalleeToken token)
{
    JS_ASSERT(IsCalleeTokenFunction(token));
    return (JSFunction *)token;
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

