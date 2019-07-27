
















#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/android_alarm.h>
#include <math.h>
#include <regex.h>
#include <sched.h>
#include <stdio.h>
#include <sys/klog.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

#include "mozilla/DebugOnly.h"

#include "android/log.h"
#include "cutils/properties.h"
#include "hardware/hardware.h"
#include "hardware/lights.h"
#include "hardware_legacy/uevent.h"
#include "hardware_legacy/vibrator.h"
#include "hardware_legacy/power.h"
#include "libdisplay/GonkDisplay.h"
#include "utils/threads.h"

#include "base/message_loop.h"

#include "Hal.h"
#include "HalImpl.h"
#include "HalLog.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/battery/Constants.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/FileUtils.h"
#include "mozilla/Monitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/Preferences.h"
#include "nsAlgorithm.h"
#include "nsPrintfCString.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIRecoveryService.h"
#include "nsIRunnable.h"
#include "nsScreenManagerGonk.h"
#include "nsThreadUtils.h"
#include "nsThreadUtils.h"
#include "nsIThread.h"
#include "nsXULAppAPI.h"
#include "OrientationObserver.h"
#include "UeventPoller.h"
#include "nsIWritablePropertyBag2.h"
#include <algorithm>

#define NsecPerMsec  1000000LL
#define NsecPerSec   1000000000




#ifndef OOM_DISABLE
#define OOM_DISABLE  (-17)
#endif

#ifndef OOM_ADJUST_MIN
#define OOM_ADJUST_MIN  (-16)
#endif

#ifndef OOM_ADJUST_MAX
#define OOM_ADJUST_MAX  15
#endif

#ifndef OOM_SCORE_ADJ_MIN
#define OOM_SCORE_ADJ_MIN  (-1000)
#endif

#ifndef OOM_SCORE_ADJ_MAX
#define OOM_SCORE_ADJ_MAX  1000
#endif

#ifndef BATTERY_CHARGING_ARGB
#define BATTERY_CHARGING_ARGB 0x00FF0000
#endif
#ifndef BATTERY_FULL_ARGB
#define BATTERY_FULL_ARGB 0x0000FF00
#endif

using namespace mozilla;
using namespace mozilla::hal;
using namespace mozilla::dom;

