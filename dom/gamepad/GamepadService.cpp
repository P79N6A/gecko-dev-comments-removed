



#include "mozilla/Hal.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Preferences.h"
#include "mozilla/StaticPtr.h"

#include "GamepadService.h"
#include "Gamepad.h"
#include "nsAutoPtr.h"
#include "nsIDOMEvent.h"
#include "nsIDOMDocument.h"
#include "GeneratedEvents.h"
#include "nsIDOMWindow.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsITimer.h"
#include "nsThreadUtils.h"
#include "mozilla/Services.h"

#include "mozilla/dom/GamepadAxisMoveEvent.h"
#include "mozilla/dom/GamepadButtonEvent.h"
#include "mozilla/dom/GamepadEvent.h"

#include <cstddef>

namespace mozilla {
namespace dom {

namespace {
const char* kGamepadEnabledPref = "dom.gamepad.enabled";
const char* kGamepadEventsEnabledPref =
  "dom.gamepad.non_standard_events.enabled";


const int kCleanupDelayMS = 2000;
const nsTArray<nsRefPtr<nsGlobalWindow> >::index_type NoIndex =
    nsTArray<nsRefPtr<nsGlobalWindow> >::NoIndex;

StaticRefPtr<GamepadService> gGamepadServiceSingleton;

} 

bool GamepadService::sShutdown = false;

NS_IMPL_ISUPPORTS1(GamepadService, nsIObserver)

GamepadService::GamepadService()
  : mStarted(false),
    mShuttingDown(false)
{
  mEnabled = IsAPIEnabled();
  mNonstandardEventsEnabled =
    Preferences::GetBool(kGamepadEventsEnabledPref, false);
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  observerService->AddObserver(this,
                               NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID,
                               false);
}

NS_IMETHODIMP
GamepadService::Observe(nsISupports* aSubject,
                        const char* aTopic,
                        const PRUnichar* aData)
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  observerService->RemoveObserver(this, NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID);

  BeginShutdown();
  return NS_OK;
}

void
GamepadService::BeginShutdown()
{
  mShuttingDown = true;
  if (mTimer) {
    mTimer->Cancel();
  }
  if (mStarted) {
    mozilla::hal::StopMonitoringGamepadStatus();
    mStarted = false;
  }
  
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->SetHasGamepadEventListener(false);
  }
  mListeners.Clear();
  mGamepads.Clear();
  sShutdown = true;
}

void
GamepadService::AddListener(nsGlobalWindow* aWindow)
{
  if (mShuttingDown) {
    return;
  }

  if (mListeners.IndexOf(aWindow) != NoIndex) {
    return; 
  }

  if (!mStarted && mEnabled) {
    mozilla::hal::StartMonitoringGamepadStatus();
    mStarted = true;
  }

  mListeners.AppendElement(aWindow);
}

void
GamepadService::RemoveListener(nsGlobalWindow* aWindow)
{
  if (mShuttingDown) {
    
    
    return;
  }

  if (mListeners.IndexOf(aWindow) == NoIndex) {
    return; 
  }

  mListeners.RemoveElement(aWindow);

  if (mListeners.Length() == 0 && !mShuttingDown && mStarted) {
    StartCleanupTimer();
  }
}

uint32_t
GamepadService::AddGamepad(const char* aId,
                           GamepadMappingType aMapping,
                           uint32_t aNumButtons,
                           uint32_t aNumAxes)
{
  
  nsRefPtr<Gamepad> gamepad =
    new Gamepad(nullptr,
                NS_ConvertUTF8toUTF16(nsDependentCString(aId)),
                0,
                aMapping,
                aNumButtons,
                aNumAxes);
  int index = -1;
  for (uint32_t i = 0; i < mGamepads.Length(); i++) {
    if (!mGamepads[i]) {
      mGamepads[i] = gamepad;
      index = i;
      break;
    }
  }
  if (index == -1) {
    mGamepads.AppendElement(gamepad);
    index = mGamepads.Length() - 1;
  }

  gamepad->SetIndex(index);
  NewConnectionEvent(index, true);

  return index;
}

