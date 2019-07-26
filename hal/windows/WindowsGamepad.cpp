



#include <algorithm>
#include <cstddef>

#include <stdio.h>
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "nsIComponentManager.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "mozilla/dom/GamepadService.h"
#include "mozilla/Services.h"

namespace {

using mozilla::dom::GamepadService;

const LONG kMaxAxisValue = 65535;
const DWORD BUTTON_DOWN_MASK = 0x80;




const PRUint32 kDevicesChangedStableDelay = 200;

typedef struct {
  float x,y;
} HatState;

struct Gamepad {
  
  GUID guidInstance;
  
  int id;
  
  
  char idstring[128];
  
  int vendorID;
  int productID;
  
  int numAxes;
  int numHats;
  int numButtons;
  
  char name[128];
  
  nsRefPtr<IDirectInputDevice8> device;
  
  
  HANDLE event;
  
  HatState hatState[4];
  
  bool present;
};








static void
HatPosToAxes(DWORD hatPos, HatState& axes) {
  
  if (LOWORD(hatPos) == 0xFFFF) {
    
    axes.x = axes.y = 0.0;
  }
  else if (hatPos == 0)  {
    
    axes.x = 0.0;
    axes.y = -1.0;
  }
  else if (hatPos == 45 * DI_DEGREES) {
    
    axes.x = 1.0;
    axes.y = -1.0;
  }
  else if (hatPos == 90 * DI_DEGREES) {
    
    axes.x = 1.0;
    axes.y = 0.0;
  }
  else if (hatPos == 135 * DI_DEGREES) {
    
    axes.x = 1.0;
    axes.y = 1.0;
  }
  else if (hatPos == 180 * DI_DEGREES) {
    
    axes.x = 0.0;
    axes.y = 1.0;
  }
  else if (hatPos == 225 * DI_DEGREES) {
    
    axes.x = -1.0;
    axes.y = 1.0;
  }
  else if (hatPos == 270 * DI_DEGREES) {
    
    axes.x = -1.0;
    axes.y = 0.0;
  }
  else if (hatPos == 315 * DI_DEGREES) {
    
    axes.x = -1.0;
    axes.y = -1.0;
  }
}


class GamepadEvent : public nsRunnable {
public:
  typedef enum {
    Axis,
    Button,
    HatX,
    HatY,
    HatXY,
    Unknown
  } Type;

  GamepadEvent(const Gamepad& gamepad,
               Type type,
               int which,
               DWORD data) : mGamepad(gamepad),
                             mType(type),
                             mWhich(which),
                             mData(data) {
  }

  NS_IMETHOD Run() {
    nsRefPtr<GamepadService> gamepadsvc(GamepadService::GetService());

    switch (mType) {
    case Button:
      gamepadsvc->NewButtonEvent(mGamepad.id, mWhich, mData & BUTTON_DOWN_MASK);
      break;
    case Axis: {
      float adjustedData = ((float)mData * 2.0f) / (float)kMaxAxisValue - 1.0f;
      gamepadsvc->NewAxisMoveEvent(mGamepad.id, mWhich, adjustedData);
    }
    case HatX:
    case HatY:
    case HatXY: {
      
      HatState hatState;
      HatPosToAxes(mData, hatState);
      int xAxis = mGamepad.numAxes + 2 * mWhich;
      int yAxis = mGamepad.numAxes + 2 * mWhich + 1;
      
      
      if (mType == HatX || mType == HatXY) {
        gamepadsvc->NewAxisMoveEvent(mGamepad.id, xAxis, hatState.x);
      }
      if (mType == HatY || mType == HatXY) {
        gamepadsvc->NewAxisMoveEvent(mGamepad.id, yAxis, hatState.y);
      }
      break;
    }
    case Unknown:
      break;
    }
    return NS_OK;
  }

  const Gamepad& mGamepad;
  
  Type mType;
  
  int mWhich;
  
  DWORD mData;
};

class GamepadChangeEvent : public nsRunnable {
public:
  enum Type {
    Added,
    Removed
  };
  GamepadChangeEvent(Gamepad& gamepad,
                     Type type) : mGamepad(gamepad),
                                  mID(gamepad.id),
                                  mType(type) {
  }