namespace mozilla {
namespace hal_impl {






enum LightType {
  eHalLightID_Backlight     = 0,
  eHalLightID_Keyboard      = 1,
  eHalLightID_Buttons       = 2,
  eHalLightID_Battery       = 3,
  eHalLightID_Notifications = 4,
  eHalLightID_Attention     = 5,
  eHalLightID_Bluetooth     = 6,
  eHalLightID_Wifi          = 7,
  eHalLightID_Count  
};
enum LightMode {
  eHalLightMode_User   = 0,  
  eHalLightMode_Sensor = 1,  
  eHalLightMode_Count
};
enum FlashMode {
  eHalLightFlash_None     = 0,
  eHalLightFlash_Timed    = 1,  
  eHalLightFlash_Hardware = 2,  
  eHalLightFlash_Count
};

struct LightConfiguration {
  LightType light;
  LightMode mode;
  FlashMode flash;
  uint32_t flashOnMS;
  uint32_t flashOffMS;
  uint32_t color;
};

static light_device_t* sLights[eHalLightID_Count]; 

static light_device_t*
GetDevice(hw_module_t* module, char const* name)
{
  int err;
  hw_device_t* device;
  err = module->methods->open(module, name, &device);
  if (err == 0) {
    return (light_device_t*)device;
  } else {
    return nullptr;
  }
}

static void
InitLights()
{
  
  
  if (!sLights[eHalLightID_Backlight]) {
    int err;
    hw_module_t* module;

    err = hw_get_module(LIGHTS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
    if (err == 0) {
      sLights[eHalLightID_Backlight]
             = GetDevice(module, LIGHT_ID_BACKLIGHT);
      sLights[eHalLightID_Keyboard]
             = GetDevice(module, LIGHT_ID_KEYBOARD);
      sLights[eHalLightID_Buttons]
             = GetDevice(module, LIGHT_ID_BUTTONS);
      sLights[eHalLightID_Battery]
             = GetDevice(module, LIGHT_ID_BATTERY);
      sLights[eHalLightID_Notifications]
             = GetDevice(module, LIGHT_ID_NOTIFICATIONS);
      sLights[eHalLightID_Attention]
             = GetDevice(module, LIGHT_ID_ATTENTION);
      sLights[eHalLightID_Bluetooth]
             = GetDevice(module, LIGHT_ID_BLUETOOTH);
      sLights[eHalLightID_Wifi]
             = GetDevice(module, LIGHT_ID_WIFI);
        }
    }
}





static light_state_t sStoredLightState[eHalLightID_Count];











static bool
SetLight(LightType light, const LightConfiguration& aConfig)
{
  light_state_t state;

  InitLights();

  if (light < 0 || light >= eHalLightID_Count ||
      sLights[light] == nullptr) {
    return false;
  }

  memset(&state, 0, sizeof(light_state_t));
  state.color = aConfig.color;
  state.flashMode = aConfig.flash;
  state.flashOnMS = aConfig.flashOnMS;
  state.flashOffMS = aConfig.flashOffMS;
  state.brightnessMode = aConfig.mode;

  sLights[light]->set_light(sLights[light], &state);
  sStoredLightState[light] = state;
  return true;
}





static bool
GetLight(LightType light, LightConfiguration* aConfig)
{
  light_state_t state;

  if (light < 0 || light >= eHalLightID_Count ||
      sLights[light] == nullptr) {
    return false;
  }

  memset(&state, 0, sizeof(light_state_t));
  state = sStoredLightState[light];

  aConfig->light = light;
  aConfig->color = state.color;
  aConfig->flash = FlashMode(state.flashMode);
  aConfig->flashOnMS = state.flashOnMS;
  aConfig->flashOffMS = state.flashOffMS;
  aConfig->mode = LightMode(state.brightnessMode);

  return true;
}

namespace {





class VibratorRunnable final
  : public nsIRunnable
  , public nsIObserver
{
public:
  VibratorRunnable()
    : mMonitor("VibratorRunnable")
    , mIndex(0)
  {
    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    if (!os) {
      NS_WARNING("Could not get observer service!");
      return;
    }

    os->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIOBSERVER

  
  void Vibrate(const nsTArray<uint32_t> &pattern);
  void CancelVibrate();

  static bool ShuttingDown() { return sShuttingDown; }

protected:
  ~VibratorRunnable() {}

private:
  Monitor mMonitor;

  
  nsTArray<uint32_t> mPattern;

  
  
  uint32_t mIndex;

  
  
  static bool sShuttingDown;
};

NS_IMPL_ISUPPORTS(VibratorRunnable, nsIRunnable, nsIObserver);

bool VibratorRunnable::sShuttingDown = false;

static StaticRefPtr<VibratorRunnable> sVibratorRunnable;

NS_IMETHODIMP
VibratorRunnable::Run()
{
  MonitorAutoLock lock(mMonitor);

  
  
  
  
  
  
  
  

  while (!sShuttingDown) {
    if (mIndex < mPattern.Length()) {
      uint32_t duration = mPattern[mIndex];
      if (mIndex % 2 == 0) {
        vibrator_on(duration);
      }
      mIndex++;
      mMonitor.Wait(PR_MillisecondsToInterval(duration));
    }
    else {
      mMonitor.Wait();
    }
  }
  sVibratorRunnable = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
VibratorRunnable::Observe(nsISupports *subject, const char *topic,
                          const char16_t *data)
{
  MOZ_ASSERT(strcmp(topic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0);
  MonitorAutoLock lock(mMonitor);
  sShuttingDown = true;
  mMonitor.Notify();

  return NS_OK;
}

void
VibratorRunnable::Vibrate(const nsTArray<uint32_t> &pattern)
{
  MonitorAutoLock lock(mMonitor);
  mPattern = pattern;
  mIndex = 0;
  mMonitor.Notify();
}

void
VibratorRunnable::CancelVibrate()
{
  MonitorAutoLock lock(mMonitor);
  mPattern.Clear();
  mPattern.AppendElement(0);
  mIndex = 0;
  mMonitor.Notify();
}

void
EnsureVibratorThreadInitialized()
{
  if (sVibratorRunnable) {
    return;
  }

  sVibratorRunnable = new VibratorRunnable();
  nsCOMPtr<nsIThread> thread;
  NS_NewThread(getter_AddRefs(thread), sVibratorRunnable);
}

} 

void
Vibrate(const nsTArray<uint32_t> &pattern, const hal::WindowIdentifier &)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (VibratorRunnable::ShuttingDown()) {
    return;
  }
  EnsureVibratorThreadInitialized();
  sVibratorRunnable->Vibrate(pattern);
}

void
CancelVibrate(const hal::WindowIdentifier &)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (VibratorRunnable::ShuttingDown()) {
    return;
  }
  EnsureVibratorThreadInitialized();
  sVibratorRunnable->CancelVibrate();
}

namespace {

class BatteryUpdater : public nsRunnable {
public:
  NS_IMETHOD Run()
  {
    hal::BatteryInformation info;
    hal_impl::GetCurrentBatteryInformation(&info);

    
    
    uint32_t color = 0; 
    if (info.charging() && (info.level() == 1)) {
      
      color = BATTERY_FULL_ARGB;
    } else if (info.charging() && (info.level() < 1)) {
      
      color = BATTERY_CHARGING_ARGB;
    } 

    LightConfiguration aConfig;
    aConfig.light = eHalLightID_Battery;
    aConfig.mode = eHalLightMode_User;
    aConfig.flash = eHalLightFlash_None;
    aConfig.flashOnMS = aConfig.flashOffMS = 0;
    aConfig.color = color;

    SetLight(eHalLightID_Battery, aConfig);

    hal::NotifyBatteryChange(info);

    {
      
      
      
      
      nsCOMPtr<nsIObserverService> obsService = mozilla::services::GetObserverService();
      nsCOMPtr<nsIWritablePropertyBag2> propbag =
        do_CreateInstance("@mozilla.org/hash-property-bag;1");
      if (obsService && propbag) {
        propbag->SetPropertyAsBool(NS_LITERAL_STRING("charging"),
                                   info.charging());
        propbag->SetPropertyAsDouble(NS_LITERAL_STRING("level"),
                                   info.level());

        obsService->NotifyObservers(propbag, "gonkhal-battery-notifier", nullptr);
      }
    }

    return NS_OK;
  }
};

} 

class BatteryObserver final : public IUeventObserver
{
public:
  NS_INLINE_DECL_REFCOUNTING(BatteryObserver)

  BatteryObserver()
    :mUpdater(new BatteryUpdater())
  {
  }

  virtual void Notify(const NetlinkEvent &aEvent)
  {
    
    NetlinkEvent *event = const_cast<NetlinkEvent*>(&aEvent);
    const char *subsystem = event->getSubsystem();
    
    const char *devpath = event->findParam("DEVPATH");
    if (strcmp(subsystem, "power_supply") == 0 &&
        strstr(devpath, "battery")) {
      
      NS_DispatchToMainThread(mUpdater);
    }
  }

protected:
  ~BatteryObserver() {}

private:
  nsRefPtr<BatteryUpdater> mUpdater;
};



static StaticRefPtr<BatteryObserver> sBatteryObserver;

static void
RegisterBatteryObserverIOThread()
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  MOZ_ASSERT(!sBatteryObserver);

  sBatteryObserver = new BatteryObserver();
  RegisterUeventListener(sBatteryObserver);
}

void
EnableBatteryNotifications()
{
  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(RegisterBatteryObserverIOThread));
}

static void
UnregisterBatteryObserverIOThread()
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  MOZ_ASSERT(sBatteryObserver);

  UnregisterUeventListener(sBatteryObserver);
  sBatteryObserver = nullptr;
}

void
DisableBatteryNotifications()
{
  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(UnregisterBatteryObserverIOThread));
}

static bool
GetCurrentBatteryCharge(int* aCharge)
{
  bool success = ReadSysFile("/sys/class/power_supply/battery/capacity",
                             aCharge);
  if (!success) {
    return false;
  }

  #ifdef DEBUG
  if ((*aCharge < 0) || (*aCharge > 100)) {
    HAL_LOG("charge level contains unknown value: %d", *aCharge);
  }
  #endif

  return (*aCharge >= 0) && (*aCharge <= 100);
}

static bool
GetCurrentBatteryCharging(int* aCharging)
{
  static const DebugOnly<int> BATTERY_NOT_CHARGING = 0;
  static const int BATTERY_CHARGING_USB = 1;
  static const int BATTERY_CHARGING_AC  = 2;

  

  int chargingSrc;
  bool success =
    ReadSysFile("/sys/class/power_supply/battery/charging_source", &chargingSrc);

  if (success) {
    #ifdef DEBUG
    if (chargingSrc != BATTERY_NOT_CHARGING &&
        chargingSrc != BATTERY_CHARGING_USB &&
        chargingSrc != BATTERY_CHARGING_AC) {
      HAL_LOG("charging_source contained unknown value: %d", chargingSrc);
    }
    #endif

    *aCharging = (chargingSrc == BATTERY_CHARGING_USB ||
                  chargingSrc == BATTERY_CHARGING_AC);
    return true;
  }

  

  char chargingSrcString[16];

  success = ReadSysFile("/sys/class/power_supply/battery/status",
                        chargingSrcString, sizeof(chargingSrcString));
  if (success) {
    *aCharging = strcmp(chargingSrcString, "Charging") == 0 ||
                 strcmp(chargingSrcString, "Full") == 0;
    return true;
  }

  return false;
}

