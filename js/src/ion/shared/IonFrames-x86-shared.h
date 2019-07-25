







































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
    uint8 **returnAddressPtr() {
        return &returnAddress_;
    }
};

class IonJSFrameLayout : public IonCommonFrameLayout
{
  private:
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

class IonEntryFrameLayout : public IonJSFrameLayout
{
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

