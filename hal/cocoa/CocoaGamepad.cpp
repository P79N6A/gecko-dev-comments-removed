







#include "mozilla/dom/GamepadService.h"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDBase.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDManager.h>

#include <stdio.h>
#include <vector>

namespace {

using mozilla::dom::GamepadService;

using std::vector;

struct Button {
  int id;
  bool analog;
  IOHIDElementRef element;
  CFIndex min;
  CFIndex max;

  Button(int aId, IOHIDElementRef aElement, CFIndex aMin, CFIndex aMax) :
    id(aId),
    analog((aMax - aMin) > 1),
    element(aElement),
    min(aMin),
    max(aMax) {}
};

struct Axis {
  int id;
  IOHIDElementRef element;
  uint32_t usagePage;
  uint32_t usage;
  CFIndex min;
  CFIndex max;
};

typedef bool dpad_buttons[4];



const unsigned kDesktopUsagePage = 0x01;
const unsigned kSimUsagePage = 0x02;
const unsigned kAcceleratorUsage = 0xC4;
const unsigned kBrakeUsage = 0xC5;
const unsigned kJoystickUsage = 0x04;
const unsigned kGamepadUsage = 0x05;
const unsigned kAxisUsageMin = 0x30;
const unsigned kAxisUsageMax = 0x35;
const unsigned kDpadUsage = 0x39;
const unsigned kButtonUsagePage = 0x09;
const unsigned kConsumerPage = 0x0C;
const unsigned kHomeUsage = 0x223;
const unsigned kBackUsage = 0x224;


class Gamepad {
 private:
  IOHIDDeviceRef mDevice;
  nsTArray<Button> buttons;
  nsTArray<Axis> axes;
  IOHIDElementRef mDpad;
  dpad_buttons mDpadState;

 public:
  Gamepad() : mDevice(nullptr), mDpad(nullptr), mSuperIndex(-1) {}
  bool operator==(IOHIDDeviceRef device) const { return mDevice == device; }
  bool empty() const { return mDevice == nullptr; }
  void clear()
  {
    mDevice = nullptr;
    buttons.Clear();
    axes.Clear();
    mDpad = nullptr;
    mSuperIndex = -1;
  }
  void init(IOHIDDeviceRef device);
  size_t numButtons() { return buttons.Length() + (mDpad ? 4 : 0); }
  size_t numAxes() { return axes.Length(); }

  
  uint32_t mSuperIndex;

  const bool isDpad(IOHIDElementRef element) const
  {
    return element == mDpad;
  }

  const dpad_buttons& getDpadState() const
  {
    return mDpadState;
  }

  void setDpadState(const dpad_buttons& dpadState)
  {
    for (unsigned i = 0; i < ArrayLength(mDpadState); i++) {
      mDpadState[i] = dpadState[i];
    }
  }

  const Button* lookupButton(IOHIDElementRef element) const
  {
    for (unsigned i = 0; i < buttons.Length(); i++) {
      if (buttons[i].element == element)
        return &buttons[i];
    }
    return nullptr;
  }

  const Axis* lookupAxis(IOHIDElementRef element) const
  {
    for (unsigned i = 0; i < axes.Length(); i++) {
      if (axes[i].element == element)
        return &axes[i];
    }
    return nullptr;
  }
};

class AxisComparator {
public:
  bool Equals(const Axis& a1, const Axis& a2) const
  {
    return a1.usagePage == a2.usagePage && a1.usage == a2.usage;
  }
  bool LessThan(const Axis& a1, const Axis& a2) const
  {
    if (a1.usagePage == a2.usagePage) {
      return a1.usage < a2.usage;
    }
    return a1.usagePage < a2.usagePage;
  }
};

void Gamepad::init(IOHIDDeviceRef device)
{
  clear();
  mDevice = device;

  CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device,
                                                        nullptr,
                                                        kIOHIDOptionsTypeNone);
  CFIndex n = CFArrayGetCount(elements);
  for (CFIndex i = 0; i < n; i++) {
    IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements,
                                                                      i);
    uint32_t usagePage = IOHIDElementGetUsagePage(element);
    uint32_t usage = IOHIDElementGetUsage(element);

    if (usagePage == kDesktopUsagePage &&
        usage >= kAxisUsageMin &&
        usage <= kAxisUsageMax)
    {
      Axis axis = { int(axes.Length()),
                    element,
                    usagePage,
                    usage,
                    IOHIDElementGetLogicalMin(element),
                    IOHIDElementGetLogicalMax(element) };
      axes.AppendElement(axis);
    } else if (usagePage == kDesktopUsagePage && usage == kDpadUsage &&
               
               IOHIDElementGetLogicalMax(element) - IOHIDElementGetLogicalMin(element) == 7) {
      mDpad = element;
    } else if ((usagePage == kSimUsagePage &&
                 (usage == kAcceleratorUsage ||
                  usage == kBrakeUsage)) ||
               (usagePage == kButtonUsagePage) ||
               (usagePage == kConsumerPage &&
                 (usage == kHomeUsage ||
                  usage == kBackUsage))) {
      Button button(int(buttons.Length()), element, IOHIDElementGetLogicalMin(element), IOHIDElementGetLogicalMax(element));
      buttons.AppendElement(button);
    } else {
      
    }
  }

  AxisComparator comparator;
  axes.Sort(comparator);
  for (unsigned i = 0; i < axes.Length(); i++) {
    axes[i].id = i;
  }
}