void
GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo)
{
  int charge;
  static bool previousCharging = false;
  static double previousLevel = 0.0, remainingTime = 0.0;
  static struct timespec lastLevelChange;
  struct timespec now;
  double dtime, dlevel;

  if (GetCurrentBatteryCharge(&charge)) {
    aBatteryInfo->level() = (double)charge / 100.0;
  } else {
    aBatteryInfo->level() = dom::battery::kDefaultLevel;
  }

  int charging;

  if (GetCurrentBatteryCharging(&charging)) {
    aBatteryInfo->charging() = charging;
  } else {
    aBatteryInfo->charging() = true;
  }

  if (aBatteryInfo->charging() != previousCharging){
    aBatteryInfo->remainingTime() = dom::battery::kUnknownRemainingTime;
    memset(&lastLevelChange, 0, sizeof(struct timespec));
  }

  if (aBatteryInfo->charging()) {
    if (aBatteryInfo->level() == 1.0) {
      aBatteryInfo->remainingTime() = dom::battery::kDefaultRemainingTime;
    } else if (aBatteryInfo->level() != previousLevel){
      if (lastLevelChange.tv_sec != 0) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        dtime = now.tv_sec - lastLevelChange.tv_sec;
        dlevel = aBatteryInfo->level() - previousLevel;

        if (dlevel <= 0.0) {
          aBatteryInfo->remainingTime() = dom::battery::kUnknownRemainingTime;
        } else {
          remainingTime = (double) round(dtime / dlevel * (1.0 - aBatteryInfo->level()));
          aBatteryInfo->remainingTime() = remainingTime;
        }

        lastLevelChange = now;
      } else { 
        clock_gettime(CLOCK_MONOTONIC, &lastLevelChange);
        aBatteryInfo->remainingTime() = dom::battery::kUnknownRemainingTime;
      }

    } else {
      clock_gettime(CLOCK_MONOTONIC, &now);
      dtime = now.tv_sec - lastLevelChange.tv_sec;
      if (dtime < remainingTime) {
        aBatteryInfo->remainingTime() = round(remainingTime - dtime);
      } else {
        aBatteryInfo->remainingTime() = dom::battery::kUnknownRemainingTime;
      }

    }

  } else {
    aBatteryInfo->remainingTime() = dom::battery::kUnknownRemainingTime;
  }

  previousCharging = aBatteryInfo->charging();
  previousLevel = aBatteryInfo->level();
}

namespace {





bool WriteToFile(const char *filename, const char *toWrite)
{
  int fd = open(filename, O_WRONLY);
  ScopedClose autoClose(fd);
  if (fd < 0) {
    HAL_LOG("Unable to open file %s.", filename);
    return false;
  }

  if (write(fd, toWrite, strlen(toWrite)) < 0) {
    HAL_LOG("Unable to write to file %s.", filename);
    return false;
  }

  return true;
}




bool sScreenEnabled = true;





bool sCpuSleepAllowed = true;




int32_t sInternalLockCpuCount = 0;

} 

bool
GetScreenEnabled()
{
  return sScreenEnabled;
}

void
SetScreenEnabled(bool aEnabled)
{
  GetGonkDisplay()->SetEnabled(aEnabled);
  sScreenEnabled = aEnabled;
}

bool
GetKeyLightEnabled()
{
  LightConfiguration config;
  GetLight(eHalLightID_Buttons, &config);
  return (config.color != 0x00000000);
}

void
SetKeyLightEnabled(bool aEnabled)
{
  LightConfiguration config;
  config.mode = eHalLightMode_User;
  config.flash = eHalLightFlash_None;
  config.flashOnMS = config.flashOffMS = 0;
  config.color = 0x00000000;

  if (aEnabled) {
    
    
    
    double brightness = GetScreenBrightness();
    uint32_t val = static_cast<int>(round(brightness * 255.0));
    uint32_t color = (0xff<<24) + (val<<16) + (val<<8) + val;

    config.color = color;
  }

  SetLight(eHalLightID_Buttons, config);
  SetLight(eHalLightID_Keyboard, config);
}

double
GetScreenBrightness()
{
  LightConfiguration config;
  LightType light = eHalLightID_Backlight;

  GetLight(light, &config);
  
  int brightness = config.color & 0xFF;
  return brightness / 255.0;
}

void
SetScreenBrightness(double brightness)
{
  
  
  if (!(0 <= brightness && brightness <= 1)) {
    HAL_LOG("SetScreenBrightness: Dropping illegal brightness %f.", brightness);
    return;
  }

  
  
  uint32_t val = static_cast<int>(round(brightness * 255.0));
  uint32_t color = (0xff<<24) + (val<<16) + (val<<8) + val;

  LightConfiguration config;
  config.mode = eHalLightMode_User;
  config.flash = eHalLightFlash_None;
  config.flashOnMS = config.flashOffMS = 0;
  config.color = color;
  SetLight(eHalLightID_Backlight, config);
  if (GetKeyLightEnabled()) {
    SetLight(eHalLightID_Buttons, config);
    SetLight(eHalLightID_Keyboard, config);
  }
}

static Monitor* sInternalLockCpuMonitor = nullptr;

static void
UpdateCpuSleepState()
{
  const char *wakeLockFilename = "/sys/power/wake_lock";
  const char *wakeUnlockFilename = "/sys/power/wake_unlock";

  sInternalLockCpuMonitor->AssertCurrentThreadOwns();
  bool allowed = sCpuSleepAllowed && !sInternalLockCpuCount;
  WriteToFile(allowed ? wakeUnlockFilename : wakeLockFilename, "gecko");
}

static void
InternalLockCpu() {
  MonitorAutoLock monitor(*sInternalLockCpuMonitor);
  ++sInternalLockCpuCount;
  UpdateCpuSleepState();
}

static void
InternalUnlockCpu() {
  MonitorAutoLock monitor(*sInternalLockCpuMonitor);
  --sInternalLockCpuCount;
  UpdateCpuSleepState();
}

bool
GetCpuSleepAllowed()
{
  return sCpuSleepAllowed;
}

