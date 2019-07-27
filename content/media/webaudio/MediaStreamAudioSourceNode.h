





#ifndef MediaStreamAudioSourceNode_h_
#define MediaStreamAudioSourceNode_h_

#include "AudioNode.h"
#include "DOMMediaStream.h"
#include "AudioNodeEngine.h"

namespace mozilla {

namespace dom {

class MediaStreamAudioSourceNodeEngine : public AudioNodeEngine
{
public:
  explicit MediaStreamAudioSourceNodeEngine(AudioNode* aNode)
    : AudioNodeEngine(aNode), mEnabled(false) {}

  bool IsEnabled() const { return mEnabled; }
  enum Parameters {
    ENABLE
  };
  virtual void SetInt32Parameter(uint32_t aIndex, int32_t aValue) MOZ_OVERRIDE
  {
    switch (aIndex) {
    case ENABLE:
      mEnabled = !!aValue;
      break;
    default:
      NS_ERROR("MediaStreamAudioSourceNodeEngine bad parameter index");
    }
  }
private:
  bool mEnabled;
};

class MediaStreamAudioSourceNode : public AudioNode,
                                   public DOMMediaStream::PrincipalChangeObserver
{
public:
  MediaStreamAudioSourceNode(AudioContext* aContext, DOMMediaStream* aMediaStream);

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

  virtual void PrincipalChanged(DOMMediaStream* aMediaStream) MOZ_OVERRIDE;

protected:
  virtual ~MediaStreamAudioSourceNode();

private:
  nsRefPtr<MediaInputPort> mInputPort;
  nsRefPtr<DOMMediaStream> mInputStream;
};

}
}

#endif
