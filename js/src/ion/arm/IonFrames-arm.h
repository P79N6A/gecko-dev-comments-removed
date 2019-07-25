








































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
        
        return sizeof(IonBailedRectifierFrameLayout) + sizeof(void *);
    }
};


class IonExitFrameLayout : public IonCommonFrameLayout
{
    void *padding2;

  public:
    static inline size_t Size() {
        return sizeof(IonExitFrameLayout);
    }
};

} 
} 

#endif 
