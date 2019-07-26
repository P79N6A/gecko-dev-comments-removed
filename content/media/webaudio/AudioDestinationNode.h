





#ifndef AudioDestinationNode_h_
#define AudioDestinationNode_h_

#include "AudioNode.h"

namespace mozilla {
namespace dom {

class AudioContext;

class AudioDestinationNode : public AudioNode
{
public:
  
  
  AudioDestinationNode(AudioContext* aContext,
                       bool aIsOffline,
                       uint32_t aNumberOfChannels = 0,
                       uint32_t aLength = 0,
                       float aSampleRate = 0.0f);

  virtual void DestroyMediaStream() MOZ_OVERRIDE;

  NS_DECL_ISUPPORTS_INHERITED

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

private:
  SelfReference<AudioDestinationNode> mOfflineRenderingRef;
  uint32_t mFramesToProduce;
};

}
}

#endif

