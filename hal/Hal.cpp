





#include "Hal.h"
#include "HalImpl.h"
#include "HalSandbox.h"
#include "mozilla/Util.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "mozilla/Observer.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "mozilla/Services.h"
#include "nsIWebNavigation.h"
#include "nsITabChild.h"
#include "nsIDocShell.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/ClearOnShutdown.h"
#include "WindowIdentifier.h"
#include "mozilla/dom/ScreenOrientation.h"

#ifdef XP_WIN
#include <process.h>
#define getpid _getpid
#endif

using namespace mozilla::services;

#define PROXY_IF_SANDBOXED(_call)                 \
  do {                                            \
    if (InSandbox()) {                            \
      hal_sandbox::_call;                         \
    } else {                                      \
      hal_impl::_call;                            \
    }                                             \
  } while (0)

#define RETURN_PROXY_IF_SANDBOXED(_call)          \
  do {                                            \
    if (InSandbox()) {                            \
      return hal_sandbox::_call;                  \
    } else {                                      \
      return hal_impl::_call;                     \
    }                                             \
  } while (0)

namespace mozilla {
namespace hal {

PRLogModuleInfo *sHalLog = PR_LOG_DEFINE("hal");

namespace {

void
AssertMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
}

bool
InSandbox()
{
  return GeckoProcessType_Content == XRE_GetProcessType();
}

void
AssertMainProcess()
{
  MOZ_ASSERT(GeckoProcessType_Default == XRE_GetProcessType());
}

bool
WindowIsActive(nsIDOMWindow *window)
{
  NS_ENSURE_TRUE(window, false);

  nsCOMPtr<nsIDOMDocument> doc;
  window->GetDocument(getter_AddRefs(doc));
  NS_ENSURE_TRUE(doc, false);

  bool hidden = true;
  doc->GetMozHidden(&hidden);
  return !hidden;
}

StaticAutoPtr<WindowIdentifier::IDArrayType> gLastIDToVibrate;

void InitLastIDToVibrate()
{
  gLastIDToVibrate = new WindowIdentifier::IDArrayType();
  ClearOnShutdown(&gLastIDToVibrate);
}

} 

void
Vibrate(const nsTArray<uint32_t>& pattern, nsIDOMWindow* window)
{
  Vibrate(pattern, WindowIdentifier(window));
}

void
Vibrate(const nsTArray<uint32_t>& pattern, const WindowIdentifier &id)
{
  AssertMainThread();

  
  
  
  
  
  
  if (!id.HasTraveledThroughIPC() && !WindowIsActive(id.GetWindow())) {
    HAL_LOG(("Vibrate: Window is inactive, dropping vibrate."));
    return;
  }

  if (InSandbox()) {
    hal_sandbox::Vibrate(pattern, id);
  }
  else {
    if (!gLastIDToVibrate)
      InitLastIDToVibrate();
    *gLastIDToVibrate = id.AsArray();

    HAL_LOG(("Vibrate: Forwarding to hal_impl."));

    
    
    hal_impl::Vibrate(pattern, WindowIdentifier());
  }
}

void
CancelVibrate(nsIDOMWindow* window)
{
  CancelVibrate(WindowIdentifier(window));
}

void
CancelVibrate(const WindowIdentifier &id)
{
  AssertMainThread();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (InSandbox()) {
    hal_sandbox::CancelVibrate(id);
  }
  else if (gLastIDToVibrate && *gLastIDToVibrate == id.AsArray()) {
    
    
    
    HAL_LOG(("CancelVibrate: Forwarding to hal_impl."));
    hal_impl::CancelVibrate(WindowIdentifier());
  }
}

template <class InfoType>
class ObserversManager
{
public:
  void AddObserver(Observer<InfoType>* aObserver) {
    if (!mObservers) {
      mObservers = new mozilla::ObserverList<InfoType>();
    }

    mObservers->AddObserver(aObserver);

    if (mObservers->Length() == 1) {
      EnableNotifications();
    }
  }

