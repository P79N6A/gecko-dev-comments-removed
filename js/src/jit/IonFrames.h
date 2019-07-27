





#ifndef jit_IonFrames_h
#define jit_IonFrames_h

#include <stdint.h>

#include "jscntxt.h"
#include "jsfun.h"

#include "jit/JitFrameIterator.h"

namespace js {
namespace jit {

typedef void * CalleeToken;

enum CalleeTokenTag
{
    CalleeToken_Function = 0x0, 
    CalleeToken_FunctionConstructing = 0x1,
    CalleeToken_Script = 0x2
};

static const uintptr_t CalleeTokenMask = ~uintptr_t(0x3);

static inline CalleeTokenTag
GetCalleeTokenTag(CalleeToken token)
{
    CalleeTokenTag tag = CalleeTokenTag(uintptr_t(token) & 0x3);
    JS_ASSERT(tag <= CalleeToken_Script);
    return tag;
}
static inline CalleeToken
CalleeToToken(JSFunction *fun, bool constructing)
{
    CalleeTokenTag tag = constructing ? CalleeToken_FunctionConstructing : CalleeToken_Function;
    return CalleeToken(uintptr_t(fun) | uintptr_t(tag));
}
static inline CalleeToken
CalleeToToken(JSScript *script)
{
    return CalleeToken(uintptr_t(script) | uintptr_t(CalleeToken_Script));
}
static inline bool
CalleeTokenIsFunction(CalleeToken token)
{
    CalleeTokenTag tag = GetCalleeTokenTag(token);
    return tag == CalleeToken_Function || tag == CalleeToken_FunctionConstructing;
}
static inline bool
CalleeTokenIsConstructing(CalleeToken token)
{
    return GetCalleeTokenTag(token) == CalleeToken_FunctionConstructing;
}
static inline JSFunction *
CalleeTokenToFunction(CalleeToken token)
{
    MOZ_ASSERT(CalleeTokenIsFunction(token));
    return (JSFunction *)(uintptr_t(token) & CalleeTokenMask);
}
static inline JSScript *
CalleeTokenToScript(CalleeToken token)
{
    JS_ASSERT(GetCalleeTokenTag(token) == CalleeToken_Script);
    return (JSScript *)(uintptr_t(token) & CalleeTokenMask);
}

static inline JSScript *
ScriptFromCalleeToken(CalleeToken token)
{
    switch (GetCalleeTokenTag(token)) {
      case CalleeToken_Script:
        return CalleeTokenToScript(token);
      case CalleeToken_Function:
      case CalleeToken_FunctionConstructing:
        return CalleeTokenToFunction(token)->nonLazyScript();
    }
    MOZ_CRASH("invalid callee token tag");
}













class LSafepoint;



class SafepointIndex
{
    
    
    uint32_t displacement_;

    union {
        LSafepoint *safepoint_;

        
        uint32_t safepointOffset_;
    };

#ifdef DEBUG
    bool resolved;
#endif

  public:
    SafepointIndex(uint32_t displacement, LSafepoint *safepoint)
      : displacement_(displacement),
        safepoint_(safepoint)
#ifdef DEBUG
      , resolved(false)
#endif
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

void MarkJitActivations(PerThreadData *ptd, JSTracer *trc);
void MarkIonCompilerRoots(JSTracer *trc);

JSCompartment *
TopmostIonActivationCompartment(JSRuntime *rt);

#ifdef JSGC_GENERATIONAL
template<typename T>
void UpdateJitActivationsForMinorGC(PerThreadData *ptd, JSTracer *trc);
#endif

static inline uint32_t
MakeFrameDescriptor(uint32_t frameSize, FrameType type)
{
    return (frameSize << FRAMESIZE_SHIFT) | type;
}


inline JSScript *
GetTopIonJSScript(uint8_t *jitTop, void **returnAddrOut, ExecutionMode mode)
{
    JitFrameIterator iter(jitTop, mode);
    JS_ASSERT(iter.type() == JitFrame_Exit);
    ++iter;

    JS_ASSERT(iter.returnAddressToFp() != nullptr);
    if (returnAddrOut)
        *returnAddrOut = (void *) iter.returnAddressToFp();

    if (iter.isBaselineStub()) {
        ++iter;
        JS_ASSERT(iter.isBaselineJS());
    }

    JS_ASSERT(iter.isScripted());
    return iter.script();
}

#ifdef JS_CODEGEN_MIPS
uint8_t *alignDoubleSpillWithOffset(uint8_t *pointer, int32_t offset);
#else
inline uint8_t *
alignDoubleSpillWithOffset(uint8_t *pointer, int32_t offset)
{
    
    return pointer;
}
#endif



class IonCommonFrameLayout
{
    uint8_t *returnAddress_;
    uintptr_t descriptor_;

