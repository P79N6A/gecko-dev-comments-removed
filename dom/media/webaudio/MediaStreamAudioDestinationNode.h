





#ifndef MediaStreamAudioDestinationNode_h_
#define MediaStreamAudioDestinationNode_h_

#include "AudioNode.h"

namespace mozilla {
namespace dom {

class MediaStreamAudioDestinationNode final : public AudioNode
{
public:
  explicit MediaStreamAudioDestinationNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MediaStreamAudioDestinationNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual uint16_t NumberOfOutputs() const final override
  {
    return 0;
  }

  virtual void DestroyMediaStream() override;

  DOMMediaStream* DOMStream() const
  {
    return mDOMStream;
  }

  virtual const char* NodeType() const override
  {
    return "MediaStreamAudioDestinationNode";
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override;
  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override;

protected:
  virtual ~MediaStreamAudioDestinationNode();

private:
  nsRefPtr<DOMMediaStream> mDOMStream;
  nsRefPtr<MediaInputPort> mPort;
};

}
}

#endif
