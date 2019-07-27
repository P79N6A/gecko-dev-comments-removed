





#include "BluetoothGattManager.h"

#include "BluetoothCommon.h"
#include "BluetoothInterface.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothUtils.h"
#include "MainThreadUtils.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "nsIObserverService.h"
#include "nsThreadUtils.h"

#define ENSURE_GATT_CLIENT_INTF_IS_READY_VOID(runnable)                       \
  do {                                                                        \
    if (!sBluetoothGattInterface) {                                           \
      DispatchReplyError(runnable,                                            \
        NS_LITERAL_STRING("BluetoothGattClientInterface is not ready"));      \
      return;                                                                 \
    }                                                                         \
  } while(0)

using namespace mozilla;
USING_BLUETOOTH_NAMESPACE

namespace {
  StaticRefPtr<BluetoothGattManager> sBluetoothGattManager;
  static BluetoothGattInterface* sBluetoothGattInterface;
  static BluetoothGattClientInterface* sBluetoothGattClientInterface;
} 

bool BluetoothGattManager::mInShutdown = false;

class BluetoothGattClient;
static StaticAutoPtr<nsTArray<nsRefPtr<BluetoothGattClient> > > sClients;

class BluetoothGattClient MOZ_FINAL : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  BluetoothGattClient(const nsAString& aAppUuid, const nsAString& aDeviceAddr)
  : mAppUuid(aAppUuid)
  , mDeviceAddr(aDeviceAddr)
  , mClientIf(0)
  , mConnId(0)
  { }

  ~BluetoothGattClient()
  {
    mConnectRunnable = nullptr;
    mDisconnectRunnable = nullptr;
    mUnregisterClientRunnable = nullptr;
  }

  nsString mAppUuid;
  nsString mDeviceAddr;
  int mClientIf;
  int mConnId;
  nsRefPtr<BluetoothReplyRunnable> mConnectRunnable;
  nsRefPtr<BluetoothReplyRunnable> mDisconnectRunnable;
  nsRefPtr<BluetoothReplyRunnable> mUnregisterClientRunnable;
};

NS_IMPL_ISUPPORTS0(BluetoothGattClient)

class UuidComparator
{
public:
  bool Equals(const nsRefPtr<BluetoothGattClient>& aClient,
              const nsAString& aAppUuid) const
  {
    return aClient->mAppUuid.Equals(aAppUuid);
  }
};

class ClientIfComparator
{
public:
  bool Equals(const nsRefPtr<BluetoothGattClient>& aClient,
              int aClientIf) const
  {
    return aClient->mClientIf == aClientIf;
  }
};

BluetoothGattManager*
BluetoothGattManager::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (sBluetoothGattManager) {
    return sBluetoothGattManager;
  }

  
  NS_ENSURE_FALSE(mInShutdown, nullptr);

  
  BluetoothGattManager* manager = new BluetoothGattManager();
  sBluetoothGattManager = manager;
  return sBluetoothGattManager;
}

class BluetoothGattManager::InitGattResultHandler MOZ_FINAL
  : public BluetoothGattResultHandler
{
public:
  InitGattResultHandler(BluetoothProfileResultHandler* aRes)
  : mRes(aRes)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothGattInterface::Init failed: %d",
               (int)aStatus);
    if (mRes) {
      mRes->OnError(NS_ERROR_FAILURE);
    }
  }

  void Init() MOZ_OVERRIDE
  {
    if (mRes) {
      mRes->Init();
    }
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};


