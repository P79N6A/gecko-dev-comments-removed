








































#include "nsIChannel.h"
#include "mozilla/net/ChannelEventQueue.h"

namespace mozilla {
namespace net {

void
ChannelEventQueue::FlushQueue()
{
  
  
  
  nsCOMPtr<nsIChannel> kungFuDeathGrip(mOwner);

  
  mFlushing = true;

  PRUint32 i;
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
