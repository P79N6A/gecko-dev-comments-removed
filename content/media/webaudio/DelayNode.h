





#ifndef DelayNode_h_
#define DelayNode_h_

#include "AudioNode.h"
#include "AudioParam.h"
#include "PlayingRefChanged.h"

namespace mozilla {
namespace dom {

class AudioContext;

class DelayNode : public AudioNode
{
public:
  DelayNode(AudioContext* aContext, double aMaxDelay);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DelayNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  AudioParam* DelayTime() const
  {
    return mDelay;
  }

private:
  static void SendDelayToStream(AudioNode* aNode);
  friend class DelayNodeEngine;
  friend class PlayingRefChanged<DelayNode>;

private:
  nsRefPtr<AudioParam> mDelay;
  SelfReference<DelayNode> mPlayingRef;
};

}
}

#endif

