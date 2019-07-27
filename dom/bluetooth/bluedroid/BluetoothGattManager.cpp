





#include "BluetoothGattManager.h"

#include "BluetoothInterface.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothUtils.h"
#include "MainThreadUtils.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"
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

static StaticAutoPtr<nsTArray<nsRefPtr<BluetoothGattClient> > > sClients;

struct BluetoothGattClientReadCharState
{
  bool mAuthRetry;
  nsRefPtr<BluetoothReplyRunnable> mRunnable;

  void Assign(bool aAuthRetry,
              BluetoothReplyRunnable* aRunnable)
  {
    mAuthRetry = aAuthRetry;
    mRunnable = aRunnable;
  }

  void Reset()
  {
    mAuthRetry = false;
    mRunnable = nullptr;
  }
};

struct BluetoothGattClientWriteCharState
{
  BluetoothGattWriteType mWriteType;
  nsTArray<uint8_t> mWriteValue;
  bool mAuthRetry;
  nsRefPtr<BluetoothReplyRunnable> mRunnable;

  void Assign(BluetoothGattWriteType aWriteType,
              const nsTArray<uint8_t>& aWriteValue,
              bool aAuthRetry,
              BluetoothReplyRunnable* aRunnable)
  {
    mWriteType = aWriteType;
    mWriteValue = aWriteValue;
    mAuthRetry = aAuthRetry;
    mRunnable = aRunnable;
  }

  void Reset()
  {
    mWriteType = GATT_WRITE_TYPE_NORMAL;
    mWriteValue.Clear();
    mAuthRetry = false;
    mRunnable = nullptr;
  }
};

struct BluetoothGattClientReadDescState
{
  bool mAuthRetry;
  nsRefPtr<BluetoothReplyRunnable> mRunnable;

  void Assign(bool aAuthRetry,
              BluetoothReplyRunnable* aRunnable)
  {
    mAuthRetry = aAuthRetry;
    mRunnable = aRunnable;
  }

  void Reset()
  {
    mAuthRetry = false;
    mRunnable = nullptr;
  }
};

struct BluetoothGattClientWriteDescState
{
  nsTArray<uint8_t> mWriteValue;
  bool mAuthRetry;
  nsRefPtr<BluetoothReplyRunnable> mRunnable;

  void Assign(const nsTArray<uint8_t>& aWriteValue,
              bool aAuthRetry,
              BluetoothReplyRunnable* aRunnable)
  {
    mWriteValue = aWriteValue;
    mAuthRetry = aAuthRetry;
    mRunnable = aRunnable;
  }

  void Reset()
  {
    mWriteValue.Clear();
    mAuthRetry = false;
    mRunnable = nullptr;
  }
};

class mozilla::dom::bluetooth::BluetoothGattClient final : public nsISupports
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
    mDiscoverRunnable = nullptr;
    mUnregisterClientRunnable = nullptr;
    mReadRemoteRssiRunnable = nullptr;
    mRegisterNotificationsRunnable = nullptr;
    mDeregisterNotificationsRunnable = nullptr;
    mReadCharacteristicState.Reset();
    mWriteCharacteristicState.Reset();
    mReadDescriptorState.Reset();
    mWriteDescriptorState.Reset();
  }

  void NotifyDiscoverCompleted(bool aSuccess)
  {
    MOZ_ASSERT(!mAppUuid.IsEmpty());
    MOZ_ASSERT(mDiscoverRunnable);

    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE_VOID(bs);

    
    
    bs->DistributeSignal(NS_LITERAL_STRING("DiscoverCompleted"),
                         mAppUuid,
                         BluetoothValue(aSuccess));

    
    if (aSuccess) {
      DispatchReplySuccess(mDiscoverRunnable);
    } else {
      DispatchReplyError(mDiscoverRunnable,
                         NS_LITERAL_STRING("Discover failed"));
    }

    
    mServices.Clear();
    mIncludedServices.Clear();
    mCharacteristics.Clear();
    mDescriptors.Clear();
    mDiscoverRunnable = nullptr;
  }

  nsString mAppUuid;
  nsString mDeviceAddr;
  int mClientIf;
  int mConnId;
  nsRefPtr<BluetoothReplyRunnable> mConnectRunnable;
  nsRefPtr<BluetoothReplyRunnable> mDisconnectRunnable;
  nsRefPtr<BluetoothReplyRunnable> mDiscoverRunnable;
  nsRefPtr<BluetoothReplyRunnable> mReadRemoteRssiRunnable;
  nsRefPtr<BluetoothReplyRunnable> mRegisterNotificationsRunnable;
  nsRefPtr<BluetoothReplyRunnable> mDeregisterNotificationsRunnable;
  nsRefPtr<BluetoothReplyRunnable> mUnregisterClientRunnable;

  BluetoothGattClientReadCharState mReadCharacteristicState;
  BluetoothGattClientWriteCharState mWriteCharacteristicState;
  BluetoothGattClientReadDescState mReadDescriptorState;
  BluetoothGattClientWriteDescState mWriteDescriptorState;

  



  nsTArray<BluetoothGattServiceId> mServices;
  nsTArray<BluetoothGattServiceId> mIncludedServices;
  nsTArray<BluetoothGattCharAttribute> mCharacteristics;
  nsTArray<BluetoothGattId> mDescriptors;
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