void
SetCpuSleepAllowed(bool aAllowed)
{
  MonitorAutoLock monitor(*sInternalLockCpuMonitor);
  sCpuSleepAllowed = aAllowed;
  UpdateCpuSleepState();
}

void
AdjustSystemClock(int64_t aDeltaMilliseconds)
{
  int fd;
  struct timespec now;

  if (aDeltaMilliseconds == 0) {
    return;
  }

  
  sched_yield();
  clock_gettime(CLOCK_REALTIME, &now);
  now.tv_sec += (time_t)(aDeltaMilliseconds / 1000LL);
  now.tv_nsec += (long)((aDeltaMilliseconds % 1000LL) * NsecPerMsec);
  if (now.tv_nsec >= NsecPerSec) {
    now.tv_sec += 1;
    now.tv_nsec -= NsecPerSec;
  }

  if (now.tv_nsec < 0) {
    now.tv_nsec += NsecPerSec;
    now.tv_sec -= 1;
  }

  do {
    fd = open("/dev/alarm", O_RDWR);
  } while (fd == -1 && errno == EINTR);
  ScopedClose autoClose(fd);
  if (fd < 0) {
    HAL_LOG("Failed to open /dev/alarm: %s", strerror(errno));
    return;
  }

  if (ioctl(fd, ANDROID_ALARM_SET_RTC, &now) < 0) {
    HAL_LOG("ANDROID_ALARM_SET_RTC failed: %s", strerror(errno));
  }

  hal::NotifySystemClockChange(aDeltaMilliseconds);
}

int32_t
GetTimezoneOffset()
{
  PRExplodedTime prTime;
  PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &prTime);

  
  int32_t offset = prTime.tm_params.tp_gmt_offset;
  offset += prTime.tm_params.tp_dst_offset;

  
  return -(offset / 60);
}

static int32_t sKernelTimezoneOffset = 0;

static void
UpdateKernelTimezone(int32_t timezoneOffset)
{
  if (sKernelTimezoneOffset == timezoneOffset) {
    return;
  }

  
  
  
  
  
  
  
  struct timezone tz;
  memset(&tz, 0, sizeof(tz));
  tz.tz_minuteswest = timezoneOffset;
  settimeofday(nullptr, &tz);
  sKernelTimezoneOffset = timezoneOffset;
}

void
SetTimezone(const nsCString& aTimezoneSpec)
{
  if (aTimezoneSpec.Equals(GetTimezone())) {
    
    
    
    
    UpdateKernelTimezone(GetTimezoneOffset());
    return;
  }

  int32_t oldTimezoneOffsetMinutes = GetTimezoneOffset();
  property_set("persist.sys.timezone", aTimezoneSpec.get());
  
  
  tzset();
  int32_t newTimezoneOffsetMinutes = GetTimezoneOffset();
  UpdateKernelTimezone(newTimezoneOffsetMinutes);
  hal::NotifySystemTimezoneChange(
    hal::SystemTimezoneChangeInformation(
      oldTimezoneOffsetMinutes, newTimezoneOffsetMinutes));
}

nsCString
GetTimezone()
{
  char timezone[32];
  property_get("persist.sys.timezone", timezone, "");
  return nsCString(timezone);
}

void
EnableSystemClockChangeNotifications()
{
}

void
DisableSystemClockChangeNotifications()
{
}

void
EnableSystemTimezoneChangeNotifications()
{
}

void
DisableSystemTimezoneChangeNotifications()
{
}



void
EnableScreenConfigurationNotifications()
{
}

void
DisableScreenConfigurationNotifications()
{
}

void
GetCurrentScreenConfiguration(hal::ScreenConfiguration* aScreenConfiguration)
{
  nsRefPtr<nsScreenGonk> screen = nsScreenManagerGonk::GetPrimaryScreen();
  *aScreenConfiguration = screen->GetConfiguration();
}

bool
LockScreenOrientation(const dom::ScreenOrientation& aOrientation)
{
  return OrientationObserver::GetInstance()->LockScreenOrientation(aOrientation);
}

void
UnlockScreenOrientation()
{
  OrientationObserver::GetInstance()->UnlockScreenOrientation();
}


static pthread_t sAlarmFireWatcherThread;


struct AlarmData {
public:
  AlarmData(int aFd) : mFd(aFd),
                       mGeneration(sNextGeneration++),
                       mShuttingDown(false) {}
  ScopedClose mFd;
  int mGeneration;
  bool mShuttingDown;

  static int sNextGeneration;

};

int AlarmData::sNextGeneration = 0;

AlarmData* sAlarmData = nullptr;

class AlarmFiredEvent : public nsRunnable {
public:
  AlarmFiredEvent(int aGeneration) : mGeneration(aGeneration) {}

  NS_IMETHOD Run() {
    
    
    if (sAlarmData && !sAlarmData->mShuttingDown &&
        mGeneration == sAlarmData->mGeneration) {
      hal::NotifyAlarmFired();
    }
    
    
    InternalUnlockCpu();
    return NS_OK;
  }

private:
  int mGeneration;
};


static void
DestroyAlarmData(void* aData)
{
  AlarmData* alarmData = static_cast<AlarmData*>(aData);
  delete alarmData;
}


void ShutDownAlarm(int aSigno)
{
  if (aSigno == SIGUSR1 && sAlarmData) {
    sAlarmData->mShuttingDown = true;
  }
  return;
}

static void*
WaitForAlarm(void* aData)
{
  pthread_cleanup_push(DestroyAlarmData, aData);

  AlarmData* alarmData = static_cast<AlarmData*>(aData);

  while (!alarmData->mShuttingDown) {
    int alarmTypeFlags = 0;

    
    
    
    
    do {
      alarmTypeFlags = ioctl(alarmData->mFd, ANDROID_ALARM_WAIT);
    } while (alarmTypeFlags < 0 && errno == EINTR &&
             !alarmData->mShuttingDown);

    if (!alarmData->mShuttingDown && alarmTypeFlags >= 0 &&
        (alarmTypeFlags & ANDROID_ALARM_RTC_WAKEUP_MASK)) {
      
      
      
      InternalLockCpu();
      nsRefPtr<AlarmFiredEvent> event =
        new AlarmFiredEvent(alarmData->mGeneration);
      NS_DispatchToMainThread(event);
    }
  }

  pthread_cleanup_pop(1);
  return nullptr;
}

