





#ifndef mozilla_dom_audiochannelservice_h__
#define mozilla_dom_audiochannelservice_h__

#include "nsAutoPtr.h"
#include "nsIObserver.h"
#include "nsTArray.h"
#include "nsITimer.h"

#include "AudioChannelCommon.h"
#include "AudioChannelAgent.h"
#include "nsClassHashtable.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {
#ifdef MOZ_WIDGET_GONK
class SpeakerManagerService;
#endif
class AudioChannelService
: public nsIObserver
, public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITIMERCALLBACK

  





  static AudioChannelService* GetAudioChannelService();

  


  static void Shutdown();

  



  virtual void RegisterAudioChannelAgent(AudioChannelAgent* aAgent,
                                         AudioChannelType aType,
                                         bool aWithVideo);

  



  virtual void UnregisterAudioChannelAgent(AudioChannelAgent* aAgent);

  



  virtual AudioChannelState GetState(AudioChannelAgent* aAgent,
                                     bool aElementHidden);

  



  virtual bool ContentOrNormalChannelIsActive();

  



  virtual bool ProcessContentOrNormalChannelIsActive(uint64_t aChildID);

  



  virtual void SetDefaultVolumeControlChannel(AudioChannelType aType,
                                              bool aHidden);

  bool AnyAudioChannelIsActive();

  void RefreshAgentsVolume(nsPIDOMWindow* aWindow);

#ifdef MOZ_WIDGET_GONK
  void RegisterSpeakerManager(SpeakerManagerService* aSpeakerManager)
  {
    if (!mSpeakerManager.Contains(aSpeakerManager)) {
      mSpeakerManager.AppendElement(aSpeakerManager);
    }
  }

  void UnregisterSpeakerManager(SpeakerManagerService* aSpeakerManager)
  {
    mSpeakerManager.RemoveElement(aSpeakerManager);
  }
#endif
protected:
  void Notify();

  



  void SendAudioChannelChangedNotification(uint64_t aChildID);

  
  void RegisterType(AudioChannelType aType, uint64_t aChildID, bool aWithVideo);
  void UnregisterType(AudioChannelType aType, bool aElementHidden,
                      uint64_t aChildID, bool aWithVideo);
  void UnregisterTypeInternal(AudioChannelType aType, bool aElementHidden,
                              uint64_t aChildID, bool aWithVideo);

  AudioChannelState GetStateInternal(AudioChannelType aType, uint64_t aChildID,
                                     bool aElementHidden,
                                     bool aElementWasHidden);

  
  void UpdateChannelType(AudioChannelType aType, uint64_t aChildID,
                         bool aElementHidden, bool aElementWasHidden);

  
  void SetDefaultVolumeControlChannelInternal(AudioChannelType aType,
                                              bool aHidden, uint64_t aChildID);

  AudioChannelService();
  virtual ~AudioChannelService();

  enum AudioChannelInternalType {
    AUDIO_CHANNEL_INT_NORMAL = 0,
    AUDIO_CHANNEL_INT_NORMAL_HIDDEN,
    AUDIO_CHANNEL_INT_CONTENT,
    AUDIO_CHANNEL_INT_CONTENT_HIDDEN,
    AUDIO_CHANNEL_INT_NOTIFICATION,
    AUDIO_CHANNEL_INT_NOTIFICATION_HIDDEN,
    AUDIO_CHANNEL_INT_ALARM,
    AUDIO_CHANNEL_INT_ALARM_HIDDEN,
    AUDIO_CHANNEL_INT_TELEPHONY,
    AUDIO_CHANNEL_INT_TELEPHONY_HIDDEN,
    AUDIO_CHANNEL_INT_RINGER,
    AUDIO_CHANNEL_INT_RINGER_HIDDEN,
    AUDIO_CHANNEL_INT_PUBLICNOTIFICATION,
    AUDIO_CHANNEL_INT_PUBLICNOTIFICATION_HIDDEN,
    AUDIO_CHANNEL_INT_LAST
  };

  bool ChannelsActiveWithHigherPriorityThan(AudioChannelInternalType aType);

  bool CheckVolumeFadedCondition(AudioChannelInternalType aType,
                                 bool aElementHidden);

  const char* ChannelName(AudioChannelType aType);

  AudioChannelInternalType GetInternalType(AudioChannelType aType,
                                           bool aElementHidden);

  class AudioChannelAgentData {
  public:
    AudioChannelAgentData(AudioChannelType aType,
                          bool aElementHidden,
                          AudioChannelState aState,
                          bool aWithVideo)
    : mType(aType)
    , mElementHidden(aElementHidden)
    , mState(aState)
    , mWithVideo(aWithVideo)
    {}

    AudioChannelType mType;
    bool mElementHidden;
    AudioChannelState mState;
    const bool mWithVideo;
  };

  static PLDHashOperator
  NotifyEnumerator(AudioChannelAgent* aAgent,
                   AudioChannelAgentData* aData, void *aUnused);

  static PLDHashOperator
  RefreshAgentsVolumeEnumerator(AudioChannelAgent* aAgent,
                                AudioChannelAgentData* aUnused,
                                void *aPtr);

  static PLDHashOperator
  CountWindowEnumerator(AudioChannelAgent* aAgent,
                        AudioChannelAgentData* aUnused,
                        void *aPtr);

  uint32_t CountWindow(nsIDOMWindow* aWindow);

  nsClassHashtable< nsPtrHashKey<AudioChannelAgent>, AudioChannelAgentData > mAgents;
#ifdef MOZ_WIDGET_GONK
  nsTArray<SpeakerManagerService*>  mSpeakerManager;
#endif
  nsTArray<uint64_t> mChannelCounters[AUDIO_CHANNEL_INT_LAST];

  AudioChannelType mCurrentHigherChannel;
  AudioChannelType mCurrentVisibleHigherChannel;

  nsTArray<uint64_t> mWithVideoChildIDs;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  uint64_t mPlayableHiddenContentChildID;

  bool mDisabled;

  nsCOMPtr<nsITimer> mDeferTelChannelTimer;
  bool mTimerElementHidden;
  uint64_t mTimerChildID;

  uint64_t mDefChannelChildID;

  
  
  friend class ContentParent;
  friend class ContentChild;
};

} 
} 

#endif
