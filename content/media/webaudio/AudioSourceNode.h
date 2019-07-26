





#ifndef AudioSourceNode_h_
#define AudioSourceNode_h_

#include "AudioNode.h"

namespace mozilla {
namespace dom {

class AudioSourceNode : public AudioNode
{
public:
  explicit AudioSourceNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED

  virtual uint32_t NumberOfInputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 0;
  }
};

}
}

#endif

