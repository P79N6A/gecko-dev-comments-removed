





#ifndef AudioDestinationNode_h_
#define AudioDestinationNode_h_

#include "mozilla/dom/AudioChannelBinding.h"
#include "AudioNode.h"
#include "nsIDOMEventListener.h"
#include "nsIAudioChannelAgent.h"
#include "AudioChannelCommon.h"

namespace mozilla {
namespace dom {

class AudioContext;
class EventProxyHandler;

class AudioDestinationNode : public AudioNode
                           , public nsIDOMEventListener
                           , public nsIAudioChannelAgentCallback
                           , public MainThreadMediaStreamListener
{
public:
  
  
  AudioDestinationNode(AudioContext* aContext,
                       bool aIsOffline,
                       AudioChannel aChannel = AudioChannel::Normal,
                       uint32_t aNumberOfChannels = 0,
                       uint32_t aLength = 0,
                       float aSampleRate = 0.0f);

  virtual void DestroyMediaStream() MOZ_OVERRIDE;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioDestinationNode, AudioNode)
  NS_DECL_NSIAUDIOCHANNELAGENTCALLBACK

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual uint16_t NumberOfOutputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 0;
  }

  uint32_t MaxChannelCount() const;
  virtual void SetChannelCount(uint32_t aChannelCount,
                               ErrorResult& aRv) MOZ_OVERRIDE;

  void Mute();
  void Unmute();

  void StartRendering(Promise* aPromise);

  void OfflineShutdown();

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  AudioChannel MozAudioChannelType() const;
  void SetMozAudioChannelType(AudioChannel aValue, ErrorResult& aRv);

  virtual void NotifyMainThreadStateChanged() MOZ_OVERRIDE;
  void FireOfflineCompletionEvent();

  
  
  double ExtraCurrentTime();

  
  void SetIsOnlyNodeForContext(bool aIsOnlyNode);

  void CreateAudioChannelAgent();

  virtual const char* NodeType() const
  {
    return "AudioDestinationNode";
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

  void InputMuted(bool aInputMuted);
  void ResolvePromise(AudioBuffer* aRenderedBuffer);

protected:
  virtual ~AudioDestinationNode();

private:
  bool CheckAudioChannelPermissions(AudioChannel aValue);

  void SetCanPlay(bool aCanPlay);

  void NotifyStableState();
  void ScheduleStableStateNotification();

  SelfReference<AudioDestinationNode> mOfflineRenderingRef;
  uint32_t mFramesToProduce;

  nsCOMPtr<nsIAudioChannelAgent> mAudioChannelAgent;

  nsRefPtr<EventProxyHandler> mEventProxyHelper;
  nsRefPtr<Promise> mOfflineRenderingPromise;

  
  AudioChannel mAudioChannel;
  bool mIsOffline;
  bool mHasFinished;
  bool mAudioChannelAgentPlaying;

  TimeStamp mStartedBlockingDueToBeingOnlyNode;
  double mExtraCurrentTime;
  double mExtraCurrentTimeSinceLastStartedBlocking;
  bool mExtraCurrentTimeUpdatedSinceLastStableState;
};

}
}

#endif

