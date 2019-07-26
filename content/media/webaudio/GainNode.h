





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

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope,
                               bool* aTriedToWrap);

  virtual uint32_t MaxNumberOfInputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 1;
  }
  virtual uint32_t MaxNumberOfOutputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 1;
  }

  AudioParam* Gain() const
  {
    return mGain;
  }

private:
  nsRefPtr<AudioParam> mGain;
};

}
}

#endif