class ConnIdComparator
{
public:
  bool Equals(const nsRefPtr<BluetoothGattClient>& aClient,
              int aConnId) const
  {
    return aClient->mConnId == aConnId;
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

class BluetoothGattManager::InitGattResultHandler final
  : public BluetoothGattResultHandler
{
public:
  InitGattResultHandler(BluetoothProfileResultHandler* aRes)
  : mRes(aRes)
  { }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING("BluetoothGattInterface::Init failed: %d",
               (int)aStatus);
    if (mRes) {
      mRes->OnError(NS_ERROR_FAILURE);
    }
  }

  void Init() override
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

class BluetoothGattManager::CleanupResultHandler final
  : public BluetoothGattResultHandler
{
public:
  CleanupResultHandler(BluetoothProfileResultHandler* aRes)
  : mRes(aRes)
  { }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING("BluetoothGattInterface::Cleanup failed: %d",
               (int)aStatus);
    if (mRes) {
      mRes->OnError(NS_ERROR_FAILURE);
    }
  }

  void Cleanup() override
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

class BluetoothGattManager::CleanupResultHandlerRunnable final
  : public nsRunnable
{
public:
  CleanupResultHandlerRunnable(BluetoothProfileResultHandler* aRes)
  : mRes(aRes)
  {
    MOZ_ASSERT(mRes);
  }

  NS_IMETHOD Run() override
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

class BluetoothGattManager::RegisterClientResultHandler final
  : public BluetoothGattClientResultHandler
{
public:
  RegisterClientResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void OnError(BluetoothStatus aStatus) override
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

class BluetoothGattManager::UnregisterClientResultHandler final
  : public BluetoothGattClientResultHandler
{
public:
  UnregisterClientResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void UnregisterClient() override
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

  void OnError(BluetoothStatus aStatus) override
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
  MOZ_ASSERT(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);
  client->mUnregisterClientRunnable = aRunnable;

  sBluetoothGattClientInterface->UnregisterClient(
    aClientIf,
    new UnregisterClientResultHandler(client));
}

class BluetoothGattManager::ConnectResultHandler final
  : public BluetoothGattClientResultHandler
{
public:
  ConnectResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void OnError(BluetoothStatus aStatus) override
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

class BluetoothGattManager::DisconnectResultHandler final
  : public BluetoothGattClientResultHandler
{
public:
  DisconnectResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void OnError(BluetoothStatus aStatus) override
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
  MOZ_ASSERT(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);
  client->mDisconnectRunnable = aRunnable;

  sBluetoothGattClientInterface->Disconnect(
    client->mClientIf,
    aDeviceAddr,
    client->mConnId,
    new DisconnectResultHandler(client));
}

class BluetoothGattManager::DiscoverResultHandler final
  : public BluetoothGattClientResultHandler
{
public:
  DiscoverResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING("BluetoothGattClientInterface::Discover failed: %d",
               (int)aStatus);

    mClient->NotifyDiscoverCompleted(false);
  }

private:
  nsRefPtr<BluetoothGattClient> mClient;
};

void
BluetoothGattManager::Discover(const nsAString& aAppUuid,
                               BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  ENSURE_GATT_CLIENT_INTF_IS_READY_VOID(aRunnable);

  size_t index = sClients->IndexOf(aAppUuid, 0 , UuidComparator());
  MOZ_ASSERT(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);
  MOZ_ASSERT(client->mConnId > 0);
  MOZ_ASSERT(!client->mDiscoverRunnable);

  client->mDiscoverRunnable = aRunnable;

  












  sBluetoothGattClientInterface->SearchService(
    client->mConnId,
    true, 
    BluetoothUuid(),
    new DiscoverResultHandler(client));
}

class BluetoothGattManager::ReadRemoteRssiResultHandler final
  : public BluetoothGattClientResultHandler
{
public:
  ReadRemoteRssiResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING("BluetoothGattClientInterface::ReadRemoteRssi failed: %d",
               (int)aStatus);
    MOZ_ASSERT(mClient->mReadRemoteRssiRunnable);

    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE_VOID(bs);

    
    DispatchReplyError(mClient->mReadRemoteRssiRunnable,
                       NS_LITERAL_STRING("ReadRemoteRssi failed"));
    mClient->mReadRemoteRssiRunnable = nullptr;
  }

private:
  nsRefPtr<BluetoothGattClient> mClient;
};

void
BluetoothGattManager::ReadRemoteRssi(int aClientIf,
                                     const nsAString& aDeviceAddr,
                                     BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  ENSURE_GATT_CLIENT_INTF_IS_READY_VOID(aRunnable);

  size_t index = sClients->IndexOf(aClientIf, 0 ,
                                   ClientIfComparator());
  MOZ_ASSERT(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);
  client->mReadRemoteRssiRunnable = aRunnable;

  sBluetoothGattClientInterface->ReadRemoteRssi(
    aClientIf, aDeviceAddr,
    new ReadRemoteRssiResultHandler(client));
}

class BluetoothGattManager::RegisterNotificationsResultHandler final
  : public BluetoothGattClientResultHandler
{
public:
  RegisterNotificationsResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void RegisterNotification() override
  {
    MOZ_ASSERT(mClient->mRegisterNotificationsRunnable);

    








    DispatchReplySuccess(mClient->mRegisterNotificationsRunnable);
    mClient->mRegisterNotificationsRunnable = nullptr;
  }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING(
      "BluetoothGattClientInterface::RegisterNotifications failed: %d",
      (int)aStatus);
    MOZ_ASSERT(mClient->mRegisterNotificationsRunnable);

    DispatchReplyError(mClient->mRegisterNotificationsRunnable,
                       NS_LITERAL_STRING("RegisterNotifications failed"));
    mClient->mRegisterNotificationsRunnable = nullptr;
  }

private:
  nsRefPtr<BluetoothGattClient> mClient;
};

void
BluetoothGattManager::RegisterNotifications(
  const nsAString& aAppUuid, const BluetoothGattServiceId& aServId,
  const BluetoothGattId& aCharId, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  ENSURE_GATT_CLIENT_INTF_IS_READY_VOID(aRunnable);

  size_t index = sClients->IndexOf(aAppUuid, 0 , UuidComparator());
  MOZ_ASSERT(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  
  
  if (client->mRegisterNotificationsRunnable || client->mConnId <= 0) {
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("RegisterNotifications failed"));
    return;
  }

  client->mRegisterNotificationsRunnable = aRunnable;

  sBluetoothGattClientInterface->RegisterNotification(
    client->mClientIf, client->mDeviceAddr, aServId, aCharId,
    new RegisterNotificationsResultHandler(client));
}

class BluetoothGattManager::DeregisterNotificationsResultHandler final
  : public BluetoothGattClientResultHandler
{
public:
  DeregisterNotificationsResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void DeregisterNotification() override
  {
    MOZ_ASSERT(mClient->mDeregisterNotificationsRunnable);

    








    DispatchReplySuccess(mClient->mDeregisterNotificationsRunnable);
    mClient->mDeregisterNotificationsRunnable = nullptr;
  }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING(
      "BluetoothGattClientInterface::DeregisterNotifications failed: %d",
      (int)aStatus);
    MOZ_ASSERT(mClient->mDeregisterNotificationsRunnable);

    DispatchReplyError(mClient->mDeregisterNotificationsRunnable,
                       NS_LITERAL_STRING("DeregisterNotifications failed"));
    mClient->mDeregisterNotificationsRunnable = nullptr;
  }

private:
  nsRefPtr<BluetoothGattClient> mClient;
};

void
BluetoothGattManager::DeregisterNotifications(
  const nsAString& aAppUuid, const BluetoothGattServiceId& aServId,
  const BluetoothGattId& aCharId, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  ENSURE_GATT_CLIENT_INTF_IS_READY_VOID(aRunnable);

  size_t index = sClients->IndexOf(aAppUuid, 0 , UuidComparator());
  MOZ_ASSERT(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  
  if (client->mDeregisterNotificationsRunnable) {
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("DeregisterNotifications failed"));
    return;
  }

  client->mDeregisterNotificationsRunnable = aRunnable;

  sBluetoothGattClientInterface->DeregisterNotification(
    client->mClientIf, client->mDeviceAddr, aServId, aCharId,
    new DeregisterNotificationsResultHandler(client));
}

class BluetoothGattManager::ReadCharacteristicValueResultHandler final
  : public BluetoothGattClientResultHandler
{
public:
  ReadCharacteristicValueResultHandler(BluetoothGattClient* aClient)
    : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING("BluetoothGattClientInterface::ReadCharacteristicValue failed" \
               ": %d", (int)aStatus);
    MOZ_ASSERT(mClient->mReadCharacteristicState.mRunnable);

    nsRefPtr<BluetoothReplyRunnable> runnable =
      mClient->mReadCharacteristicState.mRunnable;
    mClient->mReadCharacteristicState.Reset();

    
    DispatchReplyError(runnable,
                       NS_LITERAL_STRING("ReadCharacteristicValue failed"));
  }

private:
  nsRefPtr<BluetoothGattClient> mClient;
};

