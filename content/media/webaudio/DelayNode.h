





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

  AudioParam* DelayTime() const
  {
    return mDelay;
  }

private:
  nsRefPtr<AudioParam> mDelay;
};

}
}

#endif

