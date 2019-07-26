





#ifndef GainNode_h_
#define GainNode_h_

#include "AudioNode.h"
#include "AudioParam.h"

namespace mozilla {
namespace dom {

class AudioContext;

class GainNode : public AudioNode
{
public:
  explicit GainNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(GainNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  AudioParam* Gain() const
  {
    return mGain;
  }

private:
  static void SendGainToStream(AudioNode* aNode);

private:
  nsRefPtr<AudioParam> mGain;
};

}
}

#endif