void
GamepadService::RemoveGamepad(uint32_t aIndex)
{
  if (aIndex < mGamepads.Length()) {
    mGamepads[aIndex]->SetConnected(false);
    NewConnectionEvent(aIndex, false);
    
    if (aIndex == mGamepads.Length() - 1) {
      mGamepads.RemoveElementAt(aIndex);
    } else {
      
      
      mGamepads[aIndex] = nullptr;
    }
  }
}

void
GamepadService::NewButtonEvent(uint32_t aIndex, uint32_t aButton, bool aPressed)
{
  
  NewButtonEvent(aIndex, aButton, aPressed, aPressed ? 1.0L : 0.0L);
}

void
GamepadService::NewButtonEvent(uint32_t aIndex, uint32_t aButton, bool aPressed,
                               double aValue)
{
  if (mShuttingDown || aIndex >= mGamepads.Length()) {
    return;
  }

  mGamepads[aIndex]->SetButton(aButton, aPressed, aValue);

  
  
  nsTArray<nsRefPtr<nsGlobalWindow> > listeners(mListeners);

  for (uint32_t i = listeners.Length(); i > 0 ; ) {
    --i;

    
    if (!listeners[i]->GetOuterWindow() ||
        listeners[i]->GetOuterWindow()->IsBackground()) {
      continue;
    }

    bool first_time = false;
    if (!WindowHasSeenGamepad(listeners[i], aIndex)) {
      
      
      SetWindowHasSeenGamepad(listeners[i], aIndex);
      first_time = true;
    }

    nsRefPtr<Gamepad> gamepad = listeners[i]->GetGamepad(aIndex);
    if (gamepad) {
      gamepad->SetButton(aButton, aPressed, aValue);
      if (first_time) {
        FireConnectionEvent(listeners[i], gamepad, true);
      }
      if (mNonstandardEventsEnabled) {
        
        FireButtonEvent(listeners[i], gamepad, aButton, aValue);
      }
    }
  }
}

void
GamepadService::FireButtonEvent(EventTarget* aTarget,
                                Gamepad* aGamepad,
                                uint32_t aButton,
                                double aValue)
{
  nsString name = aValue == 1.0L ? NS_LITERAL_STRING("gamepadbuttondown") :
                                   NS_LITERAL_STRING("gamepadbuttonup");
  GamepadButtonEventInitInitializer init;
  init.mBubbles = false;
  init.mCancelable = false;
  init.mGamepad = aGamepad;
  init.mButton = aButton;
  nsRefPtr<GamepadButtonEvent> event =
    GamepadButtonEvent::Constructor(aTarget, name, init);

  event->SetTrusted(true);

  bool defaultActionEnabled = true;
  aTarget->DispatchEvent(event, &defaultActionEnabled);
}

void
GamepadService::NewAxisMoveEvent(uint32_t aIndex, uint32_t aAxis, double aValue)
{
  if (mShuttingDown || aIndex >= mGamepads.Length()) {
    return;
  }
  mGamepads[aIndex]->SetAxis(aAxis, aValue);

  
  
  nsTArray<nsRefPtr<nsGlobalWindow> > listeners(mListeners);

  for (uint32_t i = listeners.Length(); i > 0 ; ) {
    --i;

    
    if (!listeners[i]->GetOuterWindow() ||
        listeners[i]->GetOuterWindow()->IsBackground()) {
      continue;
    }

    bool first_time = false;
    if (!WindowHasSeenGamepad(listeners[i], aIndex)) {
      
      
      SetWindowHasSeenGamepad(listeners[i], aIndex);
      first_time = true;
    }

    nsRefPtr<Gamepad> gamepad = listeners[i]->GetGamepad(aIndex);
    if (gamepad) {
      gamepad->SetAxis(aAxis, aValue);
      if (first_time) {
        FireConnectionEvent(listeners[i], gamepad, true);
      }
      if (mNonstandardEventsEnabled) {
        
        FireAxisMoveEvent(listeners[i], gamepad, aAxis, aValue);
      }
    }
  }
}

