








































#ifndef jsion_frames_h__
#define jsion_frames_h__

namespace js {
namespace ion {






















static const uint32 NO_FRAME_SIZE_CLASS_ID = uint32(-1);

class FrameSizeClass
{
    uint32 class_;

    explicit FrameSizeClass(uint32 class_) : class_(class_)
    { }
  
  public:
    FrameSizeClass()
    { }

    static FrameSizeClass FromDepth(uint32 frameDepth);
    static FrameSizeClass None() {
        return FrameSizeClass(NO_FRAME_SIZE_CLASS_ID);
    }

    uint32 frameSize() const;

    uint32 classId() const {
        return class_;
    }

    bool operator ==(const FrameSizeClass &other) const {
        return class_ == other.class_;
    }
    bool operator !=(const FrameSizeClass &other) const {
        return class_ != other.class_;
    }
};

}
}

#endif 

