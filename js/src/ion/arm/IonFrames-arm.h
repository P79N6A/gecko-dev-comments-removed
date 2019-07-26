






#ifndef jsion_ionframes_arm_h__
#define jsion_ionframes_arm_h__

#include "ion/shared/IonFrames-shared.h"

namespace js {
namespace ion {

class IonFramePrefix;


class IonCommonFrameLayout
{
    uint8 *returnAddress_;
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
    uint8 *returnAddress() const {
        return returnAddress_;
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

    static size_t offsetOfCalleeToken() {
        return offsetof(IonJSFrameLayout, calleeToken_);
    }
    static size_t offsetOfNumActualArgs() {
        return offsetof(IonJSFrameLayout, numActualArgs_);
    }

    void replaceCalleeToken(void *calleeToken) {
        calleeToken_ = calleeToken;
    }
    static size_t offsetOfActualArgs() {
        IonJSFrameLayout *base = NULL;
        
        return reinterpret_cast<size_t>(&base->argv()[1]);
    }
    static size_t offsetOfActualArg(size_t arg) {
        return offsetOfActualArgs() + arg * sizeof(Value);
    }

    Value *argv() {
        return (Value *)(this + 1);
    }
    uintptr_t numActualArgs() const {
        return numActualArgs_;
    }

    
    
    uintptr_t *slotRef(uint32 slot) {
        return (uintptr_t *)((uint8 *)this - (slot * STACK_SLOT_SIZE));
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

class IonBailedRectifierFrameLayout : public IonJSFrameLayout
{
  public:
    static inline size_t Size() {
        
        
        
        return sizeof(IonBailedRectifierFrameLayout);
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

    
    Value *outVp() {
        return reinterpret_cast<Value *>(reinterpret_cast<char *>(this) - sizeof(Value));
    }
};

class IonOsrFrameLayout : public IonJSFrameLayout
{
  public:
    static inline size_t Size() {
        return sizeof(IonOsrFrameLayout);
    }
};

class IonBaselineStubFrameLayout : public IonCommonFrameLayout
{
  public:
    static inline size_t Size() {
        return sizeof(IonBaselineStubFrameLayout);
    }
};

class IonNativeExitFrameLayout;
class IonOOLNativeGetterExitFrameLayout;
class IonOOLPropertyOpExitFrameLayout;
class IonDOMExitFrameLayout;


class IonExitFrameLayout : public IonCommonFrameLayout
{
    inline uint8 *top() {
        return reinterpret_cast<uint8 *>(this + 1);
    }

  public:
    static inline size_t Size() {
        return sizeof(IonExitFrameLayout);
    }
    static inline size_t SizeWithFooter() {
        return Size() + IonExitFooterFrame::Size();
    }

    inline IonExitFooterFrame *footer() {
        uint8 *sp = reinterpret_cast<uint8 *>(this);
        return reinterpret_cast<IonExitFooterFrame *>(sp - IonExitFooterFrame::Size());
    }

    
    
    
    inline uint8 *argBase() {
        JS_ASSERT(footer()->ionCode() != NULL);
        return top();
    }

    inline bool isWrapperExit() {
        return footer()->function() != NULL;
    }
    inline bool isNativeExit() {
        return footer()->ionCode() == NULL;
    }
    inline bool isOOLNativeGetterExit() {
        return footer()->ionCode() == ION_FRAME_OOL_NATIVE_GETTER;
    }
    inline bool isOOLPropertyOpExit() {
        return footer()->ionCode() == ION_FRAME_OOL_PROPERTY_OP;
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
    inline IonOOLNativeGetterExitFrameLayout *oolNativeGetterExit() {
        JS_ASSERT(isOOLNativeGetterExit());
        return reinterpret_cast<IonOOLNativeGetterExitFrameLayout *>(footer());
    }
    inline IonOOLPropertyOpExitFrameLayout *oolPropertyOpExit() {
        JS_ASSERT(isOOLPropertyOpExit());
        return reinterpret_cast<IonOOLPropertyOpExitFrameLayout *>(footer());
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

class IonOOLNativeGetterExitFrameLayout
{
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;

    
    
    uint32_t loCalleeResult_;
    uint32_t hiCalleeResult_;

    
    uint32_t loThis_;
    uint32_t hiThis_;

    
    IonCode *stubCode_;

  public:
    static inline size_t Size() {
        return sizeof(IonOOLNativeGetterExitFrameLayout);
    }

    static size_t offsetOfResult() {
        return offsetof(IonOOLNativeGetterExitFrameLayout, loCalleeResult_);
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
        return 0;
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
    inline bool isSetterFrame() {
        return footer_.ionCode() == ION_FRAME_DOMSETTER;
    }
    inline bool isMethodFrame() {
        return footer_.ionCode() == ION_FRAME_DOMMETHOD;
    }
};

class IonDOMMethodExitFrameLayout
{
    IonExitFooterFrame footer_;
    IonExitFrameLayout exit_;
    
    
    JSObject *thisObj_;
    uintptr_t argc_;

    Value CalleeResult_;

  public:
    static inline size_t Size() {
        return sizeof(IonDOMMethodExitFrameLayout);
    }

    static size_t offsetOfResult() {
        return offsetof(IonDOMMethodExitFrameLayout, CalleeResult_);
    }
    inline Value *vp() {
        JS_STATIC_ASSERT(offsetof(IonDOMMethodExitFrameLayout, CalleeResult_) ==
                         (offsetof(IonDOMMethodExitFrameLayout, argc_) + sizeof(uintptr_t)));
        return &CalleeResult_;
    }
    inline JSObject **thisObjAddress() {
        return &thisObj_;
    }
    inline uintptr_t argc() {
        return argc_;
    }
};


class InvalidationBailoutStack
{
    double      fpregs_[FloatRegisters::Total];
    uintptr_t   regs_[Registers::Total];
    IonScript   *ionScript_;
    uint8       *osiPointReturnAddress_;

  public:
    uint8 *sp() const {
        return (uint8 *) this + sizeof(InvalidationBailoutStack);
    }
    IonJSFrameLayout *fp() const;
    MachineState machine() {
        return MachineState::FromBailout(regs_, fpregs_);
    }

    IonScript *ionScript() const {
        return ionScript_;
    }
    uint8 *osiPointReturnAddress() const {
        return osiPointReturnAddress_;
    }

    void checkInvariants() const;
};

} 
} 









class BaselineFrame
{
    
    
    uint32_t loScratchValue;
    uint32_t hiScratchValue;
    size_t frameSize;

  public:
    
    
    static const uint32_t FramePointerOffset = sizeof(void *);

    static inline size_t offsetOfArg(size_t index) {
        return FramePointerOffset + js::ion::IonJSFrameLayout::offsetOfActualArg(index);
    }
    static size_t Size() {
        return sizeof(BaselineFrame);
    }

    
    
    
    static inline size_t reverseOffsetOfFrameSize() {
        return -(sizeof(BaselineFrame) - offsetof(BaselineFrame, frameSize));
    }
    static inline size_t reverseOffsetOfScratchValue() {
        return -(sizeof(BaselineFrame) - offsetof(BaselineFrame, loScratchValue));
    }
    static inline size_t reverseOffsetOfLocal(size_t index) {
        return -(sizeof(BaselineFrame) + index * sizeof(js::Value)) - sizeof(js::Value);
    }
};


JS_STATIC_ASSERT(((sizeof(BaselineFrame) + BaselineFrame::FramePointerOffset) % 8) == 0);

#endif 
