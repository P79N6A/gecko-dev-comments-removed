



#include "mozilla/Hal.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"

#include "GamepadService.h"
#include "nsAutoPtr.h"
#include "nsIDOMEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsDOMGamepad.h"
#include "nsIDOMGamepadButtonEvent.h"
#include "nsIDOMGamepadAxisMoveEvent.h"
#include "nsIDOMGamepadEvent.h"
#include "GeneratedEvents.h"
#include "nsIDOMWindow.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsITimer.h"
#include "nsThreadUtils.h"
#include "mozilla/Services.h"

#include <cstddef>

namespace mozilla {
namespace dom {

namespace {


const int kCleanupDelayMS = 2000;
const nsTArray<nsRefPtr<nsGlobalWindow> >::index_type NoIndex =
    nsTArray<nsRefPtr<nsGlobalWindow> >::NoIndex;

StaticRefPtr<GamepadService> gGamepadServiceSingleton;

} 

bool GamepadService::sShutdown = false;

NS_IMPL_ISUPPORTS0(GamepadService)

GamepadService::GamepadService()
  : mStarted(false),
    mShuttingDown(false)
{
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
  nsCOMPtr<nsIObserver> observer = do_QueryInterface(this);
  observerService->RemoveObserver(observer, NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID);

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
  mozilla::hal::StopMonitoringGamepadStatus();
  mStarted = false;
  
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

  if (!mStarted) {
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

  if (mListeners.Length() == 0 && !mShuttingDown) {
    StartCleanupTimer();
  }
}

uint32_t
GamepadService::AddGamepad(const char* aId,
                           uint32_t aNumButtons,
                           uint32_t aNumAxes)
{
  
  nsRefPtr<nsDOMGamepad> gamepad =
    new nsDOMGamepad(NS_ConvertUTF8toUTF16(nsDependentCString(aId)),
                     0,
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
  if (mShuttingDown || aIndex >= mGamepads.Length()) {
    return;
  }

  double value = aPressed ? 1.0L : 0.0L;
  mGamepads[aIndex]->SetButton(aButton, value);

  
  
  nsTArray<nsRefPtr<nsGlobalWindow> > listeners(mListeners);

  for (uint32_t i = listeners.Length(); i > 0 ; ) {
    --i;

    
    if (!listeners[i]->GetOuterWindow() ||
        listeners[i]->GetOuterWindow()->IsBackground()) {
      continue;
    }

    if (!WindowHasSeenGamepad(listeners[i], aIndex)) {
      SetWindowHasSeenGamepad(listeners[i], aIndex);
      
      
      NewConnectionEvent(aIndex, true);
    }

    nsRefPtr<nsDOMGamepad> gamepad = listeners[i]->GetGamepad(aIndex);
    if (gamepad) {
      gamepad->SetButton(aButton, value);
      
      FireButtonEvent(listeners[i], gamepad, aButton, value);
    }
  }
}

void
GamepadService::FireButtonEvent(EventTarget* aTarget,
                                nsDOMGamepad* aGamepad,
                                uint32_t aButton,
                                double aValue)
{
  nsCOMPtr<nsIDOMEvent> event;
  bool defaultActionEnabled = true;
  NS_NewDOMGamepadButtonEvent(getter_AddRefs(event), aTarget, nullptr, nullptr);
  nsCOMPtr<nsIDOMGamepadButtonEvent> je = do_QueryInterface(event);
  MOZ_ASSERT(je, "QI should not fail");


  nsString name = aValue == 1.0L ? NS_LITERAL_STRING("gamepadbuttondown") :
                                   NS_LITERAL_STRING("gamepadbuttonup");
  je->InitGamepadButtonEvent(name, false, false, aGamepad, aButton);
  je->SetTrusted(true);

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

    if (!WindowHasSeenGamepad(listeners[i], aIndex)) {
      SetWindowHasSeenGamepad(listeners[i], aIndex);
      
      
      NewConnectionEvent(aIndex, true);
    }

    nsRefPtr<nsDOMGamepad> gamepad = listeners[i]->GetGamepad(aIndex);
    if (gamepad) {
      gamepad->SetAxis(aAxis, aValue);
      
      FireAxisMoveEvent(listeners[i], gamepad, aAxis, aValue);
    }
  }
}

void
GamepadService::FireAxisMoveEvent(EventTarget* aTarget,
                                  nsDOMGamepad* aGamepad,
                                  uint32_t aAxis,
                                  double aValue)
{
  nsCOMPtr<nsIDOMEvent> event;
  bool defaultActionEnabled = true;
  NS_NewDOMGamepadAxisMoveEvent(getter_AddRefs(event), aTarget, nullptr,
                                nullptr);
  nsCOMPtr<nsIDOMGamepadAxisMoveEvent> je = do_QueryInterface(event);
  MOZ_ASSERT(je, "QI should not fail");

  je->InitGamepadAxisMoveEvent(NS_LITERAL_STRING("gamepadaxismove"),
                               false, false, aGamepad, aAxis, aValue);
  je->SetTrusted(true);

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
          listeners[i]->GetOuterWindow()->IsBackground())
        continue;

      
      
      if (aConnected && !listeners[i]->HasSeenGamepadInput()) {
        return;
      }

      SetWindowHasSeenGamepad(listeners[i], aIndex);

      nsRefPtr<nsDOMGamepad> gamepad = listeners[i]->GetGamepad(aIndex);
      if (gamepad) {
        
        FireConnectionEvent(listeners[i], gamepad, aConnected);
      }
    }
  } else {
    
    
    for (uint32_t i = listeners.Length(); i > 0 ; ) {
      --i;

      
      

      if (WindowHasSeenGamepad(listeners[i], aIndex)) {
        nsRefPtr<nsDOMGamepad> gamepad = listeners[i]->GetGamepad(aIndex);
        if (gamepad) {
          gamepad->SetConnected(false);
          
          FireConnectionEvent(listeners[i], gamepad, false);
        }

        if (gamepad) {
          listeners[i]->RemoveGamepad(aIndex);
        }
      }
    }
  }
}

void
GamepadService::FireConnectionEvent(EventTarget* aTarget,
                                    nsDOMGamepad* aGamepad,
                                    bool aConnected)
{
  nsCOMPtr<nsIDOMEvent> event;
  bool defaultActionEnabled = true;
  NS_NewDOMGamepadEvent(getter_AddRefs(event), aTarget, nullptr, nullptr);
  nsCOMPtr<nsIDOMGamepadEvent> je = do_QueryInterface(event);
  MOZ_ASSERT(je, "QI should not fail");

  nsString name = aConnected ? NS_LITERAL_STRING("gamepadconnected") :
                               NS_LITERAL_STRING("gamepaddisconnected");
  je->InitGamepadEvent(name, false, false, aGamepad);
  je->SetTrusted(true);

  aTarget->DispatchEvent(event, &defaultActionEnabled);
}

void
GamepadService::SyncGamepadState(uint32_t aIndex, nsDOMGamepad* aGamepad)
{
  if (mShuttingDown || aIndex > mGamepads.Length()) {
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
GamepadService::WindowHasSeenGamepad(nsGlobalWindow* aWindow, uint32_t aIndex)
{
  nsRefPtr<nsDOMGamepad> gamepad = aWindow->GetGamepad(aIndex);
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
    nsRefPtr<nsDOMGamepad> gamepad = mGamepads[aIndex]->Clone();
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
                                             uint32_t aNumButtons,
                                             uint32_t aNumAxes,
                                             uint32_t* aRetval)
{
  *aRetval = gGamepadServiceSingleton->AddGamepad(aID,
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
