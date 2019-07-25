








































namespace js {
namespace ion {

class IonFramePrefix;



struct IonFrameData
{
    void *returnAddress_;
    uintptr_t sizeDescriptor_;
    void *calleeToken_;
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
