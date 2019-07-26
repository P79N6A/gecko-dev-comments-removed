





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
    virtual bool RecvPositionInFramesUpdate(const PRInt64&, const PRInt64&);
    virtual bool RecvDrainDone();
    virtual PRInt32 WaitForMinWriteSize();
    virtual bool RecvMinWriteSizeDone(const PRInt32& frameCount);
    virtual void WaitForDrain();
    virtual bool RecvWriteDone();
    virtual void WaitForWrite();
    virtual void ActorDestroy(ActorDestroyReason);

    PRInt64 GetLastKnownPosition();
    PRInt64 GetLastKnownPositionTimestamp();

    bool IsIPCOpen() { return mIPCOpen; };
 private:
    nsAutoRefCnt mRefCnt;
    NS_DECL_OWNINGTHREAD
    PRInt64 mLastPosition;
    PRInt64 mLastPositionTimestamp;
    PRUint64 mWriteCounter;
    PRInt32 mMinWriteSize;
    mozilla::ReentrantMonitor mAudioReentrantMonitor;
    bool mIPCOpen;
    bool mDrained;
};

} 
} 

#endif
