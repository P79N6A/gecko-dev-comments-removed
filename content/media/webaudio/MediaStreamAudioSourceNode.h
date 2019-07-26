





#ifndef MediaStreamAudioSourceNode_h_
#define MediaStreamAudioSourceNode_h_

#include "AudioNode.h"

namespace mozilla {

class DOMMediaStream;

namespace dom {

class MediaStreamAudioSourceNode : public AudioNode
{
public:
  MediaStreamAudioSourceNode(AudioContext* aContext, DOMMediaStream* aMediaStream);
  
  virtual ~MediaStreamAudioSourceNode();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MediaStreamAudioSourceNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  virtual void DestroyMediaStream() MOZ_OVERRIDE;

  virtual uint16_t NumberOfInputs() const MOZ_OVERRIDE { return 0; }

private:
  nsRefPtr<MediaInputPort> mInputPort;
  nsRefPtr<DOMMediaStream> mInputStream;
};

}
}

#endif
