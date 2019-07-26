





#ifndef jit_arm_IonFrames_arm_h
#define jit_arm_IonFrames_arm_h

#include "jit/shared/IonFrames-shared.h"

namespace js {
namespace jit {

class IonFramePrefix;


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


class IonEntryFrameLayout : public IonCommonFrameLayout
{
  public:
    static inline size_t Size() {
        return sizeof(IonEntryFrameLayout);
    }
};

class IonJSFrameLayout : public IonEntryFrameLayout
{
  protected:
    void *calleeToken_;
    uintptr_t numActualArgs_;

  public:
    void *calleeToken() const {
        return calleeToken_;
    }
    void replaceCalleeToken(void *calleeToken) {
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
        return (uintptr_t *)((uint8_t *)this - (slot * STACK_SLOT_SIZE));
    }

    static inline size_t Size() {
        return sizeof(IonJSFrameLayout);
    }
};

class IonRectifierFrameLayout : public IonJSFrameLayout
{
  public:
    static inline size_t Size() {
        return sizeof(IonRectifierFrameLayout);
    }
};

class IonUnwoundRectifierFrameLayout : public IonJSFrameLayout
{
  public:
    static inline size_t Size() {
        
        
        
        return sizeof(IonUnwoundRectifierFrameLayout);
    }
};


class IonExitFooterFrame
{
    const VMFunction *function_;
    IonCode *ionCode_;

  public:
    static inline size_t Size() {
        return sizeof(IonExitFooterFrame);
    }
    inline IonCode *ionCode() const {
        return ionCode_;
    }
    inline IonCode **addressOfIonCode() {
        return &ionCode_;
    }
    inline const VMFunction *function() const {
        return function_;
    }

    
    template <typename T>
    T *outParam() {
        return reinterpret_cast<T *>(reinterpret_cast<char *>(this) - sizeof(T));
    }
};

class IonOsrFrameLayout : public IonJSFrameLayout
{
  public:
    static inline size_t Size() {
        return sizeof(IonOsrFrameLayout);
    }
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
        JS_ASSERT(footer()->ionCode() != nullptr);
        return top();
    }

    inline bool isWrapperExit() {
        return footer()->function() != nullptr;
    }
    inline bool isNativeExit() {
        return footer()->ionCode() == nullptr;
    }
    inline bool isOOLNativeExit() {
        return footer()->ionCode() == ION_FRAME_OOL_NATIVE;
    }
    inline bool isOOLPropertyOpExit() {
        return footer()->ionCode() == ION_FRAME_OOL_PROPERTY_OP;
    }
    inline bool isOOLProxyExit() {
        return footer()->ionCode() == ION_FRAME_OOL_PROXY;
    }
    inline bool isDomExit() {
        IonCode *code = footer()->ionCode();
        return
            code == ION_FRAME_DOMGETTER ||
            code == ION_FRAME_DOMSETTER ||
            code == ION_FRAME_DOMMETHOD;
    }

    inline IonNativeExitFrameLayout *nativeExit() {
        
        JS_ASSERT(isNativeExit());
        return reinterpret_cast<IonNativeExitFrameLayout *>(footer());
    }
    inline IonOOLNativeExitFrameLayout *oolNativeExit() {
        JS_ASSERT(isOOLNativeExit());
        return reinterpret_cast<IonOOLNativeExitFrameLayout *>(footer());
    }
    inline IonOOLPropertyOpExitFrameLayout *oolPropertyOpExit() {
        JS_ASSERT(isOOLPropertyOpExit());
        return reinterpret_cast<IonOOLPropertyOpExitFrameLayout *>(footer());
    }
    inline IonOOLProxyExitFrameLayout *oolProxyExit() {
        JS_ASSERT(isOOLProxyExit());
        return reinterpret_cast<IonOOLProxyExitFrameLayout *>(footer());
    }
    inline IonDOMExitFrameLayout *DOMExit() {
        JS_ASSERT(isDomExit());
        return reinterpret_cast<IonDOMExitFrameLayout *>(footer());
    }
};



class IonNativeExitFrameLayout
{
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;
    uintptr_t argc_;

    
    
    uint32_t loCalleeResult_;
    uint32_t hiCalleeResult_;

  public:
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
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;

    
    IonCode *stubCode_;

    uintptr_t argc_;

    
    
    uint32_t loCalleeResult_;
    uint32_t hiCalleeResult_;

    
    uint32_t loThis_;
    uint32_t hiThis_;

  public:
    static inline size_t Size(size_t argc) {
        
        return sizeof(IonOOLNativeExitFrameLayout) + (argc * sizeof(Value));
    }

    static size_t offsetOfResult() {
        return offsetof(IonOOLNativeExitFrameLayout, loCalleeResult_);
    }

    inline IonCode **stubCode() {
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
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;

    
    JSObject *obj_;

    
    jsid id_;

    
    
    uint32_t vp0_;
    uint32_t vp1_;

    
    IonCode *stubCode_;

  public:
    static inline size_t Size() {
        return sizeof(IonOOLPropertyOpExitFrameLayout);
    }

    static size_t offsetOfResult() {
        return offsetof(IonOOLPropertyOpExitFrameLayout, vp0_);
    }

    inline IonCode **stubCode() {
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

    
    IonCode *stubCode_;

  public:
    static inline size_t Size() {
        return sizeof(IonOOLProxyExitFrameLayout);
    }

    static size_t offsetOfResult() {
        return offsetof(IonOOLProxyExitFrameLayout, vp0_);
    }

    inline IonCode **stubCode() {
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
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;
    JSObject *thisObj;

    
    
    uint32_t loCalleeResult_;
    uint32_t hiCalleeResult_;

  public:
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
    inline bool isMethodFrame() {
        return footer_.ionCode() == ION_FRAME_DOMMETHOD;
    }
};

struct IonDOMMethodExitFrameLayoutTraits;

class IonDOMMethodExitFrameLayout
{
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;
    
    
    JSObject *thisObj_;
    Value *argv_;
    uintptr_t argc_;

    
    
    uint32_t loCalleeResult_;
    uint32_t hiCalleeResult_;

    friend struct IonDOMMethodExitFrameLayoutTraits;

  public:
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

struct IonDOMMethodExitFrameLayoutTraits {
    static const size_t offsetOfArgcFromArgv =
        offsetof(IonDOMMethodExitFrameLayout, argc_) -
        offsetof(IonDOMMethodExitFrameLayout, argv_);
};


class InvalidationBailoutStack
{
    double      fpregs_[FloatRegisters::Total];
    uintptr_t   regs_[Registers::Total];
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

    void checkInvariants() const;
};

} 
} 

#endif 