void
GamepadService::FireAxisMoveEvent(EventTarget* aTarget,
                                  Gamepad* aGamepad,
                                  uint32_t aAxis,
                                  double aValue)
{
  GamepadAxisMoveEventInitInitializer init;
  init.mBubbles = false;
  init.mCancelable = false;
  init.mGamepad = aGamepad;
  init.mAxis = aAxis;
  init.mValue = aValue;
  nsRefPtr<GamepadAxisMoveEvent> event =
    GamepadAxisMoveEvent::Constructor(aTarget,
                                      NS_LITERAL_STRING("gamepadaxismove"),
                                      init);

  event->SetTrusted(true);

  bool defaultActionEnabled = true;
  aTarget->DispatchEvent(event, &defaultActionEnabled);
}

void
GamepadService::NewConnectionEvent(uint32_t aIndex, bool aConnected)
{
  if (mShuttingDown || aIndex >= mGamepads.Length()) {
    return;
  }

  
  
  nsTArray<nsRefPtr<nsGlobalWindow> > listeners(mListeners);

  if (aConnected) {
    for (uint32_t i = listeners.Length(); i > 0 ; ) {
      --i;

      
      if (!listeners[i]->GetOuterWindow() ||
          listeners[i]->GetOuterWindow()->IsBackground()) {
        continue;
      }

      
      
      if (!listeners[i]->HasSeenGamepadInput()) {
        continue;
      }

      SetWindowHasSeenGamepad(listeners[i], aIndex);

      nsRefPtr<Gamepad> gamepad = listeners[i]->GetGamepad(aIndex);
      if (gamepad) {
        
        FireConnectionEvent(listeners[i], gamepad, aConnected);
      }
    }
  } else {
    
    
    for (uint32_t i = listeners.Length(); i > 0 ; ) {
      --i;

      
      

      if (WindowHasSeenGamepad(listeners[i], aIndex)) {
        nsRefPtr<Gamepad> gamepad = listeners[i]->GetGamepad(aIndex);
        if (gamepad) {
          gamepad->SetConnected(false);
          
          FireConnectionEvent(listeners[i], gamepad, false);
          listeners[i]->RemoveGamepad(aIndex);
        }
      }
    }
  }
}

void
GamepadService::FireConnectionEvent(EventTarget* aTarget,
                                    Gamepad* aGamepad,
                                    bool aConnected)
{
  nsString name = aConnected ? NS_LITERAL_STRING("gamepadconnected") :
                               NS_LITERAL_STRING("gamepaddisconnected");
  GamepadEventInitInitializer init;
  init.mBubbles = false;
  init.mCancelable = false;
  init.mGamepad = aGamepad;
  nsRefPtr<GamepadEvent> event =
    GamepadEvent::Constructor(aTarget, name, init);

  event->SetTrusted(true);

  bool defaultActionEnabled = true;
  aTarget->DispatchEvent(event, &defaultActionEnabled);
}

void
GamepadService::SyncGamepadState(uint32_t aIndex, Gamepad* aGamepad)
{
  if (mShuttingDown || !mEnabled || aIndex > mGamepads.Length()) {
    return;
  }

  aGamepad->SyncState(mGamepads[aIndex]);
}


already_AddRefed<GamepadService>
GamepadService::GetService()
{
  if (sShutdown) {
    return nullptr;
  }

  if (!gGamepadServiceSingleton) {
    gGamepadServiceSingleton = new GamepadService();
    ClearOnShutdown(&gGamepadServiceSingleton);
  }
  nsRefPtr<GamepadService> service(gGamepadServiceSingleton);
  return service.forget();
}


