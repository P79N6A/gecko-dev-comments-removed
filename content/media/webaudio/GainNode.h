





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
  virtual ~GainNode();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(GainNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  AudioParam* Gain() const
  {
    return mGain;
  }

  virtual bool SupportsMediaStreams() const MOZ_OVERRIDE
  {
    return true;
  }

private:
  static void SendGainToStream(AudioNode* aNode);

private:
  nsRefPtr<AudioParam> mGain;
};

}
}

#endif

