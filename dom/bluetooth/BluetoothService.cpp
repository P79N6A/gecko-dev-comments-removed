





#include "base/basictypes.h"

#include "BluetoothService.h"

#include "BluetoothCommon.h"
#include "BluetoothA2dpManager.h"
#include "BluetoothHfpManager.h"
#include "BluetoothHidManager.h"
#include "BluetoothManager.h"
#include "BluetoothOppManager.h"
#include "BluetoothParent.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothServiceChildProcess.h"
#include "BluetoothUtils.h"

#include "jsapi.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "mozilla/Util.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/ipc/UnixSocket.h"
#include "mozilla/LazyIdleThread.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsIObserverService.h"
#include "nsISettingsService.h"
#include "nsISystemMessagesInternal.h"
#include "nsITimer.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"

#if defined(MOZ_WIDGET_GONK)
#include "cutils/properties.h"
#endif

#if defined(MOZ_B2G_BT)
# if defined(MOZ_BLUETOOTH_GONK)
#  include "BluetoothGonkService.h"
# elif defined(MOZ_BLUETOOTH_DBUS)
#  include "BluetoothDBusService.h"
# else
#  error No_suitable_backend_for_bluetooth!
# endif
#endif

#define MOZSETTINGS_CHANGED_ID      "mozsettings-changed"
#define BLUETOOTH_ENABLED_SETTING   "bluetooth.enabled"
#define BLUETOOTH_DEBUGGING_SETTING "bluetooth.debugging.enabled"

#define PROP_BLUETOOTH_ENABLED      "bluetooth.isEnabled"

#define DEFAULT_THREAD_TIMEOUT_MS 3000
#define DEFAULT_SHUTDOWN_TIMER_MS 5000

bool gBluetoothDebugFlag = false;

using namespace mozilla;
using namespace mozilla::dom;
USING_BLUETOOTH_NAMESPACE

namespace {

StaticRefPtr<BluetoothService> gBluetoothService;

bool gInShutdown = false;
bool gToggleInProgress = false;

bool
IsMainProcess()
{
  return XRE_GetProcessType() == GeckoProcessType_Default;
}

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

class BluetoothService::ToggleBtAck : public nsRunnable
{
public:
  ToggleBtAck(bool aEnabled)
    : mEnabled(aEnabled)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (!gBluetoothService) {
      return NS_OK;
    }

    if (!gInShutdown) {
      gBluetoothService->SetEnabled(mEnabled);

      nsAutoString signalName, signalPath;
      BluetoothValue v = true;
      if (mEnabled) {
        signalName = NS_LITERAL_STRING("Enabled");
      } else {
        signalName = NS_LITERAL_STRING("Disabled");
      }
      signalPath = NS_LITERAL_STRING(KEY_MANAGER);
      BluetoothSignal signal(signalName, signalPath, v);
      gBluetoothService->DistributeSignal(signal);
    }

    if (gInShutdown) {
      gBluetoothService = nullptr;
    }

    return NS_OK;
  }

private:
  bool mEnabled;
};

class BluetoothService::ToggleBtTask : public nsRunnable
{
public:
  ToggleBtTask(bool aEnabled, bool aIsStartup)
    : mEnabled(aEnabled)
    , mIsStartup(aIsStartup)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    










    if (!mIsStartup && mEnabled == gBluetoothService->IsEnabledInternal()) {
      BT_WARNING("Bluetooth has already been enabled/disabled before.");
    } else {
      
      if (mEnabled) {
        if (NS_FAILED(gBluetoothService->StartInternal())) {
          BT_WARNING("Bluetooth service failed to start!");
          mEnabled = !mEnabled;
        }
      } else {
        if (NS_FAILED(gBluetoothService->StopInternal())) {
          BT_WARNING("Bluetooth service failed to stop!");
          mEnabled = !mEnabled;
        }
      }
    }

    
    
    
    
    
    
    
#if defined(MOZ_WIDGET_GONK)
    if (property_set(PROP_BLUETOOTH_ENABLED, mEnabled ? "true" : "false") != 0) {
      BT_WARNING("Failed to set bluetooth enabled property");
    }
#endif

