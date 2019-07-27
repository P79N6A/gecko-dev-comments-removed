














#ifndef mozilla_dom_system_b2g_audiomanager_h__
#define mozilla_dom_system_b2g_audiomanager_h__

#include "mozilla/Observer.h"
#include "nsAutoPtr.h"
#include "nsIAudioManager.h"
#include "nsIObserver.h"
#include "AudioChannelAgent.h"
#include "android_audio/AudioSystem.h"


#define NS_AUDIOMANAGER_CID {0x94f6fd70, 0x7615, 0x4af9, \
      {0x89, 0x10, 0xf9, 0x3c, 0x55, 0xe6, 0x62, 0xec}}
#define NS_AUDIOMANAGER_CONTRACTID "@mozilla.org/telephony/audiomanager;1"

namespace mozilla {
namespace hal {
class SwitchEvent;
typedef Observer<SwitchEvent> SwitchObserver;
} 

namespace dom {
namespace gonk {







enum AudioOutputProfiles {
  DEVICE_PRIMARY      = 0,
  DEVICE_HEADSET      = 1,
  DEVICE_BLUETOOTH    = 2,
  DEVICE_TOTAL_NUMBER = 3,
};










enum AudioVolumeCategories {
  VOLUME_MEDIA         = 0,
  VOLUME_NOTIFICATION  = 1,
  VOLUME_ALARM         = 2,
  VOLUME_TELEPHONY     = 3,
  VOLUME_BLUETOOTH_SCO = 4,
  VOLUME_TOTAL_NUMBER  = 5,
};

struct VolumeData {
  const char* mChannelName;
  uint32_t mCategory;
};

class RecoverTask;
class AudioChannelVolInitCallback;
class AudioProfileData;

class AudioManager final : public nsIAudioManager
                         , public nsIObserver
{
public:
  static already_AddRefed<AudioManager> GetInstance();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUDIOMANAGER
  NS_DECL_NSIOBSERVER

  
  
  friend class RecoverTask;
  friend class AudioChannelVolInitCallback;

  
  void SwitchProfileData(AudioOutputProfiles aProfile, bool aActive);

  
  nsresult ValidateVolumeIndex(uint32_t aCategory, uint32_t aIndex) const;

protected:
  int32_t mPhoneState;
  uint32_t mCurrentStreamVolumeTbl[AUDIO_STREAM_CNT];

  nsresult SetStreamVolumeIndex(int32_t aStream, uint32_t aIndex);
  nsresult GetStreamVolumeIndex(int32_t aStream, uint32_t *aIndex);

private:
  nsAutoPtr<mozilla::hal::SwitchObserver> mObserver;
  nsCOMPtr<nsIAudioChannelAgent>          mPhoneAudioAgent;
#ifdef MOZ_B2G_RIL
  bool                                    mMuteCallToRIL;
  
  bool                                    mIsMicMuted;
#endif
  nsTArray<nsAutoPtr<AudioProfileData>>   mAudioProfiles;
  AudioOutputProfiles mPresentProfile;

  void HandleBluetoothStatusChanged(nsISupports* aSubject,
                                    const char* aTopic,
                                    const nsCString aAddress);
  void HandleAudioChannelProcessChanged();

  void CreateAudioProfilesData();

  
  void InitProfilesVolume(uint32_t aCatogory, uint32_t aIndex);

  
  void UpdateVolumeToProfile(AudioProfileData* aProfileData);

  
  void UpdateVolumeFromProfile(AudioProfileData* aProfileData);

  
  void SendVolumeChangeNotification(AudioProfileData* aProfileData);

  
  void UpdateProfileState(AudioOutputProfiles aProfile, bool aActive);

  
  nsresult SetVolumeByCategory(uint32_t aCategory, uint32_t aIndex);
  uint32_t GetVolumeByCategory(uint32_t aCategory) const;
  uint32_t GetMaxVolumeByCategory(uint32_t aCategory) const;

  AudioProfileData* FindAudioProfileData(AudioOutputProfiles aProfile);

  AudioManager();
  ~AudioManager();
};

} 
} 
} 

#endif 
