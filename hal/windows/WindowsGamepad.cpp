




#include <algorithm>
#include <cstddef>

#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <hidsdi.h>
#include <stdio.h>

#include "nsIComponentManager.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/dom/GamepadService.h"
#include "mozilla/Services.h"

namespace {

using mozilla::dom::GamepadService;
using mozilla::ArrayLength;


const unsigned kUsageDpad = 0x39;

const unsigned kFirstAxis = 0x30;


const unsigned kDesktopUsagePage = 0x1;
const unsigned kButtonUsagePage = 0x9;


const unsigned kMaxButtons = 32;
const unsigned kMaxAxes = 32;





const uint32_t kDevicesChangedStableDelay = 200;

const struct {
  int usagePage;
  int usage;
} kUsagePages[] = {
  
  { kDesktopUsagePage, 4 },  
  { kDesktopUsagePage, 5 }   
};

enum GamepadType {
  kNoGamepad = 0,
  kRawInputGamepad
};

class WindowsGamepadService;
WindowsGamepadService* gService = nullptr;

struct Gamepad {
  GamepadType type;

  
  HANDLE handle;

  
  
  int id;

  
  unsigned numAxes;
  unsigned numButtons;
  bool hasDpad;
  HIDP_VALUE_CAPS dpadCaps;

  bool buttons[kMaxButtons];
  struct {
    HIDP_VALUE_CAPS caps;
    double value;
  } axes[kMaxAxes];

  
  bool present;
};

bool
GetPreparsedData(HANDLE handle, nsTArray<uint8_t>& data)
{
  UINT size;
  if (GetRawInputDeviceInfo(handle, RIDI_PREPARSEDDATA, nullptr, &size) < 0) {
    return false;
  }
  data.SetLength(size);
  return GetRawInputDeviceInfo(handle, RIDI_PREPARSEDDATA,
                               data.Elements(), &size) > 0;
}





double
ScaleAxis(ULONG value, LONG min, LONG max)
{
  return  2.0 * (value - min) / (max - min) - 1.0;
}





void
UnpackDpad(LONG dpad_value, const Gamepad* gamepad, bool buttons[kMaxButtons])
{
  const unsigned kUp = gamepad->numButtons - 4;
  const unsigned kDown = gamepad->numButtons - 3;
  const unsigned kLeft = gamepad->numButtons - 2;
  const unsigned kRight = gamepad->numButtons - 1;

  
  
  if (dpad_value < gamepad->dpadCaps.LogicalMin
      || dpad_value > gamepad->dpadCaps.LogicalMax) {
    
    return;
  }

  
  int value = dpad_value - gamepad->dpadCaps.LogicalMin;

  
  
  
  if (value < 2 || value > 6) {
    buttons[kUp] = true;
  }
  if (value > 2 && value < 6) {
    buttons[kDown] = true;
  }
  if (value > 4) {
    buttons[kLeft] = true;
  }
  if (value > 0 && value < 4) {
    buttons[kRight] = true;
  }
}





bool
SupportedUsage(USHORT page, USHORT usage)
{
  for (unsigned i = 0; i < ArrayLength(kUsagePages); i++) {
    if (page == kUsagePages[i].usagePage && usage == kUsagePages[i].usage) {
      return true;
    }
  }
  return false;
}

class Observer : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  Observer(WindowsGamepadService& svc) : mSvc(svc),
                                         mObserving(true)
  {
    nsresult rv;
    mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    observerService->AddObserver(this,
                                 NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID,
                                 false);
  }

  void Stop()
  {
    if (mTimer) {
      mTimer->Cancel();
    }
    if (mObserving) {
      nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
      observerService->RemoveObserver(this, NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID);
      mObserving = false;
    }
  }

  virtual ~Observer()
  {
    Stop();
  }

  void SetDeviceChangeTimer()
  {
    
    
    if (mTimer) {
      mTimer->Cancel();
      mTimer->Init(this, kDevicesChangedStableDelay, nsITimer::TYPE_ONE_SHOT);
    }
  }

private:
  
