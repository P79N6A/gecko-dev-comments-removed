








































#ifndef jsion_ionframes_arm_h__
#define jsion_ionframes_arm_h__

namespace js {
namespace ion {

class IonFramePrefix;


class IonCommonFrameLayout
{
    void *returnAddress_;
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
    uint8 *returnAddress() const {
        JS_NOT_REACHED("ReturnAddress NYI");
        return NULL;
    }
};

class IonEntryFrameLayout : public IonCommonFrameLayout
{
};

class IonJSFrameLayout : public IonEntryFrameLayout
{
  protected:
    void *calleeToken_;
  public:
    void *calleeToken() const {
        return calleeToken_;
    }
    void setCalleeToken(void *value) {
        calleeToken_ = value;
    }

    static size_t offsetOfCalleeToken() {
        return offsetof(IonJSFrameLayout, calleeToken_);
    }

};
typedef IonJSFrameLayout IonFrameData;

class IonRectifierFrameLayout : public IonJSFrameLayout
{
};


class IonExitFrameLayout : public IonCommonFrameLayout
{
};

} 
} 
#endif 
