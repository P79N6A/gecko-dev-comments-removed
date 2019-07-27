





#ifndef ChannelMergerNode_h_
#define ChannelMergerNode_h_

#include "AudioNode.h"

namespace mozilla {
namespace dom {

class AudioContext;

class ChannelMergerNode final : public AudioNode
{
public:
  ChannelMergerNode(AudioContext* aContext,
                    uint16_t aInputCount);

  NS_DECL_ISUPPORTS_INHERITED

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual uint16_t NumberOfInputs() const override { return mInputCount; }

  virtual const char* NodeType() const override
  {
    return "ChannelMergerNode";
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
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