void
BluetoothGattManager::ReadCharacteristicValue(
  const nsAString& aAppUuid,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharacteristicId,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  ENSURE_GATT_CLIENT_INTF_IS_READY_VOID(aRunnable);

  size_t index = sClients->IndexOf(aAppUuid, 0 , UuidComparator());
  if (NS_WARN_IF(index == sClients->NoIndex)) {
    
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("ReadCharacteristicValue failed"));
    return;
  }

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  







  if (client->mReadCharacteristicState.mRunnable) {
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("ReadCharacteristicValue failed"));
    return;
  }

  client->mReadCharacteristicState.Assign(false, aRunnable);

  




  sBluetoothGattClientInterface->ReadCharacteristic(
    client->mConnId,
    aServiceId,
    aCharacteristicId,
    GATT_AUTH_REQ_NONE,
    new ReadCharacteristicValueResultHandler(client));
}

class BluetoothGattManager::WriteCharacteristicValueResultHandler final
  : public BluetoothGattClientResultHandler
{
public:
  WriteCharacteristicValueResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING("BluetoothGattClientInterface::WriteCharacteristicValue failed" \
               ": %d", (int)aStatus);
    MOZ_ASSERT(mClient->mWriteCharacteristicState.mRunnable);

    nsRefPtr<BluetoothReplyRunnable> runnable =
      mClient->mWriteCharacteristicState.mRunnable;
    mClient->mWriteCharacteristicState.Reset();

    
    DispatchReplyError(runnable,
                       NS_LITERAL_STRING("WriteCharacteristicValue failed"));
  }