  void RemoveObserver(Observer<InfoType>* aObserver) {
    bool removed = mObservers && mObservers->RemoveObserver(aObserver);
    if (!removed) {
      NS_WARNING("RemoveObserver() called for unregistered observer");
      return;
    }

    if (mObservers->Length() == 0) {
      DisableNotifications();

      OnNotificationsDisabled();

      delete mObservers;
      mObservers = nullptr;
    }
  }

  void BroadcastInformation(const InfoType& aInfo) {
    
    
    
    
    
    if (!mObservers) {
      return;
    }
    mObservers->Broadcast(aInfo);
  }

protected:
  virtual void EnableNotifications() = 0;
  virtual void DisableNotifications() = 0;
  virtual void OnNotificationsDisabled() {}

private:
  mozilla::ObserverList<InfoType>* mObservers;
};

template <class InfoType>
class CachingObserversManager : public ObserversManager<InfoType>
{
public:
  InfoType GetCurrentInformation() {
    if (mHasValidCache) {
      return mInfo;
    }

    GetCurrentInformationInternal(&mInfo);
    mHasValidCache = true;
    return mInfo;
  }

  void CacheInformation(const InfoType& aInfo) {
    mHasValidCache = true;
    mInfo = aInfo;
  }

  void BroadcastCachedInformation() {
    this->BroadcastInformation(mInfo);
  }

protected:
  virtual void GetCurrentInformationInternal(InfoType*) = 0;

  virtual void OnNotificationsDisabled() {
    mHasValidCache = false;
  }

private:
  InfoType                mInfo;
  bool                    mHasValidCache;
};

class BatteryObserversManager : public CachingObserversManager<BatteryInformation>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableBatteryNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableBatteryNotifications());
  }

  void GetCurrentInformationInternal(BatteryInformation* aInfo) {
    PROXY_IF_SANDBOXED(GetCurrentBatteryInformation(aInfo));
  }
};

static BatteryObserversManager sBatteryObservers;

class NetworkObserversManager : public CachingObserversManager<NetworkInformation>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableNetworkNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableNetworkNotifications());
  }

  void GetCurrentInformationInternal(NetworkInformation* aInfo) {
    PROXY_IF_SANDBOXED(GetCurrentNetworkInformation(aInfo));
  }
};

static NetworkObserversManager sNetworkObservers;

class WakeLockObserversManager : public ObserversManager<WakeLockInformation>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableWakeLockNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableWakeLockNotifications());
  }
};

static WakeLockObserversManager sWakeLockObservers;

class ScreenConfigurationObserversManager : public CachingObserversManager<ScreenConfiguration>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableScreenConfigurationNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableScreenConfigurationNotifications());
  }

  void GetCurrentInformationInternal(ScreenConfiguration* aInfo) {
    PROXY_IF_SANDBOXED(GetCurrentScreenConfiguration(aInfo));
  }
};

static ScreenConfigurationObserversManager sScreenConfigurationObservers;

void
RegisterBatteryObserver(BatteryObserver* aObserver)
{
  AssertMainThread();
  sBatteryObservers.AddObserver(aObserver);
}

void
UnregisterBatteryObserver(BatteryObserver* aObserver)
{
  AssertMainThread();
  sBatteryObservers.RemoveObserver(aObserver);
}

void
GetCurrentBatteryInformation(BatteryInformation* aInfo)
{
  AssertMainThread();
  *aInfo = sBatteryObservers.GetCurrentInformation();
}

void
NotifyBatteryChange(const BatteryInformation& aInfo)
{
  AssertMainThread();
  sBatteryObservers.CacheInformation(aInfo);
  sBatteryObservers.BroadcastCachedInformation();
}

bool GetScreenEnabled()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetScreenEnabled());
}

void SetScreenEnabled(bool enabled)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetScreenEnabled(enabled));
}

bool GetCpuSleepAllowed()
{
  
  
  
  
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetCpuSleepAllowed());
}

void SetCpuSleepAllowed(bool allowed)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetCpuSleepAllowed(allowed));
}

double GetScreenBrightness()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetScreenBrightness());
}

