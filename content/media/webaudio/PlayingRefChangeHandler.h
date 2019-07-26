





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
    : mLastProcessedGraphUpdateIndex(aStream->GetProcessingGraphUpdateIndex())
    , mStream(aStream)
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
      } else if (mChange == RELEASE &&
                 node->AcceptPlayingRefRelease(mLastProcessedGraphUpdateIndex)) {
        node->MarkInactive();
      }
    }
    return NS_OK;
  }

private:
  int64_t mLastProcessedGraphUpdateIndex;
  nsRefPtr<AudioNodeStream> mStream;
  ChangeType mChange;
};

}
}

#endif