private:
  nsRefPtr<BluetoothGattClient> mClient;
};

void
BluetoothGattManager::WriteCharacteristicValue(
  const nsAString& aAppUuid,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharacteristicId,
  const BluetoothGattWriteType& aWriteType,
  const nsTArray<uint8_t>& aValue,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  ENSURE_GATT_CLIENT_INTF_IS_READY_VOID(aRunnable);

  size_t index = sClients->IndexOf(aAppUuid, 0 , UuidComparator());
  if (NS_WARN_IF(index == sClients->NoIndex)) {
    
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("WriteCharacteristicValue failed"));
    return;
  }

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  







  if (client->mWriteCharacteristicState.mRunnable) {
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("WriteCharacteristicValue failed"));
    return;
  }

  client->mWriteCharacteristicState.Assign(aWriteType, aValue, false, aRunnable);

  




  sBluetoothGattClientInterface->WriteCharacteristic(
    client->mConnId,
    aServiceId,
    aCharacteristicId,
    aWriteType,
    GATT_AUTH_REQ_NONE,
    aValue,
    new WriteCharacteristicValueResultHandler(client));
}

class BluetoothGattManager::ReadDescriptorValueResultHandler final
  : public BluetoothGattClientResultHandler
{
public:
  ReadDescriptorValueResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING("BluetoothGattClientInterface::ReadDescriptorValue failed: %d",
               (int)aStatus);
    MOZ_ASSERT(mClient->mReadDescriptorState.mRunnable);

    nsRefPtr<BluetoothReplyRunnable> runnable =
      mClient->mReadDescriptorState.mRunnable;
    mClient->mReadDescriptorState.Reset();

    
    DispatchReplyError(runnable,
                       NS_LITERAL_STRING("ReadDescriptorValue failed"));
  }

private:
  nsRefPtr<BluetoothGattClient> mClient;
};

void
BluetoothGattManager::ReadDescriptorValue(
  const nsAString& aAppUuid,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharacteristicId,
  const BluetoothGattId& aDescriptorId,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  ENSURE_GATT_CLIENT_INTF_IS_READY_VOID(aRunnable);

  size_t index = sClients->IndexOf(aAppUuid, 0 , UuidComparator());
  if (NS_WARN_IF(index == sClients->NoIndex)) {
    
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("ReadDescriptorValue failed"));
    return;
  }

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  







  if (client->mReadDescriptorState.mRunnable) {
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("ReadDescriptorValue failed"));
    return;
  }

  client->mReadDescriptorState.Assign(false, aRunnable);

  




  sBluetoothGattClientInterface->ReadDescriptor(
    client->mConnId,
    aServiceId,
    aCharacteristicId,
    aDescriptorId,
    GATT_AUTH_REQ_NONE,
    new ReadDescriptorValueResultHandler(client));
}

class BluetoothGattManager::WriteDescriptorValueResultHandler final
  : public BluetoothGattClientResultHandler
{
public:
  WriteDescriptorValueResultHandler(BluetoothGattClient* aClient)
  : mClient(aClient)
  {
    MOZ_ASSERT(mClient);
  }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING("BluetoothGattClientInterface::WriteDescriptorValue failed: %d",
               (int)aStatus);
    MOZ_ASSERT(mClient->mWriteDescriptorState.mRunnable);

    nsRefPtr<BluetoothReplyRunnable> runnable =
      mClient->mWriteDescriptorState.mRunnable;
    mClient->mWriteDescriptorState.Reset();

    
    DispatchReplyError(runnable,
                       NS_LITERAL_STRING("WriteDescriptorValue failed"));
  }

