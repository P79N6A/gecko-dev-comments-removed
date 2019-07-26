





#pragma once

#include "AudioNode.h"

namespace mozilla {
namespace dom {

class AudioSourceNode : public AudioNode
{
public:
  explicit AudioSourceNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED

  virtual uint32_t MaxNumberOfInputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 0;
  }
  virtual uint32_t MaxNumberOfOutputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 1;
  }

};

}
}

