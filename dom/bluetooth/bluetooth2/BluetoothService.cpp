





#include "base/basictypes.h"

#include "BluetoothService.h"

#include "BluetoothCommon.h"
#include "BluetoothParent.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothServiceChildProcess.h"
#include "BluetoothUtils.h"

#include "jsapi.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/ipc/BlobChild.h"
#include "mozilla/dom/ipc/BlobParent.h"
#include "nsContentUtils.h"
#include "nsIObserverService.h"
#include "nsISettingsService.h"
#include "nsISystemMessagesInternal.h"
#include "nsITimer.h"
#include "nsServiceManagerUtils.h"
#include "nsXPCOM.h"
#include "mozilla/dom/SettingChangeNotificationBinding.h"

#if defined(MOZ_WIDGET_GONK)
#include "cutils/properties.h"
#endif

#if defined(MOZ_B2G_BT)
#if defined(MOZ_B2G_BT_BLUEZ)




#include "BluetoothDBusService.h"
#elif defined(MOZ_B2G_BT_BLUEDROID)





#include "BluetoothServiceBluedroid.h"
#elif defined(MOZ_B2G_BT_DAEMON)





#include "BluetoothServiceBluedroid.h"
#endif
#elif defined(MOZ_BLUETOOTH_DBUS)




#include "BluetoothDBusService.h"
#else
#error No backend
#endif

#define MOZSETTINGS_CHANGED_ID      "mozsettings-changed"
#define BLUETOOTH_ENABLED_SETTING   "bluetooth.enabled"
#define BLUETOOTH_DEBUGGING_SETTING "bluetooth.debugging.enabled"

#define PROP_BLUETOOTH_ENABLED      "bluetooth.isEnabled"

#define DEFAULT_SHUTDOWN_TIMER_MS 5000

bool gBluetoothDebugFlag = false;

using namespace mozilla;
using namespace mozilla::dom;
USING_BLUETOOTH_NAMESPACE

namespace {

StaticRefPtr<BluetoothService> sBluetoothService;

bool sInShutdown = false;
bool sToggleInProgress = false;

void
ShutdownTimeExceeded(nsITimer* aTimer, void* aClosure)
{
  MOZ_ASSERT(NS_IsMainThread());
  *static_cast<bool*>(aClosure) = true;
}

void
GetAllBluetoothActors(InfallibleTArray<BluetoothParent*>& aActors)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aActors.IsEmpty());

  nsAutoTArray<ContentParent*, 20> contentActors;
  ContentParent::GetAll(contentActors);

  for (uint32_t contentIndex = 0;
       contentIndex < contentActors.Length();
       contentIndex++) {
    MOZ_ASSERT(contentActors[contentIndex]);

    AutoInfallibleTArray<PBluetoothParent*, 5> bluetoothActors;
    contentActors[contentIndex]->ManagedPBluetoothParent(bluetoothActors);

    for (uint32_t bluetoothIndex = 0;
         bluetoothIndex < bluetoothActors.Length();
         bluetoothIndex++) {
      MOZ_ASSERT(bluetoothActors[bluetoothIndex]);

      BluetoothParent* actor =
        static_cast<BluetoothParent*>(bluetoothActors[bluetoothIndex]);
      aActors.AppendElement(actor);
    }
  }
}

} 

BluetoothService::ToggleBtAck::ToggleBtAck(bool aEnabled)
  : mEnabled(aEnabled)
{ }

NS_METHOD
BluetoothService::ToggleBtAck::Run()
{
  BluetoothService::AcknowledgeToggleBt(mEnabled);

  return NS_OK;
}

class BluetoothService::StartupTask final : public nsISettingsServiceCallback
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD Handle(const nsAString& aName, JS::Handle<JS::Value> aResult)
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (!aResult.isBoolean()) {
      BT_WARNING("Setting for '" BLUETOOTH_ENABLED_SETTING "' is not a boolean!");
      return NS_OK;
    }

    
    
    if (sBluetoothService) {
      return sBluetoothService->HandleStartupSettingsCheck(aResult.toBoolean());
    }

    return NS_OK;
  }

  NS_IMETHOD HandleError(const nsAString& aName)
  {
    BT_WARNING("Unable to get value for '" BLUETOOTH_ENABLED_SETTING "'");
    return NS_OK;
  }

private:
  ~StartupTask()
  { }
};

NS_IMPL_ISUPPORTS(BluetoothService::StartupTask, nsISettingsServiceCallback);

NS_IMPL_ISUPPORTS(BluetoothService, nsIObserver)

