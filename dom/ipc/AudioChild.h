






































#ifndef mozilla_dom_AudioChild_h
#define mozilla_dom_AudioChild_h

#include "mozilla/dom/PAudioChild.h"

namespace mozilla {
namespace dom {

class AudioChild : public PAudioChild
{
 public:
    nsrefcnt AddRef();
    nsrefcnt Release();

    AudioChild();
    virtual ~AudioChild();
    virtual bool RecvSampleOffsetUpdate(const PRInt64&, const PRInt64&);
    virtual void ActorDestroy(ActorDestroyReason);
    
    PRInt64 GetLastKnownSampleOffset();
    PRInt64 GetLastKnownSampleOffsetTime();

    PRBool IsIPCOpen() { return mIPCOpen; };
 private:
    nsAutoRefCnt mRefCnt;
    NS_DECL_OWNINGTHREAD
    PRInt64 mLastSampleOffset, mLastSampleOffsetTime;
    PRPackedBool mIPCOpen;
};

} 
} 

#endif
