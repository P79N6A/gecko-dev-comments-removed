








































#ifndef jsion_ionframes_arm_h__
#define jsion_ionframes_arm_h__

namespace js {
namespace ion {

class IonFramePrefix;


class IonCommonFrameLayout
{
# if 0
    
    void *returnAddress_;
#endif
    uintptr_t descriptor_;
  public:
    static size_t offsetOfDescriptor() {
        return offsetof(IonCommonFrameLayout, descriptor_);
    }
#if 0
    static size_t offsetOfReturnAddress() {
        return offsetof(IonCommonFrameLayout, returnAddress_);
    }
#endif
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
  private:
};

class IonJSFrameLayout : IonCommonFrameLayout
{
  protected:
    void *calleeToken_;
    void *padding; 
  public:
    void *calleeToken() const {
        return calleeToken_;
    }

    static size_t offsetOfCalleeToken() {
        return offsetof(IonJSFrameLayout, calleeToken_);
    }

};
typedef IonJSFrameLayout IonFrameData;




struct IonCFrame
{
    uintptr_t frameSize;
    uintptr_t snapshotOffset;
};


class IonRectifierFrameLayout : public IonJSFrameLayout
{
};


class IonExitFrameLayout : public IonCommonFrameLayout
{
};

} 
} 
#endif