bool
BluetoothService::IsToggling() const
{
  return sToggleInProgress;
}

BluetoothService::~BluetoothService()
{
  Cleanup();
}


BluetoothService*
BluetoothService::Create()
{
#if defined(MOZ_B2G_BT)
  if (!IsMainProcess()) {
    return BluetoothServiceChildProcess::Create();
  }

#if defined(MOZ_B2G_BT_BLUEZ)
  return new BluetoothDBusService();
#elif defined(MOZ_B2G_BT_BLUEDROID)
  return new BluetoothServiceBluedroid();
#elif defined(MOZ_B2G_BT_DAEMON)
  return new BluetoothServiceBluedroid();
#endif
#elif defined(MOZ_BLUETOOTH_DBUS)
  return new BluetoothDBusService();
#endif

  BT_WARNING("No platform support for bluetooth!");
  return nullptr;
}

bool
BluetoothService::Init()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE(obs, false);

  if (NS_FAILED(obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                 false))) {
    BT_WARNING("Failed to add shutdown observer!");
    return false;
  }

  
  if (IsMainProcess() &&
      NS_FAILED(obs->AddObserver(this, MOZSETTINGS_CHANGED_ID, false))) {
    BT_WARNING("Failed to add settings change observer!");
    return false;
  }

  return true;
}

void
BluetoothService::Cleanup()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (obs &&
      (NS_FAILED(obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) ||
       NS_FAILED(obs->RemoveObserver(this, MOZSETTINGS_CHANGED_ID)))) {
    BT_WARNING("Can't unregister observers!");
  }
}

void
BluetoothService::RegisterBluetoothSignalHandler(
                                              const nsAString& aNodeName,
                                              BluetoothSignalObserver* aHandler)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aHandler);

  BT_LOGD("[S] %s: %s", __FUNCTION__, NS_ConvertUTF16toUTF8(aNodeName).get());

  BluetoothSignalObserverList* ol;
  if (!mBluetoothSignalObserverTable.Get(aNodeName, &ol)) {
    ol = new BluetoothSignalObserverList();
    mBluetoothSignalObserverTable.Put(aNodeName, ol);
  }

  ol->AddObserver(aHandler);

  
  
  if (IsMainProcess() &&
      !mPendingPairReqSignals.IsEmpty() &&
      aNodeName.EqualsLiteral(KEY_PAIRING_LISTENER)) {
    for (uint32_t i = 0; i < mPendingPairReqSignals.Length(); ++i) {
      DistributeSignal(mPendingPairReqSignals[i]);
    }
    mPendingPairReqSignals.Clear();
  }
}

void
BluetoothService::UnregisterBluetoothSignalHandler(
                                              const nsAString& aNodeName,
                                              BluetoothSignalObserver* aHandler)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aHandler);

  BT_LOGD("[S] %s: %s", __FUNCTION__, NS_ConvertUTF16toUTF8(aNodeName).get());

  BluetoothSignalObserverList* ol;
  if (mBluetoothSignalObserverTable.Get(aNodeName, &ol)) {
    ol->RemoveObserver(aHandler);
    
    
    
    MOZ_ASSERT(!ol->RemoveObserver(aHandler));
    if (ol->Length() == 0) {
      mBluetoothSignalObserverTable.Remove(aNodeName);
    }
  }
  else {
    BT_WARNING("Node was never registered!");
  }
}

PLDHashOperator
RemoveAllSignalHandlers(const nsAString& aKey,
                        nsAutoPtr<BluetoothSignalObserverList>& aData,
                        void* aUserArg)
{
  BluetoothSignalObserver* handler = static_cast<BluetoothSignalObserver*>(aUserArg);
  aData->RemoveObserver(handler);
  
  
  
  MOZ_ASSERT(!aData->RemoveObserver(handler));
  return aData->Length() ? PL_DHASH_NEXT : PL_DHASH_REMOVE;
}

void
BluetoothService::UnregisterAllSignalHandlers(BluetoothSignalObserver* aHandler)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aHandler);

  mBluetoothSignalObserverTable.Enumerate(RemoveAllSignalHandlers, aHandler);
}

void
BluetoothService::DistributeSignal(const nsAString& aName, const nsAString& aPath)
{
  DistributeSignal(aName, aPath, BluetoothValue(true));
}

void
BluetoothService::DistributeSignal(const nsAString& aName, const nsAString& aPath,
                                   const BluetoothValue& aValue)
{
  BluetoothSignal signal(nsString(aName), nsString(aPath), aValue);
  DistributeSignal(signal);
}

