





#ifndef PlayingRefChangeHandler_h__
#define PlayingRefChangeHandler_h__

#include "nsThreadUtils.h"
#include "AudioNodeStream.h"

namespace mozilla {
namespace dom {

class PlayingRefChangeHandler final : public nsRunnable
{
public:
  enum ChangeType { ADDREF, RELEASE };
  PlayingRefChangeHandler(AudioNodeStream* aStream, ChangeType aChange)
    : mStream(aStream)
    , mChange(aChange)
  {
  }

  NS_IMETHOD Run()
  {
    nsRefPtr<AudioNode> node;
    {
      
      
      
      
      MutexAutoLock lock(mStream->Engine()->NodeMutex());
      node = mStream->Engine()->Node();
    }
    if (node) {
      if (mChange == ADDREF) {
        node->MarkActive();
      } else if (mChange == RELEASE) {
        node->MarkInactive();
      }
    }
    return NS_OK;
  }

private:
  nsRefPtr<AudioNodeStream> mStream;
  ChangeType mChange;
};

}
}

#endif

