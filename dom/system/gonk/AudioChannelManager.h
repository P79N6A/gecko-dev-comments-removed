



#ifndef mozilla_dom_system_AudioChannelManager_h
#define mozilla_dom_system_AudioChannelManager_h

#include "mozilla/HalTypes.h"
#include "nsIAudioChannelManager.h"
#include "nsDOMEventTargetHelper.h"

namespace mozilla {
namespace hal {
class SwitchEvent;
typedef Observer<SwitchEvent> SwitchObserver;
} 

namespace dom {
namespace system {

class AudioChannelManager : public nsDOMEventTargetHelper
                          , public nsIAudioChannelManager
                          , public hal::SwitchObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIAUDIOCHANNELMANAGER
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  AudioChannelManager();
  virtual ~AudioChannelManager();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioChannelManager,
                                           nsDOMEventTargetHelper)
  void Notify(const hal::SwitchEvent& aEvent);

  void Init(nsPIDOMWindow* aWindow);
private:
  hal::SwitchState mState;
};

} 
} 
} 

#endif 
