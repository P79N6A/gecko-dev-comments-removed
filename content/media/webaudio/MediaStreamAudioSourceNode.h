





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

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual void DestroyMediaStream() MOZ_OVERRIDE;

  virtual uint16_t NumberOfInputs() const MOZ_OVERRIDE { return 0; }

  virtual const char* NodeType() const
  {
    return "MediaStreamAudioSourceNode";
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

private:
  nsRefPtr<MediaInputPort> mInputPort;
  nsRefPtr<DOMMediaStream> mInputStream;
};

}
}

#endif