bool
EnableAlarm()
{
  MOZ_ASSERT(!sAlarmData);

  int alarmFd = open("/dev/alarm", O_RDWR);
  if (alarmFd < 0) {
    HAL_LOG("Failed to open alarm device: %s.", strerror(errno));
    return false;
  }

  nsAutoPtr<AlarmData> alarmData(new AlarmData(alarmFd));

  struct sigaction actions;
  memset(&actions, 0, sizeof(actions));
  sigemptyset(&actions.sa_mask);
  actions.sa_flags = 0;
  actions.sa_handler = ShutDownAlarm;
  if (sigaction(SIGUSR1, &actions, nullptr)) {
    HAL_LOG("Failed to set SIGUSR1 signal for alarm-watcher thread.");
    return false;
  }

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  
  
  sInternalLockCpuMonitor = new Monitor("sInternalLockCpuMonitor");
  int status = pthread_create(&sAlarmFireWatcherThread, &attr, WaitForAlarm,
                              alarmData.get());
  if (status) {
    alarmData = nullptr;
    delete sInternalLockCpuMonitor;
    HAL_LOG("Failed to create alarm-watcher thread. Status: %d.", status);
    return false;
  }

  pthread_attr_destroy(&attr);

  
  sAlarmData = alarmData.forget();
  return true;
}

void
DisableAlarm()
{
  MOZ_ASSERT(sAlarmData);

  
  sAlarmData = nullptr;

  
  
  DebugOnly<int> err = pthread_kill(sAlarmFireWatcherThread, SIGUSR1);
  MOZ_ASSERT(!err);

  delete sInternalLockCpuMonitor;
}

bool
SetAlarm(int32_t aSeconds, int32_t aNanoseconds)
{
  if (!sAlarmData) {
    HAL_LOG("We should have enabled the alarm.");
    return false;
  }

  struct timespec ts;
  ts.tv_sec = aSeconds;
  ts.tv_nsec = aNanoseconds;

  
  const int result = ioctl(sAlarmData->mFd,
                           ANDROID_ALARM_SET(ANDROID_ALARM_RTC_WAKEUP), &ts);

  if (result < 0) {
    HAL_LOG("Unable to set alarm: %s.", strerror(errno));
    return false;
  }

  return true;
}

static int
OomAdjOfOomScoreAdj(int aOomScoreAdj)
{
  
  

  int adj;

  if (aOomScoreAdj < 0) {
    adj = (OOM_DISABLE * aOomScoreAdj) / OOM_SCORE_ADJ_MIN;
  } else {
    adj = (OOM_ADJUST_MAX * aOomScoreAdj) / OOM_SCORE_ADJ_MAX;
  }

  return adj;
}

static void
RoundOomScoreAdjUpWithLRU(int& aOomScoreAdj, uint32_t aLRU)
{
  
  
  aOomScoreAdj +=
    ceil(((float)OOM_SCORE_ADJ_MAX / OOM_ADJUST_MAX) * aLRU);
}

