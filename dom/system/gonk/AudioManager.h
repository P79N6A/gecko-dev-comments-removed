














#ifndef mozilla_dom_system_b2g_audiomanager_h__
#define mozilla_dom_system_b2g_audiomanager_h__

#include "mozilla/Observer.h"
#include "nsAutoPtr.h"
#include "nsIAudioManager.h"


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

class AudioManager : public nsIAudioManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUDIOMANAGER

  AudioManager();
  ~AudioManager();

  static void SetAudioRoute(int aRoutes);
protected:
  int32_t mPhoneState;

private:
  nsAutoPtr<mozilla::hal::SwitchObserver> mObserver;
};

} 
} 
} 

#endif 