    static const uintptr_t FrameTypeMask = (1 << FRAMETYPE_BITS) - 1;

  public:
    static size_t offsetOfDescriptor() {
        return offsetof(IonCommonFrameLayout, descriptor_);
    }
    static size_t offsetOfReturnAddress() {
        return offsetof(IonCommonFrameLayout, returnAddress_);
    }
    FrameType prevType() const {
        return FrameType(descriptor_ & FrameTypeMask);
    }
    void changePrevType(FrameType type) {
        descriptor_ &= ~FrameTypeMask;
        descriptor_ |= type;
    }
    size_t prevFrameLocalSize() const {
        return descriptor_ >> FRAMESIZE_SHIFT;
    }
    void setFrameDescriptor(size_t size, FrameType type) {
        descriptor_ = (size << FRAMESIZE_SHIFT) | type;
    }
    uint8_t *returnAddress() const {
        return returnAddress_;
    }
    void setReturnAddress(uint8_t *addr) {
        returnAddress_ = addr;
    }
};

class IonJSFrameLayout : public IonCommonFrameLayout
{
    CalleeToken calleeToken_;
    uintptr_t numActualArgs_;

  public:
    CalleeToken calleeToken() const {
        return calleeToken_;
    }
    void replaceCalleeToken(CalleeToken calleeToken) {
        calleeToken_ = calleeToken;
    }

    static size_t offsetOfCalleeToken() {
        return offsetof(IonJSFrameLayout, calleeToken_);
    }
    static size_t offsetOfNumActualArgs() {
        return offsetof(IonJSFrameLayout, numActualArgs_);
    }
    static size_t offsetOfThis() {
        IonJSFrameLayout *base = nullptr;
        return reinterpret_cast<size_t>(&base->argv()[0]);
    }
    static size_t offsetOfActualArgs() {
        IonJSFrameLayout *base = nullptr;
        
        return reinterpret_cast<size_t>(&base->argv()[1]);
    }
    static size_t offsetOfActualArg(size_t arg) {
        return offsetOfActualArgs() + arg * sizeof(Value);
    }

    Value thisv() {
        return argv()[0];
    }
    Value *argv() {
        return (Value *)(this + 1);
    }
    uintptr_t numActualArgs() const {
        return numActualArgs_;
    }

    
    
    uintptr_t *slotRef(uint32_t slot) {
        return (uintptr_t *)((uint8_t *)this - slot);
    }

    static inline size_t Size() {
        return sizeof(IonJSFrameLayout);
    }
};


class IonEntryFrameLayout : public IonJSFrameLayout
{
  public:
    static inline size_t Size() {
        return sizeof(IonEntryFrameLayout);
    }
};

class IonRectifierFrameLayout : public IonJSFrameLayout
{
  public:
    static inline size_t Size() {
        return sizeof(IonRectifierFrameLayout);
    }
};


class IonUnwoundRectifierFrameLayout : public IonRectifierFrameLayout
{
  public:
    static inline size_t Size() {
        
        
        
        return sizeof(IonUnwoundRectifierFrameLayout);
    }
};


class IonExitFooterFrame
{
    const VMFunction *function_;
    JitCode *jitCode_;

  public:
    static inline size_t Size() {
        return sizeof(IonExitFooterFrame);
    }
    inline JitCode *jitCode() const {
        return jitCode_;
    }
    inline JitCode **addressOfJitCode() {
        return &jitCode_;
    }
    inline const VMFunction *function() const {
        return function_;
    }

    
    template <typename T>
    T *outParam() {
        uint8_t *address = reinterpret_cast<uint8_t *>(this);
        address = alignDoubleSpillWithOffset(address, sizeof(intptr_t));
        return reinterpret_cast<T *>(address - sizeof(T));
    }
};

class IonNativeExitFrameLayout;
class IonOOLNativeExitFrameLayout;
class IonOOLPropertyOpExitFrameLayout;
class IonOOLProxyExitFrameLayout;
class IonDOMExitFrameLayout;


class IonExitFrameLayout : public IonCommonFrameLayout
{
    inline uint8_t *top() {
        return reinterpret_cast<uint8_t *>(this + 1);
    }

  public:
    
    
    static JitCode *BareToken() { return (JitCode *)0xFF; }

    static inline size_t Size() {
        return sizeof(IonExitFrameLayout);
    }
    static inline size_t SizeWithFooter() {
        return Size() + IonExitFooterFrame::Size();
    }

