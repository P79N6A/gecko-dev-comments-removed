



#include "AudioChannelManager.h"
#include "nsIDOMClassInfo.h"
#include "mozilla/dom/AudioChannelManagerBinding.h"

using namespace mozilla::hal;

namespace mozilla {
namespace dom {
namespace system {

AudioChannelManager::AudioChannelManager()
  : mState(SWITCH_STATE_UNKNOWN)
{
  RegisterSwitchObserver(SWITCH_HEADPHONES, this);
  mState = GetCurrentSwitchState(SWITCH_HEADPHONES);
  SetIsDOMBinding();
}

AudioChannelManager::~AudioChannelManager()
{
  UnregisterSwitchObserver(SWITCH_HEADPHONES, this);
}

void
AudioChannelManager::Init(nsPIDOMWindow* aWindow)
{
  BindToOwner(aWindow->IsOuterWindow() ?
    aWindow->GetCurrentInnerWindow() : aWindow);
}

JSObject*
AudioChannelManager::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return AudioChannelManagerBinding::Wrap(aCx, aScope, this);
}

void
AudioChannelManager::Notify(const SwitchEvent& aEvent)
{
  if (aEvent.status() == SWITCH_STATE_ON ||
      aEvent.status() == SWITCH_STATE_HEADSET ||
      aEvent.status() == SWITCH_STATE_HEADPHONE) {
    mState = SWITCH_STATE_ON;
  } else {
    mState = SWITCH_STATE_OFF;
  }

  DispatchTrustedEvent(NS_LITERAL_STRING("headphoneschange"));
}

} 
} 
} 
