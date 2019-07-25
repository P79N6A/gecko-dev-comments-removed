





#ifndef mozilla_dom_AudioChild_h
#define mozilla_dom_AudioChild_h

#include "mozilla/dom/PAudioChild.h"
#include "mozilla/ReentrantMonitor.h"

namespace mozilla {
namespace dom {

class AudioChild : public PAudioChild
{
 public:
    NS_IMETHOD_(nsrefcnt) AddRef();
    NS_IMETHOD_(nsrefcnt) Release();

    AudioChild();
    virtual ~AudioChild();
    virtual bool RecvPositionInFramesUpdate(const int64_t&, const int64_t&);
    virtual bool RecvDrainDone();
    virtual int32_t WaitForMinWriteSize();
    virtual bool RecvMinWriteSizeDone(const int32_t& frameCount);
    virtual void WaitForDrain();
    virtual bool RecvWriteDone();
    virtual void WaitForWrite();
    virtual void ActorDestroy(ActorDestroyReason);

    int64_t GetLastKnownPosition();
    int64_t GetLastKnownPositionTimestamp();

    bool IsIPCOpen() { return mIPCOpen; };
 private:
    nsAutoRefCnt mRefCnt;
    NS_DECL_OWNINGTHREAD
    int64_t mLastPosition;
    int64_t mLastPositionTimestamp;
    uint64_t mWriteCounter;
    int32_t mMinWriteSize;
    mozilla::ReentrantMonitor mAudioReentrantMonitor;
    bool mIPCOpen;
    bool mDrained;
};

} 
} 

#endif
