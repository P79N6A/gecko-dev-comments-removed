








































#ifndef jsion_frames_h__
#define jsion_frames_h__

#include "jsfun.h"
#include "jstypes.h"
#include "jsutil.h"
#include "IonRegisters.h"
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
    CalleeToken_Script = 0x1,
    CalleeToken_InvalidationRecord = 0x2
};

static inline CalleeTokenTag
GetCalleeTokenTag(CalleeToken token)
{
    CalleeTokenTag tag = CalleeTokenTag(uintptr_t(token) & 0x3);
    JS_ASSERT(tag != 0x3);
    return tag;
}
static inline bool
CalleeTokenIsInvalidationRecord(CalleeToken token)
{
    return GetCalleeTokenTag(token) == CalleeToken_InvalidationRecord;
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
static inline CalleeToken
InvalidationRecordToToken(InvalidationRecord *record)
{
    return CalleeToken(uintptr_t(record) | uintptr_t(CalleeToken_InvalidationRecord));
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
static inline InvalidationRecord *
CalleeTokenToInvalidationRecord(CalleeToken token)
{
    JS_ASSERT(GetCalleeTokenTag(token) == CalleeToken_InvalidationRecord);
    return (InvalidationRecord *)(uintptr_t(token) & ~uintptr_t(0x3));
}
JSScript *
MaybeScriptFromCalleeToken(CalleeToken token);
















class IonFrameInfo
{
  private:
    
    
    uint32 displacement_;

    
    uint32 safepointOffset_;

    
    
    SnapshotOffset snapshotOffset_;

  public:
    IonFrameInfo(uint32 displacement, uint32 safepointOffset, SnapshotOffset snapshotOffset)
      : displacement_(displacement),
        safepointOffset_(safepointOffset),
        snapshotOffset_(snapshotOffset)
    { }

    uint32 displacement() const {
        return displacement_;
    }
    uint32 safepointOffset() const {
        return safepointOffset_;
    }
    inline SnapshotOffset snapshotOffset() const;
    inline bool hasSnapshotOffset() const;
};

struct InvalidationRecord
{
    void *calleeToken;
    uint8 *returnAddress;
    IonScript *ionScript;

    InvalidationRecord(void *calleeToken, uint8 *returnAddress);

    static InvalidationRecord *New(void *calleeToken, uint8 *returnAddress);
    static void Destroy(JSContext *cx, InvalidationRecord *record);
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

    void unpackCalleeToken(CalleeToken token);

    static int32 OffsetOfSlot(int32 slot);

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
    uintptr_t readSlot(int32 slot) const {
        return *(uintptr_t *)((char *)fp_ + OffsetOfSlot(slot));
    }
    double readDoubleSlot(int32 slot) const {
        return *(double *)((char *)fp_ + OffsetOfSlot(slot));
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

void MarkIonActivations(ThreadData *td, JSTracer *trc);

static inline uint32
MakeFrameDescriptor(uint32 frameSize, FrameType type)
{
    return (frameSize << FRAMETYPE_BITS) | type;
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

inline IonCommonFrameLayout *
IonFrameIterator::current() const
{
    return (IonCommonFrameLayout *)current_;
}

inline uint8 *
IonFrameIterator::returnAddress() const
{
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    return current->returnAddress();
}

inline size_t
IonFrameIterator::prevFrameLocalSize() const
{
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    return current->prevFrameLocalSize();
}

inline FrameType
IonFrameIterator::prevType() const
{
    IonCommonFrameLayout *current = (IonCommonFrameLayout *) current_;
    return current->prevType();
}

inline bool 
IonFrameIterator::more() const
{
    return type_ != IonFrame_Entry;
}

static inline IonScript *
GetTopIonFrame(JSContext *cx)
{
    IonFrameIterator iter(JS_THREAD_DATA(cx)->ionTop);
    JS_ASSERT(iter.type() == IonFrame_Exit);
    ++iter;
    JS_ASSERT(iter.type() == IonFrame_JS);
    IonJSFrameLayout *frame = static_cast<IonJSFrameLayout*>(iter.current());
    switch (GetCalleeTokenTag(frame->calleeToken())) {
      case CalleeToken_Function: {
        JSFunction *fun = CalleeTokenToFunction(frame->calleeToken());
        return fun->script()->ion;
      }
      case CalleeToken_Script:
        return CalleeTokenToScript(frame->calleeToken())->ion;
      default:
        JS_NOT_REACHED("unexpected callee token kind");
        return NULL;
    }
}

} 
} 

#endif 