  NS_IMETHOD Run() {
    nsRefPtr<GamepadService> gamepadsvc(GamepadService::GetService());
    if (mType == Added) {
      mGamepad.id = gamepadsvc->AddGamepad(mGamepad.idstring,
                                           mGamepad.numButtons,
                                           mGamepad.numAxes +
                                           mGamepad.numHats*2);
    } else {
      gamepadsvc->RemoveGamepad(mID);
    }
    return NS_OK;
  }

private:
  Gamepad& mGamepad;
  uint32_t mID;
  Type mType;
};

class WindowsGamepadService;

class Observer : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  Observer(WindowsGamepadService& svc) : mSvc(svc),
                                         mObserving(true) {
    nsresult rv;
    mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    observerService->AddObserver(this, "devices-changed", false);
    observerService->AddObserver(this,
                                 NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID,
                                 false);
  }

  void Stop() {
    if (mTimer) {
      mTimer->Cancel();
    }
    if (mObserving) {
      nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
      observerService->RemoveObserver(this, "devices-changed");
      observerService->RemoveObserver(this, NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID);
      mObserving = false;
    }
  }

  virtual ~Observer() {
    Stop();
  }

private:
  
  WindowsGamepadService& mSvc;
  nsCOMPtr<nsITimer> mTimer;
  bool mObserving;
};

NS_IMPL_ISUPPORTS1(Observer, nsIObserver);

class WindowsGamepadService {
public:
  WindowsGamepadService();
  virtual ~WindowsGamepadService() {
    Cleanup();
    CloseHandle(mThreadExitEvent);
    CloseHandle(mThreadRescanEvent);
    if (dinput) {
      dinput->Release();
      dinput = nullptr;
    }
  }

  void DevicesChanged();
  void Startup();
  void Shutdown();

private:
  void ScanForDevices();
  void Cleanup();
  void CleanupGamepad(Gamepad& gamepad);
  
  static BOOL CALLBACK EnumObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi,
                                           LPVOID pvRef);
  
  static BOOL CALLBACK EnumCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);
  
  static DWORD WINAPI DInputThread(LPVOID arg);

  
  HANDLE mThreadExitEvent;
  
  HANDLE mThreadRescanEvent;
  HANDLE mThread;

  
  nsTArray<Gamepad> mGamepads;
  
  nsTArray<HANDLE> mEvents;

  LPDIRECTINPUT8 dinput;

  nsRefPtr<Observer> mObserver;
};

WindowsGamepadService::WindowsGamepadService()
  : mThreadExitEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr)),
    mThreadRescanEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr)),
    mThread(nullptr),
    dinput(nullptr) {
  mObserver = new Observer(*this);
  
  CoInitialize(nullptr);
  if (CoCreateInstance(CLSID_DirectInput8,
                       nullptr,
                       CLSCTX_INPROC_SERVER,
                       IID_IDirectInput8W,
                       (LPVOID*)&dinput) == S_OK) {
    if (dinput->Initialize(GetModuleHandle(nullptr),
                           DIRECTINPUT_VERSION) != DI_OK) {
      dinput->Release();
      dinput = nullptr;
    }
  }
}


BOOL CALLBACK
WindowsGamepadService::EnumObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi,
                                           LPVOID pvRef) {
  
  Gamepad* gamepad = reinterpret_cast<Gamepad*>(pvRef);
  DIPROPRANGE dp;
  dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  dp.diph.dwSize = sizeof(DIPROPRANGE);
  dp.diph.dwHow = DIPH_BYID;
  dp.diph.dwObj = lpddoi->dwType;
  dp.lMin = 0;
  dp.lMax = kMaxAxisValue;
  gamepad->device->SetProperty(DIPROP_RANGE, &dp.diph);
  return DIENUM_CONTINUE;
}