private:
  nsRefPtr<BluetoothGattClient> mClient;
};

void
BluetoothGattManager::WriteDescriptorValue(
  const nsAString& aAppUuid,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharacteristicId,
  const BluetoothGattId& aDescriptorId,
  const nsTArray<uint8_t>& aValue,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  ENSURE_GATT_CLIENT_INTF_IS_READY_VOID(aRunnable);

  size_t index = sClients->IndexOf(aAppUuid, 0 , UuidComparator());
  if (NS_WARN_IF(index == sClients->NoIndex)) {
    
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("WriteDescriptorValue failed"));
    return;
  }

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  







  if (client->mWriteDescriptorState.mRunnable) {
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("WriteDescriptorValue failed"));
    return;
  }

  




  client->mWriteDescriptorState.Assign(aValue, false, aRunnable);

  sBluetoothGattClientInterface->WriteDescriptor(
    client->mConnId,
    aServiceId,
    aCharacteristicId,
    aDescriptorId,
    GATT_WRITE_TYPE_NORMAL,
    GATT_AUTH_REQ_NONE,
    aValue,
    new WriteDescriptorValueResultHandler(client));
}




void
BluetoothGattManager::RegisterClientNotification(BluetoothGattStatus aStatus,
                                                 int aClientIf,
                                                 const BluetoothUuid& aAppUuid)
{
  BT_API2_LOGR("Client Registered, clientIf = %d", aClientIf);
  MOZ_ASSERT(NS_IsMainThread());

  nsString uuid;
  UuidToString(aAppUuid, uuid);

  size_t index = sClients->IndexOf(uuid, 0 , UuidComparator());
  MOZ_ASSERT(index != sClients->NoIndex);
  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  if (aStatus != GATT_STATUS_SUCCESS) {
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
                                          BluetoothGattStatus aStatus,
                                          int aClientIf,
                                          const nsAString& aDeviceAddr)
{
  BT_API2_LOGR();
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  size_t index = sClients->IndexOf(aClientIf, 0 ,
                                   ClientIfComparator());
  MOZ_ASSERT(index != sClients->NoIndex);
  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  if (aStatus != GATT_STATUS_SUCCESS) {
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
                                             BluetoothGattStatus aStatus,
                                             int aClientIf,
                                             const nsAString& aDeviceAddr)
{
  BT_API2_LOGR();
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  size_t index = sClients->IndexOf(aClientIf, 0 ,
                                   ClientIfComparator());
  MOZ_ASSERT(index != sClients->NoIndex);
  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  if (aStatus != GATT_STATUS_SUCCESS) {
    
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
BluetoothGattManager::SearchCompleteNotification(int aConnId,
                                                 BluetoothGattStatus aStatus)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  size_t index = sClients->IndexOf(aConnId, 0 ,
                                   ConnIdComparator());
  MOZ_ASSERT(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);
  MOZ_ASSERT(client->mDiscoverRunnable);

  if (aStatus != GATT_STATUS_SUCCESS) {
    client->NotifyDiscoverCompleted(false);
    return;
  }

  
  bs->DistributeSignal(NS_LITERAL_STRING("ServicesDiscovered"),
                       client->mAppUuid,
                       BluetoothValue(client->mServices));

  
  
  if (!client->mServices.IsEmpty()) {
    sBluetoothGattClientInterface->GetIncludedService(
      aConnId,
      client->mServices[0], 
      true, 
      BluetoothGattServiceId(),
      new DiscoverResultHandler(client));
  } else {
    client->NotifyDiscoverCompleted(true);
  }
}

void
BluetoothGattManager::SearchResultNotification(
  int aConnId, const BluetoothGattServiceId& aServiceId)
{
  MOZ_ASSERT(NS_IsMainThread());

  size_t index = sClients->IndexOf(aConnId, 0 ,
                                   ConnIdComparator());
  MOZ_ASSERT(index != sClients->NoIndex);

  
  
  sClients->ElementAt(index)->mServices.AppendElement(aServiceId);
}

void
BluetoothGattManager::GetCharacteristicNotification(
  int aConnId, BluetoothGattStatus aStatus,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharId,
  const BluetoothGattCharProp& aCharProperty)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  size_t index = sClients->IndexOf(aConnId, 0 ,
                                   ConnIdComparator());
  MOZ_ASSERT(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);
  MOZ_ASSERT(client->mDiscoverRunnable);

  if (aStatus == GATT_STATUS_SUCCESS) {
    BluetoothGattCharAttribute attribute;
    attribute.mId = aCharId;
    attribute.mProperties = aCharProperty;
    attribute.mWriteType =
      aCharProperty & GATT_CHAR_PROP_BIT_WRITE_NO_RESPONSE
        ? GATT_WRITE_TYPE_NO_RESPONSE
        : GATT_WRITE_TYPE_NORMAL;

    
    
    client->mCharacteristics.AppendElement(attribute);

    
    sBluetoothGattClientInterface->GetCharacteristic(
      aConnId,
      aServiceId,
      false,
      aCharId,
      new DiscoverResultHandler(client));
  } else { 
    
    nsString path;
    GeneratePathFromGattId(aServiceId.mId, path);

    bs->DistributeSignal(NS_LITERAL_STRING("CharacteristicsDiscovered"),
                         path,
                         BluetoothValue(client->mCharacteristics));

    ProceedDiscoverProcess(client, aServiceId);
  }
}

void
BluetoothGattManager::GetDescriptorNotification(
  int aConnId, BluetoothGattStatus aStatus,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharId,
  const BluetoothGattId& aDescriptorId)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  size_t index = sClients->IndexOf(aConnId, 0 ,
                                   ConnIdComparator());
  MOZ_ASSERT(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);
  MOZ_ASSERT(client->mDiscoverRunnable);

  if (aStatus == GATT_STATUS_SUCCESS) {
    
    client->mDescriptors.AppendElement(aDescriptorId);

    
    sBluetoothGattClientInterface->GetDescriptor(
      aConnId,
      aServiceId,
      aCharId,
      false,
      aDescriptorId,
      new DiscoverResultHandler(client));
  } else { 
    
    nsString path;
    GeneratePathFromGattId(aCharId, path);

    bs->DistributeSignal(NS_LITERAL_STRING("DescriptorsDiscovered"),
                         path,
                         BluetoothValue(client->mDescriptors));
    client->mDescriptors.Clear();

    ProceedDiscoverProcess(client, aServiceId);
  }
}

void
BluetoothGattManager::GetIncludedServiceNotification(
  int aConnId, BluetoothGattStatus aStatus,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattServiceId& aIncludedServId)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  size_t index = sClients->IndexOf(aConnId, 0 ,
                                   ConnIdComparator());
  MOZ_ASSERT(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);
  MOZ_ASSERT(client->mDiscoverRunnable);

  if (aStatus == GATT_STATUS_SUCCESS) {
    
    client->mIncludedServices.AppendElement(aIncludedServId);

    
    sBluetoothGattClientInterface->GetIncludedService(
      aConnId,
      aServiceId,
      false,
      aIncludedServId,
      new DiscoverResultHandler(client));
  } else { 
    
    nsString path;
    GeneratePathFromGattId(aServiceId.mId, path);

    bs->DistributeSignal(NS_LITERAL_STRING("IncludedServicesDiscovered"),
                         path,
                         BluetoothValue(client->mIncludedServices));
    client->mIncludedServices.Clear();

    
    sBluetoothGattClientInterface->GetCharacteristic(
      aConnId,
      aServiceId,
      true, 
      BluetoothGattId(),
      new DiscoverResultHandler(client));
  }
}