void
BluetoothService::DistributeSignal(const BluetoothSignal& aSignal)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothSignalObserverList* ol;
  if (!mBluetoothSignalObserverTable.Get(aSignal.path(), &ol)) {
    
    
    
    if (aSignal.path().EqualsLiteral(KEY_PAIRING_LISTENER)) {
      mPendingPairReqSignals.AppendElement(aSignal);

      BT_ENSURE_TRUE_VOID_BROADCAST_SYSMSG(
        NS_LITERAL_STRING(SYS_MSG_BT_PAIRING_REQ),
        BluetoothValue(EmptyString()));
    } else {
      BT_WARNING("No observer registered for path %s",
                 NS_ConvertUTF16toUTF8(aSignal.path()).get());
    }
    return;
  }

  MOZ_ASSERT(ol->Length());
  ol->Broadcast(aSignal);
}

nsresult
BluetoothService::StartBluetooth(bool aIsStartup,
                                 BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (sInShutdown) {
    
    MOZ_ASSERT(false, "Start called while in shutdown!");
    return NS_ERROR_FAILURE;
  }

  






  if (aIsStartup || !IsEnabled()) {
    
    nsresult rv = StartInternal(aRunnable);
    if (NS_FAILED(rv)) {
      BT_WARNING("Bluetooth service failed to start!");
      return rv;
    }
  } else {
    BT_WARNING("Bluetooth has already been enabled before.");
    nsRefPtr<nsRunnable> runnable = new BluetoothService::ToggleBtAck(true);
    if (NS_FAILED(NS_DispatchToMainThread(runnable))) {
      BT_WARNING("Failed to dispatch to main thread!");
    }
  }

  return NS_OK;
}

nsresult
BluetoothService::StopBluetooth(bool aIsStartup,
                                BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  






  if (aIsStartup || IsEnabled()) {
    
    nsresult rv = StopInternal(aRunnable);
    if (NS_FAILED(rv)) {
      BT_WARNING("Bluetooth service failed to stop!");
      return rv;
    }
  } else {
    BT_WARNING("Bluetooth has already been enabled/disabled before.");
    nsRefPtr<nsRunnable> runnable = new BluetoothService::ToggleBtAck(false);
    if (NS_FAILED(NS_DispatchToMainThread(runnable))) {
      BT_WARNING("Failed to dispatch to main thread!");
    }
  }

  return NS_OK;
}

nsresult
BluetoothService::StartStopBluetooth(bool aStart,
                                     bool aIsStartup,
                                     BluetoothReplyRunnable* aRunnable)
{
  nsresult rv;
  if (aStart) {
    rv = StartBluetooth(aIsStartup, aRunnable);
  } else {
    rv = StopBluetooth(aIsStartup, aRunnable);
  }
  return rv;
}

void
BluetoothService::SetEnabled(bool aEnabled)
{
  MOZ_ASSERT(NS_IsMainThread());

  AutoInfallibleTArray<BluetoothParent*, 10> childActors;
  GetAllBluetoothActors(childActors);

  for (uint32_t index = 0; index < childActors.Length(); index++) {
    unused << childActors[index]->SendEnabled(aEnabled);
  }

  



  if (mEnabled == aEnabled) {
    BT_WARNING("Bluetooth is already %s, or the toggling failed.",
               mEnabled ? "enabled" : "disabled");
  }

  mEnabled = aEnabled;
}

nsresult
BluetoothService::HandleStartup()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!sToggleInProgress);

  nsCOMPtr<nsISettingsService> settings =
    do_GetService("@mozilla.org/settingsService;1");
  NS_ENSURE_TRUE(settings, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsISettingsServiceLock> settingsLock;
  nsresult rv = settings->CreateLock(nullptr, getter_AddRefs(settingsLock));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<StartupTask> callback = new StartupTask();
  rv = settingsLock->Get(BLUETOOTH_ENABLED_SETTING, callback);
  NS_ENSURE_SUCCESS(rv, rv);

  sToggleInProgress = true;
  return NS_OK;
}

nsresult
BluetoothService::HandleStartupSettingsCheck(bool aEnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  return StartStopBluetooth(aEnable, true, nullptr);
}

nsresult
BluetoothService::HandleSettingsChanged(nsISupports* aSubject)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  

  RootedDictionary<SettingChangeNotification> setting(nsContentUtils::RootingCx());
  if (!WrappedJSToDictionary(aSubject, setting)) {
    return NS_OK;
  }
  if (!setting.mKey.EqualsASCII(BLUETOOTH_DEBUGGING_SETTING)) {
    return NS_OK;
  }
  if (!setting.mValue.isBoolean()) {
    MOZ_ASSERT(false, "Expecting a boolean for 'bluetooth.debugging.enabled'!");
    return NS_ERROR_UNEXPECTED;
  }

  SWITCH_BT_DEBUG(setting.mValue.toBoolean());

  return NS_OK;
}

