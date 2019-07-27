





#ifndef ChannelSplitterNode_h_
#define ChannelSplitterNode_h_

#include "AudioNode.h"

namespace mozilla {
namespace dom {

class AudioContext;

class ChannelSplitterNode final : public AudioNode
{
public:
  ChannelSplitterNode(AudioContext* aContext,
                      uint16_t aOutputCount);

  NS_DECL_ISUPPORTS_INHERITED

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual uint16_t NumberOfOutputs() const override { return mOutputCount; }

  virtual const char* NodeType() const override
  {
    return "ChannelSplitterNode";
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
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