#define OOM_LOG(level, args...) __android_log_print(level, "OomLogger", ##args)
class OomVictimLogger final
  : public nsIObserver
{
public:
  OomVictimLogger()
    : mLastLineChecked(-1.0),
      mRegexes(nullptr)
  {
    
    WriteToFile("/sys/module/printk/parameters/time", "Y");
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

protected:
  ~OomVictimLogger() {}

private:
  double mLastLineChecked;
  ScopedFreePtr<regex_t> mRegexes;
};
NS_IMPL_ISUPPORTS(OomVictimLogger, nsIObserver);

NS_IMETHODIMP
OomVictimLogger::Observe(
  nsISupports* aSubject,
  const char* aTopic,
  const char16_t* aData)
{
  nsDependentCString event_type(aTopic);
  if (!event_type.EqualsLiteral("ipc:content-shutdown")) {
    return NS_OK;
  }

  
  const char* const regexes_raw[] = {
    ".*select.*to kill.*",
    ".*send sigkill to.*",
    ".*lowmem_shrink.*",
    ".*[Oo]ut of [Mm]emory.*",
    ".*[Kk]ill [Pp]rocess.*",
    ".*[Kk]illed [Pp]rocess.*",
    ".*oom-killer.*",
    
    
    
    "\\[ pid \\]   uid  tgid total_vm      rss cpu oom_adj oom_score_adj name",
    "\\[.*[0-9][0-9]*\\][ ]*[0-9][0-9]*[ ]*[0-9][0-9]*[ ]*[0-9][0-9]*[ ]*[0-9][0-9]*[ ]*[0-9][0-9]*[ ]*.[0-9][0-9]*[ ]*.[0-9][0-9]*.*"
  };
  const size_t regex_count = ArrayLength(regexes_raw);

  
  if (!mRegexes) {
    mRegexes = static_cast<regex_t*>(malloc(sizeof(regex_t) * regex_count));
    for (size_t i = 0; i < regex_count; i++) {
      int compilation_err = regcomp(&(mRegexes[i]), regexes_raw[i], REG_NOSUB);
      if (compilation_err) {
        OOM_LOG(ANDROID_LOG_ERROR, "Cannot compile regex \"%s\"\n", regexes_raw[i]);
        return NS_OK;
      }
    }
  }

#ifndef KLOG_SIZE_BUFFER
  
  
  
  
  
  
  
  #define KLOG_SIZE_BUFFER KLOG_WRITE
#endif
  
  int msg_buf_size = klogctl(KLOG_SIZE_BUFFER, NULL, 0);
  ScopedFreePtr<char> msg_buf(static_cast<char *>(malloc(msg_buf_size + 1)));
  int read_size = klogctl(KLOG_READ_ALL, msg_buf.rwget(), msg_buf_size);

  
  read_size = read_size > msg_buf_size ? msg_buf_size : read_size;
  msg_buf.rwget()[read_size] = '\0';

  
  char* line_end;
  char* line_begin = msg_buf.rwget();
  for (; (line_end = strchr(line_begin, '\n')); line_begin = line_end + 1) {
    
    *line_end = '\0';

    
    
    
    
    

    
    
    char*  timestamp_begin = strchr(line_begin, '[');
    char   after_float;
    double lineTimestamp = -1;
    bool   lineTimestampFound = false;
    if (timestamp_begin &&
         
         
         
         2 == sscanf(timestamp_begin, "[ %lf%c", &lineTimestamp, &after_float) &&
         after_float == ']') {
      if (lineTimestamp <= mLastLineChecked) {
        continue;
      }

      lineTimestampFound = true;
      mLastLineChecked = lineTimestamp;
    }

    
    for (size_t i = 0; i < regex_count; i++) {
      int matching = !regexec(&(mRegexes[i]), line_begin, 0, NULL, 0);
      if (matching) {
        
        
        
        char* endOfTimestamp = strchr(line_begin, ']');
        if (endOfTimestamp && endOfTimestamp[1] == ' ') {
          
          line_begin = endOfTimestamp + 2;
        }
        if (!lineTimestampFound) {
          OOM_LOG(ANDROID_LOG_WARN, "following kill message may be a duplicate");
        }
        OOM_LOG(ANDROID_LOG_ERROR, "[Kill]: %s\n", line_begin);
        break;
      }
    }
  }

  return NS_OK;
}







class PriorityClass
{
public:
  




  PriorityClass(ProcessPriority aPriority);

  



  ~PriorityClass();

  PriorityClass(const PriorityClass& aOther);
  PriorityClass& operator=(const PriorityClass& aOther);

  ProcessPriority Priority()
  {
    return mPriority;
  }

  int32_t OomScoreAdj()
  {
    return clamped<int32_t>(mOomScoreAdj, OOM_SCORE_ADJ_MIN, OOM_SCORE_ADJ_MAX);
  }

  int32_t KillUnderKB()
  {
    return mKillUnderKB;
  }

  nsCString CGroup()
  {
    return mGroup;
  }

  





  void AddProcess(int aPid);

private:
  ProcessPriority mPriority;
  int32_t mOomScoreAdj;
  int32_t mKillUnderKB;
  int mCpuCGroupProcsFd;
  int mMemCGroupProcsFd;
  nsCString mGroup;

  




  nsCString PriorityPrefName(const char* aPref)
  {
    return nsPrintfCString("hal.processPriorityManager.gonk.%s.%s",
                           ProcessPriorityToString(mPriority), aPref);
  }

  


  nsCString CpuCGroupProcsFilename()
  {
    nsCString cgroupName = mGroup;

    



    if (!mGroup.IsEmpty()) {
      cgroupName.AppendLiteral("/");
    }

    return NS_LITERAL_CSTRING("/dev/cpuctl/") + cgroupName +
           NS_LITERAL_CSTRING("cgroup.procs");
  }

  nsCString MemCGroupProcsFilename()
  {
    nsCString cgroupName = mGroup;

    



    if (!mGroup.IsEmpty()) {
      cgroupName.AppendLiteral("/");
    }

    return NS_LITERAL_CSTRING("/sys/fs/cgroup/memory/") + cgroupName +
           NS_LITERAL_CSTRING("cgroup.procs");
  }

  int OpenCpuCGroupProcs()
  {
    return open(CpuCGroupProcsFilename().get(), O_WRONLY);
  }

  int OpenMemCGroupProcs()
  {
    return open(MemCGroupProcsFilename().get(), O_WRONLY);
  }
};











static bool
EnsureCpuCGroupExists(const nsACString &aGroup)
{
  NS_NAMED_LITERAL_CSTRING(kDevCpuCtl, "/dev/cpuctl/");
  NS_NAMED_LITERAL_CSTRING(kSlash, "/");

  nsAutoCString groupName(aGroup);
  HAL_LOG("EnsureCpuCGroupExists for group '%s'", groupName.get());

  nsAutoCString prefPrefix("hal.processPriorityManager.gonk.cgroups.");

  

  if (!aGroup.IsEmpty()) {
    prefPrefix += aGroup + NS_LITERAL_CSTRING(".");
  }

  nsAutoCString cpuSharesPref(prefPrefix + NS_LITERAL_CSTRING("cpu_shares"));
  int cpuShares = Preferences::GetInt(cpuSharesPref.get());

  nsAutoCString cpuNotifyOnMigratePref(prefPrefix
    + NS_LITERAL_CSTRING("cpu_notify_on_migrate"));
  int cpuNotifyOnMigrate = Preferences::GetInt(cpuNotifyOnMigratePref.get());

  
  nsCString cgroupIter = aGroup + kSlash;

  int32_t offset = 0;
  while ((offset = cgroupIter.FindChar('/', offset)) != -1) {
    nsAutoCString path = kDevCpuCtl + Substring(cgroupIter, 0, offset);
    int rv = mkdir(path.get(), 0744);

    if (rv == -1 && errno != EEXIST) {
      HAL_LOG("Could not create the %s control group.", path.get());
      return false;
    }

    offset++;
  }
  HAL_LOG("EnsureCpuCGroupExists created group '%s'", groupName.get());

  nsAutoCString pathPrefix(kDevCpuCtl + aGroup + kSlash);
  nsAutoCString cpuSharesPath(pathPrefix + NS_LITERAL_CSTRING("cpu.shares"));
  if (cpuShares && !WriteToFile(cpuSharesPath.get(),
                                nsPrintfCString("%d", cpuShares).get())) {
    HAL_LOG("Could not set the cpu share for group %s", cpuSharesPath.get());
    return false;
  }

  nsAutoCString notifyOnMigratePath(pathPrefix
    + NS_LITERAL_CSTRING("cpu.notify_on_migrate"));
  if (!WriteToFile(notifyOnMigratePath.get(),
                   nsPrintfCString("%d", cpuNotifyOnMigrate).get())) {
    HAL_LOG("Could not set the cpu migration notification flag for group %s",
            notifyOnMigratePath.get());
    return false;
  }

  return true;
}

static bool
EnsureMemCGroupExists(const nsACString &aGroup)
{
  NS_NAMED_LITERAL_CSTRING(kMemCtl, "/sys/fs/cgroup/memory/");
  NS_NAMED_LITERAL_CSTRING(kSlash, "/");

  nsAutoCString groupName(aGroup);
  HAL_LOG("EnsureMemCGroupExists for group '%s'", groupName.get());

  nsAutoCString prefPrefix("hal.processPriorityManager.gonk.cgroups.");

  

  if (!aGroup.IsEmpty()) {
    prefPrefix += aGroup + NS_LITERAL_CSTRING(".");
  }

  nsAutoCString memSwappinessPref(prefPrefix + NS_LITERAL_CSTRING("memory_swappiness"));
  int memSwappiness = Preferences::GetInt(memSwappinessPref.get());

  
  nsCString cgroupIter = aGroup + kSlash;

  int32_t offset = 0;
  while ((offset = cgroupIter.FindChar('/', offset)) != -1) {
    nsAutoCString path = kMemCtl + Substring(cgroupIter, 0, offset);
    int rv = mkdir(path.get(), 0744);

    if (rv == -1 && errno != EEXIST) {
      HAL_LOG("Could not create the %s control group.", path.get());
      return false;
    }

    offset++;
  }
  HAL_LOG("EnsureMemCGroupExists created group '%s'", groupName.get());

  nsAutoCString pathPrefix(kMemCtl + aGroup + kSlash);
  nsAutoCString memSwappinessPath(pathPrefix + NS_LITERAL_CSTRING("memory.swappiness"));
  if (!WriteToFile(memSwappinessPath.get(),
                   nsPrintfCString("%d", memSwappiness).get())) {
    HAL_LOG("Could not set the memory.swappiness for group %s", memSwappinessPath.get());
    return false;
  }
  HAL_LOG("Set memory.swappiness for group %s to %d", memSwappinessPath.get(), memSwappiness);

  return true;
}

PriorityClass::PriorityClass(ProcessPriority aPriority)
  : mPriority(aPriority)
  , mOomScoreAdj(0)
  , mKillUnderKB(0)
  , mCpuCGroupProcsFd(-1)
  , mMemCGroupProcsFd(-1)
{
  DebugOnly<nsresult> rv;

  rv = Preferences::GetInt(PriorityPrefName("OomScoreAdjust").get(),
                           &mOomScoreAdj);
  MOZ_ASSERT(NS_SUCCEEDED(rv), "Missing oom_score_adj preference");

  rv = Preferences::GetInt(PriorityPrefName("KillUnderKB").get(),
                           &mKillUnderKB);

  rv = Preferences::GetCString(PriorityPrefName("cgroup").get(), &mGroup);
  MOZ_ASSERT(NS_SUCCEEDED(rv), "Missing control group preference");

  if (EnsureCpuCGroupExists(mGroup)) {
    mCpuCGroupProcsFd = OpenCpuCGroupProcs();
  }
  if (EnsureMemCGroupExists(mGroup)) {
    mMemCGroupProcsFd = OpenMemCGroupProcs();
  }
}

PriorityClass::~PriorityClass()
{
  close(mCpuCGroupProcsFd);
  close(mMemCGroupProcsFd);
}

PriorityClass::PriorityClass(const PriorityClass& aOther)
  : mPriority(aOther.mPriority)
  , mOomScoreAdj(aOther.mOomScoreAdj)
  , mKillUnderKB(aOther.mKillUnderKB)
  , mGroup(aOther.mGroup)
{
  mCpuCGroupProcsFd = OpenCpuCGroupProcs();
  mMemCGroupProcsFd = OpenMemCGroupProcs();
}

PriorityClass& PriorityClass::operator=(const PriorityClass& aOther)
{
  mPriority = aOther.mPriority;
  mOomScoreAdj = aOther.mOomScoreAdj;
  mKillUnderKB = aOther.mKillUnderKB;
  mGroup = aOther.mGroup;
  mCpuCGroupProcsFd = OpenCpuCGroupProcs();
  mMemCGroupProcsFd = OpenMemCGroupProcs();
  return *this;
}

void PriorityClass::AddProcess(int aPid)
{
  if (mCpuCGroupProcsFd >= 0) {
    nsPrintfCString str("%d", aPid);

    if (write(mCpuCGroupProcsFd, str.get(), strlen(str.get())) < 0) {
      HAL_ERR("Couldn't add PID %d to the %s cpu control group", aPid, mGroup.get());
    }
  }
  if (mMemCGroupProcsFd >= 0) {
    nsPrintfCString str("%d", aPid);

    if (write(mMemCGroupProcsFd, str.get(), strlen(str.get())) < 0) {
      HAL_ERR("Couldn't add PID %d to the %s memory control group", aPid, mGroup.get());
    }
  }
}









PriorityClass*
GetPriorityClass(ProcessPriority aPriority)
{
  static StaticAutoPtr<nsTArray<PriorityClass>> priorityClasses;

  
  
  if (!priorityClasses) {
    priorityClasses = new nsTArray<PriorityClass>();
    ClearOnShutdown(&priorityClasses);

    for (int32_t i = 0; i < NUM_PROCESS_PRIORITY; i++) {
      priorityClasses->AppendElement(PriorityClass(ProcessPriority(i)));
    }
  }

  if (aPriority < 0 ||
      static_cast<uint32_t>(aPriority) >= priorityClasses->Length()) {
    return nullptr;
  }

  return &(*priorityClasses)[aPriority];
}

static void
EnsureKernelLowMemKillerParamsSet()
{
  static bool kernelLowMemKillerParamsSet;
  if (kernelLowMemKillerParamsSet) {
    return;
  }
  kernelLowMemKillerParamsSet = true;

  HAL_LOG("Setting kernel's low-mem killer parameters.");

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  nsAutoCString adjParams;
  nsAutoCString minfreeParams;

  DebugOnly<int32_t> lowerBoundOfNextOomScoreAdj = OOM_SCORE_ADJ_MIN - 1;
  DebugOnly<int32_t> lowerBoundOfNextKillUnderKB = 0;
  int32_t countOfLowmemorykillerParametersSets = 0;

  long page_size = sysconf(_SC_PAGESIZE);

  for (int i = NUM_PROCESS_PRIORITY - 1; i >= 0; i--) {
    
    

    PriorityClass* pc = GetPriorityClass(static_cast<ProcessPriority>(i));

    int32_t oomScoreAdj = pc->OomScoreAdj();
    int32_t killUnderKB = pc->KillUnderKB();

    if (killUnderKB == 0) {
      
      
      
      continue;
    }

    
    
    MOZ_ASSERT(oomScoreAdj > lowerBoundOfNextOomScoreAdj);
    MOZ_ASSERT(killUnderKB > lowerBoundOfNextKillUnderKB);

    
    MOZ_ASSERT(countOfLowmemorykillerParametersSets < 6);

    
    adjParams.AppendPrintf("%d,", OomAdjOfOomScoreAdj(oomScoreAdj));

    
    minfreeParams.AppendPrintf("%ld,", killUnderKB * 1024 / page_size);

    lowerBoundOfNextOomScoreAdj = oomScoreAdj;
    lowerBoundOfNextKillUnderKB = killUnderKB;
    countOfLowmemorykillerParametersSets++;
  }

  
  adjParams.Cut(adjParams.Length() - 1, 1);
  minfreeParams.Cut(minfreeParams.Length() - 1, 1);
  if (!adjParams.IsEmpty() && !minfreeParams.IsEmpty()) {
    WriteToFile("/sys/module/lowmemorykiller/parameters/adj", adjParams.get());
    WriteToFile("/sys/module/lowmemorykiller/parameters/minfree",
                minfreeParams.get());
  }

  
  int32_t lowMemNotifyThresholdKB;
  if (NS_SUCCEEDED(Preferences::GetInt(
        "hal.processPriorityManager.gonk.notifyLowMemUnderKB",
        &lowMemNotifyThresholdKB))) {

    
    WriteToFile("/sys/module/lowmemorykiller/parameters/notify_trigger",
      nsPrintfCString("%ld", lowMemNotifyThresholdKB * 1024 / page_size).get());
  }

  
  nsRefPtr<OomVictimLogger> oomLogger = new OomVictimLogger();
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    os->AddObserver(oomLogger, "ipc:content-shutdown", false);
  }
}

