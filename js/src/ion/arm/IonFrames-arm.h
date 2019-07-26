








































#ifndef jsion_ionframes_arm_h__
#define jsion_ionframes_arm_h__

namespace js {
namespace ion {

class IonFramePrefix;


class IonCommonFrameLayout
{
    uint8 *returnAddress_;
    void *padding;
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

  public:
    void *calleeToken() const {
        return calleeToken_;
    }

    static size_t offsetOfCalleeToken() {
        return offsetof(IonJSFrameLayout, calleeToken_);
    }
    void replaceCalleeToken(void *calleeToken) {
        calleeToken_ = calleeToken;
    }

    Value *argv() {
        return (Value *)(this + 1);
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


class IonExitFrameLayout : public IonCommonFrameLayout
{
    void *padding2;

  public:
    static inline size_t Size() {
        return sizeof(IonExitFrameLayout);
    }
    static inline size_t SizeWithFooter() {
        return Size() + sizeof(IonCode *);
    }
    inline IonCode ** ionCodePointer() {
        return ((IonCode**)this)-1;
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