BOOL CALLBACK
WindowsGamepadService::EnumCallback(LPCDIDEVICEINSTANCE lpddi,
                                    LPVOID pvRef) {
  WindowsGamepadService* self =
    reinterpret_cast<WindowsGamepadService*>(pvRef);
  
  for (unsigned int i = 0; i < self->mGamepads.Length(); i++) {
    if (memcmp(&lpddi->guidInstance, &self->mGamepads[i].guidInstance,
               sizeof(GUID)) == 0) {
      self->mGamepads[i].present = true;
      return DIENUM_CONTINUE;
    }
  }

  Gamepad gamepad;
  memset(&gamepad, 0, sizeof(Gamepad));
  if (self->dinput->CreateDevice(lpddi->guidInstance,
                                 getter_AddRefs(gamepad.device),
                                 nullptr)
      == DI_OK) {
    gamepad.present = true;
    memcpy(&gamepad.guidInstance, &lpddi->guidInstance, sizeof(GUID));

    DIDEVICEINSTANCE info;
    info.dwSize = sizeof(DIDEVICEINSTANCE);
    if (gamepad.device->GetDeviceInfo(&info) == DI_OK) {
      WideCharToMultiByte(CP_UTF8, 0, info.tszProductName, -1,
                          gamepad.name, sizeof(gamepad.name), nullptr, nullptr);
    }
    
    DIPROPDWORD dp;
    dp.diph.dwSize = sizeof(DIPROPDWORD);
    dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dp.diph.dwObj = 0;
    dp.diph.dwHow = DIPH_DEVICE;
    if (gamepad.device->GetProperty(DIPROP_VIDPID, &dp.diph) == DI_OK) {
      sprintf(gamepad.idstring, "%x-%x-%s",
              LOWORD(dp.dwData), HIWORD(dp.dwData), gamepad.name);
    }
    DIDEVCAPS caps;
    caps.dwSize = sizeof(DIDEVCAPS);
    if (gamepad.device->GetCapabilities(&caps) == DI_OK) {
      gamepad.numAxes = caps.dwAxes;
      gamepad.numHats = caps.dwPOVs;
      gamepad.numButtons = caps.dwButtons;
      
      
    }
    
    gamepad.device->EnumObjects(EnumObjectsCallback, &gamepad, DIDFT_AXIS);
    
    dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dp.diph.dwSize = sizeof(DIPROPDWORD);
    dp.diph.dwObj = 0;
    dp.diph.dwHow = DIPH_DEVICE;
    dp.dwData = 64; 
    
    gamepad.event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    
    if (gamepad.device->SetDataFormat(&c_dfDIJoystick) == DI_OK &&
        gamepad.device->SetProperty(DIPROP_BUFFERSIZE, &dp.diph) == DI_OK &&
        gamepad.device->SetEventNotification(gamepad.event) == DI_OK &&
        gamepad.device->Acquire() == DI_OK) {
      self->mGamepads.AppendElement(gamepad);
      
      nsRefPtr<GamepadChangeEvent> event =
        new GamepadChangeEvent(self->mGamepads[self->mGamepads.Length() - 1],
                               GamepadChangeEvent::Added);
      NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    }
    else {
      if (gamepad.device) {
        gamepad.device->SetEventNotification(nullptr);
      }
      CloseHandle(gamepad.event);
    }
  }
  return DIENUM_CONTINUE;
}

void
WindowsGamepadService::ScanForDevices() {
  for (unsigned int i = 0; i < mGamepads.Length(); i++) {
    mGamepads[i].present = false;
  }

  dinput->EnumDevices(DI8DEVCLASS_GAMECTRL,
                      (LPDIENUMDEVICESCALLBACK)EnumCallback,
                      this,
                      DIEDFL_ATTACHEDONLY);

  
  for (int i = mGamepads.Length() - 1; i >= 0; i--) {
    if (!mGamepads[i].present) {
      nsRefPtr<GamepadChangeEvent> event =
        new GamepadChangeEvent(mGamepads[i],
                               GamepadChangeEvent::Removed);
      NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
      CleanupGamepad(mGamepads[i]);
      mGamepads.RemoveElementAt(i);
    }
  }

  mEvents.Clear();
  for (unsigned int i = 0; i < mGamepads.Length(); i++) {
    mEvents.AppendElement(mGamepads[i].event);
  }

  
  
  mEvents.AppendElement(mThreadRescanEvent);
  mEvents.AppendElement(mThreadExitEvent);
}


