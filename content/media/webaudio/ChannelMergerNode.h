





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

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual uint16_t NumberOfInputs() const MOZ_OVERRIDE { return mInputCount; }

  virtual const char* NodeType() const
  {
    return "ChannelMergerNode";
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  virtual ~ChannelMergerNode();

private:
  const uint16_t mInputCount;
};

}
}

#endif

