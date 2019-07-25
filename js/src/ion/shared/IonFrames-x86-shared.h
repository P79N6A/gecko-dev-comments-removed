







































#ifndef js_ion_frame_layouts_x86_h__
#define js_ion_frame_layouts_x86_h__

namespace js {
namespace ion {

class IonFramePrefix;

class IonCommonFrameLayout
{
  private:
    void *returnAddress_;
    uintptr_t descriptor_;

  public:
    static size_t offsetOfDescriptor() {
        return offsetof(IonCommonFrameLayout, descriptor_);
    }
    FrameType prevType() const {
        return FrameType(descriptor_ & ((1 << FRAMETYPE_BITS) - 1));
    }
    size_t prevFrameLocalSize() const {
        return descriptor_ >> FRAMETYPE_BITS;
    }
};

class IonEntryFrameLayout : public IonCommonFrameLayout
{
  private:
};

class IonJSFrameLayout : public IonCommonFrameLayout
{
  private:
    void *calleeToken_;

  public:
    void *calleeToken() const {
        return calleeToken_;
    }

    static size_t offsetOfCalleeToken() {
        return offsetof(IonJSFrameLayout, calleeToken_);
    }
};

class IonRectifierFrameLayout : public IonJSFrameLayout
{
};

class IonExitFrameLayout : public IonCommonFrameLayout
{
};


struct IonCFrame
{
    IonFramePrefix *topFrame;
    void *returnAddress;
    uintptr_t frameSize;
    uintptr_t snapshotOffset;
};

}
}

#endif