void SetScreenBrightness(double brightness)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetScreenBrightness(clamped(brightness, 0.0, 1.0)));
}

bool SetLight(LightType light, const hal::LightConfiguration& aConfig)
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(SetLight(light, aConfig));
}

bool GetLight(LightType light, hal::LightConfiguration* aConfig)
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetLight(light, aConfig));
}

class SystemTimeObserversManager : public ObserversManager<SystemTimeChange>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableSystemTimeChangeNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableSystemTimeChangeNotifications());
  }
};

static SystemTimeObserversManager sSystemTimeObservers;

void
RegisterSystemTimeChangeObserver(SystemTimeObserver *aObserver)
{
  AssertMainThread();
  sSystemTimeObservers.AddObserver(aObserver);
}

void
UnregisterSystemTimeChangeObserver(SystemTimeObserver *aObserver)
{
  AssertMainThread();
  sSystemTimeObservers.RemoveObserver(aObserver);
}

void
NotifySystemTimeChange(const hal::SystemTimeChange& aReason)
{
  sSystemTimeObservers.BroadcastInformation(aReason);
}
 
void 
AdjustSystemClock(int64_t aDeltaMilliseconds)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(AdjustSystemClock(aDeltaMilliseconds));
}

void 
SetTimezone(const nsCString& aTimezoneSpec)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetTimezone(aTimezoneSpec));
}

nsCString
GetTimezone()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetTimezone());
}

void
EnableSensorNotifications(SensorType aSensor) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(EnableSensorNotifications(aSensor));
}

void
DisableSensorNotifications(SensorType aSensor) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(DisableSensorNotifications(aSensor));
}

typedef mozilla::ObserverList<SensorData> SensorObserverList;
static SensorObserverList* gSensorObservers = nullptr;

static SensorObserverList &
GetSensorObservers(SensorType sensor_type) {
  MOZ_ASSERT(sensor_type < NUM_SENSOR_TYPE);
  
  if(!gSensorObservers) {
    gSensorObservers = new SensorObserverList[NUM_SENSOR_TYPE];
  }
  return gSensorObservers[sensor_type];
}

void
RegisterSensorObserver(SensorType aSensor, ISensorObserver *aObserver) {
  SensorObserverList &observers = GetSensorObservers(aSensor);

  AssertMainThread();
  
  observers.AddObserver(aObserver);
  if(observers.Length() == 1) {
    EnableSensorNotifications(aSensor);
  }
}

void
UnregisterSensorObserver(SensorType aSensor, ISensorObserver *aObserver) {
  AssertMainThread();

  if (!gSensorObservers) {
    return;
  }

  SensorObserverList &observers = GetSensorObservers(aSensor);
  if (!observers.RemoveObserver(aObserver) || observers.Length() > 0) {
    return;
  }
  DisableSensorNotifications(aSensor);

  
  for (int i = 0; i < NUM_SENSOR_TYPE; i++) {
    if (gSensorObservers[i].Length() > 0) {
      return;
    }
  }
  delete [] gSensorObservers;
  gSensorObservers = nullptr;
}

void
NotifySensorChange(const SensorData &aSensorData) {
  SensorObserverList &observers = GetSensorObservers(aSensorData.sensor());

  AssertMainThread();
  
  observers.Broadcast(aSensorData);
}

void
RegisterNetworkObserver(NetworkObserver* aObserver)
{
  AssertMainThread();
  sNetworkObservers.AddObserver(aObserver);
}

void
UnregisterNetworkObserver(NetworkObserver* aObserver)
{
  AssertMainThread();
  sNetworkObservers.RemoveObserver(aObserver);
}

void
GetCurrentNetworkInformation(NetworkInformation* aInfo)
{
  AssertMainThread();
  *aInfo = sNetworkObservers.GetCurrentInformation();
}

void
NotifyNetworkChange(const NetworkInformation& aInfo)
{
  sNetworkObservers.CacheInformation(aInfo);
  sNetworkObservers.BroadcastCachedInformation();
}

void Reboot()
{
  AssertMainProcess();
  AssertMainThread();
  PROXY_IF_SANDBOXED(Reboot());
}

