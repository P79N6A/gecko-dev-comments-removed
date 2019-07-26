





#ifndef PlayingRefChanged_h__
#define PlayingRefChanged_h__

#include "nsThreadUtils.h"
#include "AudioNodeStream.h"

namespace mozilla {
namespace dom {

template<class NodeType>
class PlayingRefChanged : public nsRunnable
{
public:
  enum ChangeType { ADDREF, RELEASE };
  PlayingRefChanged(AudioNodeStream* aStream, ChangeType aChange)
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
        node->mPlayingRef.Take(node);
      } else if (mChange == RELEASE) {
        node->mPlayingRef.Drop(node);
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

