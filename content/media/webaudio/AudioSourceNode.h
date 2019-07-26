





#pragma once

#include "AudioNode.h"

namespace mozilla {
namespace dom {

class AudioSourceNode : public AudioNode
{
public:
  explicit AudioSourceNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED

};

}
}

