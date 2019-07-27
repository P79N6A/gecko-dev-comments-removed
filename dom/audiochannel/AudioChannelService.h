





#ifndef mozilla_dom_audiochannelservice_h__
#define mozilla_dom_audiochannelservice_h__

#include "nsIAudioChannelService.h"
#include "nsAutoPtr.h"
#include "nsIObserver.h"
#include "nsTArray.h"

#include "AudioChannelAgent.h"
#include "nsAttrValue.h"
#include "nsClassHashtable.h"
#include "mozilla/dom/AudioChannelBinding.h"

class nsIRunnable;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {
#ifdef MOZ_WIDGET_GONK
class SpeakerManagerService;
#endif

#define NUMBER_OF_AUDIO_CHANNELS (uint32_t)AudioChannel::Publicnotification + 1

class AudioChannelService final : public nsIAudioChannelService
                                , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIAUDIOCHANNELSERVICE

  




  static already_AddRefed<AudioChannelService> GetOrCreate();

  


  static void Shutdown();

  



  void RegisterAudioChannelAgent(AudioChannelAgent* aAgent, AudioChannel aChannel);

  



  void UnregisterAudioChannelAgent(AudioChannelAgent* aAgent);

  



  void GetState(nsPIDOMWindow* aWindow, uint32_t aChannel,
                float* aVolume, bool* aMuted);

  
  float GetAudioChannelVolume(nsPIDOMWindow* aWindow, AudioChannel aChannel);

  void SetAudioChannelVolume(nsPIDOMWindow* aWindow, AudioChannel aChannel,
                             float aVolume);

  bool GetAudioChannelMuted(nsPIDOMWindow* aWindow, AudioChannel aChannel);

  void SetAudioChannelMuted(nsPIDOMWindow* aWindow, AudioChannel aChannel,
                            bool aMuted);

  bool IsAudioChannelActive(nsPIDOMWindow* aWindow, AudioChannel aChannel);

  



  bool TelephonyChannelIsActive();

  



  bool ProcessContentOrNormalChannelIsActive(uint64_t aChildID);

  





  virtual void SetDefaultVolumeControlChannel(int32_t aChannel,
                                              bool aVisible);

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

  static const nsAttrValue::EnumTable* GetAudioChannelTable();
  static AudioChannel GetAudioChannel(const nsAString& aString);
  static AudioChannel GetDefaultAudioChannel();
  static void GetAudioChannelString(AudioChannel aChannel, nsAString& aString);
  static void GetDefaultAudioChannelString(nsAString& aString);

  void Notify(uint64_t aWindowID);

private:
  AudioChannelService();
  ~AudioChannelService();

  
  void SetDefaultVolumeControlChannelInternal(int32_t aChannel,
                                              bool aVisible, uint64_t aChildID);

  struct AudioChannelConfig final
  {
    AudioChannelConfig()
      : mVolume(1.0)
      , mMuted(false)
      , mNumberOfAgents(0)
    {}

    float mVolume;
    bool mMuted;

    uint32_t mNumberOfAgents;
  };

  struct AudioChannelWindow final
  {
    AudioChannelConfig mChannels[NUMBER_OF_AUDIO_CHANNELS];
    nsClassHashtable<nsPtrHashKey<AudioChannelAgent>, AudioChannel> mAgents;
  };

  AudioChannelWindow&
  GetOrCreateWindowData(nsPIDOMWindow* aWindow);

  static PLDHashOperator
  TelephonyChannelIsActiveEnumerator(const uint64_t& aWindowID,
                                     nsAutoPtr<AudioChannelWindow>& aWinData,
                                     void *aPtr);

  static PLDHashOperator
  AnyAudioChannelIsActiveEnumerator(const uint64_t& aWindowID,
                                    nsAutoPtr<AudioChannelWindow>& aWinData,
                                    void *aPtr);

  static PLDHashOperator
  RefreshAgentsVolumeEnumerator(AudioChannelAgent* aAgent,
                                AudioChannel* aUnused,
                                void *aPtr);

  static PLDHashOperator
  NotifyEnumerator(AudioChannelAgent* aAgent,
                   AudioChannel* aAudioChannel,
                   void* aUnused);

  nsClassHashtable<nsUint64HashKey, AudioChannelWindow> mWindows;

#ifdef MOZ_WIDGET_GONK
  nsTArray<SpeakerManagerService*>  mSpeakerManager;
#endif

  bool mDisabled;

  nsCOMPtr<nsIRunnable> mRunnable;

  uint64_t mDefChannelChildID;

  
  
  friend class ContentParent;
  friend class ContentChild;
};

} 
} 

#endif
