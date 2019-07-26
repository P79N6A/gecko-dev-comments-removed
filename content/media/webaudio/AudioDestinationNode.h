





#ifndef AudioDestinationNode_h_
#define AudioDestinationNode_h_

#include "mozilla/dom/AudioChannelBinding.h"
#include "AudioNode.h"
#include "nsIDOMEventListener.h"
#include "nsIAudioChannelAgent.h"
#include "AudioChannelCommon.h"
#include "nsWeakReference.h"

namespace mozilla {
namespace dom {

class AudioContext;

class AudioDestinationNode : public AudioNode
                           , public nsIDOMEventListener
                           , public nsIAudioChannelAgentCallback
                           , public nsSupportsWeakReference
                           , public MainThreadMediaStreamListener
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

  
  NS_IMETHOD CanPlayChanged(int32_t aCanPlay);

  AudioChannel MozAudioChannelType() const;
  void SetMozAudioChannelType(AudioChannel aValue, ErrorResult& aRv);

  virtual void NotifyMainThreadStateChanged() MOZ_OVERRIDE;
  void FireOfflineCompletionEvent();

  
  
  double ExtraCurrentTime();

  
  void SetIsOnlyNodeForContext(bool aIsOnlyNode);

private:
  bool CheckAudioChannelPermissions(AudioChannel aValue);
  void CreateAudioChannelAgent();

  void SetCanPlay(bool aCanPlay);

  void NotifyStableState();
  void ScheduleStableStateNotification();

  SelfReference<AudioDestinationNode> mOfflineRenderingRef;
  uint32_t mFramesToProduce;

  nsCOMPtr<nsIAudioChannelAgent> mAudioChannelAgent;

  
  AudioChannel mAudioChannel;
<<<<<<< /home/roc/mozilla-central/content/media/webaudio/AudioDestinationNode.h
  bool mIsOffline;
  bool mHasFinished;
=======

  TimeStamp mStartedBlockingDueToBeingOnlyNode;
  double mExtraCurrentTime;
  double mExtraCurrentTimeSinceLastStartedBlocking;
  bool mExtraCurrentTimeUpdatedSinceLastStableState;
>>>>>>> /tmp/AudioDestinationNode.h~other.MvuUBx
};

}
}

#endif

