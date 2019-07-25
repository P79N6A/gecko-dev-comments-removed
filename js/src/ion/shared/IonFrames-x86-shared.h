







































#ifndef js_ion_frame_layouts_x86_h__
#define js_ion_frame_layouts_x86_h__

namespace js {
namespace ion {

class IonCommonFrameLayout
{
  private:
    uint8 *returnAddress_;
    uintptr_t descriptor_;

  public:
    static size_t offsetOfDescriptor() {
        return offsetof(IonCommonFrameLayout, descriptor_);
    }
    static size_t offsetOfReturnAddress() {
        return offsetof(IonCommonFrameLayout, returnAddress_);
    }
    FrameType prevType() const {
        return FrameType(descriptor_ & ((1 << FRAMETYPE_BITS) - 1));
    }
    size_t prevFrameLocalSize() const {
        return descriptor_ >> FRAMETYPE_BITS;
    }
    void setFrameDescriptor(size_t size, FrameType type) {
        descriptor_ = (size << FRAMETYPE_BITS) | type;
    }
    uint8 *returnAddress() const {
        return returnAddress_;
    }
};

class IonJSFrameLayout : public IonCommonFrameLayout
{
  private:
    void *calleeToken_;

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

    Value *argv() {
        return (Value *)(this + 1);
    }

    
    
    uintptr_t *slotRef(uint32 slot) {
        return (uintptr_t *)((uint8 *)this - (slot * STACK_SLOT_SIZE));
    }
};

class IonEntryFrameLayout : public IonJSFrameLayout
{
};

class IonRectifierFrameLayout : public IonJSFrameLayout
{
};

class IonExitFrameLayout : public IonCommonFrameLayout
{
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
        return MachineState(regs_, fpregs_);
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