class DarwinGamepadService {
 private:
  IOHIDManagerRef mManager;
  vector<Gamepad> mGamepads;

  static void DeviceAddedCallback(void* data, IOReturn result,
                                  void* sender, IOHIDDeviceRef device);
  static void DeviceRemovedCallback(void* data, IOReturn result,
                                    void* sender, IOHIDDeviceRef device);
  static void InputValueChangedCallback(void* data, IOReturn result,
                                        void* sender, IOHIDValueRef newValue);

  void DeviceAdded(IOHIDDeviceRef device);
  void DeviceRemoved(IOHIDDeviceRef device);
  void InputValueChanged(IOHIDValueRef value);

 public:
  DarwinGamepadService();
  ~DarwinGamepadService();
  void Startup();
  void Shutdown();
};

void
DarwinGamepadService::DeviceAdded(IOHIDDeviceRef device)
{
  size_t slot = size_t(-1);
  for (size_t i = 0; i < mGamepads.size(); i++) {
    if (mGamepads[i] == device)
      return;
    if (slot == size_t(-1) && mGamepads[i].empty())
      slot = i;
  }

  if (slot == size_t(-1)) {
    slot = mGamepads.size();
    mGamepads.push_back(Gamepad());
  }
  mGamepads[slot].init(device);

  
  CFNumberRef vendorIdRef =
    (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey));
  CFNumberRef productIdRef =
    (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey));
  CFStringRef productRef =
    (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
  int vendorId, productId;
  CFNumberGetValue(vendorIdRef, kCFNumberIntType, &vendorId);
  CFNumberGetValue(productIdRef, kCFNumberIntType, &productId);
  char product_name[128];
  CFStringGetCString(productRef, product_name,
                     sizeof(product_name), kCFStringEncodingASCII);
  char buffer[256];
  sprintf(buffer, "%x-%x-%s", vendorId, productId, product_name);
  nsRefPtr<GamepadService> service(GamepadService::GetService());
  mGamepads[slot].mSuperIndex = service->AddGamepad(buffer,
                                                    mozilla::dom::GamepadMappingType::_empty,
                                                    (int)mGamepads[slot].numButtons(),
                                                    (int)mGamepads[slot].numAxes());
}

void
DarwinGamepadService::DeviceRemoved(IOHIDDeviceRef device)
{
  nsRefPtr<GamepadService> service(GamepadService::GetService());
  for (size_t i = 0; i < mGamepads.size(); i++) {
    if (mGamepads[i] == device) {
      service->RemoveGamepad(mGamepads[i].mSuperIndex);
      mGamepads[i].clear();
      return;
    }
  }
}





