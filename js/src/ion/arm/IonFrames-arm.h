








































#ifndef jsion_ionframes_arm_h__
#define jsion_ionframes_arm_h__

namespace js {
namespace ion {


class IonFrameData
{
  protected:
    void *returnAddress_;
    uintptr_t sizeDescriptor_;
    void *calleeToken_;
    void *padding; 
};


struct IonCFrame
{
    uintptr_t frameSize;
    uintptr_t snapshotOffset;
};

} 
} 
#endif 