  WindowsGamepadService& mSvc;
  nsCOMPtr<nsITimer> mTimer;
  bool mObserving;
};

NS_IMPL_ISUPPORTS(Observer, nsIObserver);

class HIDLoader {
public:
  HIDLoader() : mModule(LoadLibraryW(L"hid.dll")),
                mHidD_GetProductString(nullptr),
                mHidP_GetCaps(nullptr),
                mHidP_GetButtonCaps(nullptr),
                mHidP_GetValueCaps(nullptr),
                mHidP_GetUsages(nullptr),
                mHidP_GetUsageValue(nullptr),
                mHidP_GetScaledUsageValue(nullptr)
  {
    if (mModule) {
      mHidD_GetProductString = reinterpret_cast<decltype(HidD_GetProductString)*>(GetProcAddress(mModule, "HidD_GetProductString"));
      mHidP_GetCaps = reinterpret_cast<decltype(HidP_GetCaps)*>(GetProcAddress(mModule, "HidP_GetCaps"));
      mHidP_GetButtonCaps = reinterpret_cast<decltype(HidP_GetButtonCaps)*>(GetProcAddress(mModule, "HidP_GetButtonCaps"));
      mHidP_GetValueCaps = reinterpret_cast<decltype(HidP_GetValueCaps)*>(GetProcAddress(mModule, "HidP_GetValueCaps"));
      mHidP_GetUsages = reinterpret_cast<decltype(HidP_GetUsages)*>(GetProcAddress(mModule, "HidP_GetUsages"));
      mHidP_GetUsageValue = reinterpret_cast<decltype(HidP_GetUsageValue)*>(GetProcAddress(mModule, "HidP_GetUsageValue"));
      mHidP_GetScaledUsageValue = reinterpret_cast<decltype(HidP_GetScaledUsageValue)*>(GetProcAddress(mModule, "HidP_GetScaledUsageValue"));
    }
  }

  ~HIDLoader() {
    if (mModule) {
      FreeLibrary(mModule);
    }
  }

  operator bool() {
    return mModule &&
      mHidD_GetProductString &&
      mHidP_GetCaps &&
      mHidP_GetButtonCaps &&
      mHidP_GetValueCaps &&
      mHidP_GetUsages &&
      mHidP_GetUsageValue &&
      mHidP_GetScaledUsageValue;
  }

  decltype(HidD_GetProductString) *mHidD_GetProductString;
  decltype(HidP_GetCaps) *mHidP_GetCaps;
  decltype(HidP_GetButtonCaps) *mHidP_GetButtonCaps;
  decltype(HidP_GetValueCaps) *mHidP_GetValueCaps;
  decltype(HidP_GetUsages) *mHidP_GetUsages;
  decltype(HidP_GetUsageValue) *mHidP_GetUsageValue;
  decltype(HidP_GetScaledUsageValue) *mHidP_GetScaledUsageValue;

private:
  HMODULE mModule;
};

class WindowsGamepadService {
public:
  WindowsGamepadService();
  virtual ~WindowsGamepadService()
  {
    Cleanup();
  }

  enum DeviceChangeType {
    DeviceChangeNotification,
    DeviceChangeStable
  };
  void DevicesChanged(DeviceChangeType type);
  void Startup();
  void Shutdown();
  
  bool HandleRawInput(HRAWINPUT handle);

private:
  void ScanForDevices();
  
  void ScanForRawInputDevices();
  
  bool GetRawGamepad(HANDLE handle);
  void Cleanup();

  
  nsTArray<Gamepad> mGamepads;

  nsRefPtr<Observer> mObserver;

  HIDLoader mHID;
};


WindowsGamepadService::WindowsGamepadService()
{
  mObserver = new Observer(*this);
}

