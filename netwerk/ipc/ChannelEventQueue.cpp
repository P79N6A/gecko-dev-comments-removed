






#include "nsISupports.h"
#include "mozilla/net/ChannelEventQueue.h"

namespace mozilla {
namespace net {

void
ChannelEventQueue::FlushQueue()
{
  
  
  
  nsCOMPtr<nsISupports> kungFuDeathGrip(mOwner);

  
  mFlushing = true;

  uint32_t i;
  for (i = 0; i < mEventQueue.Length(); i++) {
    mEventQueue[i]->Run();
    if (mSuspended)
      break;
  }

  
  if (i < mEventQueue.Length())
    i++;

  
  
  
  mEventQueue.RemoveElementsAt(0, i);

  mFlushing = false;
}


}
}