void PowerOff()
{
  AssertMainProcess();
  AssertMainThread();
  PROXY_IF_SANDBOXED(PowerOff());
}

void StartForceQuitWatchdog(ShutdownMode aMode, int32_t aTimeoutSecs)
{
  AssertMainProcess();
  AssertMainThread();
  PROXY_IF_SANDBOXED(StartForceQuitWatchdog(aMode, aTimeoutSecs));
}

void
RegisterWakeLockObserver(WakeLockObserver* aObserver)
{
  AssertMainThread();
  sWakeLockObservers.AddObserver(aObserver);
}

void
UnregisterWakeLockObserver(WakeLockObserver* aObserver)
{
  AssertMainThread();
  sWakeLockObservers.RemoveObserver(aObserver);
}

void
ModifyWakeLock(const nsAString &aTopic,
               hal::WakeLockControl aLockAdjust,
               hal::WakeLockControl aHiddenAdjust)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(ModifyWakeLock(aTopic, aLockAdjust, aHiddenAdjust));
}

void
GetWakeLockInfo(const nsAString &aTopic, WakeLockInformation *aWakeLockInfo)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(GetWakeLockInfo(aTopic, aWakeLockInfo));
}

void
NotifyWakeLockChange(const WakeLockInformation& aInfo)
{
  AssertMainThread();
  sWakeLockObservers.BroadcastInformation(aInfo);
}

void
RegisterScreenConfigurationObserver(ScreenConfigurationObserver* aObserver)
{
  AssertMainThread();
  sScreenConfigurationObservers.AddObserver(aObserver);
}

void
UnregisterScreenConfigurationObserver(ScreenConfigurationObserver* aObserver)
{
  AssertMainThread();
  sScreenConfigurationObservers.RemoveObserver(aObserver);
}

void
GetCurrentScreenConfiguration(ScreenConfiguration* aScreenConfiguration)
{
  AssertMainThread();
  *aScreenConfiguration = sScreenConfigurationObservers.GetCurrentInformation();
}

void
NotifyScreenConfigurationChange(const ScreenConfiguration& aScreenConfiguration)
{
  sScreenConfigurationObservers.CacheInformation(aScreenConfiguration);
  sScreenConfigurationObservers.BroadcastCachedInformation();
}

bool
LockScreenOrientation(const dom::ScreenOrientation& aOrientation)
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(LockScreenOrientation(aOrientation));
}

void
UnlockScreenOrientation()
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(UnlockScreenOrientation());
}

void
EnableSwitchNotifications(hal::SwitchDevice aDevice) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(EnableSwitchNotifications(aDevice));
}

void
DisableSwitchNotifications(hal::SwitchDevice aDevice) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(DisableSwitchNotifications(aDevice));
}

hal::SwitchState GetCurrentSwitchState(hal::SwitchDevice aDevice)
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetCurrentSwitchState(aDevice));
}

typedef mozilla::ObserverList<SwitchEvent> SwitchObserverList;

static SwitchObserverList *sSwitchObserverLists = NULL;

static SwitchObserverList&
GetSwitchObserverList(hal::SwitchDevice aDevice) {
  MOZ_ASSERT(0 <= aDevice && aDevice < NUM_SWITCH_DEVICE); 
  if (sSwitchObserverLists == NULL) {
    sSwitchObserverLists = new SwitchObserverList[NUM_SWITCH_DEVICE];
  }
  return sSwitchObserverLists[aDevice];
}

static void
ReleaseObserversIfNeeded() {
  for (int i = 0; i < NUM_SWITCH_DEVICE; i++) {
    if (sSwitchObserverLists[i].Length() != 0)
      return;
  }

  
  delete [] sSwitchObserverLists;
  sSwitchObserverLists = NULL;
}

void
RegisterSwitchObserver(hal::SwitchDevice aDevice, hal::SwitchObserver *aObserver)
{
  AssertMainThread();
  SwitchObserverList& observer = GetSwitchObserverList(aDevice);
  observer.AddObserver(aObserver);
  if (observer.Length() == 1) {
    EnableSwitchNotifications(aDevice);
  }
}

