





#ifndef ChannelSplitterNode_h_
#define ChannelSplitterNode_h_

#include "AudioNode.h"

namespace mozilla {
namespace dom {

class AudioContext;

class ChannelSplitterNode : public AudioNode
{
public:
  ChannelSplitterNode(AudioContext* aContext,
                      uint16_t aOutputCount);

  NS_DECL_ISUPPORTS_INHERITED

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual uint16_t NumberOfOutputs() const MOZ_OVERRIDE { return mOutputCount; }

  virtual const char* NodeType() const
  {
    return "ChannelSplitterNode";
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  virtual ~ChannelSplitterNode();

private:
  const uint16_t mOutputCount;
};

}
}

#endif

