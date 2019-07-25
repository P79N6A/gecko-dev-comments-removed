




































#ifndef mozilla_dom_system_b2g_audiomanager_h__
#define mozilla_dom_system_b2g_audiomanager_h__

#include "nsIAudioManager.h"



#define NS_AUDIOMANAGER_CID {0x94f6fd70, 0x7615, 0x4af9, \
      {0x89, 0x10, 0xf9, 0x3c, 0x55, 0xe6, 0x62, 0xec}}
#define NS_AUDIOMANAGER_CONTRACTID "@mozilla.org/telephony/audiomanager;1"

namespace mozilla {
namespace dom {
namespace telephony {

class AudioManager : public nsIAudioManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUDIOMANAGER

  AudioManager() : mPhoneState(PHONE_STATE_CURRENT)
  {
  }

protected:
  PRInt32 mPhoneState;
};


} 
} 
} 

#endif 