    nsCOMPtr<nsIRunnable> ackTask = new BluetoothService::ToggleBtAck(mEnabled);
    if (NS_FAILED(NS_DispatchToMainThread(ackTask))) {
      BT_WARNING("Failed to dispatch to main thread!");
    }

    return NS_OK;
  }

private:
  bool mEnabled;
  bool mIsStartup;
};

class BluetoothService::StartupTask : public nsISettingsServiceCallback
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD Handle(const nsAString& aName, const JS::Value& aResult)
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (!aResult.isBoolean()) {
      BT_WARNING("Setting for '" BLUETOOTH_ENABLED_SETTING "' is not a boolean!");
      return NS_OK;
    }

    
    
    if (gBluetoothService) {
      return gBluetoothService->HandleStartupSettingsCheck(aResult.toBoolean());
    }

    return NS_OK;
  }

  NS_IMETHOD HandleError(const nsAString& aName)
  {
    BT_WARNING("Unable to get value for '" BLUETOOTH_ENABLED_SETTING "'");
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(BluetoothService::StartupTask, nsISettingsServiceCallback);

NS_IMPL_ISUPPORTS1(BluetoothService, nsIObserver)

bool
BluetoothService::IsToggling() const
{
  return gToggleInProgress;
}

BluetoothService::~BluetoothService()
{
  Cleanup();
}

PLDHashOperator
RemoveObserversExceptBluetoothManager
  (const nsAString& key,
   nsAutoPtr<BluetoothSignalObserverList>& value,
   void* arg)
{
  if (!key.EqualsLiteral(KEY_MANAGER)) {
    return PL_DHASH_REMOVE;
  }

  return PL_DHASH_NEXT;
}

void
BluetoothService::RemoveObserverFromTable(const nsAString& key)
{
  mBluetoothSignalObserverTable.Remove(key);
}


BluetoothService*
BluetoothService::Create()
{
#if defined(MOZ_B2G_BT)
  if (!IsMainProcess()) {
    return BluetoothServiceChildProcess::Create();
  }
#endif

#if defined(MOZ_BLUETOOTH_GONK)
  return new BluetoothGonkService();
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
BluetoothService::DistributeSignal(const BluetoothSignal& aSignal)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (aSignal.path().EqualsLiteral(KEY_LOCAL_AGENT)) {
    Notify(aSignal);
    return;
  } else if (aSignal.path().EqualsLiteral(KEY_REMOTE_AGENT)) {
    Notify(aSignal);
    return;
  }

  BluetoothSignalObserverList* ol;
  if (!mBluetoothSignalObserverTable.Get(aSignal.path(), &ol)) {
#if DEBUG
    nsAutoCString msg("No observer registered for path ");
    msg.Append(NS_ConvertUTF16toUTF8(aSignal.path()));
    BT_WARNING(msg.get());
#endif
    return;
  }
  MOZ_ASSERT(ol->Length());
  ol->Broadcast(aSignal);
}

nsresult
BluetoothService::StartStopBluetooth(bool aStart, bool aIsStartup)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (gInShutdown) {
    if (aStart) {
      
      MOZ_ASSERT(false, "Start called while in shutdown!");
      return NS_ERROR_FAILURE;
    }

    if (!mBluetoothThread) {
      
      
      return NS_OK;
    }
  }

  if (!aStart) {
    BluetoothProfileManagerBase* profile;
    profile = BluetoothHfpManager::Get();
    NS_ENSURE_TRUE(profile, NS_ERROR_FAILURE);
    if (profile->IsConnected()) {
      profile->Disconnect(nullptr);
    }

    profile = BluetoothOppManager::Get();
    NS_ENSURE_TRUE(profile, NS_ERROR_FAILURE);
    if (profile->IsConnected()) {
      profile->Disconnect(nullptr);
    }

    profile = BluetoothA2dpManager::Get();
    NS_ENSURE_TRUE(profile, NS_ERROR_FAILURE);
    if (profile->IsConnected()) {
      profile->Disconnect(nullptr);
    }

    profile = BluetoothHidManager::Get();
    NS_ENSURE_TRUE(profile, NS_ERROR_FAILURE);
    if (profile->IsConnected()) {
      profile->Disconnect(nullptr);
    }
  }

  if (!mBluetoothThread) {
    mBluetoothThread = new LazyIdleThread(DEFAULT_THREAD_TIMEOUT_MS,
                                          NS_LITERAL_CSTRING("Bluetooth"),
                                          LazyIdleThread::ManualShutdown);
  }

  nsCOMPtr<nsIRunnable> runnable = new ToggleBtTask(aStart, aIsStartup);
  nsresult rv = mBluetoothThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
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

  if (!aEnabled) {
    





    mBluetoothSignalObserverTable.Enumerate(
      RemoveObserversExceptBluetoothManager, nullptr);
  }

  



  if (mEnabled == aEnabled) {
    BT_WARNING("Bluetooth has already been enabled/disabled before\
                or the toggling is failed.");
  }

  mEnabled = aEnabled;

  gToggleInProgress = false;
}