void
WindowsGamepadService::ScanForRawInputDevices()
{
  if (!mHID) {
    return;
  }

  UINT numDevices;
  if (GetRawInputDeviceList(nullptr, &numDevices, sizeof(RAWINPUTDEVICELIST))
      == -1) {
    return;
  }
  nsTArray<RAWINPUTDEVICELIST> devices(numDevices);
  devices.SetLength(numDevices);
  if (GetRawInputDeviceList(devices.Elements(), &numDevices,
                            sizeof(RAWINPUTDEVICELIST)) == -1) {
    return;
  }

  for (unsigned i = 0; i < devices.Length(); i++) {
    if (devices[i].dwType == RIM_TYPEHID) {
      GetRawGamepad(devices[i].hDevice);
    }
  }
}

void
WindowsGamepadService::ScanForDevices()
{
  for (int i = mGamepads.Length() - 1; i >= 0; i--) {
    mGamepads[i].present = false;
  }

  if (mHID) {
    ScanForRawInputDevices();
  }

  nsRefPtr<GamepadService> gamepadsvc(GamepadService::GetService());
  if (!gamepadsvc) {
    return;
  }
  
  for (int i = mGamepads.Length() - 1; i >= 0; i--) {
    if (!mGamepads[i].present) {
      gamepadsvc->RemoveGamepad(mGamepads[i].id);
      mGamepads.RemoveElementAt(i);
    }
  }
}


class HidValueComparator {
public:
  bool Equals(const HIDP_VALUE_CAPS& c1, const HIDP_VALUE_CAPS& c2) const
  {
    return c1.UsagePage == c2.UsagePage && c1.Range.UsageMin == c2.Range.UsageMin;
  }
  bool LessThan(const HIDP_VALUE_CAPS& c1, const HIDP_VALUE_CAPS& c2) const
  {
    if (c1.UsagePage == c2.UsagePage) {
      return c1.Range.UsageMin < c2.Range.UsageMin;
    }
    return c1.UsagePage < c2.UsagePage;
  }
};

