














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

using namespace mozilla::dom;

namespace mozilla {
namespace hal {
class SwitchEvent;
typedef Observer<SwitchEvent> SwitchObserver;
} 

namespace dom {
namespace gonk {
class RecoverTask;
class AudioManager : public nsIAudioManager
                   , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUDIOMANAGER
  NS_DECL_NSIOBSERVER

  AudioManager();
  ~AudioManager();

  
  
  friend class RecoverTask;

protected:
  int32_t mPhoneState;
  int mCurrentStreamVolumeTbl[AUDIO_STREAM_CNT];

  android::status_t SetStreamVolumeIndex(int32_t aStream, int32_t aIndex);
  android::status_t GetStreamVolumeIndex(int32_t aStream, int32_t *aIndex);

private:
  nsAutoPtr<mozilla::hal::SwitchObserver> mObserver;
  nsCOMPtr<AudioChannelAgent>             mPhoneAudioAgent;
};

} 
} 
} 

#endif 