nsresult
BluetoothService::HandleStartup()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!gToggleInProgress);

  nsCOMPtr<nsISettingsService> settings =
    do_GetService("@mozilla.org/settingsService;1");
  NS_ENSURE_TRUE(settings, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsISettingsServiceLock> settingsLock;
  nsresult rv = settings->CreateLock(getter_AddRefs(settingsLock));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<StartupTask> callback = new StartupTask();
  rv = settingsLock->Get(BLUETOOTH_ENABLED_SETTING, callback);
  NS_ENSURE_SUCCESS(rv, rv);

  gToggleInProgress = true;
  return NS_OK;
}

nsresult
BluetoothService::HandleStartupSettingsCheck(bool aEnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  return StartStopBluetooth(aEnable, true);
}

nsresult
BluetoothService::HandleSettingsChanged(const nsAString& aData)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  

  AutoSafeJSContext cx;
  if (!cx) {
    return NS_OK;
  }

  JS::Rooted<JS::Value> val(cx);
  if (!JS_ParseJSON(cx, aData.BeginReading(), aData.Length(), &val)) {
    return JS_ReportPendingException(cx) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }

  if (!val.isObject()) {
    return NS_OK;
  }

  JSObject& obj(val.toObject());

  JS::Rooted<JS::Value> key(cx);
  if (!JS_GetProperty(cx, &obj, "key", &key)) {
    MOZ_ASSERT(!JS_IsExceptionPending(cx));
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!key.isString()) {
    return NS_OK;
  }

  
  bool match;
  if (!JS_StringEqualsAscii(cx, key.toString(), BLUETOOTH_DEBUGGING_SETTING, &match)) {
    MOZ_ASSERT(!JS_IsExceptionPending(cx));
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (match) {
    JS::Rooted<JS::Value> value(cx);
    if (!JS_GetProperty(cx, &obj, "value", &value)) {
      MOZ_ASSERT(!JS_IsExceptionPending(cx));
      return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!value.isBoolean()) {
      MOZ_ASSERT(false, "Expecting a boolean for 'bluetooth.debugging.enabled'!");
      return NS_ERROR_UNEXPECTED;
    }

    SWITCH_BT_DEBUG(value.toBoolean());

    return NS_OK;
  }

  
  if (!JS_StringEqualsAscii(cx, key.toString(), BLUETOOTH_ENABLED_SETTING, &match)) {
    MOZ_ASSERT(!JS_IsExceptionPending(cx));
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (match) {
    JS::Rooted<JS::Value> value(cx);
    if (!JS_GetProperty(cx, &obj, "value", &value)) {
      MOZ_ASSERT(!JS_IsExceptionPending(cx));
      return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!value.isBoolean()) {
      MOZ_ASSERT(false, "Expecting a boolean for 'bluetooth.enabled'!");
      return NS_ERROR_UNEXPECTED;
    }

    if (gToggleInProgress || value.toBoolean() == IsEnabled()) {
      
      return NS_OK;
    }

    gToggleInProgress = true;

    nsresult rv = StartStopBluetooth(value.toBoolean(), false);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
BluetoothService::HandleShutdown()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  

  gInShutdown = true;

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

  if (IsEnabled() && NS_FAILED(StartStopBluetooth(false, false))) {
    MOZ_ASSERT(false, "Failed to deliver stop message!");
  }

  return NS_OK;
}


BluetoothService*
BluetoothService::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (gBluetoothService) {
    return gBluetoothService;
  }

  
  if (gInShutdown) {
    BT_WARNING("BluetoothService can't be created during shutdown");
    return nullptr;
  }

  
  nsRefPtr<BluetoothService> service = BluetoothService::Create();
  NS_ENSURE_TRUE(service, nullptr);

  if (!service->Init()) {
    service->Cleanup();
    return nullptr;
  }

  gBluetoothService = service;
  return gBluetoothService;
}

nsresult
BluetoothService::Observe(nsISupports* aSubject, const char* aTopic,
                          const PRUnichar* aData)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!strcmp(aTopic, "profile-after-change")) {
    return HandleStartup();
  }

  if (!strcmp(aTopic, MOZSETTINGS_CHANGED_ID)) {
    return HandleSettingsChanged(nsDependentString(aData));
  }

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    return HandleShutdown();
  }

  MOZ_ASSERT(false, "BluetoothService got unexpected topic!");
  return NS_ERROR_UNEXPECTED;
}