    inline IonExitFooterFrame *footer() {
        uint8_t *sp = reinterpret_cast<uint8_t *>(this);
        return reinterpret_cast<IonExitFooterFrame *>(sp - IonExitFooterFrame::Size());
    }

    
    
    
    inline uint8_t *argBase() {
        JS_ASSERT(footer()->jitCode() != nullptr);
        return top();
    }

    inline bool isWrapperExit() {
        return footer()->function() != nullptr;
    }
    inline bool isBareExit() {
        return footer()->jitCode() == BareToken();
    }

    
    template <typename T> inline bool is() {
        return footer()->jitCode() == T::Token();
    }
    template <typename T> inline T *as() {
        MOZ_ASSERT(this->is<T>());
        return reinterpret_cast<T *>(footer());
    }
};



class IonNativeExitFrameLayout
{
  protected: 
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;
    uintptr_t argc_;

    
    
    uint32_t loCalleeResult_;
    uint32_t hiCalleeResult_;

  public:
    static JitCode *Token() { return (JitCode *)0x0; }

    static inline size_t Size() {
        return sizeof(IonNativeExitFrameLayout);
    }

    static size_t offsetOfResult() {
        return offsetof(IonNativeExitFrameLayout, loCalleeResult_);
    }
    inline Value *vp() {
        return reinterpret_cast<Value*>(&loCalleeResult_);
    }
    inline uintptr_t argc() const {
        return argc_;
    }
};

class IonOOLNativeExitFrameLayout
{
  protected: 
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;

    
    JitCode *stubCode_;

    uintptr_t argc_;

    
    
    uint32_t loCalleeResult_;
    uint32_t hiCalleeResult_;

    
    uint32_t loThis_;
    uint32_t hiThis_;

  public:
    static JitCode *Token() { return (JitCode *)0x4; }

    static inline size_t Size(size_t argc) {
        
        return sizeof(IonOOLNativeExitFrameLayout) + (argc * sizeof(Value));
    }

    static size_t offsetOfResult() {
        return offsetof(IonOOLNativeExitFrameLayout, loCalleeResult_);
    }

    inline JitCode **stubCode() {
        return &stubCode_;
    }
    inline Value *vp() {
        return reinterpret_cast<Value*>(&loCalleeResult_);
    }
    inline Value *thisp() {
        return reinterpret_cast<Value*>(&loThis_);
    }
    inline uintptr_t argc() const {
        return argc_;
    }
};

class IonOOLPropertyOpExitFrameLayout
{
  protected: 
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;

    
    JSObject *obj_;

    
    jsid id_;

    
    
    uint32_t vp0_;
    uint32_t vp1_;

    
    JitCode *stubCode_;

  public:
    static JitCode *Token() { return (JitCode *)0x5; }

    static inline size_t Size() {
        return sizeof(IonOOLPropertyOpExitFrameLayout);
    }

    static size_t offsetOfResult() {
        return offsetof(IonOOLPropertyOpExitFrameLayout, vp0_);
    }

    inline JitCode **stubCode() {
        return &stubCode_;
    }
    inline Value *vp() {
        return reinterpret_cast<Value*>(&vp0_);
    }
    inline jsid *id() {
        return &id_;
    }
    inline JSObject **obj() {
        return &obj_;
    }
};





class IonOOLProxyExitFrameLayout
{
  protected: 
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;

    
    JSObject *proxy_;

    
    JSObject *receiver_;

    
    jsid id_;

    
    
    uint32_t vp0_;
    uint32_t vp1_;

    
    JitCode *stubCode_;

  public:
    static JitCode *Token() { return (JitCode *)0x6; }

    static inline size_t Size() {
        return sizeof(IonOOLProxyExitFrameLayout);
    }

    static size_t offsetOfResult() {
        return offsetof(IonOOLProxyExitFrameLayout, vp0_);
    }

    inline JitCode **stubCode() {
        return &stubCode_;
    }
    inline Value *vp() {
        return reinterpret_cast<Value*>(&vp0_);
    }
    inline jsid *id() {
        return &id_;
    }
    inline JSObject **receiver() {
        return &receiver_;
    }
    inline JSObject **proxy() {
        return &proxy_;
    }
};

class IonDOMExitFrameLayout
{
  protected: 
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;
    JSObject *thisObj;

    
    
    uint32_t loCalleeResult_;
    uint32_t hiCalleeResult_;

  public:
    static JitCode *GetterToken() { return (JitCode *)0x1; }
    static JitCode *SetterToken() { return (JitCode *)0x2; }

    static inline size_t Size() {
        return sizeof(IonDOMExitFrameLayout);
    }

