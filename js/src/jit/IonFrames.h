





#ifndef jit_IonFrames_h
#define jit_IonFrames_h

#ifdef JS_ION

#include "mozilla/DebugOnly.h"

#include "jscntxt.h"
#include "jsfun.h"

#include "jit/IonFrameIterator.h"

namespace js {
namespace jit {

typedef void * CalleeToken;

enum CalleeTokenTag
{
    CalleeToken_Function = 0x0, 
    CalleeToken_Script = 0x1,
    CalleeToken_ParallelFunction = 0x2
};

static inline CalleeTokenTag
GetCalleeTokenTag(CalleeToken token)
{
    CalleeTokenTag tag = CalleeTokenTag(uintptr_t(token) & 0x3);
    JS_ASSERT(tag <= CalleeToken_ParallelFunction);
    return tag;
}
static inline CalleeToken
CalleeToToken(JSFunction *fun)
{
    return CalleeToken(uintptr_t(fun) | uintptr_t(CalleeToken_Function));
}
static inline CalleeToken
CalleeToParallelToken(JSFunction *fun)
{
    return CalleeToken(uintptr_t(fun) | uintptr_t(CalleeToken_ParallelFunction));
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
static inline JSFunction *
CalleeTokenToParallelFunction(CalleeToken token)
{
    JS_ASSERT(GetCalleeTokenTag(token) == CalleeToken_ParallelFunction);
    return (JSFunction *)(uintptr_t(token) & ~uintptr_t(0x3));
}
static inline JSScript *
CalleeTokenToScript(CalleeToken token)
{
    JS_ASSERT(GetCalleeTokenTag(token) == CalleeToken_Script);
    return (JSScript *)(uintptr_t(token) & ~uintptr_t(0x3));
}

static inline JSScript *
ScriptFromCalleeToken(CalleeToken token)
{
    switch (GetCalleeTokenTag(token)) {
      case CalleeToken_Script:
        return CalleeTokenToScript(token);
      case CalleeToken_Function:
        return CalleeTokenToFunction(token)->nonLazyScript();
      case CalleeToken_ParallelFunction:
        return CalleeTokenToParallelFunction(token)->nonLazyScript();
    }
    MOZ_ASSUME_UNREACHABLE("invalid callee token tag");
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














static const uintptr_t FRAMESIZE_SHIFT = 4;
static const uintptr_t FRAMETYPE_BITS = 4;
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

struct BaselineBailoutInfo;


struct ResumeFromException
{
    static const uint32_t RESUME_ENTRY_FRAME = 0;
    static const uint32_t RESUME_CATCH = 1;
    static const uint32_t RESUME_FINALLY = 2;
    static const uint32_t RESUME_FORCED_RETURN = 3;
    static const uint32_t RESUME_BAILOUT = 4;

    uint8_t *framePointer;
    uint8_t *stackPointer;
    uint8_t *target;
    uint32_t kind;

    
    Value exception;

    BaselineBailoutInfo *bailoutInfo;
};

void HandleException(ResumeFromException *rfe);
void HandleParallelFailure(ResumeFromException *rfe);

void EnsureExitFrame(IonCommonFrameLayout *frame);

void MarkJitActivations(JSRuntime *rt, JSTracer *trc);
void MarkIonCompilerRoots(JSTracer *trc);

static inline uint32_t
MakeFrameDescriptor(uint32_t frameSize, FrameType type)
{
    return (frameSize << FRAMESIZE_SHIFT) | type;
}


inline JSScript *
GetTopIonJSScript(PerThreadData *pt, void **returnAddrOut)
{
    IonFrameIterator iter(pt->ionTop);
    JS_ASSERT(iter.type() == IonFrame_Exit);
    ++iter;

    JS_ASSERT(iter.returnAddressToFp() != NULL);
    if (returnAddrOut)
        *returnAddrOut = (void *) iter.returnAddressToFp();

    if (iter.isBaselineStub()) {
        ++iter;
        JS_ASSERT(iter.isBaselineJS());
    }

    JS_ASSERT(iter.isScripted());
    return iter.script();
}

inline JSScript *
GetTopIonJSScript(ThreadSafeContext *cx, void **returnAddrOut = NULL)
{
    return GetTopIonJSScript(cx->perThreadData, returnAddrOut);
}

} 
} 

#if defined(JS_CPU_X86) || defined (JS_CPU_X64)
# include "jit/shared/IonFrames-x86-shared.h"
#elif defined (JS_CPU_ARM)
# include "jit/arm/IonFrames-arm.h"
#else
# error "unsupported architecture"
#endif

namespace js {
namespace jit {

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

CalleeToken
MarkCalleeToken(JSTracer *trc, CalleeToken token);

} 
} 

#endif 

#endif 