void
BluetoothGattManager::RegisterNotificationNotification(
  int aConnId, int aIsRegister, BluetoothGattStatus aStatus,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharId)
{
  MOZ_ASSERT(NS_IsMainThread());
  BT_LOGD("aStatus = %d, aConnId = %d, aIsRegister = %d",
          aStatus, aConnId, aIsRegister);

  












}

void
BluetoothGattManager::NotifyNotification(
  int aConnId, const BluetoothGattNotifyParam& aNotifyParam)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  size_t index = sClients->IndexOf(aConnId, 0 , ConnIdComparator());
  NS_ENSURE_TRUE_VOID(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  
  nsString path;
  GeneratePathFromGattId(aNotifyParam.mCharId, path);

  nsTArray<uint8_t> value;
  value.AppendElements(aNotifyParam.mValue, aNotifyParam.mLength);

  bs->DistributeSignal(NS_LITERAL_STRING("CharacteristicValueUpdated"),
                       path,
                       BluetoothValue(value));

  
  nsTArray<BluetoothNamedValue> ids;
  ids.AppendElement(BluetoothNamedValue(NS_LITERAL_STRING("serviceId"),
                                        aNotifyParam.mServiceId));
  ids.AppendElement(BluetoothNamedValue(NS_LITERAL_STRING("charId"),
                                        aNotifyParam.mCharId));

  bs->DistributeSignal(NS_LITERAL_STRING(GATT_CHARACTERISTIC_CHANGED_ID),
                       client->mAppUuid,
                       BluetoothValue(ids));
}

