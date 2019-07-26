
















#include <errno.h>
#include <fcntl.h>
#include <linux/android_alarm.h>
#include <math.h>
#include <stdio.h>
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

#include "base/message_loop.h"

#include "Hal.h"
#include "HalImpl.h"
#include "mozilla/dom/battery/Constants.h"
#include "mozilla/FileUtils.h"
#include "mozilla/Monitor.h"
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
#include <algorithm>

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args)
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

namespace mozilla {
namespace hal_impl {

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

  NS_DECL_ISUPPORTS
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

NS_IMPL_THREADSAFE_ISUPPORTS2(VibratorRunnable, nsIRunnable, nsIObserver);

bool VibratorRunnable::sShuttingDown = false;

static nsRefPtr<VibratorRunnable> sVibratorRunnable;

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
  sVibratorRunnable = NULL;
  return NS_OK;
}

NS_IMETHODIMP
VibratorRunnable::Observe(nsISupports *subject, const char *topic,
                          const PRUnichar *data)
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

    hal::LightConfiguration aConfig(hal::eHalLightID_Battery,
                                    hal::eHalLightMode_User,
                                    hal::eHalLightFlash_None,
                                    0,
                                    0,
                                    color);
    hal_impl::SetLight(hal::eHalLightID_Battery, aConfig);

    hal::NotifyBatteryChange(info);
    return NS_OK;
  }
};

} 

class BatteryObserver : public IUeventObserver,
                        public RefCounted<BatteryObserver>
{
public:
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
  sBatteryObserver = NULL;
}

void
DisableBatteryNotifications()
{
  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(UnregisterBatteryObserverIOThread));
}



static bool
ReadSysFile(const char *aFilename, char *aBuf, size_t aBufSize,
            size_t *aBytesRead = NULL)
{
  int fd = TEMP_FAILURE_RETRY(open(aFilename, O_RDONLY));
  if (fd < 0) {
    HAL_LOG(("Unable to open file '%s' for reading", aFilename));
    return false;
  }
  ScopedClose autoClose(fd);
  ssize_t bytesRead = TEMP_FAILURE_RETRY(read(fd, aBuf, aBufSize - 1));
  if (bytesRead < 0) {
    HAL_LOG(("Unable to read from file '%s'", aFilename));
    return false;
  }
  if (bytesRead && (aBuf[bytesRead - 1] == '\n')) {
    bytesRead--;
  }
  aBuf[bytesRead] = '\0';
  if (aBytesRead) {
    *aBytesRead = bytesRead;
  }
  return true;
}

static bool
ReadSysFile(const char *aFilename, int *aVal)
{
  char valBuf[20];
  if (!ReadSysFile(aFilename, valBuf, sizeof(valBuf))) {
    return false;
  }
  return sscanf(valBuf, "%d", aVal) == 1;
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
    HAL_LOG(("charge level containes unknown value: %d", *aCharge));
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
  size_t chargingSrcLen;

  success = ReadSysFile("/sys/class/power_supply/battery/status",
                        chargingSrcString, sizeof(chargingSrcString),
                        &chargingSrcLen);
  if (success) {
    *aCharging = !memcmp(chargingSrcString, "Charging", chargingSrcLen) ||
                 !memcmp(chargingSrcString, "Full", chargingSrcLen);
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

  if (aBatteryInfo->charging() && (aBatteryInfo->level() < 1.0)) {
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

} 

bool
GetScreenEnabled()
{
  return sScreenEnabled;
}

void
SetScreenEnabled(bool enabled)
{
  set_screen_state(enabled);
  sScreenEnabled = enabled;
}

double
GetScreenBrightness()
{
  hal::LightConfiguration aConfig;
  hal::LightType light = hal::eHalLightID_Backlight;

  hal::GetLight(light, &aConfig);
  
  int brightness = aConfig.color() & 0xFF;
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

  
  
  int val = static_cast<int>(round(brightness * 255));
  uint32_t color = (0xff<<24) + (val<<16) + (val<<8) + val;

  hal::LightConfiguration aConfig;
  aConfig.mode() = hal::eHalLightMode_User;
  aConfig.flash() = hal::eHalLightFlash_None;
  aConfig.flashOnMS() = aConfig.flashOffMS() = 0;
  aConfig.color() = color;
  hal::SetLight(hal::eHalLightID_Backlight, aConfig);
  hal::SetLight(hal::eHalLightID_Buttons, aConfig);
}

bool
GetCpuSleepAllowed()
{
  return sCpuSleepAllowed;
}

void
SetCpuSleepAllowed(bool aAllowed)
{
  WriteToFile(aAllowed ? wakeUnlockFilename : wakeLockFilename, "gecko");
  sCpuSleepAllowed = aAllowed;
}

static light_device_t* sLights[hal::eHalLightID_Count];	

light_device_t* GetDevice(hw_module_t* module, char const* name)
{
  int err;
  hw_device_t* device;
  err = module->methods->open(module, name, &device);
  if (err == 0) {
    return (light_device_t*)device;
  } else {
    return NULL;
  }
}

void
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

bool
SetLight(hal::LightType light, const hal::LightConfiguration& aConfig)
{
  light_state_t state;

  InitLights();

  if (light < 0 || light >= hal::eHalLightID_Count || sLights[light] == NULL) {
    return false;
  }

  memset(&state, 0, sizeof(light_state_t));
  state.color = aConfig.color();
  state.flashMode = aConfig.flash();
  state.flashOnMS = aConfig.flashOnMS();
  state.flashOffMS = aConfig.flashOffMS();
  state.brightnessMode = aConfig.mode();

  sLights[light]->set_light(sLights[light], &state);
  sStoredLightState[light] = state;
  return true;
}

bool
GetLight(hal::LightType light, hal::LightConfiguration* aConfig)
{
  light_state_t state;

#ifdef HAVEGETLIGHT
  InitLights();
#endif

  if (light < 0 || light >= hal::eHalLightID_Count || sLights[light] == NULL) {
    return false;
  }

  memset(&state, 0, sizeof(light_state_t));

#ifdef HAVEGETLIGHT
  sLights[light]->get_light(sLights[light], &state);
#else
  state = sStoredLightState[light];
#endif

  aConfig->light() = light;
  aConfig->color() = state.color;
  aConfig->flash() = hal::FlashMode(state.flashMode);
  aConfig->flashOnMS() = state.flashOnMS;
  aConfig->flashOffMS() = state.flashOffMS;
  aConfig->mode() = hal::LightMode(state.brightnessMode);

  return true;
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
    return;
  }

  hal::NotifySystemClockChange(aDeltaMilliseconds);
}

static int32_t
GetTimezoneOffset()
{
  PRExplodedTime prTime;
  PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &prTime);

  
  int32_t offset = prTime.tm_params.tp_gmt_offset;
  offset += prTime.tm_params.tp_dst_offset;

  
  return -(offset / 60);
}