void
SetProcessPriority(int aPid, ProcessPriority aPriority, uint32_t aLRU)
{
  HAL_LOG("SetProcessPriority(pid=%d, priority=%d, LRU=%u)",
          aPid, aPriority, aLRU);

  
  
  
  
  
  
  
  EnsureKernelLowMemKillerParamsSet();

  PriorityClass* pc = GetPriorityClass(aPriority);

  int oomScoreAdj = pc->OomScoreAdj();

  RoundOomScoreAdjUpWithLRU(oomScoreAdj, aLRU);

  
  
  if (!WriteToFile(nsPrintfCString("/proc/%d/oom_score_adj", aPid).get(),
                   nsPrintfCString("%d", oomScoreAdj).get()))
  {
    WriteToFile(nsPrintfCString("/proc/%d/oom_adj", aPid).get(),
                nsPrintfCString("%d", OomAdjOfOomScoreAdj(oomScoreAdj)).get());
  }

  HAL_LOG("Assigning pid %d to cgroup %s", aPid, pc->CGroup().get());
  pc->AddProcess(aPid);
}

static bool
IsValidRealTimePriority(int aValue, int aSchedulePolicy)
{
  return (aValue >= sched_get_priority_min(aSchedulePolicy)) &&
         (aValue <= sched_get_priority_max(aSchedulePolicy));
}