void
BluetoothGattManager::ReadCharacteristicNotification(
  int aConnId, BluetoothGattStatus aStatus,
  const BluetoothGattReadParam& aReadParam)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  size_t index = sClients->IndexOf(aConnId, 0 , ConnIdComparator());
  NS_ENSURE_TRUE_VOID(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  MOZ_ASSERT(client->mReadCharacteristicState.mRunnable);
  nsRefPtr<BluetoothReplyRunnable> runnable =
    client->mReadCharacteristicState.mRunnable;

  if (aStatus == GATT_STATUS_SUCCESS) {
    client->mReadCharacteristicState.Reset();
    
    nsString path;
    GeneratePathFromGattId(aReadParam.mCharId, path);

    nsTArray<uint8_t> value;
    value.AppendElements(aReadParam.mValue, aReadParam.mValueLength);

    bs->DistributeSignal(NS_LITERAL_STRING("CharacteristicValueUpdated"),
                         path,
                         BluetoothValue(value));

    
    nsTArray<BluetoothNamedValue> ids;
    ids.AppendElement(BluetoothNamedValue(NS_LITERAL_STRING("serviceId"),
                                          aReadParam.mServiceId));
    ids.AppendElement(BluetoothNamedValue(NS_LITERAL_STRING("charId"),
                                          aReadParam.mCharId));

    bs->DistributeSignal(NS_LITERAL_STRING(GATT_CHARACTERISTIC_CHANGED_ID),
                         client->mAppUuid,
                         BluetoothValue(ids));

    
    DispatchReplySuccess(runnable, BluetoothValue(value));
  } else if (!client->mReadCharacteristicState.mAuthRetry &&
             (aStatus == GATT_STATUS_INSUFFICIENT_AUTHENTICATION ||
              aStatus == GATT_STATUS_INSUFFICIENT_ENCRYPTION)) {
    client->mReadCharacteristicState.mAuthRetry = true;
    
    sBluetoothGattClientInterface->ReadCharacteristic(
      aConnId,
      aReadParam.mServiceId,
      aReadParam.mCharId,
      GATT_AUTH_REQ_MITM,
      new ReadCharacteristicValueResultHandler(client));
  } else {
    client->mReadCharacteristicState.Reset();
    
    DispatchReplyError(runnable,
                       NS_LITERAL_STRING("ReadCharacteristicValue failed"));
  }
}

