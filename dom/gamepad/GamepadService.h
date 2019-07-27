



#ifndef mozilla_dom_GamepadService_h_
#define mozilla_dom_GamepadService_h_

#include <stdint.h>
#include "Gamepad.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsIGamepadServiceTest.h"
#include "nsGlobalWindow.h"
#include "nsIFocusManager.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {

class EventTarget;

class GamepadService : public nsIObserver
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  static already_AddRefed<GamepadService> GetService();
  
  static bool IsAPIEnabled();

  void BeginShutdown();

  
  void AddListener(nsGlobalWindow* aWindow);
  
  void RemoveListener(nsGlobalWindow* aWindow);

  
  uint32_t AddGamepad(const char* aID, GamepadMappingType aMapping,
                      uint32_t aNumButtons, uint32_t aNumAxes);
  
  void RemoveGamepad(uint32_t aIndex);

  
  
  
  
  void NewButtonEvent(uint32_t aIndex, uint32_t aButton, bool aPressed,
                      double aValue);
  
  void NewButtonEvent(uint32_t aIndex, uint32_t aButton, bool aPressed);

  
  
  
  void NewAxisMoveEvent(uint32_t aIndex, uint32_t aAxis, double aValue);

  
  void SyncGamepadState(uint32_t aIndex, Gamepad* aGamepad);

 protected:
  GamepadService();
  virtual ~GamepadService() {};
  void StartCleanupTimer();

  
  
  
  void NewConnectionEvent(uint32_t aIndex, bool aConnected);

  
  void FireAxisMoveEvent(EventTarget* aTarget,
                         Gamepad* aGamepad,
                         uint32_t axis,
                         double value);

  
  
  void FireButtonEvent(EventTarget* aTarget,
                       Gamepad* aGamepad,
                       uint32_t aButton,
                       double aValue);

  
  
  void FireConnectionEvent(EventTarget* aTarget,
                           Gamepad* aGamepad,
                           bool aConnected);

  
  bool mEnabled;
  
  bool mNonstandardEventsEnabled;
  
  bool mStarted;
  
  bool mShuttingDown;

 private:
  
  
  
  
  bool WindowHasSeenGamepad(nsGlobalWindow* aWindow, uint32_t aIndex);
  
  void SetWindowHasSeenGamepad(nsGlobalWindow* aWindow, uint32_t aIndex,
                               bool aHasSeen = true);

  static void TimeoutHandler(nsITimer* aTimer, void* aClosure);
  static bool sShutdown;

  
  
  nsTArray<nsRefPtr<Gamepad> > mGamepads;
  
  
  nsTArray<nsRefPtr<nsGlobalWindow> > mListeners;
  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<nsIFocusManager> mFocusManager;
  nsCOMPtr<nsIObserver> mObserver;
};


class GamepadServiceTest : public nsIGamepadServiceTest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGAMEPADSERVICETEST

  GamepadServiceTest();

  static already_AddRefed<GamepadServiceTest> CreateService();

private:
  static GamepadServiceTest* sSingleton;
  virtual ~GamepadServiceTest() {};
};

}  
}  

#define NS_GAMEPAD_TEST_CID \
{ 0xfb1fcb57, 0xebab, 0x4cf4, \
{ 0x96, 0x3b, 0x1e, 0x4d, 0xb8, 0x52, 0x16, 0x96 } }
#define NS_GAMEPAD_TEST_CONTRACTID "@mozilla.org/gamepad-test;1"

#endif 