void
SetTimezone(const nsCString& aTimezoneSpec)
{
  if (aTimezoneSpec.Equals(GetTimezone())) {
    return;
  }

  int32_t oldTimezoneOffsetMinutes = GetTimezoneOffset();
  property_set("persist.sys.timezone", aTimezoneSpec.get());
  
  
  tzset();
  int32_t newTimezoneOffsetMinutes = GetTimezoneOffset();
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


typedef struct AlarmData {

public:
  AlarmData(int aFd) : mFd(aFd), mGeneration(sNextGeneration++), mShuttingDown(false) {}
  ScopedClose mFd;
  int mGeneration;
  bool mShuttingDown;

  static int sNextGeneration;

} AlarmData;

int AlarmData::sNextGeneration = 0;

AlarmData* sAlarmData = NULL;

class AlarmFiredEvent : public nsRunnable {

public:
  AlarmFiredEvent(int aGeneration) : mGeneration(aGeneration) {}

  NS_IMETHOD Run() {
    
    
    if (sAlarmData && !sAlarmData->mShuttingDown && mGeneration == sAlarmData->mGeneration) {
      hal::NotifyAlarmFired();
    }

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
  if (aSigno == SIGUSR1) {
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
    } while (alarmTypeFlags < 0 && errno == EINTR && !alarmData->mShuttingDown);

    if (!alarmData->mShuttingDown &&
        alarmTypeFlags >= 0 && (alarmTypeFlags & ANDROID_ALARM_RTC_WAKEUP_MASK)) {
      NS_DispatchToMainThread(new AlarmFiredEvent(alarmData->mGeneration));
    }
  }

  pthread_cleanup_pop(1);
  return NULL;
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
  if (sigaction(SIGUSR1, &actions, NULL)) {
    HAL_LOG(("Failed to set SIGUSR1 signal for alarm-watcher thread."));
    return false;
  }

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  int status = pthread_create(&sAlarmFireWatcherThread, &attr, WaitForAlarm, alarmData.get());
  if (status) {
    alarmData = NULL;
    HAL_LOG(("Failed to create alarm watcher thread. Status: %d.", status));
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

  
  sAlarmData = NULL;

  
  
  DebugOnly<int> err = pthread_kill(sAlarmFireWatcherThread, SIGUSR1);
  MOZ_ASSERT(!err);
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

  
  const int result = ioctl(sAlarmData->mFd, ANDROID_ALARM_SET(ANDROID_ALARM_RTC_WAKEUP), &ts);

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

  const char* priorityClasses[] = {
    "master",
    "foregroundHigh",
    "foreground",
    "backgroundPerceivable",
    "backgroundHomescreen",
    "background"
  };
  for (size_t i = 0; i < NS_ARRAY_LENGTH(priorityClasses); i++) {
    
    

    int32_t oomScoreAdj;
    if (!NS_SUCCEEDED(Preferences::GetInt(nsPrintfCString(
          "hal.processPriorityManager.gonk.%sOomScoreAdjust",
          priorityClasses[i]).get(), &oomScoreAdj))) {
      MOZ_CRASH();
    }

    int32_t killUnderMB;
    if (!NS_SUCCEEDED(Preferences::GetInt(nsPrintfCString(
          "hal.processPriorityManager.gonk.%sKillUnderMB",
          priorityClasses[i]).get(), &killUnderMB))) {
      MOZ_CRASH();
    }

    
    adjParams.AppendPrintf("%d,", OomAdjOfOomScoreAdj(oomScoreAdj));

    
    minfreeParams.AppendPrintf("%d,", killUnderMB * 1024 * 1024 / PAGE_SIZE);
  }

  
  adjParams.Cut(adjParams.Length() - 1, 1);
  minfreeParams.Cut(minfreeParams.Length() - 1, 1);
  if (!adjParams.IsEmpty() && !minfreeParams.IsEmpty()) {
    WriteToFile("/sys/module/lowmemorykiller/parameters/adj", adjParams.get());
    WriteToFile("/sys/module/lowmemorykiller/parameters/minfree", minfreeParams.get());
  }

  
  int32_t lowMemNotifyThresholdMB;
  if (NS_SUCCEEDED(Preferences::GetInt(
        "hal.processPriorityManager.gonk.notifyLowMemUnderMB",
        &lowMemNotifyThresholdMB))) {

    
    WriteToFile("/sys/module/lowmemorykiller/parameters/notify_trigger",
      nsPrintfCString("%d", lowMemNotifyThresholdMB * 1024 * 1024 / PAGE_SIZE).get());
  }
}

void
SetProcessPriority(int aPid, ProcessPriority aPriority)
{
  HAL_LOG(("SetProcessPriority(pid=%d, priority=%d)", aPid, aPriority));

  
  
  
  
  
  
  
  EnsureKernelLowMemKillerParamsSet();

  const char* priorityStr = NULL;
  switch (aPriority) {
  case PROCESS_PRIORITY_BACKGROUND:
    priorityStr = "background";
    break;
  case PROCESS_PRIORITY_BACKGROUND_HOMESCREEN:
    priorityStr = "backgroundHomescreen";
    break;
  case PROCESS_PRIORITY_BACKGROUND_PERCEIVABLE:
    priorityStr = "backgroundPerceivable";
    break;
  case PROCESS_PRIORITY_FOREGROUND:
    priorityStr = "foreground";
    break;
  case PROCESS_PRIORITY_FOREGROUND_HIGH:
    priorityStr = "foregroundHigh";
    break;
  case PROCESS_PRIORITY_MASTER:
    priorityStr = "master";
    break;
  default:
    
    
    NS_ERROR("Invalid process priority!");
    return;
  }

  
  

  int32_t oomScoreAdj = 0;
  nsresult rv = Preferences::GetInt(nsPrintfCString(
    "hal.processPriorityManager.gonk.%sOomScoreAdjust",
    priorityStr).get(), &oomScoreAdj);

  if (NS_SUCCEEDED(rv)) {

    int clampedOomScoreAdj = clamped<int>(oomScoreAdj, OOM_SCORE_ADJ_MIN,
                                                       OOM_SCORE_ADJ_MAX);
    if(clampedOomScoreAdj != oomScoreAdj) {
      HAL_LOG(("Clamping OOM adjustment for pid %d to %d",
               aPid, clampedOomScoreAdj));
    } else {
      HAL_LOG(("Setting OOM adjustment for pid %d to %d",
               aPid, clampedOomScoreAdj));
    }

    
    

    if (!WriteToFile(nsPrintfCString("/proc/%d/oom_score_adj", aPid).get(),
                     nsPrintfCString("%d", clampedOomScoreAdj).get()))
    {
      int oomAdj = OomAdjOfOomScoreAdj(clampedOomScoreAdj);

      WriteToFile(nsPrintfCString("/proc/%d/oom_adj", aPid).get(),
                  nsPrintfCString("%d", oomAdj).get());
    }
  }

  int32_t nice = 0;
  rv = Preferences::GetInt(nsPrintfCString(
    "hal.processPriorityManager.gonk.%sNice", priorityStr).get(), &nice);
  if (NS_SUCCEEDED(rv)) {
    HAL_LOG(("Setting nice for pid %d to %d", aPid, nice));

    int success = setpriority(PRIO_PROCESS, aPid, nice);
    if (success != 0) {
      HAL_LOG(("Failed to set nice for pid %d to %d", aPid, nice));
    }
  }
}

void
FactoryReset()
{
  nsCOMPtr<nsIRecoveryService> recoveryService =
    do_GetService("@mozilla.org/recovery-service;1");
  if (!recoveryService) {
    NS_WARNING("Could not get recovery service!");
    return;
  }

  recoveryService->FactoryReset();
}

} 
} 
