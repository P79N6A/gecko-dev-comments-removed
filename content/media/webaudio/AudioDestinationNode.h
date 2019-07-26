





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

};

}
}

