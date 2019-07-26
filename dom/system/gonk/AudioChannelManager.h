



#ifndef mozilla_dom_system_AudioChannelManager_h
#define mozilla_dom_system_AudioChannelManager_h

#include "mozilla/Hal.h"
#include "mozilla/HalTypes.h"
#include "nsDOMEventTargetHelper.h"

namespace mozilla {
namespace hal {
class SwitchEvent;
typedef Observer<SwitchEvent> SwitchObserver;
} 

namespace dom {
namespace system {

class AudioChannelManager : public nsDOMEventTargetHelper
                          , public hal::SwitchObserver
{
public:
  AudioChannelManager();
  virtual ~AudioChannelManager();

  void Notify(const hal::SwitchEvent& aEvent);

  void Init(nsPIDOMWindow* aWindow);

  



  nsPIDOMWindow* GetParentObject() const
  {
     return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  bool Headphones() const
  {
    return mState == hal::SWITCH_STATE_ON;
  }

  IMPL_EVENT_HANDLER(headphoneschange)

private:
  hal::SwitchState mState;
};

} 
} 
} 

#endif 