static void
UnpackDpad(int dpad_value, int min, int max, dpad_buttons& buttons)
{
  const unsigned kUp = 0;
  const unsigned kDown = 1;
  const unsigned kLeft = 2;
  const unsigned kRight = 3;

  
  
  if (dpad_value < min || dpad_value > max) {
    
    return;
  }

  
  int value = dpad_value - min;

  
  
  
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

void
DarwinGamepadService::InputValueChanged(IOHIDValueRef value)
{
  uint32_t value_length = IOHIDValueGetLength(value);
  if (value_length > 4) {
    
    
    return;
  }
  nsRefPtr<GamepadService> service(GamepadService::GetService());
  IOHIDElementRef element = IOHIDValueGetElement(value);
  IOHIDDeviceRef device = IOHIDElementGetDevice(element);
  for (unsigned i = 0; i < mGamepads.size(); i++) {
    Gamepad &gamepad = mGamepads[i];
    if (gamepad == device) {
      if (gamepad.isDpad(element)) {
        const dpad_buttons& oldState = gamepad.getDpadState();
        dpad_buttons newState = { false, false, false, false };
        UnpackDpad(IOHIDValueGetIntegerValue(value),
                   IOHIDElementGetLogicalMin(element),
                   IOHIDElementGetLogicalMax(element),
                   newState);
        const int numButtons = gamepad.numButtons();
        for (unsigned b = 0; b < ArrayLength(newState); b++) {
          if (newState[b] != oldState[b]) {
            service->NewButtonEvent(i, numButtons - 4 + b, newState[b]);
          }
        }
        gamepad.setDpadState(newState);
      } else if (const Axis* axis = gamepad.lookupAxis(element)) {
        double d = IOHIDValueGetIntegerValue(value);
        double v = 2.0f * (d - axis->min) /
          (double)(axis->max - axis->min) - 1.0f;
        service->NewAxisMoveEvent(i, axis->id, v);
      } else if (const Button* button = gamepad.lookupButton(element)) {
        int iv = IOHIDValueGetIntegerValue(value);
        bool pressed = iv != 0;
        double v = 0;
        if (button->analog) {
          double dv = iv;
          v = (dv - button->min) / (double)(button->max - button->min);
        } else {
          v = pressed ? 1.0 : 0.0;
        }
        service->NewButtonEvent(i, button->id, pressed, v);
      }
      return;
    }
  }
}

void
DarwinGamepadService::DeviceAddedCallback(void* data, IOReturn result,
                                         void* sender, IOHIDDeviceRef device)
{
  DarwinGamepadService* service = (DarwinGamepadService*)data;
  service->DeviceAdded(device);
}

void
DarwinGamepadService::DeviceRemovedCallback(void* data, IOReturn result,
                                           void* sender, IOHIDDeviceRef device)
{
  DarwinGamepadService* service = (DarwinGamepadService*)data;
  service->DeviceRemoved(device);
}

void
DarwinGamepadService::InputValueChangedCallback(void* data,
                                               IOReturn result,
                                               void* sender,
                                               IOHIDValueRef newValue)
{
  DarwinGamepadService* service = (DarwinGamepadService*)data;
  service->InputValueChanged(newValue);
}

static CFMutableDictionaryRef
MatchingDictionary(UInt32 inUsagePage, UInt32 inUsage)
{
  CFMutableDictionaryRef dict =
    CFDictionaryCreateMutable(kCFAllocatorDefault,
                              0,
                              &kCFTypeDictionaryKeyCallBacks,
                              &kCFTypeDictionaryValueCallBacks);
  if (!dict)
    return nullptr;
  CFNumberRef number = CFNumberCreate(kCFAllocatorDefault,
                                      kCFNumberIntType,
                                      &inUsagePage);
  if (!number) {
    CFRelease(dict);
    return nullptr;
  }
  CFDictionarySetValue(dict, CFSTR(kIOHIDDeviceUsagePageKey), number);
  CFRelease(number);

  number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &inUsage);
  if (!number) {
    CFRelease(dict);
    return nullptr;
  }
  CFDictionarySetValue(dict, CFSTR(kIOHIDDeviceUsageKey), number);
  CFRelease(number);

  return dict;
}

DarwinGamepadService::DarwinGamepadService() : mManager(nullptr) {}

DarwinGamepadService::~DarwinGamepadService()
{
  if (mManager != nullptr)
    CFRelease(mManager);
}

void DarwinGamepadService::Startup()
{
  if (mManager != nullptr)
    return;

  IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault,
                                               kIOHIDOptionsTypeNone);

  CFMutableDictionaryRef criteria_arr[2];
  criteria_arr[0] = MatchingDictionary(kDesktopUsagePage,
                                       kJoystickUsage);
  if (!criteria_arr[0]) {
    CFRelease(manager);
    return;
  }

  criteria_arr[1] = MatchingDictionary(kDesktopUsagePage,
                                       kGamepadUsage);
  if (!criteria_arr[1]) {
    CFRelease(criteria_arr[0]);
    CFRelease(manager);
    return;
  }

  CFArrayRef criteria =
    CFArrayCreate(kCFAllocatorDefault, (const void**)criteria_arr, 2, nullptr);
  if (!criteria) {
    CFRelease(criteria_arr[1]);
    CFRelease(criteria_arr[0]);
    CFRelease(manager);
    return;
  }

  IOHIDManagerSetDeviceMatchingMultiple(manager, criteria);
  CFRelease(criteria);
  CFRelease(criteria_arr[1]);
  CFRelease(criteria_arr[0]);

  IOHIDManagerRegisterDeviceMatchingCallback(manager,
                                             DeviceAddedCallback,
                                             this);
  IOHIDManagerRegisterDeviceRemovalCallback(manager,
                                            DeviceRemovedCallback,
                                            this);
  IOHIDManagerRegisterInputValueCallback(manager,
                                         InputValueChangedCallback,
                                         this);
  IOHIDManagerScheduleWithRunLoop(manager,
                                  CFRunLoopGetCurrent(),
                                  kCFRunLoopDefaultMode);
  IOReturn rv = IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
  if (rv != kIOReturnSuccess) {
    CFRelease(manager);
    return;
  }

  mManager = manager;
}

void DarwinGamepadService::Shutdown()
{
  IOHIDManagerRef manager = (IOHIDManagerRef)mManager;
  if (manager) {
    IOHIDManagerClose(manager, 0);
    CFRelease(manager);
    mManager = nullptr;
  }
}

} 

namespace mozilla {
namespace hal_impl {

DarwinGamepadService* gService = nullptr;

void StartMonitoringGamepadStatus()
{
  if (gService)
    return;

  gService = new DarwinGamepadService();
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