void
BluetoothGattManager::InitGattInterface(BluetoothProfileResultHandler* aRes)
{
  BluetoothInterface* btInf = BluetoothInterface::GetInstance();
  if (!btInf) {
    BT_LOGR("Error: Bluetooth interface not available");
    if (aRes) {
      aRes->OnError(NS_ERROR_FAILURE);
    }
    return;
  }

  sBluetoothGattInterface = btInf->GetBluetoothGattInterface();
  if (!sBluetoothGattInterface) {
    BT_LOGR("Error: Bluetooth GATT interface not available");
    if (aRes) {
      aRes->OnError(NS_ERROR_FAILURE);
    }
    return;
  }

  sBluetoothGattClientInterface =
    sBluetoothGattInterface->GetBluetoothGattClientInterface();
  NS_ENSURE_TRUE_VOID(sBluetoothGattClientInterface);

  if (!sClients) {
    sClients = new nsTArray<nsRefPtr<BluetoothGattClient> >;
  }

  BluetoothGattManager* gattManager = BluetoothGattManager::Get();
  sBluetoothGattInterface->Init(gattManager,
                                new InitGattResultHandler(aRes));
}

class BluetoothGattManager::CleanupResultHandler MOZ_FINAL
  : public BluetoothGattResultHandler
{
public:
  CleanupResultHandler(BluetoothProfileResultHandler* aRes)
  : mRes(aRes)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothGattInterface::Cleanup failed: %d",
               (int)aStatus);
    if (mRes) {
      mRes->OnError(NS_ERROR_FAILURE);
    }
  }

  void Cleanup() MOZ_OVERRIDE
  {
    sBluetoothGattClientInterface = nullptr;
    sBluetoothGattInterface = nullptr;
    sClients = nullptr;

    if (mRes) {
      mRes->Deinit();
    }
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};

class BluetoothGattManager::CleanupResultHandlerRunnable MOZ_FINAL
  : public nsRunnable
{
public:
  CleanupResultHandlerRunnable(BluetoothProfileResultHandler* aRes)
  : mRes(aRes)
  {
    MOZ_ASSERT(mRes);
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    mRes->Deinit();
    return NS_OK;
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};


void
BluetoothGattManager::DeinitGattInterface(BluetoothProfileResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (sBluetoothGattInterface) {
    sBluetoothGattInterface->Cleanup(new CleanupResultHandler(aRes));
  } else if (aRes) {
    
    
    nsRefPtr<nsRunnable> r = new CleanupResultHandlerRunnable(aRes);
    if (NS_FAILED(NS_DispatchToMainThread(r))) {
      BT_LOGR("Failed to dispatch cleanup-result-handler runnable");
    }
  }
}

class BluetoothGattManager::RegisterClientResultHandler MOZ_FINAL
  : public BluetoothGattClientResultHandler
{
public:
  RegisterClientResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothGattClientInterface::RegisterClient failed: %d",
               (int)aStatus);

    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE_VOID(bs);

    
    bs->DistributeSignal(
      NS_LITERAL_STRING(GATT_CONNECTION_STATE_CHANGED_ID),
      mClient->mAppUuid,
      BluetoothValue(false)); 

    
    if (mClient->mConnectRunnable) {
      DispatchReplyError(mClient->mConnectRunnable,
                         NS_LITERAL_STRING("Register GATT client failed"));
      mClient->mConnectRunnable = nullptr;
    }

    sClients->RemoveElement(mClient);
  }

private:
  nsRefPtr<BluetoothGattClient> mClient;
};

class BluetoothGattManager::UnregisterClientResultHandler MOZ_FINAL
  : public BluetoothGattClientResultHandler
{
public:
  UnregisterClientResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void UnregisterClient() MOZ_OVERRIDE
  {
    MOZ_ASSERT(mClient->mUnregisterClientRunnable);
    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE_VOID(bs);

    
    bs->DistributeSignal(
      NS_LITERAL_STRING("ClientUnregistered"),
      mClient->mAppUuid,
      BluetoothValue(true));

    
    DispatchReplySuccess(mClient->mUnregisterClientRunnable);
    mClient->mUnregisterClientRunnable = nullptr;

    sClients->RemoveElement(mClient);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothGattClientInterface::UnregisterClient failed: %d",
               (int)aStatus);
    MOZ_ASSERT(mClient->mUnregisterClientRunnable);

    
    DispatchReplyError(mClient->mUnregisterClientRunnable,
                       NS_LITERAL_STRING("Unregister GATT client failed"));
    mClient->mUnregisterClientRunnable = nullptr;
  }