void
UnregisterSwitchObserver(hal::SwitchDevice aDevice, hal::SwitchObserver *aObserver)
{
  AssertMainThread();

  if (!sSwitchObserverLists) {
    return;
  }

  SwitchObserverList& observer = GetSwitchObserverList(aDevice);
  if (!observer.RemoveObserver(aObserver) || observer.Length() > 0) {
    return;
  }

  DisableSwitchNotifications(aDevice);
  ReleaseObserversIfNeeded();
}

void
NotifySwitchChange(const hal::SwitchEvent& aEvent)
{
  
  
  if (!sSwitchObserverLists)
    return;

  SwitchObserverList& observer = GetSwitchObserverList(aEvent.device());
  observer.Broadcast(aEvent);
}

static AlarmObserver* sAlarmObserver;

bool
RegisterTheOneAlarmObserver(AlarmObserver* aObserver)
{
  MOZ_ASSERT(!InSandbox());
  MOZ_ASSERT(!sAlarmObserver);

  sAlarmObserver = aObserver;
  RETURN_PROXY_IF_SANDBOXED(EnableAlarm());
}

void
UnregisterTheOneAlarmObserver()
{
  if (sAlarmObserver) {
    sAlarmObserver = nullptr;
    PROXY_IF_SANDBOXED(DisableAlarm());
  }
}

void
NotifyAlarmFired()
{
  if (sAlarmObserver) {
    sAlarmObserver->Notify(void_t());
  }
}

bool
SetAlarm(int32_t aSeconds, int32_t aNanoseconds)
{
  
  MOZ_ASSERT(sAlarmObserver);
  RETURN_PROXY_IF_SANDBOXED(SetAlarm(aSeconds, aNanoseconds));
}

void
SetProcessPriority(int aPid, ProcessPriority aPriority)
{
  PROXY_IF_SANDBOXED(SetProcessPriority(aPid, aPriority));
}

static StaticAutoPtr<ObserverList<FMRadioOperationInformation> > sFMRadioObservers;

static void
InitializeFMRadioObserver()
{
  if (!sFMRadioObservers) {
    sFMRadioObservers = new ObserverList<FMRadioOperationInformation>;
    ClearOnShutdown(&sFMRadioObservers);
  }
}

void
RegisterFMRadioObserver(FMRadioObserver* aFMRadioObserver) {
  AssertMainThread();
  InitializeFMRadioObserver();
  sFMRadioObservers->AddObserver(aFMRadioObserver);
}

void
UnregisterFMRadioObserver(FMRadioObserver* aFMRadioObserver) {
  AssertMainThread();
  InitializeFMRadioObserver();
  sFMRadioObservers->RemoveObserver(aFMRadioObserver);
}

void
NotifyFMRadioStatus(const FMRadioOperationInformation& aFMRadioState) {
  InitializeFMRadioObserver();
  sFMRadioObservers->Broadcast(aFMRadioState);
}

void
EnableFMRadio(const FMRadioSettings& aInfo) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(EnableFMRadio(aInfo));
}

void
DisableFMRadio() {
  AssertMainThread();
  PROXY_IF_SANDBOXED(DisableFMRadio());
}

void
FMRadioSeek(const FMRadioSeekDirection& aDirection) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(FMRadioSeek(aDirection));
}

void
GetFMRadioSettings(FMRadioSettings* aInfo) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(GetFMRadioSettings(aInfo));
}

void
SetFMRadioFrequency(const uint32_t aFrequency) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetFMRadioFrequency(aFrequency));
}

uint32_t
GetFMRadioFrequency() {
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetFMRadioFrequency());
}

bool
IsFMRadioOn() {
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(IsFMRadioOn());
}

uint32_t
GetFMRadioSignalStrength() {
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetFMRadioSignalStrength());
}

void
CancelFMRadioSeek() {
  AssertMainThread();
  PROXY_IF_SANDBOXED(CancelFMRadioSeek());
}