    static size_t offsetOfResult() {
        return offsetof(IonDOMExitFrameLayout, loCalleeResult_);
    }
    inline Value *vp() {
        return reinterpret_cast<Value*>(&loCalleeResult_);
    }
    inline JSObject **thisObjAddress() {
        return &thisObj;
    }
    inline bool isMethodFrame();
};

struct IonDOMMethodExitFrameLayoutTraits;

class IonDOMMethodExitFrameLayout
{
  protected: 
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;
    
    
    JSObject *thisObj_;
    Value *argv_;
    uintptr_t argc_;

    
    
    uint32_t loCalleeResult_;
    uint32_t hiCalleeResult_;

    friend struct IonDOMMethodExitFrameLayoutTraits;

  public:
    static JitCode *Token() { return (JitCode *)0x3; }

    static inline size_t Size() {
        return sizeof(IonDOMMethodExitFrameLayout);
    }

    static size_t offsetOfResult() {
        return offsetof(IonDOMMethodExitFrameLayout, loCalleeResult_);
    }

    inline Value *vp() {
        
        JS_STATIC_ASSERT(offsetof(IonDOMMethodExitFrameLayout, loCalleeResult_) ==
                         (offsetof(IonDOMMethodExitFrameLayout, argc_) + sizeof(uintptr_t)));
        return reinterpret_cast<Value*>(&loCalleeResult_);
    }
    inline JSObject **thisObjAddress() {
        return &thisObj_;
    }
    inline uintptr_t argc() {
        return argc_;
    }
};

inline bool
IonDOMExitFrameLayout::isMethodFrame()
{
    return footer_.jitCode() == IonDOMMethodExitFrameLayout::Token();
}

template <>
inline bool
IonExitFrameLayout::is<IonDOMExitFrameLayout>()
{
    JitCode *code = footer()->jitCode();
    return
        code == IonDOMExitFrameLayout::GetterToken() ||
        code == IonDOMExitFrameLayout::SetterToken() ||
        code == IonDOMMethodExitFrameLayout::Token();
}

template <>
inline IonDOMExitFrameLayout *
IonExitFrameLayout::as<IonDOMExitFrameLayout>()
{
    MOZ_ASSERT(is<IonDOMExitFrameLayout>());
    return reinterpret_cast<IonDOMExitFrameLayout *>(footer());
}

struct IonDOMMethodExitFrameLayoutTraits {
    static const size_t offsetOfArgcFromArgv =
        offsetof(IonDOMMethodExitFrameLayout, argc_) -
        offsetof(IonDOMMethodExitFrameLayout, argv_);
};

class ICStub;

class IonBaselineStubFrameLayout : public IonCommonFrameLayout
{
  public:
    static inline size_t Size() {
        return sizeof(IonBaselineStubFrameLayout);
    }

    static inline int reverseOffsetOfStubPtr() {
        return -int(sizeof(void *));
    }
    static inline int reverseOffsetOfSavedFramePtr() {
        return -int(2 * sizeof(void *));
    }

    inline ICStub *maybeStubPtr() {
        uint8_t *fp = reinterpret_cast<uint8_t *>(this);
        return *reinterpret_cast<ICStub **>(fp + reverseOffsetOfStubPtr());
    }
    inline void setStubPtr(ICStub *stub) {
        uint8_t *fp = reinterpret_cast<uint8_t *>(this);
        *reinterpret_cast<ICStub **>(fp + reverseOffsetOfStubPtr()) = stub;
    }
};


class InvalidationBailoutStack
{
    mozilla::Array<double, FloatRegisters::TotalPhys> fpregs_;
    mozilla::Array<uintptr_t, Registers::Total> regs_;
    IonScript   *ionScript_;
    uint8_t       *osiPointReturnAddress_;

  public:
    uint8_t *sp() const {
        return (uint8_t *) this + sizeof(InvalidationBailoutStack);
    }
    IonJSFrameLayout *fp() const;
    MachineState machine() {
        return MachineState::FromBailout(regs_, fpregs_);
    }

    IonScript *ionScript() const {
        return ionScript_;
    }
    uint8_t *osiPointReturnAddress() const {
        return osiPointReturnAddress_;
    }
    static size_t offsetOfFpRegs() {
        return offsetof(InvalidationBailoutStack, fpregs_);
    }
    static size_t offsetOfRegs() {
        return offsetof(InvalidationBailoutStack, regs_);
    }

    void checkInvariants() const;
};

void
GetPcScript(JSContext *cx, JSScript **scriptRes, jsbytecode **pcRes);

CalleeToken
MarkCalleeToken(JSTracer *trc, CalleeToken token);

} 
} 

#endif 