void
BluetoothGattManager::WriteCharacteristicNotification(
  int aConnId, BluetoothGattStatus aStatus,
  const BluetoothGattWriteParam& aWriteParam)
{
  MOZ_ASSERT(NS_IsMainThread());

  size_t index = sClients->IndexOf(aConnId, 0 , ConnIdComparator());
  NS_ENSURE_TRUE_VOID(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  MOZ_ASSERT(client->mWriteCharacteristicState.mRunnable);
  nsRefPtr<BluetoothReplyRunnable> runnable =
    client->mWriteCharacteristicState.mRunnable;

  if (aStatus == GATT_STATUS_SUCCESS) {
    client->mWriteCharacteristicState.Reset();
    
    DispatchReplySuccess(runnable);
  } else if (!client->mWriteCharacteristicState.mAuthRetry &&
             (aStatus == GATT_STATUS_INSUFFICIENT_AUTHENTICATION ||
              aStatus == GATT_STATUS_INSUFFICIENT_ENCRYPTION)) {
    client->mWriteCharacteristicState.mAuthRetry = true;
    
    sBluetoothGattClientInterface->WriteCharacteristic(
      aConnId,
      aWriteParam.mServiceId,
      aWriteParam.mCharId,
      client->mWriteCharacteristicState.mWriteType,
      GATT_AUTH_REQ_MITM,
      client->mWriteCharacteristicState.mWriteValue,
      new WriteCharacteristicValueResultHandler(client));
  } else {
    client->mWriteCharacteristicState.Reset();
    
    DispatchReplyError(runnable,
                       NS_LITERAL_STRING("WriteCharacteristicValue failed"));
  }
}

void
BluetoothGattManager::ReadDescriptorNotification(
  int aConnId, BluetoothGattStatus aStatus,
  const BluetoothGattReadParam& aReadParam)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  size_t index = sClients->IndexOf(aConnId, 0 , ConnIdComparator());
  NS_ENSURE_TRUE_VOID(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  MOZ_ASSERT(client->mReadDescriptorState.mRunnable);
  nsRefPtr<BluetoothReplyRunnable> runnable =
    client->mReadDescriptorState.mRunnable;

  if (aStatus == GATT_STATUS_SUCCESS) {
    client->mReadDescriptorState.Reset();
    
    nsString path;
    GeneratePathFromGattId(aReadParam.mDescriptorId, path);

    nsTArray<uint8_t> value;
    value.AppendElements(aReadParam.mValue, aReadParam.mValueLength);

    bs->DistributeSignal(NS_LITERAL_STRING("DescriptorValueUpdated"),
                         path,
                         BluetoothValue(value));

    
    DispatchReplySuccess(runnable, BluetoothValue(value));
  } else if (!client->mReadDescriptorState.mAuthRetry &&
             (aStatus == GATT_STATUS_INSUFFICIENT_AUTHENTICATION ||
              aStatus == GATT_STATUS_INSUFFICIENT_ENCRYPTION)) {
    client->mReadDescriptorState.mAuthRetry = true;
    
    sBluetoothGattClientInterface->ReadDescriptor(
      aConnId,
      aReadParam.mServiceId,
      aReadParam.mCharId,
      aReadParam.mDescriptorId,
      GATT_AUTH_REQ_MITM,
      new ReadDescriptorValueResultHandler(client));
  } else {
    client->mReadDescriptorState.Reset();
    
    DispatchReplyError(runnable,
                       NS_LITERAL_STRING("ReadDescriptorValue failed"));
  }
}

void
BluetoothGattManager::WriteDescriptorNotification(
  int aConnId, BluetoothGattStatus aStatus,
  const BluetoothGattWriteParam& aWriteParam)
{
  MOZ_ASSERT(NS_IsMainThread());

  size_t index = sClients->IndexOf(aConnId, 0 , ConnIdComparator());
  NS_ENSURE_TRUE_VOID(index != sClients->NoIndex);

  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  MOZ_ASSERT(client->mWriteDescriptorState.mRunnable);
  nsRefPtr<BluetoothReplyRunnable> runnable =
    client->mWriteDescriptorState.mRunnable;

  if (aStatus == GATT_STATUS_SUCCESS) {
    client->mWriteDescriptorState.Reset();
    
    DispatchReplySuccess(runnable);
  } else if (!client->mWriteDescriptorState.mAuthRetry &&
             (aStatus == GATT_STATUS_INSUFFICIENT_AUTHENTICATION ||
              aStatus == GATT_STATUS_INSUFFICIENT_ENCRYPTION)) {
    client->mWriteDescriptorState.mAuthRetry = true;
    
    sBluetoothGattClientInterface->WriteDescriptor(
      aConnId,
      aWriteParam.mServiceId,
      aWriteParam.mCharId,
      aWriteParam.mDescriptorId,
      GATT_WRITE_TYPE_NORMAL,
      GATT_AUTH_REQ_MITM,
      client->mWriteDescriptorState.mWriteValue,
      new WriteDescriptorValueResultHandler(client));
  } else {
    client->mWriteDescriptorState.Reset();
    
    DispatchReplyError(runnable,
                       NS_LITERAL_STRING("WriteDescriptorValue failed"));
  }
}

void
BluetoothGattManager::ExecuteWriteNotification(int aConnId,
                                               BluetoothGattStatus aStatus)
{ }

void
BluetoothGattManager::ReadRemoteRssiNotification(int aClientIf,
                                                 const nsAString& aBdAddr,
                                                 int aRssi,
                                                 BluetoothGattStatus aStatus)
{
  BT_API2_LOGR();
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  size_t index = sClients->IndexOf(aClientIf, 0 ,
                                   ClientIfComparator());
  NS_ENSURE_TRUE_VOID(index != sClients->NoIndex);
  nsRefPtr<BluetoothGattClient> client = sClients->ElementAt(index);

  if (aStatus != GATT_STATUS_SUCCESS) { 
    BT_API2_LOGR("ReadRemoteRssi failed, clientIf = %d, bdAddr = %s, " \
                 "rssi = %d, status = %d", aClientIf,
                 NS_ConvertUTF16toUTF8(aBdAddr).get(), aRssi, (int)aStatus);

    
    if (client->mReadRemoteRssiRunnable) {
      DispatchReplyError(client->mReadRemoteRssiRunnable,
                         NS_LITERAL_STRING("ReadRemoteRssi failed"));
      client->mReadRemoteRssiRunnable = nullptr;
    }

    return;
  }

  
  if (client->mReadRemoteRssiRunnable) {
    DispatchReplySuccess(client->mReadRemoteRssiRunnable,
                         BluetoothValue(static_cast<uint32_t>(aRssi)));
    client->mReadRemoteRssiRunnable = nullptr;
  }
}

void
BluetoothGattManager::ListenNotification(BluetoothGattStatus aStatus,
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

void
BluetoothGattManager::ProceedDiscoverProcess(
  BluetoothGattClient* aClient,
  const BluetoothGattServiceId& aServiceId)
{
  













  if (!aClient->mCharacteristics.IsEmpty()) {
    sBluetoothGattClientInterface->GetDescriptor(
      aClient->mConnId,
      aServiceId,
      aClient->mCharacteristics[0].mId,
      true, 
      BluetoothGattId(),
      new DiscoverResultHandler(aClient));
    aClient->mCharacteristics.RemoveElementAt(0);
  } else if (!aClient->mServices.IsEmpty()) {
    sBluetoothGattClientInterface->GetIncludedService(
      aClient->mConnId,
      aClient->mServices[0],
      true, 
      BluetoothGattServiceId(),
      new DiscoverResultHandler(aClient));
    aClient->mServices.RemoveElementAt(0);
  } else {
    aClient->NotifyDiscoverCompleted(true);
  }
}

NS_IMPL_ISUPPORTS(BluetoothGattManager, nsIObserver)