void
BluetoothService::Notify(const BluetoothSignal& aData)
{
  nsString type = NS_LITERAL_STRING("bluetooth-pairing-request");

  AutoSafeJSContext cx;
  JS::Rooted<JSObject*> obj(cx, JS_NewObject(cx, nullptr, nullptr, nullptr));
  NS_ENSURE_TRUE_VOID(obj);

  if (!SetJsObject(cx, aData.value(), obj)) {
    BT_WARNING("Failed to set properties of system message!");
    return;
  }

  BT_LOGD("[S] %s: %s", __FUNCTION__, NS_ConvertUTF16toUTF8(aData.name()).get());

  if (aData.name().EqualsLiteral("RequestConfirmation")) {
    MOZ_ASSERT(aData.value().get_ArrayOfBluetoothNamedValue().Length() == 4,
      "RequestConfirmation: Wrong length of parameters");
  } else if (aData.name().EqualsLiteral("RequestPinCode")) {
    MOZ_ASSERT(aData.value().get_ArrayOfBluetoothNamedValue().Length() == 3,
      "RequestPinCode: Wrong length of parameters");
  } else if (aData.name().EqualsLiteral("RequestPasskey")) {
    MOZ_ASSERT(aData.value().get_ArrayOfBluetoothNamedValue().Length() == 3,
      "RequestPinCode: Wrong length of parameters");
  } else if (aData.name().EqualsLiteral("Cancel")) {
    MOZ_ASSERT(aData.value().get_ArrayOfBluetoothNamedValue().Length() == 0,
      "Cancel: Wrong length of parameters");
    type.AssignLiteral("bluetooth-cancel");
  } else if (aData.name().EqualsLiteral(PAIRED_STATUS_CHANGED_ID)) {
    MOZ_ASSERT(aData.value().get_ArrayOfBluetoothNamedValue().Length() == 1,
      "pairedstatuschanged: Wrong length of parameters");
    type.AssignLiteral("bluetooth-pairedstatuschanged");
  } else {
    nsCString warningMsg;
    warningMsg.AssignLiteral("Not handling service signal: ");
    warningMsg.Append(NS_ConvertUTF16toUTF8(aData.name()));
    BT_WARNING(warningMsg.get());
    return;
  }

  nsCOMPtr<nsISystemMessagesInternal> systemMessenger =
    do_GetService("@mozilla.org/system-message-internal;1");
  NS_ENSURE_TRUE_VOID(systemMessenger);

  systemMessenger->BroadcastMessage(type,
                                    OBJECT_TO_JSVAL(obj),
                                    JS::UndefinedValue());
}