nsresult
BluetoothService::HandleShutdown()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  

  sInShutdown = true;

  Cleanup();

  AutoInfallibleTArray<BluetoothParent*, 10> childActors;
  GetAllBluetoothActors(childActors);

  if (!childActors.IsEmpty()) {
    
    for (uint32_t index = 0; index < childActors.Length(); index++) {
      childActors[index]->BeginShutdown();
    }

    
    
    
    
    nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
    MOZ_ASSERT(timer);

    if (timer) {
      bool timeExceeded = false;

      if (NS_SUCCEEDED(timer->InitWithFuncCallback(ShutdownTimeExceeded,
                                                   &timeExceeded,
                                                   DEFAULT_SHUTDOWN_TIMER_MS,
                                                   nsITimer::TYPE_ONE_SHOT))) {
        nsIThread* currentThread = NS_GetCurrentThread();
        MOZ_ASSERT(currentThread);

        
        while (!timeExceeded && !childActors.IsEmpty()) {
          if (!NS_ProcessNextEvent(currentThread)) {
            MOZ_ASSERT(false, "Something horribly wrong here!");
            break;
          }
          GetAllBluetoothActors(childActors);
        }

        if (NS_FAILED(timer->Cancel())) {
          MOZ_CRASH("Failed to cancel shutdown timer, this will crash!");
        }
      }
      else {
        MOZ_ASSERT(false, "Failed to initialize shutdown timer!");
      }
    }
  }

  if (IsEnabled() && NS_FAILED(StopBluetooth(false, nullptr))) {
    MOZ_ASSERT(false, "Failed to deliver stop message!");
  }

  return NS_OK;
}


BluetoothService*
BluetoothService::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (sBluetoothService) {
    return sBluetoothService;
  }

  
  if (sInShutdown) {
    BT_WARNING("BluetoothService can't be created during shutdown");
    return nullptr;
  }

  
  sBluetoothService = BluetoothService::Create();
  NS_ENSURE_TRUE(sBluetoothService, nullptr);

  if (!sBluetoothService->Init()) {
    sBluetoothService->Cleanup();
    return nullptr;
  }

  ClearOnShutdown(&sBluetoothService);
  return sBluetoothService;
}

nsresult
BluetoothService::Observe(nsISupports* aSubject, const char* aTopic,
                          const char16_t* aData)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!strcmp(aTopic, "profile-after-change")) {
    return HandleStartup();
  }

  if (!strcmp(aTopic, MOZSETTINGS_CHANGED_ID)) {
    return HandleSettingsChanged(aSubject);
  }

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    return HandleShutdown();
  }

  MOZ_ASSERT(false, "BluetoothService got unexpected topic!");
  return NS_ERROR_UNEXPECTED;
}











nsresult
BluetoothService::EnableDisable(bool aEnable,
                                BluetoothReplyRunnable* aRunnable)
{
  sToggleInProgress = true;
  return StartStopBluetooth(aEnable, false, aRunnable);
}

void
BluetoothService::FireAdapterStateChanged(bool aEnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  InfallibleTArray<BluetoothNamedValue> props;
  BT_APPEND_NAMED_VALUE(props, "State", aEnable);

  DistributeSignal(NS_LITERAL_STRING("PropertyChanged"),
                   NS_LITERAL_STRING(KEY_ADAPTER),
                   BluetoothValue(props));
}

void
BluetoothService::AcknowledgeToggleBt(bool aEnabled)
{
  MOZ_ASSERT(NS_IsMainThread());

#if defined(MOZ_WIDGET_GONK)
  
  
  
  
  
  
  
  if (property_set(PROP_BLUETOOTH_ENABLED, aEnabled ? "true" : "false") != 0) {
    BT_WARNING("Failed to set bluetooth enabled property");
  }
#endif

  if (sInShutdown) {
    sBluetoothService = nullptr;
    return;
  }

  NS_ENSURE_TRUE_VOID(sBluetoothService);

  sBluetoothService->CompleteToggleBt(aEnabled);
}

void
BluetoothService::CompleteToggleBt(bool aEnabled)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  SetEnabled(aEnabled);
  sToggleInProgress = false;

  FireAdapterStateChanged(aEnabled);
}
