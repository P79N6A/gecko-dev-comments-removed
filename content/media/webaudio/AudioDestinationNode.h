





#ifndef AudioDestinationNode_h_
#define AudioDestinationNode_h_

#include "AudioNode.h"

namespace mozilla {
namespace dom {

class AudioContext;

class AudioDestinationNode : public AudioNode
{
public:
  AudioDestinationNode(AudioContext* aContext, MediaStreamGraph* aGraph);

  NS_DECL_ISUPPORTS_INHERITED

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  virtual uint32_t NumberOfOutputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 0;
  }

};

}
}

#endif

