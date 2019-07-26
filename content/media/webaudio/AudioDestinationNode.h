





#ifndef AudioDestinationNode_h_
#define AudioDestinationNode_h_

#include "AudioNode.h"
#include "nsIDOMEventListener.h"

namespace mozilla {
namespace dom {

class AudioContext;
class AudioChannelAgent;

class AudioDestinationNode : public AudioNode
                           , public nsIDOMEventListener
{
public:
  
  
  AudioDestinationNode(AudioContext* aContext,
                       bool aIsOffline,
                       uint32_t aNumberOfChannels = 0,
                       uint32_t aLength = 0,
                       float aSampleRate = 0.0f);

  virtual void DestroyMediaStream() MOZ_OVERRIDE;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioDestinationNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  virtual uint16_t NumberOfOutputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 0;
  }

  uint32_t MaxChannelCount() const;
  virtual void SetChannelCount(uint32_t aChannelCount,
                               ErrorResult& aRv) MOZ_OVERRIDE;

  void Mute();
  void Unmute();

  void StartRendering();

  void OfflineShutdown();

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  
  void SetCanPlay(bool aCanPlay);

private:
  SelfReference<AudioDestinationNode> mOfflineRenderingRef;
  uint32_t mFramesToProduce;

  nsRefPtr<AudioChannelAgent> mAudioChannelAgent;
};

}
}

#endif