DWORD WINAPI
WindowsGamepadService::DInputThread(LPVOID arg) {
  WindowsGamepadService* self = reinterpret_cast<WindowsGamepadService*>(arg);
  self->ScanForDevices();

  while (true) {
    DWORD result = WaitForMultipleObjects(self->mEvents.Length(),
                                          self->mEvents.Elements(),
                                          FALSE,
                                          INFINITE);
    if (result == WAIT_FAILED ||
        result == WAIT_OBJECT_0 + self->mEvents.Length() - 1) {
      
      break;
    }

    unsigned int i = result - WAIT_OBJECT_0;

    if (i == self->mEvents.Length() - 2) {
      
      self->ScanForDevices();
      continue;
    }

    if (i >= self->mGamepads.Length()) {
      
      
      continue;
    }

    
    DWORD items = INFINITE;
    nsRefPtr<IDirectInputDevice8> device = self->mGamepads[i].device;
    if (device->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),
                              nullptr,
                              &items,
                              DIGDD_PEEK)== DI_OK) {
      while (items > 0) {
        
        
        DIDEVICEOBJECTDATA data;
        DWORD readCount = sizeof(data) / sizeof(DIDEVICEOBJECTDATA);
        if (device->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),
                                  &data, &readCount, 0) == DI_OK) {
          
          GamepadEvent::Type type = GamepadEvent::Unknown;
          int which;
          if (data.dwOfs >= DIJOFS_BUTTON0 && data.dwOfs < DIJOFS_BUTTON(32)) {
            type = GamepadEvent::Button;
            which = data.dwOfs - DIJOFS_BUTTON0;
          }
          else if(data.dwOfs >= DIJOFS_X  && data.dwOfs < DIJOFS_SLIDER(2)) {
            
            type = GamepadEvent::Axis;
            which = (data.dwOfs - DIJOFS_X) / sizeof(LONG);
          }
          else if (data.dwOfs >= DIJOFS_POV(0) && data.dwOfs < DIJOFS_POV(4)) {
            HatState hatState;
            HatPosToAxes(data.dwData, hatState);
            which = (data.dwOfs - DIJOFS_POV(0)) / sizeof(DWORD);
            
            
            if (hatState.x != self->mGamepads[i].hatState[which].x) {
              type = GamepadEvent::HatX;
            }
            if (hatState.y != self->mGamepads[i].hatState[which].y) {
              if (type == GamepadEvent::HatX) {
                type = GamepadEvent::HatXY;
              }
              else {
                type = GamepadEvent::HatY;
              }
            }
            self->mGamepads[i].hatState[which].x = hatState.x;
            self->mGamepads[i].hatState[which].y = hatState.y;
          }

          if (type != GamepadEvent::Unknown) {
            nsRefPtr<GamepadEvent> event =
              new GamepadEvent(self->mGamepads[i], type, which, data.dwData);
            NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
          }
        }
        items--;
      }
    }
  }
  return 0;
}

void
WindowsGamepadService::Startup() {
  mThread = CreateThread(nullptr,
                         0,
                         DInputThread,
                         this,
                         0,
                         nullptr);
}

void
WindowsGamepadService::Shutdown() {
  if (mThread) {
    SetEvent(mThreadExitEvent);
    WaitForSingleObject(mThread, INFINITE);
    CloseHandle(mThread);
  }
  Cleanup();
}

void
WindowsGamepadService::Cleanup() {
  for (unsigned int i = 0; i < mGamepads.Length(); i++) {
    CleanupGamepad(mGamepads[i]);
  }
  mGamepads.Clear();
}

void
WindowsGamepadService::CleanupGamepad(Gamepad& gamepad) {
  gamepad.device->Unacquire();
  gamepad.device->SetEventNotification(nullptr);
  CloseHandle(gamepad.event);
}

void
WindowsGamepadService::DevicesChanged() {
  SetEvent(mThreadRescanEvent);
}

NS_IMETHODIMP
Observer::Observe(nsISupports* aSubject,
                  const char* aTopic,
                  const PRUnichar* aData) {
  if (strcmp(aTopic, "timer-callback") == 0) {
    mSvc.DevicesChanged();
  } else if (strcmp(aTopic, NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID) == 0) {
    Stop();
  } else if (strcmp(aTopic, "devices-changed")) {
    
    
    if (mTimer) {
      mTimer->Cancel();
      mTimer->Init(this, kDevicesChangedStableDelay, nsITimer::TYPE_ONE_SHOT);
    }
  }
  return NS_OK;
}

} 

namespace mozilla {
namespace hal_impl {

WindowsGamepadService* gService = nullptr;

void StartMonitoringGamepadStatus()
{
  if (gService)
    return;

  gService = new WindowsGamepadService();
  gService->Startup();
}

void StopMonitoringGamepadStatus()
{
  if (!gService)
    return;

  gService->Shutdown();
  delete gService;
  gService = nullptr;
}

} 
} 

