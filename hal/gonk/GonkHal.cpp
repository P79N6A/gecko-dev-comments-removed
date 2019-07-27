
















#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/android_alarm.h>
#include <math.h>
#include <regex.h>
#include <sched.h>
#include <stdio.h>
#include <sys/klog.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <time.h>
#include <asm/page.h>

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
#include "mozilla/ArrayUtils.h"
#include "mozilla/dom/battery/Constants.h"
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

struct LightConfiguration {
  hal::LightType light;
  hal::LightMode mode;
  hal::FlashMode flash;
  uint32_t flashOnMS;
  uint32_t flashOffMS;
  uint32_t color;
};

static light_device_t* sLights[hal::eHalLightID_Count]; 

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
  
  
  if (!sLights[hal::eHalLightID_Backlight]) {
    int err;
    hw_module_t* module;

    err = hw_get_module(LIGHTS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
    if (err == 0) {
      sLights[hal::eHalLightID_Backlight]
             = GetDevice(module, LIGHT_ID_BACKLIGHT);
      sLights[hal::eHalLightID_Keyboard]
             = GetDevice(module, LIGHT_ID_KEYBOARD);
      sLights[hal::eHalLightID_Buttons]
             = GetDevice(module, LIGHT_ID_BUTTONS);
      sLights[hal::eHalLightID_Battery]
             = GetDevice(module, LIGHT_ID_BATTERY);
      sLights[hal::eHalLightID_Notifications]
             = GetDevice(module, LIGHT_ID_NOTIFICATIONS);
      sLights[hal::eHalLightID_Attention]
             = GetDevice(module, LIGHT_ID_ATTENTION);
      sLights[hal::eHalLightID_Bluetooth]
             = GetDevice(module, LIGHT_ID_BLUETOOTH);
      sLights[hal::eHalLightID_Wifi]
             = GetDevice(module, LIGHT_ID_WIFI);
        }
    }
}





static light_state_t sStoredLightState[hal::eHalLightID_Count];