bool
WindowsGamepadService::GetRawGamepad(HANDLE handle)
{
  if (!mHID) {
    return false;
  }

  for (unsigned i = 0; i < mGamepads.Length(); i++) {
    if (mGamepads[i].type == kRawInputGamepad && mGamepads[i].handle == handle) {
      mGamepads[i].present = true;
      return true;
    }
  }

  RID_DEVICE_INFO rdi = {};
  UINT size = rdi.cbSize = sizeof(RID_DEVICE_INFO);
  if (GetRawInputDeviceInfo(handle, RIDI_DEVICEINFO, &rdi, &size) < 0) {
    return false;
  }
  
  if (!SupportedUsage(rdi.hid.usUsagePage, rdi.hid.usUsage)) {
    return false;
  }

  Gamepad gamepad = {};

  
  if (GetRawInputDeviceInfo(handle, RIDI_DEVICENAME, nullptr, &size) < 0) {
    return false;
  }

  nsTArray<wchar_t> devname(size);
  devname.SetLength(size);
  if (GetRawInputDeviceInfo(handle, RIDI_DEVICENAME, devname.Elements(), &size)
      <= 0) {
    return false;
  }

  
  
  
  if (wcsstr(devname.Elements(), L"IG_")) {
    return false;
  }

  
  
  
  wchar_t name[128] = { 0 };
  size = sizeof(name);
  nsTArray<char> gamepad_name;
  HANDLE hid_handle = CreateFile(devname.Elements(), GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
  if (hid_handle) {
    if (mHID.mHidD_GetProductString(hid_handle, &name, size)) {
      int bytes = WideCharToMultiByte(CP_UTF8, 0, name, -1, nullptr, 0, nullptr,
                                      nullptr);
      gamepad_name.SetLength(bytes);
      WideCharToMultiByte(CP_UTF8, 0, name, -1, gamepad_name.Elements(),
                          bytes, nullptr, nullptr);
    }
    CloseHandle(hid_handle);
  }
  if (gamepad_name.Length() == 0 || !gamepad_name[0]) {
    const char kUnknown[] = "Unknown Gamepad";
    gamepad_name.SetLength(ArrayLength(kUnknown));
    strcpy_s(gamepad_name.Elements(), gamepad_name.Length(), kUnknown);
  }

  char gamepad_id[256] = { 0 };
  _snprintf_s(gamepad_id, _TRUNCATE, "%04x-%04x-%s", rdi.hid.dwVendorId,
              rdi.hid.dwProductId, gamepad_name.Elements());

  nsTArray<uint8_t> preparsedbytes;
  if (!GetPreparsedData(handle, preparsedbytes)) {
    return false;
  }

  PHIDP_PREPARSED_DATA parsed =
    reinterpret_cast<PHIDP_PREPARSED_DATA>(preparsedbytes.Elements());
  HIDP_CAPS caps;
  if (mHID.mHidP_GetCaps(parsed, &caps) != HIDP_STATUS_SUCCESS) {
    return false;
  }

  
  USHORT count = caps.NumberInputButtonCaps;
  nsTArray<HIDP_BUTTON_CAPS> buttonCaps(count);
  buttonCaps.SetLength(count);
  if (mHID.mHidP_GetButtonCaps(HidP_Input, buttonCaps.Elements(), &count, parsed)
      != HIDP_STATUS_SUCCESS) {
    return false;
  }
  for (unsigned i = 0; i < count; i++) {
    
    gamepad.numButtons +=
      buttonCaps[i].Range.UsageMax - buttonCaps[i].Range.UsageMin + 1;
  }
  gamepad.numButtons = std::min(gamepad.numButtons, kMaxButtons);

  
  count = caps.NumberInputValueCaps;
  nsTArray<HIDP_VALUE_CAPS> valueCaps(count);
  valueCaps.SetLength(count);
  if (mHID.mHidP_GetValueCaps(HidP_Input, valueCaps.Elements(), &count, parsed)
      != HIDP_STATUS_SUCCESS) {
    return false;
  }
  nsTArray<HIDP_VALUE_CAPS> axes;
  
  HidValueComparator comparator;
  for (unsigned i = 0; i < count; i++) {
    if (valueCaps[i].UsagePage == kDesktopUsagePage
        && valueCaps[i].Range.UsageMin == kUsageDpad
        
        && valueCaps[i].LogicalMax - valueCaps[i].LogicalMin == 7
        
        && gamepad.numButtons + 4 < kMaxButtons) {
      
      
      
      gamepad.hasDpad = true;
      gamepad.dpadCaps = valueCaps[i];
      
      gamepad.numButtons += 4;
    } else {
      axes.InsertElementSorted(valueCaps[i], comparator);
    }
  }

  gamepad.numAxes = std::min(axes.Length(), kMaxAxes);
  for (unsigned i = 0; i < gamepad.numAxes; i++) {
    if (i >= kMaxAxes) {
      break;
    }
    gamepad.axes[i].caps = axes[i];
  }
  gamepad.type = kRawInputGamepad;
  gamepad.handle = handle;
  gamepad.present = true;

  nsRefPtr<GamepadService> gamepadsvc(GamepadService::GetService());
  if (!gamepadsvc) {
    return false;
  }

  gamepad.id = gamepadsvc->AddGamepad(gamepad_id,
                                      mozilla::dom::NoMapping,
                                      gamepad.numButtons,
                                      gamepad.numAxes);
  mGamepads.AppendElement(gamepad);
  return true;
}

bool
WindowsGamepadService::HandleRawInput(HRAWINPUT handle)
{
  if (!mHID) {
    return false;
  }
  nsRefPtr<GamepadService> gamepadsvc(GamepadService::GetService());
  if (!gamepadsvc) {
    return false;
  }

  
  UINT size;
  GetRawInputData(handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
  nsTArray<uint8_t> data(size);
  data.SetLength(size);
  if (GetRawInputData(handle, RID_INPUT, data.Elements(), &size,
                      sizeof(RAWINPUTHEADER)) < 0) {
    return false;
  }
  PRAWINPUT raw = reinterpret_cast<PRAWINPUT>(data.Elements());

  Gamepad* gamepad = nullptr;
  for (unsigned i = 0; i < mGamepads.Length(); i++) {
    if (mGamepads[i].type == kRawInputGamepad
        && mGamepads[i].handle == raw->header.hDevice) {
      gamepad = &mGamepads[i];
      break;
    }
  }
  if (gamepad == nullptr) {
    return false;
  }

  
  nsTArray<uint8_t> parsedbytes;
  if (!GetPreparsedData(raw->header.hDevice, parsedbytes)) {
    return false;
  }
  PHIDP_PREPARSED_DATA parsed =
    reinterpret_cast<PHIDP_PREPARSED_DATA>(parsedbytes.Elements());

  
  nsTArray<USAGE> usages(gamepad->numButtons);
  usages.SetLength(gamepad->numButtons);
  ULONG usageLength = gamepad->numButtons;
  if (mHID.mHidP_GetUsages(HidP_Input, kButtonUsagePage, 0, usages.Elements(),
                     &usageLength, parsed, (PCHAR)raw->data.hid.bRawData,
                     raw->data.hid.dwSizeHid) != HIDP_STATUS_SUCCESS) {
    return false;
  }

  bool buttons[kMaxButtons] = { false };
  usageLength = std::min<ULONG>(usageLength, kMaxButtons);
  for (unsigned i = 0; i < usageLength; i++) {
    buttons[usages[i] - 1] = true;
  }

  if (gamepad->hasDpad) {
    
    ULONG value;
    if (mHID.mHidP_GetUsageValue(HidP_Input, gamepad->dpadCaps.UsagePage, 0, gamepad->dpadCaps.Range.UsageMin, &value, parsed, (PCHAR)raw->data.hid.bRawData, raw->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS) {
      UnpackDpad(static_cast<LONG>(value), gamepad, buttons);
    }
  }

  for (unsigned i = 0; i < gamepad->numButtons; i++) {
    if (gamepad->buttons[i] != buttons[i]) {
      gamepadsvc->NewButtonEvent(gamepad->id, i, buttons[i]);
      gamepad->buttons[i] = buttons[i];
    }
  }

  
  for (unsigned i = 0; i < gamepad->numAxes; i++) {
    double new_value;
    if (gamepad->axes[i].caps.LogicalMin < 0) {
LONG value;
      if (mHID.mHidP_GetScaledUsageValue(HidP_Input, gamepad->axes[i].caps.UsagePage,
                                   0, gamepad->axes[i].caps.Range.UsageMin,
                                   &value, parsed,
                                   (PCHAR)raw->data.hid.bRawData,
                                   raw->data.hid.dwSizeHid)
          != HIDP_STATUS_SUCCESS) {
        continue;
      }
      new_value = ScaleAxis(value, gamepad->axes[i].caps.LogicalMin,
                            gamepad->axes[i].caps.LogicalMax);
    }
    else {
      ULONG value;
      if (mHID.mHidP_GetUsageValue(HidP_Input, gamepad->axes[i].caps.UsagePage, 0,
                             gamepad->axes[i].caps.Range.UsageMin, &value,
                             parsed, (PCHAR)raw->data.hid.bRawData,
                             raw->data.hid.dwSizeHid) != HIDP_STATUS_SUCCESS) {
        continue;
      }

      new_value = ScaleAxis(value, gamepad->axes[i].caps.LogicalMin,
                            gamepad->axes[i].caps.LogicalMax);
    }
    if (gamepad->axes[i].value != new_value) {
      gamepadsvc->NewAxisMoveEvent(gamepad->id, i, new_value);
      gamepad->axes[i].value = new_value;
    }
  }

  return true;
}

void
WindowsGamepadService::Startup()
{
  ScanForDevices();
}

void
WindowsGamepadService::Shutdown()
{
  Cleanup();
}

void
WindowsGamepadService::Cleanup()
{
  mGamepads.Clear();
}

void
WindowsGamepadService::DevicesChanged(DeviceChangeType type)
{
  if (type == DeviceChangeNotification) {
    mObserver->SetDeviceChangeTimer();
  } else if (type == DeviceChangeStable) {
    ScanForDevices();
  }
}

NS_IMETHODIMP
Observer::Observe(nsISupports* aSubject,
                  const char* aTopic,
                  const char16_t* aData)
{
  if (strcmp(aTopic, "timer-callback") == 0) {
    mSvc.DevicesChanged(WindowsGamepadService::DeviceChangeStable);
  } else if (strcmp(aTopic, NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID) == 0) {
    Stop();
  }
  return NS_OK;
}

HWND sHWnd = nullptr;

bool
RegisterRawInput(HWND hwnd, bool enable)
{
  nsTArray<RAWINPUTDEVICE> rid(ArrayLength(kUsagePages));
  rid.SetLength(ArrayLength(kUsagePages));

  for (unsigned i = 0; i < rid.Length(); i++) {
    rid[i].usUsagePage = kUsagePages[i].usagePage;
    rid[i].usUsage = kUsagePages[i].usage;
    rid[i].dwFlags =
      enable ? RIDEV_EXINPUTSINK | RIDEV_DEVNOTIFY : RIDEV_REMOVE;
    rid[i].hwndTarget = hwnd;
  }

  if (!RegisterRawInputDevices(rid.Elements(), rid.Length(),
                               sizeof(RAWINPUTDEVICE))) {
    return false;
  }
  return true;
}

static
LRESULT CALLBACK
GamepadWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  const unsigned int DBT_DEVICEARRIVAL        = 0x8000;
  const unsigned int DBT_DEVICEREMOVECOMPLETE = 0x8004;
  const unsigned int DBT_DEVNODES_CHANGED     = 0x7;

  switch (msg) {
  case WM_DEVICECHANGE:
    if (wParam == DBT_DEVICEARRIVAL ||
        wParam == DBT_DEVICEREMOVECOMPLETE ||
        wParam == DBT_DEVNODES_CHANGED) {
      if (gService) {
        gService->DevicesChanged(WindowsGamepadService::DeviceChangeNotification);
      }
    }
    break;
  case WM_INPUT:
    if (gService) {
      gService->HandleRawInput(reinterpret_cast<HRAWINPUT>(lParam));
    }
    break;
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

} 

namespace mozilla {
namespace hal_impl {

void StartMonitoringGamepadStatus()
{
  if (gService) {
    return;
  }

  gService = new WindowsGamepadService();
  gService->Startup();

  if (sHWnd == nullptr) {
    WNDCLASSW wc;
    HMODULE hSelf = GetModuleHandle(nullptr);

    if (!GetClassInfoW(hSelf, L"MozillaGamepadClass", &wc)) {
      ZeroMemory(&wc, sizeof(WNDCLASSW));
      wc.hInstance = hSelf;
      wc.lpfnWndProc = GamepadWindowProc;
      wc.lpszClassName = L"MozillaGamepadClass";
      RegisterClassW(&wc);
    }

    sHWnd = CreateWindowW(L"MozillaGamepadClass", L"Gamepad Watcher",
                          0, 0, 0, 0, 0,
                          nullptr, nullptr, hSelf, nullptr);
    RegisterRawInput(sHWnd, true);
  }
}

void StopMonitoringGamepadStatus()
{
  if (!gService) {
    return;
  }

  if (sHWnd) {
    RegisterRawInput(sHWnd, false);
    DestroyWindow(sHWnd);
    sHWnd = nullptr;
  }

  gService->Shutdown();
  delete gService;
  gService = nullptr;
}

} 
} 

