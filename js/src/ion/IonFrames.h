








































#ifndef jsion_frames_h__
#define jsion_frames_h__

namespace js {
namespace ion {



















class FrameSizeClass
{
    uint32 class_;

    explicit FrameSizeClass(uint32 class_) : class_(class_)
    { }
  
  public:
    FrameSizeClass()
    { }

    static FrameSizeClass FromDepth(uint32 frameDepth);

    uint32 frameSize() const;

    uint32 classId() const {
        return class_;
    }
};

}
}

#endif 

