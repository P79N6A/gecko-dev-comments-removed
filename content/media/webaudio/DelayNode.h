





#ifndef DelayNode_h_
#define DelayNode_h_

#include "AudioNode.h"
#include "AudioParam.h"

namespace mozilla {
namespace dom {

class AudioContext;

class DelayNode : public AudioNode
{
public:
  DelayNode(AudioContext* aContext, double aMaxDelay);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DelayNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  AudioParam* DelayTime() const
  {
    return mDelay;
  }

  virtual bool SupportsMediaStreams() const MOZ_OVERRIDE
  {
    return true;
  }

private:
  static void SendDelayToStream(AudioNode* aNode);
  friend class DelayNodeEngine;

private:
  nsRefPtr<AudioParam> mDelay;
  SelfReference<DelayNode> mPlayingRef;
};

}
}

#endif

