





#ifndef MediaStreamAudioDestinationNode_h_
#define MediaStreamAudioDestinationNode_h_

#include "AudioNode.h"

namespace mozilla {
class DOMMediaStream;

namespace dom {

class MediaStreamAudioDestinationNode : public AudioNode
{
public:
  explicit MediaStreamAudioDestinationNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MediaStreamAudioDestinationNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  virtual uint16_t NumberOfOutputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 0;
  }

  virtual void DestroyMediaStream() MOZ_OVERRIDE;

  DOMMediaStream* DOMStream() const
  {
    return mDOMStream;
  }

private:
  nsRefPtr<DOMMediaStream> mDOMStream;
  nsRefPtr<MediaInputPort> mPort;
};

}
}

#endif