private:
  nsRefPtr<BluetoothGattClient> mClient;
};

void
BluetoothGattManager::UnregisterClient(int aClientIf,
                                       BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  ENSURE_GATT_CLIENT_INTF_IS_READY_VOID(aRunnable);

  size_t index = sClients->IndexOf(aClientIf, 0 ,
                                   ClientIfComparator());

  
  if (index == sClients->NoIndex) {
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("Unregister GATT client failed"));
    return;
  }

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);
  client->mUnregisterClientRunnable = aRunnable;

  sBluetoothGattClientInterface->UnregisterClient(
    aClientIf,
    new UnregisterClientResultHandler(client));
}

class BluetoothGattManager::ConnectResultHandler MOZ_FINAL
  : public BluetoothGattClientResultHandler
{
public:
  ConnectResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothGattClientInterface::Connect failed: %d",
               (int)aStatus);
    MOZ_ASSERT(mClient->mConnectRunnable);

    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE_VOID(bs);

    
    bs->DistributeSignal(
      NS_LITERAL_STRING(GATT_CONNECTION_STATE_CHANGED_ID),
      mClient->mAppUuid,
      BluetoothValue(false)); 

    
    DispatchReplyError(mClient->mConnectRunnable,
                       NS_LITERAL_STRING("Connect failed"));
    mClient->mConnectRunnable = nullptr;
  }

private:
  nsRefPtr<BluetoothGattClient> mClient;
};

void
BluetoothGattManager::Connect(const nsAString& aAppUuid,
                              const nsAString& aDeviceAddr,
                              BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  ENSURE_GATT_CLIENT_INTF_IS_READY_VOID(aRunnable);

  size_t index = sClients->IndexOf(aAppUuid, 0 , UuidComparator());
  if (index == sClients->NoIndex) {
    index = sClients->Length();
    sClients->AppendElement(new BluetoothGattClient(aAppUuid, aDeviceAddr));
  }

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);
  client->mConnectRunnable = aRunnable;

  if (client->mClientIf > 0) {
    sBluetoothGattClientInterface->Connect(client->mClientIf,
                                           aDeviceAddr,
                                           true, 
                                           new ConnectResultHandler(client));
  } else {
    BluetoothUuid uuid;
    StringToUuid(NS_ConvertUTF16toUTF8(aAppUuid).get(), uuid);

    
    sBluetoothGattClientInterface->RegisterClient(
      uuid, new RegisterClientResultHandler(client));
  }
}

class BluetoothGattManager::DisconnectResultHandler MOZ_FINAL
  : public BluetoothGattClientResultHandler
{
public:
  DisconnectResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothGattClientInterface::Disconnect failed: %d",
               (int)aStatus);
    MOZ_ASSERT(mClient->mDisconnectRunnable);

    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE_VOID(bs);

    
    bs->DistributeSignal(
      NS_LITERAL_STRING(GATT_CONNECTION_STATE_CHANGED_ID),
      mClient->mAppUuid,
      BluetoothValue(true)); 

    
    DispatchReplyError(mClient->mDisconnectRunnable,
                       NS_LITERAL_STRING("Disconnect failed"));
    mClient->mDisconnectRunnable = nullptr;
  }

private:
  nsRefPtr<BluetoothGattClient> mClient;
};