static void
SetThreadNiceValue(pid_t aTid, ThreadPriority aThreadPriority, int aValue)
{
  MOZ_ASSERT(aThreadPriority < NUM_THREAD_PRIORITY);
  MOZ_ASSERT(aThreadPriority >= 0);

  HAL_LOG("Setting thread %d to priority level %s; nice level %d",
          aTid, ThreadPriorityToString(aThreadPriority), aValue);
  int rv = setpriority(PRIO_PROCESS, aTid, aValue);

  if (rv) {
    HAL_LOG("Failed to set thread %d to priority level %s; error %s", aTid,
            ThreadPriorityToString(aThreadPriority), strerror(errno));
  }
}

static void
SetRealTimeThreadPriority(pid_t aTid,
                          ThreadPriority aThreadPriority,
                          int aValue)
{
  int policy = SCHED_FIFO;

  MOZ_ASSERT(aThreadPriority < NUM_THREAD_PRIORITY);
  MOZ_ASSERT(aThreadPriority >= 0);
  MOZ_ASSERT(IsValidRealTimePriority(aValue, policy), "Invalid real time priority");

  
  HAL_LOG("Setting thread %d to priority level %s; Real Time priority %d, "
          "Schedule FIFO", aTid, ThreadPriorityToString(aThreadPriority),
          aValue);
  sched_param schedParam;
  schedParam.sched_priority = aValue;
  int rv = sched_setscheduler(aTid, policy, &schedParam);

  if (rv) {
    HAL_LOG("Failed to set thread %d to real time priority level %s; error %s",
            aTid, ThreadPriorityToString(aThreadPriority), strerror(errno));
  }
}





struct ThreadPriorityPrefs {
  bool initialized;
  struct {
    int nice;
    int realTime;
  } priorities[NUM_THREAD_PRIORITY];
};






void
EnsureThreadPriorityPrefs(ThreadPriorityPrefs* prefs)
{
  if (prefs->initialized) {
    return;
  }

  for (int i = THREAD_PRIORITY_COMPOSITOR; i < NUM_THREAD_PRIORITY; i++) {
    ThreadPriority priority = static_cast<ThreadPriority>(i);

    
    const char* threadPriorityStr = ThreadPriorityToString(priority);
    nsPrintfCString niceStr("hal.gonk.%s.nice", threadPriorityStr);
    Preferences::AddIntVarCache(&prefs->priorities[i].nice, niceStr.get());

    
    nsPrintfCString realTimeStr("hal.gonk.%s.rt_priority", threadPriorityStr);
    Preferences::AddIntVarCache(&prefs->priorities[i].realTime,
                                realTimeStr.get());
  }

  prefs->initialized = true;
}

static void
SetThreadPriority(pid_t aTid, hal::ThreadPriority aThreadPriority)
{
  
  
  MOZ_ASSERT(NS_IsMainThread(), "Can only set thread priorities on main thread");
  MOZ_ASSERT(aThreadPriority >= 0);

  static ThreadPriorityPrefs prefs = { 0 };
  EnsureThreadPriorityPrefs(&prefs);

  switch (aThreadPriority) {
    case THREAD_PRIORITY_COMPOSITOR:
      break;
    default:
      HAL_ERR("Unrecognized thread priority %d; Doing nothing",
              aThreadPriority);
      return;
  }

  int realTimePriority = prefs.priorities[aThreadPriority].realTime;

  if (IsValidRealTimePriority(realTimePriority, SCHED_FIFO)) {
    SetRealTimeThreadPriority(aTid, aThreadPriority, realTimePriority);
    return;
  }

  SetThreadNiceValue(aTid, aThreadPriority,
                     prefs.priorities[aThreadPriority].nice);
}

namespace {









class SetThreadPriorityRunnable : public nsRunnable
{
public:
  SetThreadPriorityRunnable(pid_t aThreadId, hal::ThreadPriority aThreadPriority)
    : mThreadId(aThreadId)
    , mThreadPriority(aThreadPriority)
  { }

  NS_IMETHOD Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Can only set thread priorities on main thread");
    hal_impl::SetThreadPriority(mThreadId, mThreadPriority);
    return NS_OK;
  }

private:
  pid_t mThreadId;
  hal::ThreadPriority mThreadPriority;
};

} 

void
SetCurrentThreadPriority(ThreadPriority aThreadPriority)
{
  switch (aThreadPriority) {
    case THREAD_PRIORITY_COMPOSITOR: {
      pid_t threadId = gettid();
      nsCOMPtr<nsIRunnable> runnable =
        new SetThreadPriorityRunnable(threadId, aThreadPriority);
      NS_DispatchToMainThread(runnable);
      break;
    }
    default:
      HAL_LOG("Unrecognized thread priority %d; Doing nothing",
              aThreadPriority);
      return;
  }
}

void
FactoryReset(FactoryResetReason& aReason)
{
  nsCOMPtr<nsIRecoveryService> recoveryService =
    do_GetService("@mozilla.org/recovery-service;1");
  if (!recoveryService) {
    NS_WARNING("Could not get recovery service!");
    return;
  }

  if (aReason == FactoryResetReason::Wipe) {
    recoveryService->FactoryReset("wipe");
  } else if (aReason == FactoryResetReason::Root) {
    recoveryService->FactoryReset("root");
  } else {
    recoveryService->FactoryReset("normal");
  }
}

} 
} 
