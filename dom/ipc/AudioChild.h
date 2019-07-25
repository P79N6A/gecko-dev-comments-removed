






































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
    virtual bool RecvSampleOffsetUpdate(const PRInt64&, const PRInt64&);
    virtual bool RecvDrainDone();
    virtual void WaitForDrain();
    virtual void ActorDestroy(ActorDestroyReason);
    
    PRInt64 GetLastKnownSampleOffset();
    PRInt64 GetLastKnownSampleOffsetTime();

    PRBool IsIPCOpen() { return mIPCOpen; };
 private:
    nsAutoRefCnt mRefCnt;
    NS_DECL_OWNINGTHREAD
    PRInt64 mLastSampleOffset, mLastSampleOffsetTime;
    mozilla::ReentrantMonitor mAudioReentrantMonitor;
    PRPackedBool mIPCOpen;
    PRPackedBool mDrained;
};

} 
} 

#endif