void
BluetoothGattManager::Disconnect(const nsAString& aAppUuid,
                                 const nsAString& aDeviceAddr,
                                 BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  ENSURE_GATT_CLIENT_INTF_IS_READY_VOID(aRunnable);

  size_t index = sClients->IndexOf(aAppUuid, 0 , UuidComparator());

  
  if (index == sClients->NoIndex) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("Disconnect failed"));
    return;
  }

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);
  client->mDisconnectRunnable = aRunnable;

  sBluetoothGattClientInterface->Disconnect(
    client->mClientIf,
    aDeviceAddr,
    client->mConnId,
    new DisconnectResultHandler(client));
}




void
BluetoothGattManager::RegisterClientNotification(int aStatus,
                                                 int aClientIf,
                                                 const BluetoothUuid& aAppUuid)
{
  BT_API2_LOGR("Client Registered, clientIf = %d", aClientIf);
  MOZ_ASSERT(NS_IsMainThread());

  nsString uuid;
  UuidToString(aAppUuid, uuid);

  size_t index = sClients->IndexOf(uuid, 0 , UuidComparator());
  NS_ENSURE_TRUE_VOID(index != sClients->NoIndex);
  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  if (aStatus) { 
    BT_API2_LOGR(
      "RegisterClient failed, clientIf = %d, status = %d, appUuid = %s",
      aClientIf, aStatus, NS_ConvertUTF16toUTF8(uuid).get());

    
    bs->DistributeSignal(
      NS_LITERAL_STRING(GATT_CONNECTION_STATE_CHANGED_ID),
      uuid, BluetoothValue(false)); 

    
    if (client->mConnectRunnable) {
      DispatchReplyError(client->mConnectRunnable,
                         NS_LITERAL_STRING(
                           "Connect failed due to registration failed"));
      client->mConnectRunnable = nullptr;
    }

    sClients->RemoveElement(client);
    return;
  }

  client->mClientIf = aClientIf;

  
  bs->DistributeSignal(
    NS_LITERAL_STRING("ClientRegistered"),
    uuid, BluetoothValue(uint32_t(aClientIf)));

  
  if (client->mConnectRunnable) {
    sBluetoothGattClientInterface->Connect(
      aClientIf, client->mDeviceAddr, true ,
      new ConnectResultHandler(client));
  }
}

void
BluetoothGattManager::ScanResultNotification(
  const nsAString& aBdAddr, int aRssi,
  const BluetoothGattAdvData& aAdvData)
{ }

void
BluetoothGattManager::ConnectNotification(int aConnId,
                                          int aStatus,
                                          int aClientIf,
                                          const nsAString& aDeviceAddr)
{
  BT_API2_LOGR();
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  size_t index = sClients->IndexOf(aClientIf, 0 ,
                                   ClientIfComparator());
  NS_ENSURE_TRUE_VOID(index != sClients->NoIndex);
  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  if (aStatus) { 
    BT_API2_LOGR("Connect failed, clientIf = %d, connId = %d, status = %d",
                 aClientIf, aConnId, aStatus);

    
    bs->DistributeSignal(
      NS_LITERAL_STRING(GATT_CONNECTION_STATE_CHANGED_ID),
      client->mAppUuid,
      BluetoothValue(false)); 

    
    if (client->mConnectRunnable) {
      DispatchReplyError(client->mConnectRunnable,
                         NS_LITERAL_STRING("Connect failed"));
      client->mConnectRunnable = nullptr;
    }

    return;
  }

  client->mConnId = aConnId;

  
  bs->DistributeSignal(
    NS_LITERAL_STRING(GATT_CONNECTION_STATE_CHANGED_ID),
    client->mAppUuid,
    BluetoothValue(true)); 

  
  if (client->mConnectRunnable) {
    DispatchReplySuccess(client->mConnectRunnable);
    client->mConnectRunnable = nullptr;
  }
}

