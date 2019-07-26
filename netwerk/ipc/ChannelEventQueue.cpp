






#include "nsISupports.h"
#include "mozilla/net/ChannelEventQueue.h"
#include "nsThreadUtils.h"

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

void
ChannelEventQueue::Resume()
{
  
  MOZ_ASSERT(mSuspendCount > 0);
  if (mSuspendCount <= 0) {
    return;
  }

  if (!--mSuspendCount) {
    nsRefPtr<nsRunnableMethod<ChannelEventQueue> > event =
      NS_NewRunnableMethod(this, &ChannelEventQueue::CompleteResume);
    NS_DispatchToCurrentThread(event);
  }
}

}
}