FMRadioSettings
GetFMBandSettings(FMRadioCountry aCountry) {
  FMRadioSettings settings;

  switch (aCountry) {
    case FM_RADIO_COUNTRY_US:
    case FM_RADIO_COUNTRY_EU:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 87800;
      settings.spaceType() = 200;
      settings.preEmphasis() = 75;
      break;
    case FM_RADIO_COUNTRY_JP_STANDARD:
      settings.upperLimit() = 76000;
      settings.lowerLimit() = 90000;
      settings.spaceType() = 100;
      settings.preEmphasis() = 50;
      break;
    case FM_RADIO_COUNTRY_CY:
    case FM_RADIO_COUNTRY_DE:
    case FM_RADIO_COUNTRY_DK:
    case FM_RADIO_COUNTRY_ES:
    case FM_RADIO_COUNTRY_FI:
    case FM_RADIO_COUNTRY_FR:
    case FM_RADIO_COUNTRY_HU:
    case FM_RADIO_COUNTRY_IR:
    case FM_RADIO_COUNTRY_IT:
    case FM_RADIO_COUNTRY_KW:
    case FM_RADIO_COUNTRY_LT:
    case FM_RADIO_COUNTRY_ML:
    case FM_RADIO_COUNTRY_NO:
    case FM_RADIO_COUNTRY_OM:
    case FM_RADIO_COUNTRY_PG:
    case FM_RADIO_COUNTRY_NL:
    case FM_RADIO_COUNTRY_CZ:
    case FM_RADIO_COUNTRY_UK:
    case FM_RADIO_COUNTRY_RW:
    case FM_RADIO_COUNTRY_SN:
    case FM_RADIO_COUNTRY_SI:
    case FM_RADIO_COUNTRY_ZA:
    case FM_RADIO_COUNTRY_SE:
    case FM_RADIO_COUNTRY_CH:
    case FM_RADIO_COUNTRY_TW:
    case FM_RADIO_COUNTRY_UA:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 87500;
      settings.spaceType() = 100;
      settings.preEmphasis() = 50;
      break;
    case FM_RADIO_COUNTRY_VA:
    case FM_RADIO_COUNTRY_MA:
    case FM_RADIO_COUNTRY_TR:
      settings.upperLimit() = 10800;
      settings.lowerLimit() = 87500;
      settings.spaceType() = 100;
      settings.preEmphasis() = 75;
      break;
    case FM_RADIO_COUNTRY_AU:
    case FM_RADIO_COUNTRY_BD:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 87500;
      settings.spaceType() = 200;
      settings.preEmphasis() = 75;
      break;
    case FM_RADIO_COUNTRY_AW:
    case FM_RADIO_COUNTRY_BS:
    case FM_RADIO_COUNTRY_CO:
    case FM_RADIO_COUNTRY_KR:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 88000;
      settings.spaceType() = 200;
      settings.preEmphasis() = 75;
      break;
    case FM_RADIO_COUNTRY_EC:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 88000;
      settings.spaceType() = 200;
      settings.preEmphasis() = 0;
      break;
    case FM_RADIO_COUNTRY_GM:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 88000;
      settings.spaceType() = 0;
      settings.preEmphasis() = 75;
      break;
    case FM_RADIO_COUNTRY_QA:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 88000;
      settings.spaceType() = 200;
      settings.preEmphasis() = 50;
      break;
    case FM_RADIO_COUNTRY_SG:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 88000;
      settings.spaceType() = 200;
      settings.preEmphasis() = 50;
      break;
    case FM_RADIO_COUNTRY_IN:
      settings.upperLimit() = 100000;
      settings.lowerLimit() = 108000;
      settings.spaceType() = 100;
      settings.preEmphasis() = 50;
      break;
    case FM_RADIO_COUNTRY_NZ:
      settings.upperLimit() = 100000;
      settings.lowerLimit() = 88000;
      settings.spaceType() = 50;
      settings.preEmphasis() = 50;
      break;
    case FM_RADIO_COUNTRY_USER_DEFINED:
      break;
    default:
      MOZ_ASSERT(0);
      break;
    };
    return settings;
}

} 
} 
