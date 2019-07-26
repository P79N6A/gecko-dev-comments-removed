





#ifndef PlayingRefChangeHandler_h__
#define PlayingRefChangeHandler_h__

#include "nsThreadUtils.h"
#include "AudioNodeStream.h"

namespace mozilla {
namespace dom {

template<class NodeType>
class PlayingRefChangeHandler : public nsRunnable
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
    nsRefPtr<NodeType> node;
    {
      
      
      
      
      MutexAutoLock lock(mStream->Engine()->NodeMutex());
      node = static_cast<NodeType*>(mStream->Engine()->Node());
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

