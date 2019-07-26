





#ifndef ChannelMergerNode_h_
#define ChannelMergerNode_h_

#include "AudioNode.h"

namespace mozilla {
namespace dom {

class AudioContext;

class ChannelMergerNode : public AudioNode
{
public:
  ChannelMergerNode(AudioContext* aContext,
                    uint16_t aInputCount);

  NS_DECL_ISUPPORTS_INHERITED

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  virtual uint16_t NumberOfInputs() const MOZ_OVERRIDE { return mInputCount; }

private:
  const uint16_t mInputCount;
};

}
}

#endif