void
BluetoothGattManager::DisconnectNotification(int aConnId,
                                             int aStatus,
                                             int aClientIf,
                                             const nsAString& aDeviceAddr)
{
  BT_API2_LOGR();
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  size_t index = sClients->IndexOf(aClientIf, 0 ,
                                   ClientIfComparator());
  NS_ENSURE_TRUE_VOID(index != sClients->NoIndex);
  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  if (aStatus) { 
    
    bs->DistributeSignal(
      NS_LITERAL_STRING(GATT_CONNECTION_STATE_CHANGED_ID),
      client->mAppUuid,
      BluetoothValue(true)); 

    
    if (client->mDisconnectRunnable) {
      DispatchReplyError(client->mDisconnectRunnable,
                         NS_LITERAL_STRING("Disconnect failed"));
      client->mDisconnectRunnable = nullptr;
    }

    return;
  }

  client->mConnId = 0;

  
  bs->DistributeSignal(
    NS_LITERAL_STRING(GATT_CONNECTION_STATE_CHANGED_ID),
    client->mAppUuid,
    BluetoothValue(false)); 

  
  if (client->mDisconnectRunnable) {
    DispatchReplySuccess(client->mDisconnectRunnable);
    client->mDisconnectRunnable = nullptr;
  }
}

void
BluetoothGattManager::SearchCompleteNotification(int aConnId, int aStatus)
{ }

void
BluetoothGattManager::SearchResultNotification(
  int aConnId, const BluetoothGattServiceId& aServiceId)
{ }

void
BluetoothGattManager::GetCharacteristicNotification(
  int aConnId, int aStatus,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharId,
  int aCharProperty)
{ }

void
BluetoothGattManager::GetDescriptorNotification(
  int aConnId, int aStatus,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharId,
  const BluetoothGattId& aDescriptorId)
{ }

void
BluetoothGattManager::GetIncludedServiceNotification(
  int aConnId, int aStatus,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattServiceId& aIncludedServId)
{ }

void
BluetoothGattManager::RegisterNotificationNotification(
  int aConnId, int aIsRegister, int aStatus,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharId)
{ }

void
BluetoothGattManager::NotifyNotification(
  int aConnId, const BluetoothGattNotifyParam& aNotifyParam)
{ }

void
BluetoothGattManager::ReadCharacteristicNotification(
  int aConnId, int aStatus, const BluetoothGattReadParam& aReadParam)
{ }

void
BluetoothGattManager::WriteCharacteristicNotification(
  int aConnId, int aStatus, const BluetoothGattWriteParam& aWriteParam)
{ }

void
BluetoothGattManager::ReadDescriptorNotification(
  int aConnId, int aStatus, const BluetoothGattReadParam& aReadParam)
{ }

void
BluetoothGattManager::WriteDescriptorNotification(
  int aConnId, int aStatus, const BluetoothGattWriteParam& aWriteParam)
{ }

void
BluetoothGattManager::ExecuteWriteNotification(int aConnId, int aStatus)
{ }

void
BluetoothGattManager::ReadRemoteRssiNotification(int aClientIf,
                                                 const nsAString& aBdAddr,
                                                 int aRssi,
                                                 int aStatus)
{ }

void
BluetoothGattManager::ListenNotification(int aStatus,
                                         int aServerIf)
{ }

BluetoothGattManager::BluetoothGattManager()
{ }

BluetoothGattManager::~BluetoothGattManager()
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE_VOID(obs);
  if (NS_FAILED(obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID))) {
    BT_WARNING("Failed to remove shutdown observer!");
  }
}

NS_IMETHODIMP
BluetoothGattManager::Observe(nsISupports* aSubject,
                              const char* aTopic,
                              const char16_t* aData)
{
  MOZ_ASSERT(sBluetoothGattManager);

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    HandleShutdown();
    return NS_OK;
  }

  MOZ_ASSERT(false, "BluetoothGattManager got unexpected topic!");
  return NS_ERROR_UNEXPECTED;
}

void
BluetoothGattManager::HandleShutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  mInShutdown = true;
  sBluetoothGattManager = nullptr;
}

NS_IMPL_ISUPPORTS(BluetoothGattManager, nsIObserver)