static bool
SetLight(hal::LightType light, const LightConfiguration& aConfig)
{
  light_state_t state;

  InitLights();

  if (light < 0 || light >= hal::eHalLightID_Count ||
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
GetLight(hal::LightType light, LightConfiguration* aConfig)
{
  light_state_t state;

  if (light < 0 || light >= hal::eHalLightID_Count ||
      sLights[light] == nullptr) {
    return false;
  }

  memset(&state, 0, sizeof(light_state_t));
  state = sStoredLightState[light];

  aConfig->light = light;
  aConfig->color = state.color;
  aConfig->flash = hal::FlashMode(state.flashMode);
  aConfig->flashOnMS = state.flashOnMS;
  aConfig->flashOffMS = state.flashOffMS;
  aConfig->mode = hal::LightMode(state.brightnessMode);

  return true;
}

namespace {





class VibratorRunnable
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
    aConfig.light = hal::eHalLightID_Battery;
    aConfig.mode = hal::eHalLightMode_User;
    aConfig.flash = hal::eHalLightFlash_None;
    aConfig.flashOnMS = aConfig.flashOffMS = 0;
    aConfig.color = color;

    SetLight(hal::eHalLightID_Battery, aConfig);

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

class BatteryObserver : public IUeventObserver
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
    HAL_LOG(("charge level contains unknown value: %d", *aCharge));
  }
  #endif

  return (*aCharge >= 0) && (*aCharge <= 100);
}

static bool
GetCurrentBatteryCharging(int* aCharging)
{
  static const int BATTERY_NOT_CHARGING = 0;
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
      HAL_LOG(("charging_source contained unknown value: %d", chargingSrc));
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

  if (!aBatteryInfo->charging() || (aBatteryInfo->level() < 1.0)) {
    aBatteryInfo->remainingTime() = dom::battery::kUnknownRemainingTime;
  } else {
    aBatteryInfo->remainingTime() = dom::battery::kDefaultRemainingTime;
  }
}

namespace {




const char *wakeLockFilename = "/sys/power/wake_lock";
const char *wakeUnlockFilename = "/sys/power/wake_unlock";

template<ssize_t n>
bool ReadFromFile(const char *filename, char (&buf)[n])
{
  int fd = open(filename, O_RDONLY);
  ScopedClose autoClose(fd);
  if (fd < 0) {
    HAL_LOG(("Unable to open file %s.", filename));
    return false;
  }

  ssize_t numRead = read(fd, buf, n);
  if (numRead < 0) {
    HAL_LOG(("Error reading from file %s.", filename));
    return false;
  }

  buf[std::min(numRead, n - 1)] = '\0';
  return true;
}

bool WriteToFile(const char *filename, const char *toWrite)
{
  int fd = open(filename, O_WRONLY);
  ScopedClose autoClose(fd);
  if (fd < 0) {
    HAL_LOG(("Unable to open file %s.", filename));
    return false;
  }

  if (write(fd, toWrite, strlen(toWrite)) < 0) {
    HAL_LOG(("Unable to write to file %s.", filename));
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
  GetLight(hal::eHalLightID_Buttons, &config);
  return (config.color != 0x00000000);
}

void
SetKeyLightEnabled(bool aEnabled)
{
  LightConfiguration config;
  config.mode = hal::eHalLightMode_User;
  config.flash = hal::eHalLightFlash_None;
  config.flashOnMS = config.flashOffMS = 0;
  config.color = 0x00000000;

  if (aEnabled) {
    
    
    
    double brightness = GetScreenBrightness();
    uint32_t val = static_cast<int>(round(brightness * 255.0));
    uint32_t color = (0xff<<24) + (val<<16) + (val<<8) + val;

    config.color = color;
  }

  SetLight(hal::eHalLightID_Buttons, config);
  SetLight(hal::eHalLightID_Keyboard, config);
}

double
GetScreenBrightness()
{
  LightConfiguration config;
  hal::LightType light = hal::eHalLightID_Backlight;

  GetLight(light, &config);
  
  int brightness = config.color & 0xFF;
  return brightness / 255.0;
}

void
SetScreenBrightness(double brightness)
{
  
  
  if (!(0 <= brightness && brightness <= 1)) {
    HAL_LOG(("SetScreenBrightness: Dropping illegal brightness %f.",
             brightness));
    return;
  }

  
  
  uint32_t val = static_cast<int>(round(brightness * 255.0));
  uint32_t color = (0xff<<24) + (val<<16) + (val<<8) + val;

  LightConfiguration config;
  config.mode = hal::eHalLightMode_User;
  config.flash = hal::eHalLightFlash_None;
  config.flashOnMS = config.flashOffMS = 0;
  config.color = color;
  SetLight(hal::eHalLightID_Backlight, config);
  if (GetKeyLightEnabled()) {
    SetLight(hal::eHalLightID_Buttons, config);
    SetLight(hal::eHalLightID_Keyboard, config);
  }
}

static Monitor* sInternalLockCpuMonitor = nullptr;

static void
UpdateCpuSleepState()
{
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
    HAL_LOG(("Failed to open /dev/alarm: %s", strerror(errno)));
    return;
  }

  if (ioctl(fd, ANDROID_ALARM_SET_RTC, &now) < 0) {
    HAL_LOG(("ANDROID_ALARM_SET_RTC failed: %s", strerror(errno)));
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
  *aScreenConfiguration = nsScreenGonk::GetConfiguration();
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
    HAL_LOG(("Failed to open alarm device: %s.", strerror(errno)));
    return false;
  }

  nsAutoPtr<AlarmData> alarmData(new AlarmData(alarmFd));

  struct sigaction actions;
  memset(&actions, 0, sizeof(actions));
  sigemptyset(&actions.sa_mask);
  actions.sa_flags = 0;
  actions.sa_handler = ShutDownAlarm;
  if (sigaction(SIGUSR1, &actions, nullptr)) {
    HAL_LOG(("Failed to set SIGUSR1 signal for alarm-watcher thread."));
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
    HAL_LOG(("Failed to create alarm-watcher thread. Status: %d.", status));
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
    HAL_LOG(("We should have enabled the alarm."));
    return false;
  }

  struct timespec ts;
  ts.tv_sec = aSeconds;
  ts.tv_nsec = aNanoseconds;

  
  const int result = ioctl(sAlarmData->mFd,
                           ANDROID_ALARM_SET(ANDROID_ALARM_RTC_WAKEUP), &ts);

  if (result < 0) {
    HAL_LOG(("Unable to set alarm: %s.", strerror(errno)));
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
RoundOomScoreAdjUpWithBackroundLRU(int& aOomScoreAdj, uint32_t aBackgroundLRU)
{
  
  
  aOomScoreAdj +=
    ceil(((float)OOM_SCORE_ADJ_MAX / OOM_ADJUST_MAX) * aBackgroundLRU);
}

#define OOM_LOG(level, args...) __android_log_print(level, "OomLogger", ##args)
class OomVictimLogger MOZ_FINAL
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
#else
  
  
  #warning "Please remove KLOG_UNREAD_SIZE compatability def"
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

static void
EnsureKernelLowMemKillerParamsSet()
{
  static bool kernelLowMemKillerParamsSet;
  if (kernelLowMemKillerParamsSet) {
    return;
  }
  kernelLowMemKillerParamsSet = true;

  HAL_LOG(("Setting kernel's low-mem killer parameters."));

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  nsAutoCString adjParams;
  nsAutoCString minfreeParams;

  int32_t lowerBoundOfNextOomScoreAdj = OOM_SCORE_ADJ_MIN - 1;
  int32_t lowerBoundOfNextKillUnderKB = 0;
  int32_t countOfLowmemorykillerParametersSets = 0;

  for (int i = NUM_PROCESS_PRIORITY - 1; i >= 0; i--) {
    
    

    ProcessPriority priority = static_cast<ProcessPriority>(i);

    int32_t oomScoreAdj;
    if (!NS_SUCCEEDED(Preferences::GetInt(
          nsPrintfCString("hal.processPriorityManager.gonk.%s.OomScoreAdjust",
                          ProcessPriorityToString(priority)).get(),
          &oomScoreAdj))) {
      MOZ_CRASH();
    }

    int32_t killUnderKB;
    if (!NS_SUCCEEDED(Preferences::GetInt(
          nsPrintfCString("hal.processPriorityManager.gonk.%s.KillUnderKB",
                          ProcessPriorityToString(priority)).get(),
          &killUnderKB))) {
      
      
      
      continue;
    }

    
    
    MOZ_ASSERT(oomScoreAdj > lowerBoundOfNextOomScoreAdj);
    MOZ_ASSERT(killUnderKB > lowerBoundOfNextKillUnderKB);

    
    MOZ_ASSERT(countOfLowmemorykillerParametersSets < 6);

    
    adjParams.AppendPrintf("%d,", OomAdjOfOomScoreAdj(oomScoreAdj));

    
    minfreeParams.AppendPrintf("%d,", killUnderKB * 1024 / PAGE_SIZE);

    lowerBoundOfNextOomScoreAdj = oomScoreAdj;
    lowerBoundOfNextKillUnderKB = killUnderKB;
    countOfLowmemorykillerParametersSets++;
  }

  
  adjParams.Cut(adjParams.Length() - 1, 1);
  minfreeParams.Cut(minfreeParams.Length() - 1, 1);
  if (!adjParams.IsEmpty() && !minfreeParams.IsEmpty()) {
    WriteToFile("/sys/module/lowmemorykiller/parameters/adj", adjParams.get());
    WriteToFile("/sys/module/lowmemorykiller/parameters/minfree", minfreeParams.get());
  }

  
  int32_t lowMemNotifyThresholdKB;
  if (NS_SUCCEEDED(Preferences::GetInt(
        "hal.processPriorityManager.gonk.notifyLowMemUnderKB",
        &lowMemNotifyThresholdKB))) {

    
    WriteToFile("/sys/module/lowmemorykiller/parameters/notify_trigger",
      nsPrintfCString("%d", lowMemNotifyThresholdKB * 1024 / PAGE_SIZE).get());
  }

  
  nsRefPtr<OomVictimLogger> oomLogger = new OomVictimLogger();
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    os->AddObserver(oomLogger, "ipc:content-shutdown", false);
  }
}

static void
SetNiceForPid(int aPid, int aNice)
{
  errno = 0;
  int origProcPriority = getpriority(PRIO_PROCESS, aPid);
  if (errno) {
    HAL_LOG(("Unable to get nice for pid=%d; error %d.  SetNiceForPid bailing.",
             aPid, errno));
    return;
  }

  int rv = setpriority(PRIO_PROCESS, aPid, aNice);
  if (rv) {
    HAL_LOG(("Unable to set nice for pid=%d; error %d.  SetNiceForPid bailing.",
             aPid, errno));
    return;
  }

  
  
  
  
  
  
  

  DIR* tasksDir = opendir(nsPrintfCString("/proc/%d/task/", aPid).get());
  if (!tasksDir) {
    HAL_LOG(("Unable to open /proc/%d/task.  SetNiceForPid bailing.", aPid));
    return;
  }

  

  while (struct dirent* de = readdir(tasksDir)) {
    char* endptr = nullptr;
    long tidlong = strtol(de->d_name, &endptr,  10);
    if (*endptr || tidlong < 0 || tidlong > INT32_MAX || tidlong == aPid) {
      
      
      
      
      
      
      continue;
    }

    int tid = static_cast<int>(tidlong);

    
    
    
    int schedPolicy = sched_getscheduler(tid);
    if (schedPolicy == SCHED_FIFO || schedPolicy == SCHED_RR) {
      continue;
    }

    errno = 0;
    
    int origtaskpriority = getpriority(PRIO_PROCESS, tid);
    if (errno) {
      HAL_LOG(("Unable to get nice for tid=%d (pid=%d); error %d.  This isn't "
               "necessarily a problem; it could be a benign race condition.",
               tid, aPid, errno));
      continue;
    }

    int newtaskpriority =
      std::max(origtaskpriority - origProcPriority + aNice, aNice);

    
    
    
    if (newtaskpriority > origtaskpriority &&
        origtaskpriority < ANDROID_PRIORITY_NORMAL) {
      continue;
    }

    rv = setpriority(PRIO_PROCESS, tid, newtaskpriority);

    if (rv) {
      HAL_LOG(("Unable to set nice for tid=%d (pid=%d); error %d.  This isn't "
               "necessarily a problem; it could be a benign race condition.",
               tid, aPid, errno));
      continue;
    }
  }

  HAL_LOG(("Changed nice for pid %d from %d to %d.",
           aPid, origProcPriority, aNice));

  closedir(tasksDir);
}

void
SetProcessPriority(int aPid,
                   ProcessPriority aPriority,
                   ProcessCPUPriority aCPUPriority,
                   uint32_t aBackgroundLRU)
{
  HAL_LOG(("SetProcessPriority(pid=%d, priority=%d, cpuPriority=%d, LRU=%u)",
           aPid, aPriority, aCPUPriority, aBackgroundLRU));

  
  
  
  
  
  
  
  EnsureKernelLowMemKillerParamsSet();

  int32_t oomScoreAdj = 0;
  nsresult rv = Preferences::GetInt(nsPrintfCString(
    "hal.processPriorityManager.gonk.%s.OomScoreAdjust",
    ProcessPriorityToString(aPriority)).get(), &oomScoreAdj);

  RoundOomScoreAdjUpWithBackroundLRU(oomScoreAdj, aBackgroundLRU);

  if (NS_SUCCEEDED(rv)) {
    int clampedOomScoreAdj = clamped<int>(oomScoreAdj, OOM_SCORE_ADJ_MIN,
                                                       OOM_SCORE_ADJ_MAX);
    if(clampedOomScoreAdj != oomScoreAdj) {
      HAL_LOG(("Clamping OOM adjustment for pid %d to %d", aPid,
               clampedOomScoreAdj));
    } else {
      HAL_LOG(("Setting OOM adjustment for pid %d to %d", aPid,
               clampedOomScoreAdj));
    }

    
    

    if (!WriteToFile(nsPrintfCString("/proc/%d/oom_score_adj", aPid).get(),
                     nsPrintfCString("%d", clampedOomScoreAdj).get()))
    {
      int oomAdj = OomAdjOfOomScoreAdj(clampedOomScoreAdj);

      WriteToFile(nsPrintfCString("/proc/%d/oom_adj", aPid).get(),
                  nsPrintfCString("%d", oomAdj).get());
    }
  } else {
    HAL_ERR(("Unable to read oom_score_adj pref for priority %s; "
             "are the prefs messed up?", ProcessPriorityToString(aPriority)));
    MOZ_ASSERT(false);
  }

  int32_t nice = 0;

  if (aCPUPriority == PROCESS_CPU_PRIORITY_NORMAL) {
    rv = Preferences::GetInt(
      nsPrintfCString("hal.processPriorityManager.gonk.%s.Nice",
                      ProcessPriorityToString(aPriority)).get(),
      &nice);
  } else if (aCPUPriority == PROCESS_CPU_PRIORITY_LOW) {
    rv = Preferences::GetInt("hal.processPriorityManager.gonk.LowCPUNice",
                             &nice);
  } else {
    HAL_ERR(("Unable to read niceness pref for priority %s; "
             "are the prefs messed up?", ProcessPriorityToString(aPriority)));
    MOZ_ASSERT(false);
    rv = NS_ERROR_FAILURE;
  }

  if (NS_SUCCEEDED(rv)) {
    HAL_LOG(("Setting nice for pid %d to %d", aPid, nice));
    SetNiceForPid(aPid, nice);
  }
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

  HAL_LOG(("Setting thread %d to priority level %s; nice level %d",
           aTid, ThreadPriorityToString(aThreadPriority), aValue));
  int rv = setpriority(PRIO_PROCESS, aTid, aValue);

  if (rv) {
    HAL_LOG(("Failed to set thread %d to priority level %s; error %s", aTid,
             ThreadPriorityToString(aThreadPriority), strerror(errno)));
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

  
  HAL_LOG(("Setting thread %d to priority level %s; Real Time priority %d, "
           "Schedule FIFO", aTid, ThreadPriorityToString(aThreadPriority),
           aValue));
  sched_param schedParam;
  schedParam.sched_priority = aValue;
  int rv = sched_setscheduler(aTid, policy, &schedParam);

  if (rv) {
    HAL_LOG(("Failed to set thread %d to real time priority level %s; error %s",
             aTid, ThreadPriorityToString(aThreadPriority), strerror(errno)));
  }
}

static void
SetThreadPriority(pid_t aTid, hal::ThreadPriority aThreadPriority)
{
  
  
  MOZ_ASSERT(NS_IsMainThread(), "Can only set thread priorities on main thread");
  MOZ_ASSERT(aThreadPriority >= 0);

  const char* threadPriorityStr;
  switch (aThreadPriority) {
    case THREAD_PRIORITY_COMPOSITOR:
      threadPriorityStr = ThreadPriorityToString(aThreadPriority);
      break;
    default:
      HAL_ERR(("Unrecognized thread priority %d; Doing nothing",
               aThreadPriority));
      return;
  }

  int realTimePriority = Preferences::GetInt(
    nsPrintfCString("hal.gonk.%s.rt_priority", threadPriorityStr).get());

  if (IsValidRealTimePriority(realTimePriority, SCHED_FIFO)) {
    SetRealTimeThreadPriority(aTid, aThreadPriority, realTimePriority);
    return;
  }

  int niceValue = Preferences::GetInt(
    nsPrintfCString("hal.gonk.%s.nice", threadPriorityStr).get());

  SetThreadNiceValue(aTid, aThreadPriority, niceValue);
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
      HAL_LOG(("Unrecognized thread priority %d; Doing nothing",
               aThreadPriority));
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
  } else {
    recoveryService->FactoryReset("normal");
  }
}

} 
} 
