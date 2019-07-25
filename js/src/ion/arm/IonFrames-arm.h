








































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
    void setFrameDescriptor(size_t size, FrameType type) {
        descriptor_ = (size << FRAMETYPE_BITS) | type;
    }
    uint8 *returnAddress() const {
        JS_NOT_REACHED("ReturnAddress NYI");
        return NULL;
    }
    uint8 **returnAddressPtr() {
        JS_NOT_REACHED("NYI");
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
    void replaceCalleeToken(void *value) {
        calleeToken_ = value;
    }
    void setInvalidationRecord(InvalidationRecord *record) {
        replaceCalleeToken(InvalidationRecordToToken(record));
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
