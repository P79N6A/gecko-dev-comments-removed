





#pragma once

#include "AudioNode.h"

namespace mozilla {
namespace dom {

class AudioContext;

class AudioDestinationNode : public AudioNode
{
public:
  explicit AudioDestinationNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope,
                               bool* aTriedToWrap);

  virtual uint32_t MaxNumberOfInputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 1;
  }
  virtual uint32_t MaxNumberOfOutputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 0;
  }

};

}
}

