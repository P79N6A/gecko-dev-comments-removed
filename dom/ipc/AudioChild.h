






































#ifndef mozilla_dom_AudioChild_h
#define mozilla_dom_AudioChild_h

#include "mozilla/dom/PAudioChild.h"

namespace mozilla {
namespace dom {

class AudioChild : public PAudioChild
{
 public:
    AudioChild();
    virtual ~AudioChild();
    virtual bool RecvSampleOffsetUpdate(const PRInt64&, const PRInt64&);
    
    PRInt64 GetLastKnownSampleOffset();
    PRInt64 GetLastKnownSampleOffsetTime();
 private:
    PRInt64 mLastSampleOffset, mLastSampleOffsetTime;
};

} 
} 

#endif
