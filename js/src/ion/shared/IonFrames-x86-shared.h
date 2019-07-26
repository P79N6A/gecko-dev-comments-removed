






#ifndef js_ion_frame_layouts_x86_h__
#define js_ion_frame_layouts_x86_h__

namespace js {
namespace ion {

class IonCommonFrameLayout
{
  private:
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

class IonJSFrameLayout : public IonCommonFrameLayout
{
    void *calleeToken_;
    uintptr_t numActualArgs_;

  public:
    CalleeToken calleeToken() const {
        return calleeToken_;
    }
    void replaceCalleeToken(void *value) {
        calleeToken_ = value;
    }

    static size_t offsetOfCalleeToken() {
        return offsetof(IonJSFrameLayout, calleeToken_);
    }
    static size_t offsetOfNumActualArgs() {
        return offsetof(IonJSFrameLayout, numActualArgs_);
    }
    static size_t offsetOfActualArgs() {
        IonJSFrameLayout *base = NULL;
        
        return reinterpret_cast<size_t>(&base->argv()[1]);
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


class IonBailedRectifierFrameLayout : public IonRectifierFrameLayout
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

class IonNativeExitFrameLayout;

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
    inline IonNativeExitFrameLayout *nativeExit() {
        
        JS_ASSERT(footer()->ionCode() == NULL);
        return reinterpret_cast<IonNativeExitFrameLayout *>(footer());
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

class IonOsrFrameLayout : public IonJSFrameLayout
{
  public:
    static inline size_t Size() {
        return sizeof(IonOsrFrameLayout);
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

#endif 

