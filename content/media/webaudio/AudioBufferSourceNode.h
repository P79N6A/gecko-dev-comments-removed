





#pragma once

#include "AudioSourceNode.h"

namespace mozilla {
namespace dom {

class AudioBufferSourceNode : public AudioSourceNode
{
public:
  explicit AudioBufferSourceNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED

  void NoteOn(double) {  }
  void NoteOff(double) {  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope,
                               bool* aTriedToWrap);

};

}
}

