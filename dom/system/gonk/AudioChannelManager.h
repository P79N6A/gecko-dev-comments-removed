



#ifndef mozilla_dom_system_AudioChannelManager_h
#define mozilla_dom_system_AudioChannelManager_h

#include "mozilla/Hal.h"
#include "mozilla/HalTypes.h"
#include "nsDOMEventTargetHelper.h"
#include "AudioChannelService.h"

namespace mozilla {
namespace hal {
class SwitchEvent;
typedef Observer<SwitchEvent> SwitchObserver;
} 

namespace dom {
namespace system {

class AudioChannelManager MOZ_FINAL
  : public nsDOMEventTargetHelper
  , public hal::SwitchObserver
  , public nsIDOMEventListener
{
public:
  AudioChannelManager();
  virtual ~AudioChannelManager();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMEVENTLISTENER

  void Notify(const hal::SwitchEvent& aEvent);

  void Init(nsPIDOMWindow* aWindow);

  



  nsPIDOMWindow* GetParentObject() const
  {
     return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  bool Headphones() const
  {
    
    
    
    
    return mState != hal::SWITCH_STATE_OFF &&
           mState != hal::SWITCH_STATE_UNKNOWN;
  }

  bool SetVolumeControlChannel(const nsAString& aChannel);

  bool GetVolumeControlChannel(nsAString& aChannel);

  IMPL_EVENT_HANDLER(headphoneschange)

private:
  void NotifyVolumeControlChannelChanged();

  hal::SwitchState mState;
  AudioChannelType mVolumeChannel;
};

} 
} 
} 

#endif 