bool
GamepadService::IsAPIEnabled() {
  return Preferences::GetBool(kGamepadEnabledPref, false);
}

bool
GamepadService::WindowHasSeenGamepad(nsGlobalWindow* aWindow, uint32_t aIndex)
{
  nsRefPtr<Gamepad> gamepad = aWindow->GetGamepad(aIndex);
  return gamepad != nullptr;
}

void
GamepadService::SetWindowHasSeenGamepad(nsGlobalWindow* aWindow,
                                        uint32_t aIndex,
                                        bool aHasSeen)
{
  if (mListeners.IndexOf(aWindow) == NoIndex) {
    
    return;
  }

  if (aHasSeen) {
    aWindow->SetHasSeenGamepadInput(true);
    nsCOMPtr<nsISupports> window = nsGlobalWindow::ToSupports(aWindow);
    nsRefPtr<Gamepad> gamepad = mGamepads[aIndex]->Clone(window);
    aWindow->AddGamepad(aIndex, gamepad);
  } else {
    aWindow->RemoveGamepad(aIndex);
  }
}


void
GamepadService::TimeoutHandler(nsITimer* aTimer, void* aClosure)
{
  
  
  
  GamepadService* self = reinterpret_cast<GamepadService*>(aClosure);
  if (!self) {
    NS_ERROR("no self");
    return;
  }

  if (self->mShuttingDown) {
    return;
  }

  if (self->mListeners.Length() == 0) {
    mozilla::hal::StopMonitoringGamepadStatus();
    self->mStarted = false;
    if (!self->mGamepads.IsEmpty()) {
      self->mGamepads.Clear();
    }
  }
}

void
GamepadService::StartCleanupTimer()
{
  if (mTimer) {
    mTimer->Cancel();
  }

  mTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mTimer) {
    mTimer->InitWithFuncCallback(TimeoutHandler,
                                 this,
                                 kCleanupDelayMS,
                                 nsITimer::TYPE_ONE_SHOT);
  }
}






NS_IMPL_ISUPPORTS1(GamepadServiceTest, nsIGamepadServiceTest)

GamepadServiceTest* GamepadServiceTest::sSingleton = nullptr;


already_AddRefed<GamepadServiceTest>
GamepadServiceTest::CreateService()
{
  if (sSingleton == nullptr) {
    sSingleton = new GamepadServiceTest();
  }
  nsRefPtr<GamepadServiceTest> service = sSingleton;
  return service.forget();
}

GamepadServiceTest::GamepadServiceTest()
{
  
}


NS_IMETHODIMP GamepadServiceTest::AddGamepad(const char* aID,
                                             uint32_t aMapping,
                                             uint32_t aNumButtons,
                                             uint32_t aNumAxes,
                                             uint32_t* aRetval)
{
  *aRetval = gGamepadServiceSingleton->AddGamepad(aID,
                                                  static_cast<GamepadMappingType>(aMapping),
                                                  aNumButtons,
                                                  aNumAxes);
  return NS_OK;
}


NS_IMETHODIMP GamepadServiceTest::RemoveGamepad(uint32_t aIndex)
{
  gGamepadServiceSingleton->RemoveGamepad(aIndex);
  return NS_OK;
}



NS_IMETHODIMP GamepadServiceTest::NewButtonEvent(uint32_t aIndex,
                                                 uint32_t aButton,
                                                 bool aPressed)
{
  gGamepadServiceSingleton->NewButtonEvent(aIndex, aButton, aPressed);
  return NS_OK;
}



NS_IMETHODIMP GamepadServiceTest::NewAxisMoveEvent(uint32_t aIndex,
                                                   uint32_t aAxis,
                                                   double aValue)
{
  gGamepadServiceSingleton->NewAxisMoveEvent(aIndex, aAxis, aValue);
  return NS_OK;
}

} 
} 
