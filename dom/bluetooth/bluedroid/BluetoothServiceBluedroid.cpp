

















#include "BluetoothServiceBluedroid.h"

#include "BluetoothA2dpManager.h"
#ifdef MOZ_B2G_BT_API_V2
#include "BluetoothGattManager.h"
#else

#endif
#include "BluetoothHfpManager.h"
#ifdef MOZ_B2G_BT_API_V2
#include "BluetoothHidManager.h"
#else

#endif
#include "BluetoothOppManager.h"
#include "BluetoothProfileController.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothUtils.h"
#include "BluetoothUuid.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/ipc/SocketBase.h"
#include "mozilla/StaticMutex.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#ifdef MOZ_B2G_BT_API_V2
#include "nsDataHashtable.h"
#endif

#ifdef MOZ_B2G_BT_API_V2

#define ENSURE_BLUETOOTH_IS_READY(runnable, result)                    \
  do {                                                                 \
    if (!sBtInterface || !IsEnabled()) {                               \
      DispatchReplyError(runnable,                                     \
        NS_LITERAL_STRING("Bluetooth is not ready"));                  \
      return result;                                                   \
    }                                                                  \
  } while(0)

#define ENSURE_BLUETOOTH_IS_READY_VOID(runnable)                       \
  do {                                                                 \
    if (!sBtInterface || !IsEnabled()) {                               \
      DispatchReplyError(runnable,                                     \
        NS_LITERAL_STRING("Bluetooth is not ready"));                  \
      return;                                                          \
    }                                                                  \
  } while(0)

#define ENSURE_GATT_MGR_IS_READY_VOID(gatt, runnable)                  \
  do {                                                                 \
    if (!gatt) {                                                       \
      DispatchReplyError(runnable,                                     \
        NS_LITERAL_STRING("GattManager is not ready"));                \
      return;                                                          \
    }                                                                  \
  } while(0)

using namespace mozilla;
using namespace mozilla::ipc;
USING_BLUETOOTH_NAMESPACE

static nsString sAdapterBdAddress;
static nsString sAdapterBdName;
static bool sAdapterDiscoverable(false);
static bool sAdapterDiscovering(false);
static bool sAdapterEnabled(false);

static InfallibleTArray<nsString> sAdapterBondedAddressArray;





static nsDataHashtable<nsStringHashKey, nsString> sPairingNameTable;

static BluetoothInterface* sBtInterface;
static nsTArray<nsRefPtr<BluetoothProfileController> > sControllerArray;
static InfallibleTArray<BluetoothNamedValue> sRemoteDevicesPack;
static nsTArray<int> sRequestedDeviceCountArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sChangeAdapterStateRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sChangeDiscoveryRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sSetPropertyRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sGetDeviceRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sFetchUuidsRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sBondingRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sUnbondingRunnableArray;
static bool sIsRestart(false);
static bool sIsFirstTimeToggleOffBt(false);




ControlPlayStatus
BluetoothServiceBluedroid::PlayStatusStringToControlPlayStatus(
  const nsAString& aPlayStatus)
{
  ControlPlayStatus playStatus = ControlPlayStatus::PLAYSTATUS_UNKNOWN;
  if (aPlayStatus.EqualsLiteral("STOPPED")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_STOPPED;
  } else if (aPlayStatus.EqualsLiteral("PLAYING")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_PLAYING;
  } else if (aPlayStatus.EqualsLiteral("PAUSED")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_PAUSED;
  } else if (aPlayStatus.EqualsLiteral("FWD_SEEK")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_FWD_SEEK;
  } else if (aPlayStatus.EqualsLiteral("REV_SEEK")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_REV_SEEK;
  } else if (aPlayStatus.EqualsLiteral("ERROR")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_ERROR;
  }

  return playStatus;
}




bool
BluetoothServiceBluedroid::EnsureBluetoothHalLoad()
{
  sBtInterface = BluetoothInterface::GetInstance();
  NS_ENSURE_TRUE(sBtInterface, false);

  return true;
}

class BluetoothServiceBluedroid::EnableResultHandler final
  : public BluetoothResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());

    BT_LOGR("BluetoothInterface::Enable failed: %d", aStatus);

    BluetoothService::AcknowledgeToggleBt(false);
  }
};





class BluetoothServiceBluedroid::ProfileInitResultHandler final
  : public BluetoothProfileResultHandler
{
public:
  ProfileInitResultHandler(unsigned char aNumProfiles)
    : mNumProfiles(aNumProfiles)
  {
    MOZ_ASSERT(mNumProfiles);
  }

  void Init() override
  {
    if (!(--mNumProfiles)) {
      Proceed();
    }
  }

  void OnError(nsresult aResult) override
  {
    if (!(--mNumProfiles)) {
      Proceed();
    }
  }

private:
  void Proceed() const
  {
    sBtInterface->Enable(new EnableResultHandler());
  }

  unsigned char mNumProfiles;
};

class BluetoothServiceBluedroid::InitResultHandler final
  : public BluetoothResultHandler
{
public:
  void Init() override
  {
    static void (* const sInitManager[])(BluetoothProfileResultHandler*) = {
      BluetoothHfpManager::InitHfpInterface,
      BluetoothA2dpManager::InitA2dpInterface,
      BluetoothGattManager::InitGattInterface
    };

    MOZ_ASSERT(NS_IsMainThread());

    
    
    
    nsRefPtr<ProfileInitResultHandler> res =
      new ProfileInitResultHandler(MOZ_ARRAY_LENGTH(sInitManager));

    for (size_t i = 0; i < MOZ_ARRAY_LENGTH(sInitManager); ++i) {
      sInitManager[i](res);
    }
  }

  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());

    BT_LOGR("BluetoothInterface::Init failed: %d", aStatus);

    sBtInterface = nullptr;

    BluetoothService::AcknowledgeToggleBt(false);
  }
};

nsresult
BluetoothServiceBluedroid::StartGonkBluetooth()
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE(sBtInterface, NS_ERROR_FAILURE);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE(bs, NS_ERROR_FAILURE);

  if (bs->IsEnabled()) {
    
    BluetoothService::AcknowledgeToggleBt(true);
    return NS_OK;
  }

  sBtInterface->Init(reinterpret_cast<BluetoothServiceBluedroid*>(bs),
                     new InitResultHandler());

  return NS_OK;
}

class BluetoothServiceBluedroid::DisableResultHandler final
  : public BluetoothResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());

    BT_LOGR("BluetoothInterface::Disable failed: %d", aStatus);

    BluetoothService::AcknowledgeToggleBt(true);
  }
};

nsresult
BluetoothServiceBluedroid::StopGonkBluetooth()
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE(sBtInterface, NS_ERROR_FAILURE);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE(bs, NS_ERROR_FAILURE);

  if (!bs->IsEnabled()) {
    
    BluetoothService::AcknowledgeToggleBt(false);
    return NS_OK;
  }

  sBtInterface->Disable(new DisableResultHandler());

  return NS_OK;
}




BluetoothServiceBluedroid::BluetoothServiceBluedroid()
{
  if (!EnsureBluetoothHalLoad()) {
    BT_LOGR("Error! Failed to load bluedroid library.");
    return;
  }
}

BluetoothServiceBluedroid::~BluetoothServiceBluedroid()
{
}

nsresult
BluetoothServiceBluedroid::StartInternal(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if(aRunnable) {
    sChangeAdapterStateRunnableArray.AppendElement(aRunnable);
  }

  nsresult ret = StartGonkBluetooth();
  if (NS_FAILED(ret)) {
    BluetoothService::AcknowledgeToggleBt(false);

    
    if(aRunnable) {
      DispatchReplyError(aRunnable, NS_LITERAL_STRING("StartBluetoothError"));
      sChangeAdapterStateRunnableArray.RemoveElement(aRunnable);
    }

    BT_LOGR("Error");
  }

  return ret;
}

nsresult
BluetoothServiceBluedroid::StopInternal(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothProfileManagerBase* profile;
  profile = BluetoothHfpManager::Get();
  NS_ENSURE_TRUE(profile, NS_ERROR_FAILURE);
  if (profile->IsConnected()) {
    profile->Disconnect(nullptr);
  } else {
    profile->Reset();
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
  } else {
    profile->Reset();
  }

  profile = BluetoothHidManager::Get();
  NS_ENSURE_TRUE(profile, NS_ERROR_FAILURE);
  if (profile->IsConnected()) {
    profile->Disconnect(nullptr);
  } else {
    profile->Reset();
  }

  
  if(aRunnable) {
    sChangeAdapterStateRunnableArray.AppendElement(aRunnable);
  }

  nsresult ret = StopGonkBluetooth();
  if (NS_FAILED(ret)) {
    BluetoothService::AcknowledgeToggleBt(true);

    
    if(aRunnable) {
      DispatchReplyError(aRunnable, NS_LITERAL_STRING("StopBluetoothError"));
      sChangeAdapterStateRunnableArray.RemoveElement(aRunnable);
    }

    BT_LOGR("Error");
  }

  return ret;
}





void
BluetoothServiceBluedroid::ConnectGattClientInternal(
  const nsAString& aAppUuid, const nsAString& aDeviceAddress,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  BluetoothGattManager* gatt = BluetoothGattManager::Get();
  ENSURE_GATT_MGR_IS_READY_VOID(gatt, aRunnable);

  gatt->Connect(aAppUuid, aDeviceAddress, aRunnable);
}

void
BluetoothServiceBluedroid::DisconnectGattClientInternal(
  const nsAString& aAppUuid, const nsAString& aDeviceAddress,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  BluetoothGattManager* gatt = BluetoothGattManager::Get();
  ENSURE_GATT_MGR_IS_READY_VOID(gatt, aRunnable);

  gatt->Disconnect(aAppUuid, aDeviceAddress, aRunnable);
}

void
BluetoothServiceBluedroid::DiscoverGattServicesInternal(
  const nsAString& aAppUuid, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  BluetoothGattManager* gatt = BluetoothGattManager::Get();
  ENSURE_GATT_MGR_IS_READY_VOID(gatt, aRunnable);

  gatt->Discover(aAppUuid, aRunnable);
}

void
BluetoothServiceBluedroid::GattClientStartNotificationsInternal(
  const nsAString& aAppUuid, const BluetoothGattServiceId& aServId,
  const BluetoothGattId& aCharId, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  BluetoothGattManager* gatt = BluetoothGattManager::Get();
  ENSURE_GATT_MGR_IS_READY_VOID(gatt, aRunnable);

  gatt->RegisterNotifications(aAppUuid, aServId, aCharId, aRunnable);
}

void
BluetoothServiceBluedroid::GattClientStopNotificationsInternal(
  const nsAString& aAppUuid, const BluetoothGattServiceId& aServId,
  const BluetoothGattId& aCharId, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  BluetoothGattManager* gatt = BluetoothGattManager::Get();
  ENSURE_GATT_MGR_IS_READY_VOID(gatt, aRunnable);

  gatt->DeregisterNotifications(aAppUuid, aServId, aCharId, aRunnable);
}

void
BluetoothServiceBluedroid::UnregisterGattClientInternal(
  int aClientIf, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  BluetoothGattManager* gatt = BluetoothGattManager::Get();
  ENSURE_GATT_MGR_IS_READY_VOID(gatt, aRunnable);

  gatt->UnregisterClient(aClientIf, aRunnable);
}

void
BluetoothServiceBluedroid::GattClientReadRemoteRssiInternal(
  int aClientIf, const nsAString& aDeviceAddress,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  BluetoothGattManager* gatt = BluetoothGattManager::Get();
  ENSURE_GATT_MGR_IS_READY_VOID(gatt, aRunnable);

  gatt->ReadRemoteRssi(aClientIf, aDeviceAddress, aRunnable);
}

void
BluetoothServiceBluedroid::GattClientReadCharacteristicValueInternal(
  const nsAString& aAppUuid,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharacteristicId,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  BluetoothGattManager* gatt = BluetoothGattManager::Get();
  ENSURE_GATT_MGR_IS_READY_VOID(gatt, aRunnable);

  gatt->ReadCharacteristicValue(aAppUuid, aServiceId, aCharacteristicId,
                                aRunnable);
}

void
BluetoothServiceBluedroid::GattClientWriteCharacteristicValueInternal(
  const nsAString& aAppUuid,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharacteristicId,
  const BluetoothGattWriteType& aWriteType,
  const nsTArray<uint8_t>& aValue,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  BluetoothGattManager* gatt = BluetoothGattManager::Get();
  ENSURE_GATT_MGR_IS_READY_VOID(gatt, aRunnable);

  gatt->WriteCharacteristicValue(aAppUuid, aServiceId, aCharacteristicId,
                                 aWriteType, aValue, aRunnable);
}

void
BluetoothServiceBluedroid::GattClientReadDescriptorValueInternal(
  const nsAString& aAppUuid,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharacteristicId,
  const BluetoothGattId& aDescriptorId,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  BluetoothGattManager* gatt = BluetoothGattManager::Get();
  ENSURE_GATT_MGR_IS_READY_VOID(gatt, aRunnable);

  gatt->ReadDescriptorValue(aAppUuid, aServiceId, aCharacteristicId,
                            aDescriptorId, aRunnable);
}

void
BluetoothServiceBluedroid::GattClientWriteDescriptorValueInternal(
  const nsAString& aAppUuid,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharacteristicId,
  const BluetoothGattId& aDescriptorId,
  const nsTArray<uint8_t>& aValue,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  BluetoothGattManager* gatt = BluetoothGattManager::Get();
  ENSURE_GATT_MGR_IS_READY_VOID(gatt, aRunnable);

  gatt->WriteDescriptorValue(aAppUuid, aServiceId, aCharacteristicId,
                             aDescriptorId, aValue, aRunnable);
}
#else

#define ENSURE_BLUETOOTH_IS_READY(runnable, result)                    \
  do {                                                                 \
    if (!sBtInterface || !IsEnabled()) {                               \
      NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth is not ready");     \
      DispatchBluetoothReply(runnable, BluetoothValue(), errorStr);    \
      return result;                                                   \
    }                                                                  \
  } while(0)


#define SET_AUDIO_BIT(cod)               (cod |= 0x200000)

#define SET_RENDERING_BIT(cod)           (cod |= 0x40000)

using namespace mozilla;
using namespace mozilla::ipc;
USING_BLUETOOTH_NAMESPACE

static nsString sAdapterBdAddress;
static nsString sAdapterBdName;
static InfallibleTArray<nsString> sAdapterBondedAddressArray;
static BluetoothInterface* sBtInterface;
static nsTArray<nsRefPtr<BluetoothProfileController> > sControllerArray;
static InfallibleTArray<BluetoothNamedValue> sRemoteDevicesPack;
static nsTArray<int> sRequestedDeviceCountArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sSetPropertyRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sGetDeviceRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sBondingRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sUnbondingRunnableArray;
static bool sAdapterDiscoverable(false);
static bool sIsRestart(false);
static bool sIsFirstTimeToggleOffBt(false);
static uint32_t sAdapterDiscoverableTimeout(0);




void
BluetoothServiceBluedroid::ClassToIcon(uint32_t aClass, nsAString& aRetIcon)
{
  switch ((aClass & 0x1f00) >> 8) {
    case 0x01:
      aRetIcon.AssignLiteral("computer");
      break;
    case 0x02:
      switch ((aClass & 0xfc) >> 2) {
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x05:
          aRetIcon.AssignLiteral("phone");
          break;
        case 0x04:
          aRetIcon.AssignLiteral("modem");
          break;
      }
      break;
    case 0x03:
      aRetIcon.AssignLiteral("network-wireless");
      break;
    case 0x04:
      switch ((aClass & 0xfc) >> 2) {
        case 0x01:
        case 0x02:
        case 0x06:
          aRetIcon.AssignLiteral("audio-card");
          break;
        case 0x0b:
        case 0x0c:
        case 0x0d:
          aRetIcon.AssignLiteral("camera-video");
          break;
        default:
          aRetIcon.AssignLiteral("audio-card");
          break;
      }
      break;
    case 0x05:
      switch ((aClass & 0xc0) >> 6) {
        case 0x00:
          switch ((aClass && 0x1e) >> 2) {
            case 0x01:
            case 0x02:
              aRetIcon.AssignLiteral("input-gaming");
              break;
          }
          break;
        case 0x01:
          aRetIcon.AssignLiteral("input-keyboard");
          break;
        case 0x02:
          switch ((aClass && 0x1e) >> 2) {
            case 0x05:
              aRetIcon.AssignLiteral("input-tablet");
              break;
            default:
              aRetIcon.AssignLiteral("input-mouse");
              break;
          }
      }
      break;
    case 0x06:
      if (aClass & 0x80) {
        aRetIcon.AssignLiteral("printer");
        break;
      }
      if (aClass & 0x20) {
        aRetIcon.AssignLiteral("camera-photo");
        break;
      }
      break;
  }

  if (aRetIcon.IsEmpty()) {
    if (HAS_AUDIO(aClass)) {
      





      aRetIcon.AssignLiteral("audio-card");
    } else {
      BT_LOGR("No icon to match class: %x", aClass);
    }
  }
}

ControlPlayStatus
BluetoothServiceBluedroid::PlayStatusStringToControlPlayStatus(
  const nsAString& aPlayStatus)
{
  ControlPlayStatus playStatus = ControlPlayStatus::PLAYSTATUS_UNKNOWN;
  if (aPlayStatus.EqualsLiteral("STOPPED")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_STOPPED;
  } else if (aPlayStatus.EqualsLiteral("PLAYING")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_PLAYING;
  } else if (aPlayStatus.EqualsLiteral("PAUSED")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_PAUSED;
  } else if (aPlayStatus.EqualsLiteral("FWD_SEEK")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_FWD_SEEK;
  } else if (aPlayStatus.EqualsLiteral("REV_SEEK")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_REV_SEEK;
  } else if (aPlayStatus.EqualsLiteral("ERROR")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_ERROR;
  }

  return playStatus;
}




bool
BluetoothServiceBluedroid::EnsureBluetoothHalLoad()
{
  sBtInterface = BluetoothInterface::GetInstance();
  NS_ENSURE_TRUE(sBtInterface, false);

  return true;
}

class BluetoothServiceBluedroid::EnableResultHandler final
  : public BluetoothResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());

    BT_LOGR("BluetoothInterface::Enable failed: %d", aStatus);

    BluetoothService::AcknowledgeToggleBt(false);
  }
};





class BluetoothServiceBluedroid::ProfileInitResultHandler final
: public BluetoothProfileResultHandler
{
public:
  ProfileInitResultHandler(unsigned char aNumProfiles)
  : mNumProfiles(aNumProfiles)
  {
    MOZ_ASSERT(mNumProfiles);
  }

  void Init() override
  {
    if (!(--mNumProfiles)) {
      Proceed();
    }
  }

  void OnError(nsresult aResult) override
  {
    if (!(--mNumProfiles)) {
      Proceed();
    }
  }

private:
  void Proceed() const
  {
    sBtInterface->Enable(new EnableResultHandler());
  }

  unsigned char mNumProfiles;
};

class BluetoothServiceBluedroid::InitResultHandler final
  : public BluetoothResultHandler
{
public:
  void Init() override
  {
    static void (* const sInitManager[])(BluetoothProfileResultHandler*) = {
      BluetoothHfpManager::InitHfpInterface,
      BluetoothA2dpManager::InitA2dpInterface
    };

    MOZ_ASSERT(NS_IsMainThread());

    
    
    
    nsRefPtr<ProfileInitResultHandler> res =
      new ProfileInitResultHandler(MOZ_ARRAY_LENGTH(sInitManager));

    for (size_t i = 0; i < MOZ_ARRAY_LENGTH(sInitManager); ++i) {
      sInitManager[i](res);
    }
  }

  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());

    BT_LOGR("BluetoothInterface::Init failed: %d", aStatus);

    sBtInterface = nullptr;

    BluetoothService::AcknowledgeToggleBt(false);
  }
};

nsresult
BluetoothServiceBluedroid::StartGonkBluetooth()
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE(sBtInterface, NS_ERROR_FAILURE);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE(bs, NS_ERROR_FAILURE);

  if (bs->IsEnabled()) {
    
    BluetoothService::AcknowledgeToggleBt(true);
    return NS_OK;
  }

  sBtInterface->Init(reinterpret_cast<BluetoothServiceBluedroid*>(bs),
                     new InitResultHandler());

  return NS_OK;
}

class BluetoothServiceBluedroid::DisableResultHandler final
  : public BluetoothResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());

    BT_LOGR("BluetoothInterface::Disable failed: %d", aStatus);

    
    BluetoothService::AcknowledgeToggleBt(false);
  }
};

nsresult
BluetoothServiceBluedroid::StopGonkBluetooth()
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE(sBtInterface, NS_ERROR_FAILURE);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE(bs, NS_ERROR_FAILURE);

  if (!bs->IsEnabled()) {
    
    BluetoothService::AcknowledgeToggleBt(false);
    return NS_OK;
  }

  sBtInterface->Disable(new DisableResultHandler());

  return NS_OK;
}

static void
ReplyStatusError(BluetoothReplyRunnable* aBluetoothReplyRunnable,
                 BluetoothStatus aStatusCode, const nsAString& aCustomMsg)
{
  MOZ_ASSERT(aBluetoothReplyRunnable, "Reply runnable is nullptr");

  BT_LOGR("error code(%d)", aStatusCode);

  nsAutoString replyError;
  replyError.Assign(aCustomMsg);

  if (aStatusCode == STATUS_BUSY) {
    replyError.AppendLiteral(":BT_STATUS_BUSY");
  } else if (aStatusCode == STATUS_NOT_READY) {
    replyError.AppendLiteral(":BT_STATUS_NOT_READY");
  } else if (aStatusCode == STATUS_DONE) {
    replyError.AppendLiteral(":BT_STATUS_DONE");
  } else if (aStatusCode == STATUS_AUTH_FAILURE) {
    replyError.AppendLiteral(":BT_STATUS_AUTH_FAILURE");
  } else if (aStatusCode == STATUS_RMT_DEV_DOWN) {
    replyError.AppendLiteral(":BT_STATUS_RMT_DEV_DOWN");
  } else if (aStatusCode == STATUS_FAIL) {
    replyError.AppendLiteral(":BT_STATUS_FAIL");
  }

  DispatchBluetoothReply(aBluetoothReplyRunnable, BluetoothValue(true),
                         replyError);
}




BluetoothServiceBluedroid::BluetoothServiceBluedroid()
{
  if (!EnsureBluetoothHalLoad()) {
    BT_LOGR("Error! Failed to load bluedroid library.");
    return;
  }
}

BluetoothServiceBluedroid::~BluetoothServiceBluedroid()
{
}

nsresult
BluetoothServiceBluedroid::StartInternal()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsresult ret = StartGonkBluetooth();
  if (NS_FAILED(ret)) {
    BluetoothService::AcknowledgeToggleBt(false);
    BT_LOGR("Error");
  }

  return ret;
}

nsresult
BluetoothServiceBluedroid::StopInternal()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsresult ret = StopGonkBluetooth();
  if (NS_FAILED(ret)) {
    BluetoothService::AcknowledgeToggleBt(true);
    BT_LOGR("Error");
  }

  return ret;
}
#endif

#ifdef MOZ_B2G_BT_API_V2
nsresult
BluetoothServiceBluedroid::GetAdaptersInternal(
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  










  BluetoothValue adaptersProperties = InfallibleTArray<BluetoothNamedValue>();
  uint32_t numAdapters = 1; 

  for (uint32_t i = 0; i < numAdapters; i++) {
    BluetoothValue properties = InfallibleTArray<BluetoothNamedValue>();

    BT_APPEND_NAMED_VALUE(properties.get_ArrayOfBluetoothNamedValue(),
                          "State", sAdapterEnabled);
    BT_APPEND_NAMED_VALUE(properties.get_ArrayOfBluetoothNamedValue(),
                          "Address", sAdapterBdAddress);
    BT_APPEND_NAMED_VALUE(properties.get_ArrayOfBluetoothNamedValue(),
                          "Name", sAdapterBdName);
    BT_APPEND_NAMED_VALUE(properties.get_ArrayOfBluetoothNamedValue(),
                          "Discoverable", sAdapterDiscoverable);
    BT_APPEND_NAMED_VALUE(properties.get_ArrayOfBluetoothNamedValue(),
                          "Discovering", sAdapterDiscovering);
    BT_APPEND_NAMED_VALUE(properties.get_ArrayOfBluetoothNamedValue(),
                          "PairedDevices", sAdapterBondedAddressArray);

    BT_APPEND_NAMED_VALUE(adaptersProperties.get_ArrayOfBluetoothNamedValue(),
                          "Adapter", properties);
  }

  DispatchReplySuccess(aRunnable, adaptersProperties);
  return NS_OK;
}
#else
nsresult
BluetoothServiceBluedroid::GetDefaultAdapterPathInternal(
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  bool discoverable = sAdapterDiscoverable;
  uint32_t discoverableTimeout = sAdapterDiscoverableTimeout;

  BluetoothValue v = InfallibleTArray<BluetoothNamedValue>();

  BT_APPEND_NAMED_VALUE(v.get_ArrayOfBluetoothNamedValue(),
                        "Address", sAdapterBdAddress);

  BT_APPEND_NAMED_VALUE(v.get_ArrayOfBluetoothNamedValue(),
                        "Name", sAdapterBdName);

  BT_APPEND_NAMED_VALUE(v.get_ArrayOfBluetoothNamedValue(),
                        "Discoverable", discoverable);

  BT_APPEND_NAMED_VALUE(v.get_ArrayOfBluetoothNamedValue(),
                        "DiscoverableTimeout", discoverableTimeout);

  BT_APPEND_NAMED_VALUE(v.get_ArrayOfBluetoothNamedValue(),
                        "Devices", sAdapterBondedAddressArray);

  DispatchBluetoothReply(aRunnable, v, EmptyString());

  return NS_OK;
}
#endif

class BluetoothServiceBluedroid::GetRemoteDevicePropertiesResultHandler
  final
  : public BluetoothResultHandler
{
public:
  GetRemoteDevicePropertiesResultHandler(const nsAString& aDeviceAddress)
  : mDeviceAddress(aDeviceAddress)
  { }

  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());

#ifdef MOZ_B2G_BT_API_V2
    BT_WARNING("GetRemoteDeviceProperties(%s) failed: %d",
               NS_ConvertUTF16toUTF8(mDeviceAddress).get(), aStatus);

    
    if (--sRequestedDeviceCountArray[0] == 0) {
      if (!sGetDeviceRunnableArray.IsEmpty()) {
        DispatchReplyError(sGetDeviceRunnableArray[0],
          NS_LITERAL_STRING("GetRemoteDeviceProperties failed"));
        sGetDeviceRunnableArray.RemoveElementAt(0);
      }

      sRequestedDeviceCountArray.RemoveElementAt(0);
      sRemoteDevicesPack.Clear();
    }
#else
    BT_WARNING("GetRemoteDeviceProperties(%s) failed: %d",
               mDeviceAddress.get(), aStatus);

    
    if (--sRequestedDeviceCountArray[0] == 0) {
      if (!sGetDeviceRunnableArray.IsEmpty()) {
        DispatchBluetoothReply(sGetDeviceRunnableArray[0],
                               sRemoteDevicesPack, EmptyString());
        sGetDeviceRunnableArray.RemoveElementAt(0);
      }

      sRequestedDeviceCountArray.RemoveElementAt(0);
      sRemoteDevicesPack.Clear();
    }
#endif
  }

private:
  nsString mDeviceAddress;
};

nsresult
BluetoothServiceBluedroid::GetConnectedDevicePropertiesInternal(
  uint16_t aServiceUuid, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY(aRunnable, NS_OK);

#ifdef MOZ_B2G_BT_API_V2
  BluetoothProfileManagerBase* profile =
    BluetoothUuidHelper::GetBluetoothProfileManager(aServiceUuid);
  if (!profile) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING(ERR_UNKNOWN_PROFILE));
    return NS_OK;
  }

  nsTArray<nsString> deviceAddresses;
  if (profile->IsConnected()) {
    nsString address;
    profile->GetAddress(address);
    deviceAddresses.AppendElement(address);
  }

  int requestedDeviceCount = deviceAddresses.Length();
  if (requestedDeviceCount == 0) {
    InfallibleTArray<BluetoothNamedValue> emptyArr;
    DispatchReplySuccess(aRunnable, emptyArr);
    return NS_OK;
  }
#else
  BluetoothProfileManagerBase* profile =
    BluetoothUuidHelper::GetBluetoothProfileManager(aServiceUuid);
  if (!profile) {
    InfallibleTArray<BluetoothNamedValue> emptyArr;
    DispatchBluetoothReply(aRunnable, emptyArr,
                           NS_LITERAL_STRING(ERR_UNKNOWN_PROFILE));
    return NS_OK;
  }

  nsTArray<nsString> deviceAddresses;
  if (profile->IsConnected()) {
    nsString address;
    profile->GetAddress(address);
    deviceAddresses.AppendElement(address);
  }

  int requestedDeviceCount = deviceAddresses.Length();
  if (requestedDeviceCount == 0) {
    InfallibleTArray<BluetoothNamedValue> emptyArr;
    DispatchBluetoothReply(aRunnable, emptyArr, EmptyString());
    return NS_OK;
  }
#endif

  sRequestedDeviceCountArray.AppendElement(requestedDeviceCount);
  sGetDeviceRunnableArray.AppendElement(aRunnable);

  for (int i = 0; i < requestedDeviceCount; i++) {
    
    sBtInterface->GetRemoteDeviceProperties(deviceAddresses[i],
      new GetRemoteDevicePropertiesResultHandler(deviceAddresses[i]));
  }

  return NS_OK;
}

nsresult
BluetoothServiceBluedroid::GetPairedDevicePropertiesInternal(
  const nsTArray<nsString>& aDeviceAddress, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY(aRunnable, NS_OK);

#ifdef MOZ_B2G_BT_API_V2
  int requestedDeviceCount = aDeviceAddress.Length();
  if (requestedDeviceCount == 0) {
    DispatchReplySuccess(aRunnable);
    return NS_OK;
  }
#else
  int requestedDeviceCount = aDeviceAddress.Length();
  if (requestedDeviceCount == 0) {
    InfallibleTArray<BluetoothNamedValue> emptyArr;
    DispatchBluetoothReply(aRunnable, BluetoothValue(emptyArr), EmptyString());
    return NS_OK;
  }

  sRequestedDeviceCountArray.AppendElement(requestedDeviceCount);
  sGetDeviceRunnableArray.AppendElement(aRunnable);
#endif

  for (int i = 0; i < requestedDeviceCount; i++) {
    
    sBtInterface->GetRemoteDeviceProperties(aDeviceAddress[i],
      new GetRemoteDevicePropertiesResultHandler(aDeviceAddress[i]));
  }

  return NS_OK;
}

class BluetoothServiceBluedroid::StartDiscoveryResultHandler final
  : public BluetoothResultHandler
{
public:
  StartDiscoveryResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

#ifdef MOZ_B2G_BT_API_V2
  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());
    sChangeDiscoveryRunnableArray.RemoveElement(mRunnable);
    DispatchReplyError(mRunnable, aStatus);
  }
#else
  void StartDiscovery() override
  {
    MOZ_ASSERT(NS_IsMainThread());
    DispatchBluetoothReply(mRunnable, true, EmptyString());
  }

  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());
    ReplyStatusError(mRunnable, aStatus, NS_LITERAL_STRING("StartDiscovery"));
  }
#endif

private:
  BluetoothReplyRunnable* mRunnable;
};

nsresult
BluetoothServiceBluedroid::StartDiscoveryInternal(
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY(aRunnable, NS_OK);

#ifdef MOZ_B2G_BT_API_V2
  sChangeDiscoveryRunnableArray.AppendElement(aRunnable);
#else
  
#endif

  sBtInterface->StartDiscovery(new StartDiscoveryResultHandler(aRunnable));

  return NS_OK;
}

class BluetoothServiceBluedroid::CancelDiscoveryResultHandler final
  : public BluetoothResultHandler
{
public:
  CancelDiscoveryResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

#ifdef MOZ_B2G_BT_API_V2
  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());
    sChangeDiscoveryRunnableArray.RemoveElement(mRunnable);
    DispatchReplyError(mRunnable, aStatus);
  }
#else
  void CancelDiscovery() override
  {
    MOZ_ASSERT(NS_IsMainThread());
    DispatchBluetoothReply(mRunnable, true, EmptyString());
  }

  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());
    ReplyStatusError(mRunnable, aStatus, NS_LITERAL_STRING("StopDiscovery"));
  }
#endif

private:
  BluetoothReplyRunnable* mRunnable;
};

#ifdef MOZ_B2G_BT_API_V2
class BluetoothServiceBluedroid::GetRemoteServicesResultHandler final
  : public BluetoothResultHandler
{
public:
  GetRemoteServicesResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());
    sFetchUuidsRunnableArray.RemoveElement(mRunnable);
    DispatchReplyError(mRunnable, aStatus);
  }

private:
  BluetoothReplyRunnable* mRunnable;
};

nsresult
BluetoothServiceBluedroid::FetchUuidsInternal(
  const nsAString& aDeviceAddress, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY(aRunnable, NS_OK);

  



  if (sAdapterDiscovering) {
    sBtInterface->CancelDiscovery(new CancelDiscoveryResultHandler(aRunnable));
  }

  sFetchUuidsRunnableArray.AppendElement(aRunnable);

  sBtInterface->GetRemoteServices(aDeviceAddress,
    new GetRemoteServicesResultHandler(aRunnable));

  return NS_OK;
}
#else

#endif

nsresult
BluetoothServiceBluedroid::StopDiscoveryInternal(
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY(aRunnable, NS_OK);

#ifdef MOZ_B2G_BT_API_V2
  sChangeDiscoveryRunnableArray.AppendElement(aRunnable);
#else
  
#endif

  sBtInterface->CancelDiscovery(new CancelDiscoveryResultHandler(aRunnable));

  return NS_OK;
}

class BluetoothServiceBluedroid::SetAdapterPropertyResultHandler final
  : public BluetoothResultHandler
{
public:
  SetAdapterPropertyResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(NS_IsMainThread());

#ifdef MOZ_B2G_BT_API_V2
    sSetPropertyRunnableArray.RemoveElement(mRunnable);
    DispatchReplyError(mRunnable, aStatus);
#else
    ReplyStatusError(mRunnable, aStatus, NS_LITERAL_STRING("SetProperty"));
#endif
  }
private:
  BluetoothReplyRunnable* mRunnable;
};

nsresult
BluetoothServiceBluedroid::SetProperty(BluetoothObjectType aType,
                                       const BluetoothNamedValue& aValue,
                                       BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY(aRunnable, NS_OK);

  sSetPropertyRunnableArray.AppendElement(aRunnable);

  sBtInterface->SetAdapterProperty(aValue,
    new SetAdapterPropertyResultHandler(aRunnable));

  return NS_OK;
}

nsresult
BluetoothServiceBluedroid::GetServiceChannel(
  const nsAString& aDeviceAddress,
  const nsAString& aServiceUuid,
  BluetoothProfileManagerBase* aManager)
{
  return NS_OK;
}

bool
BluetoothServiceBluedroid::UpdateSdpRecords(
  const nsAString& aDeviceAddress,
  BluetoothProfileManagerBase* aManager)
{
  return true;
}

class BluetoothServiceBluedroid::CreateBondResultHandler final
  : public BluetoothResultHandler
{
public:
  CreateBondResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  {
    MOZ_ASSERT(mRunnable);
  }

  void OnError(BluetoothStatus aStatus) override
  {
#ifdef MOZ_B2G_BT_API_V2
    sBondingRunnableArray.RemoveElement(mRunnable);
    DispatchReplyError(mRunnable, aStatus);
#else
    sBondingRunnableArray.RemoveElement(mRunnable);
    ReplyStatusError(mRunnable, aStatus, NS_LITERAL_STRING("CreatedPairedDevice"));
#endif
  }

private:
  nsRefPtr<BluetoothReplyRunnable> mRunnable;
};

nsresult
BluetoothServiceBluedroid::CreatePairedDeviceInternal(
  const nsAString& aDeviceAddress, int aTimeout,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY(aRunnable, NS_OK);

  sBondingRunnableArray.AppendElement(aRunnable);

  sBtInterface->CreateBond(aDeviceAddress, TRANSPORT_AUTO,
                           new CreateBondResultHandler(aRunnable));
  return NS_OK;
}

class BluetoothServiceBluedroid::RemoveBondResultHandler final
  : public BluetoothResultHandler
{
public:
  RemoveBondResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  {
    MOZ_ASSERT(mRunnable);
  }

  void OnError(BluetoothStatus aStatus) override
  {
#ifdef MOZ_B2G_BT_API_V2
    sUnbondingRunnableArray.RemoveElement(mRunnable);
    DispatchReplyError(mRunnable, aStatus);
#else
    sUnbondingRunnableArray.RemoveElement(mRunnable);
    ReplyStatusError(mRunnable, aStatus, NS_LITERAL_STRING("RemoveDevice"));
#endif
  }

private:
  nsRefPtr<BluetoothReplyRunnable> mRunnable;
};

nsresult
BluetoothServiceBluedroid::RemoveDeviceInternal(
  const nsAString& aDeviceAddress, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY(aRunnable, NS_OK);

  sUnbondingRunnableArray.AppendElement(aRunnable);

  sBtInterface->RemoveBond(aDeviceAddress,
                           new RemoveBondResultHandler(aRunnable));

  return NS_OK;
}

#ifdef MOZ_B2G_BT_API_V2
class BluetoothServiceBluedroid::PinReplyResultHandler final
  : public BluetoothResultHandler
{
public:
  PinReplyResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void PinReply() override
  {
    DispatchReplySuccess(mRunnable);
  }

  void OnError(BluetoothStatus aStatus) override
  {
    DispatchReplyError(mRunnable, aStatus);
  }

private:
  BluetoothReplyRunnable* mRunnable;
};

void
BluetoothServiceBluedroid::PinReplyInternal(
  const nsAString& aDeviceAddress, bool aAccept,
  const nsAString& aPinCode, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  sBtInterface->PinReply(aDeviceAddress, aAccept, aPinCode,
                         new PinReplyResultHandler(aRunnable));
}

void
BluetoothServiceBluedroid::SetPinCodeInternal(
  const nsAString& aDeviceAddress, const nsAString& aPinCode,
  BluetoothReplyRunnable* aRunnable)
{
  
}
#else
class BluetoothServiceBluedroid::PinReplyResultHandler final
  : public BluetoothResultHandler
{
public:
  PinReplyResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void PinReply() override
  {
    DispatchBluetoothReply(mRunnable, BluetoothValue(true), EmptyString());
  }

  void OnError(BluetoothStatus aStatus) override
  {
    ReplyStatusError(mRunnable, aStatus, NS_LITERAL_STRING("SetPinCode"));
  }

private:
  BluetoothReplyRunnable* mRunnable;
};

bool
BluetoothServiceBluedroid::SetPinCodeInternal(
  const nsAString& aDeviceAddress, const nsAString& aPinCode,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY(aRunnable, false);

  sBtInterface->PinReply(aDeviceAddress, true, aPinCode,
    new PinReplyResultHandler(aRunnable));

  return true;
}
#endif

#ifdef MOZ_B2G_BT_API_V2
void
BluetoothServiceBluedroid::SetPasskeyInternal(
  const nsAString& aDeviceAddress, uint32_t aPasskey,
  BluetoothReplyRunnable* aRunnable)
{
  
}
#else
bool
BluetoothServiceBluedroid::SetPasskeyInternal(
  const nsAString& aDeviceAddress, uint32_t aPasskey,
  BluetoothReplyRunnable* aRunnable)
{
  return true;
}
#endif

#ifdef MOZ_B2G_BT_API_V2
class BluetoothServiceBluedroid::SspReplyResultHandler final
  : public BluetoothResultHandler
{
public:
  SspReplyResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void SspReply() override
  {
    DispatchReplySuccess(mRunnable);
  }

  void OnError(BluetoothStatus aStatus) override
  {
    DispatchReplyError(mRunnable, aStatus);
  }

private:
  BluetoothReplyRunnable* mRunnable;
};

void
BluetoothServiceBluedroid::SspReplyInternal(
  const nsAString& aDeviceAddress, BluetoothSspVariant aVariant,
  bool aAccept, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  sBtInterface->SspReply(aDeviceAddress, aVariant, aAccept, 0 ,
                         new SspReplyResultHandler(aRunnable));
}

void
BluetoothServiceBluedroid::SetPairingConfirmationInternal(
  const nsAString& aDeviceAddress, bool aConfirm,
  BluetoothReplyRunnable* aRunnable)
{
  
}
#else
class BluetoothServiceBluedroid::SspReplyResultHandler final
  : public BluetoothResultHandler
{
public:
  SspReplyResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void SspReply() override
  {
    DispatchBluetoothReply(mRunnable, BluetoothValue(true), EmptyString());
  }

  void OnError(BluetoothStatus aStatus) override
  {
    ReplyStatusError(mRunnable, aStatus,
                     NS_LITERAL_STRING("SetPairingConfirmation"));
  }

private:
  BluetoothReplyRunnable* mRunnable;
};

bool
BluetoothServiceBluedroid::SetPairingConfirmationInternal(
  const nsAString& aDeviceAddress, bool aConfirm,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY(aRunnable, false);

  sBtInterface->SspReply(aDeviceAddress,
                         SSP_VARIANT_PASSKEY_CONFIRMATION,
                         aConfirm, 0, new SspReplyResultHandler(aRunnable));
  return true;
}
#endif

#ifdef MOZ_B2G_BT_API_V2

#else
bool
BluetoothServiceBluedroid::SetAuthorizationInternal(
  const nsAString& aDeviceAddress, bool aAllow,
  BluetoothReplyRunnable* aRunnable)
{
  return true;
}

nsresult
BluetoothServiceBluedroid::PrepareAdapterInternal()
{
  return NS_OK;
}
#endif

#ifdef MOZ_B2G_BT_API_V2
void
BluetoothServiceBluedroid::NextBluetoothProfileController()
#else
static void
NextBluetoothProfileController()
#endif
{
  MOZ_ASSERT(NS_IsMainThread());

  
  NS_ENSURE_FALSE_VOID(sControllerArray.IsEmpty());
  sControllerArray.RemoveElementAt(0);

  
  if (!sControllerArray.IsEmpty()) {
    sControllerArray[0]->StartSession();
  }
}

#ifdef MOZ_B2G_BT_API_V2
void
BluetoothServiceBluedroid::ConnectDisconnect(
  bool aConnect, const nsAString& aDeviceAddress,
  BluetoothReplyRunnable* aRunnable,
  uint16_t aServiceUuid, uint32_t aCod)
#else
static void
ConnectDisconnect(bool aConnect, const nsAString& aDeviceAddress,
                  BluetoothReplyRunnable* aRunnable,
                  uint16_t aServiceUuid, uint32_t aCod = 0)
#endif
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  BluetoothProfileController* controller =
    new BluetoothProfileController(aConnect, aDeviceAddress, aRunnable,
                                   NextBluetoothProfileController,
                                   aServiceUuid, aCod);
  sControllerArray.AppendElement(controller);

  




  if (sControllerArray.Length() == 1) {
    sControllerArray[0]->StartSession();
  }
}

void
BluetoothServiceBluedroid::Connect(const nsAString& aDeviceAddress,
                                   uint32_t aCod,
                                   uint16_t aServiceUuid,
                                   BluetoothReplyRunnable* aRunnable)
{
  ConnectDisconnect(true, aDeviceAddress, aRunnable, aServiceUuid, aCod);
}

void
BluetoothServiceBluedroid::Disconnect(
  const nsAString& aDeviceAddress, uint16_t aServiceUuid,
  BluetoothReplyRunnable* aRunnable)
{
  ConnectDisconnect(false, aDeviceAddress, aRunnable, aServiceUuid);
}

#ifdef MOZ_B2G_BT_API_V2
bool
BluetoothServiceBluedroid::IsConnected(uint16_t aProfileId)
{
  return true;
}
#else
void
BluetoothServiceBluedroid::IsConnected(const uint16_t aServiceUuid,
                                       BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  BluetoothProfileManagerBase* profile =
    BluetoothUuidHelper::GetBluetoothProfileManager(aServiceUuid);
  if (profile) {
    DispatchBluetoothReply(aRunnable, profile->IsConnected(), EmptyString());
  } else {
    BT_WARNING("Can't find profile manager with uuid: %x", aServiceUuid);
    DispatchBluetoothReply(aRunnable, false, EmptyString());
  }
}
#endif

void
BluetoothServiceBluedroid::SendFile(const nsAString& aDeviceAddress,
                                    BlobParent* aBlobParent,
                                    BlobChild* aBlobChild,
                                    BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  

#ifdef MOZ_B2G_BT_API_V2
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  if (!opp || !opp->SendFile(aDeviceAddress, aBlobParent)) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("SendFile failed"));
    return;
  }

  DispatchReplySuccess(aRunnable);
#else
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  nsAutoString errorStr;
  if (!opp || !opp->SendFile(aDeviceAddress, aBlobParent)) {
    errorStr.AssignLiteral("Calling SendFile() failed");
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), errorStr);
#endif
}

void
BluetoothServiceBluedroid::SendFile(const nsAString& aDeviceAddress,
                                    nsIDOMBlob* aBlob,
                                    BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  

#ifdef MOZ_B2G_BT_API_V2
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  if (!opp || !opp->SendFile(aDeviceAddress, aBlob)) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("SendFile failed"));
    return;
  }

  DispatchReplySuccess(aRunnable);
#else
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  nsAutoString errorStr;
  if (!opp || !opp->SendFile(aDeviceAddress, aBlob)) {
    errorStr.AssignLiteral("Calling SendFile() failed");
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), errorStr);
#endif
}

void
BluetoothServiceBluedroid::StopSendingFile(const nsAString& aDeviceAddress,
                                           BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  

#ifdef MOZ_B2G_BT_API_V2
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  nsAutoString errorStr;
  if (!opp || !opp->StopSendingFile()) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("StopSendingFile failed"));
    return;
  }

  DispatchReplySuccess(aRunnable);
#else
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  nsAutoString errorStr;
  if (!opp || !opp->StopSendingFile()) {
    errorStr.AssignLiteral("Calling StopSendingFile() failed");
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), errorStr);
#endif
}

void
BluetoothServiceBluedroid::ConfirmReceivingFile(
  const nsAString& aDeviceAddress, bool aConfirm,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  

#ifdef MOZ_B2G_BT_API_V2
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  nsAutoString errorStr;
  if (!opp || !opp->ConfirmReceivingFile(aConfirm)) {
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("ConfirmReceivingFile failed"));
    return;
  }

  DispatchReplySuccess(aRunnable);
#else
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  nsAutoString errorStr;
  if (!opp || !opp->ConfirmReceivingFile(aConfirm)) {
    errorStr.AssignLiteral("Calling ConfirmReceivingFile() failed");
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), errorStr);
#endif
}

void
BluetoothServiceBluedroid::ConnectSco(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

#ifdef MOZ_B2G_BT_API_V2
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  if (!hfp || !hfp->ConnectSco()) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("ConnectSco failed"));
    return;
  }

  DispatchReplySuccess(aRunnable);
#else
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  if (!hfp || !hfp->ConnectSco()) {
    NS_NAMED_LITERAL_STRING(replyError, "Calling ConnectSco() failed");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), replyError);
    return;
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), EmptyString());
#endif
}

void
BluetoothServiceBluedroid::DisconnectSco(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

#ifdef MOZ_B2G_BT_API_V2
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  if (!hfp || !hfp->DisconnectSco()) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("DisconnectSco failed"));
    return;
  }

  DispatchReplySuccess(aRunnable);
#else
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  if (!hfp || !hfp->DisconnectSco()) {
    NS_NAMED_LITERAL_STRING(replyError, "Calling DisconnectSco() failed");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), replyError);
    return;
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), EmptyString());
#endif
}

void
BluetoothServiceBluedroid::IsScoConnected(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

#ifdef MOZ_B2G_BT_API_V2
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  if (!hfp) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("IsScoConnected failed"));
    return;
  }

  DispatchReplySuccess(aRunnable, BluetoothValue(hfp->IsScoConnected()));
#else
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  if (!hfp) {
    NS_NAMED_LITERAL_STRING(replyError, "Fail to get BluetoothHfpManager");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), replyError);
    return;
  }

  DispatchBluetoothReply(aRunnable, hfp->IsScoConnected(), EmptyString());
#endif
}

void
BluetoothServiceBluedroid::SendMetaData(const nsAString& aTitle,
                                        const nsAString& aArtist,
                                        const nsAString& aAlbum,
                                        int64_t aMediaNumber,
                                        int64_t aTotalMediaCount,
                                        int64_t aDuration,
                                        BluetoothReplyRunnable* aRunnable)
{
  BluetoothA2dpManager* a2dp = BluetoothA2dpManager::Get();
  if (a2dp) {
    a2dp->UpdateMetaData(aTitle, aArtist, aAlbum, aMediaNumber,
                         aTotalMediaCount, aDuration);
  }
#ifdef MOZ_B2G_BT_API_V2
  DispatchReplySuccess(aRunnable);
#else
  DispatchBluetoothReply(aRunnable, BluetoothValue(true), EmptyString());
#endif
}

void
BluetoothServiceBluedroid::SendPlayStatus(
  int64_t aDuration, int64_t aPosition,
  const nsAString& aPlayStatus,
  BluetoothReplyRunnable* aRunnable)
{
  BluetoothA2dpManager* a2dp = BluetoothA2dpManager::Get();
  if (a2dp) {
    ControlPlayStatus playStatus =
      PlayStatusStringToControlPlayStatus(aPlayStatus);
    a2dp->UpdatePlayStatus(aDuration, aPosition, playStatus);
  }
#ifdef MOZ_B2G_BT_API_V2
  DispatchReplySuccess(aRunnable);
#else
  DispatchBluetoothReply(aRunnable, BluetoothValue(true), EmptyString());
#endif
}

void
BluetoothServiceBluedroid::UpdatePlayStatus(
  uint32_t aDuration, uint32_t aPosition, ControlPlayStatus aPlayStatus)
{
  
  
  
  MOZ_ASSERT(false);
}

nsresult
BluetoothServiceBluedroid::SendSinkMessage(const nsAString& aDeviceAddresses,
                                           const nsAString& aMessage)
{
  return NS_OK;
}

nsresult
BluetoothServiceBluedroid::SendInputMessage(const nsAString& aDeviceAddresses,
                                            const nsAString& aMessage)
{
  return NS_OK;
}

void
BluetoothServiceBluedroid::AnswerWaitingCall(BluetoothReplyRunnable* aRunnable)
{
}

void
BluetoothServiceBluedroid::IgnoreWaitingCall(BluetoothReplyRunnable* aRunnable)
{
}

void
BluetoothServiceBluedroid::ToggleCalls(BluetoothReplyRunnable* aRunnable)
{
}

#ifdef MOZ_B2G_BT_API_V2

#else
uint16_t
BluetoothServiceBluedroid::UuidToServiceClassInt(const BluetoothUuid& mUuid)
{
  
  uint16_t shortUuid;
  memcpy(&shortUuid, mUuid.mUuid + 2, sizeof(uint16_t));
  return ntohs(shortUuid);
}

bool
BluetoothServiceBluedroid::IsConnected(const nsAString& aRemoteBdAddr)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsString connectedAddress;

  
  BluetoothProfileManagerBase* profile;
  profile = BluetoothHfpManager::Get();
  if (profile && profile->IsConnected()) {
    profile->GetAddress(connectedAddress);
    if (aRemoteBdAddr.Equals(connectedAddress)) {
      return true;
    }
  }

  
  profile = BluetoothOppManager::Get();
  if (profile->IsConnected()) {
    profile->GetAddress(connectedAddress);
    if (aRemoteBdAddr.Equals(connectedAddress)) {
      return true;
    }
  }

  
  profile = BluetoothA2dpManager::Get();
  if (profile->IsConnected()) {
    profile->GetAddress(connectedAddress);
    if (aRemoteBdAddr.Equals(connectedAddress)) {
      return true;
    }
  }

  return false;
}
#endif









class BluetoothServiceBluedroid::ProfileDeinitResultHandler final
  : public BluetoothProfileResultHandler
{
public:
  ProfileDeinitResultHandler(unsigned char aNumProfiles)
    : mNumProfiles(aNumProfiles)
  {
    MOZ_ASSERT(mNumProfiles);
  }

  void Deinit() override
  {
    if (!(--mNumProfiles)) {
      Proceed();
    }
  }

  void OnError(nsresult aResult) override
  {
    if (!(--mNumProfiles)) {
      Proceed();
    }
  }

private:
  void Proceed() const
  {
    if (!sIsRestart) {
      sBtInterface->Cleanup(nullptr);
    } else {
      BT_LOGR("ProfileDeinitResultHandler::Proceed cancel cleanup() ");
    }
  }

  unsigned char mNumProfiles;
};

class BluetoothServiceBluedroid::SetAdapterPropertyDiscoverableResultHandler
  final
  : public BluetoothResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) override
  {
    BT_LOGR("Fail to set: BT_SCAN_MODE_CONNECTABLE");
  }
};

void
BluetoothServiceBluedroid::AdapterStateChangedNotification(bool aState)
{
#ifdef MOZ_B2G_BT_API_V2
  MOZ_ASSERT(NS_IsMainThread());

  BT_LOGR("BT_STATE: %d", aState);

  sAdapterEnabled = aState;

  if (!sAdapterEnabled) {
    static void (* const sDeinitManager[])(BluetoothProfileResultHandler*) = {
      BluetoothHfpManager::DeinitHfpInterface,
      BluetoothA2dpManager::DeinitA2dpInterface,
      BluetoothGattManager::DeinitGattInterface
    };

    
    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE_VOID(bs);

    
    sAdapterBdAddress.Truncate();
    sAdapterBdName.Truncate();

    InfallibleTArray<BluetoothNamedValue> props;
    BT_APPEND_NAMED_VALUE(props, "Name", sAdapterBdName);
    BT_APPEND_NAMED_VALUE(props, "Address", sAdapterBdAddress);
    if (sAdapterDiscoverable) {
      sAdapterDiscoverable = false;
      BT_APPEND_NAMED_VALUE(props, "Discoverable", false);
    }
    if (sAdapterDiscovering) {
      sAdapterDiscovering = false;
      BT_APPEND_NAMED_VALUE(props, "Discovering", false);
    }

    bs->DistributeSignal(NS_LITERAL_STRING("PropertyChanged"),
                         NS_LITERAL_STRING(KEY_ADAPTER),
                         BluetoothValue(props));

    
    nsRefPtr<ProfileDeinitResultHandler> res =
      new ProfileDeinitResultHandler(MOZ_ARRAY_LENGTH(sDeinitManager));

    for (size_t i = 0; i < MOZ_ARRAY_LENGTH(sDeinitManager); ++i) {
      sDeinitManager[i](res);
    }
  }

  BluetoothService::AcknowledgeToggleBt(sAdapterEnabled);

  if (sAdapterEnabled) {
    
    sControllerArray.Clear();
    sChangeDiscoveryRunnableArray.Clear();
    sSetPropertyRunnableArray.Clear();
    sGetDeviceRunnableArray.Clear();
    sFetchUuidsRunnableArray.Clear();
    sBondingRunnableArray.Clear();
    sUnbondingRunnableArray.Clear();
    sPairingNameTable.Clear();

    
    
    NS_ENSURE_TRUE_VOID(sBtInterface);
    sBtInterface->SetAdapterProperty(
      BluetoothNamedValue(NS_ConvertUTF8toUTF16("Discoverable"), false),
      new SetAdapterPropertyDiscoverableResultHandler());

    
    BluetoothOppManager* opp = BluetoothOppManager::Get();
    if (!opp || !opp->Listen()) {
      BT_LOGR("Fail to start BluetoothOppManager listening");
    }
  }

  
  if (!sChangeAdapterStateRunnableArray.IsEmpty()) {
    DispatchReplySuccess(sChangeAdapterStateRunnableArray[0]);
    sChangeAdapterStateRunnableArray.RemoveElementAt(0);
  }
#else
  MOZ_ASSERT(NS_IsMainThread());

  BT_LOGR("BT_STATE: %d", aState);

  if (sIsRestart && aState) {
    
    BT_LOGR("daemon restarted, reset flag");
    sIsRestart = false;
    sIsFirstTimeToggleOffBt = false;
  }
  bool isBtEnabled = (aState == true);

  if (!isBtEnabled) {
    static void (* const sDeinitManager[])(BluetoothProfileResultHandler*) = {
      BluetoothHfpManager::DeinitHfpInterface,
      BluetoothA2dpManager::DeinitA2dpInterface
    };

    
    nsRefPtr<ProfileDeinitResultHandler> res =
      new ProfileDeinitResultHandler(MOZ_ARRAY_LENGTH(sDeinitManager));

    for (size_t i = 0; i < MOZ_ARRAY_LENGTH(sDeinitManager); ++i) {
      sDeinitManager[i](res);
    }
  }

  BluetoothService::AcknowledgeToggleBt(isBtEnabled);

  if (isBtEnabled) {
    
    sControllerArray.Clear();
    sBondingRunnableArray.Clear();
    sGetDeviceRunnableArray.Clear();
    sSetPropertyRunnableArray.Clear();
    sUnbondingRunnableArray.Clear();

    
    
    NS_ENSURE_TRUE_VOID(sBtInterface);
    sBtInterface->SetAdapterProperty(
      BluetoothNamedValue(NS_ConvertUTF8toUTF16("Discoverable"), false),
      new SetAdapterPropertyDiscoverableResultHandler());

    
    
    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE_VOID(bs);

    bs->AdapterAddedReceived();
    bs->TryFiringAdapterAdded();

    
    BluetoothOppManager* opp = BluetoothOppManager::Get();
    if (!opp || !opp->Listen()) {
      BT_LOGR("Fail to start BluetoothOppManager listening");
    }
  }
  
  if (sIsRestart && !aState) {
    BT_LOGR("sIsRestart and off, now restart");
    StartBluetooth(false);
  }
#endif
}







void
BluetoothServiceBluedroid::AdapterPropertiesNotification(
  BluetoothStatus aStatus, int aNumProperties,
  const BluetoothProperty* aProperties)
{
#ifdef MOZ_B2G_BT_API_V2
  MOZ_ASSERT(NS_IsMainThread());

  InfallibleTArray<BluetoothNamedValue> propertiesArray;

  for (int i = 0; i < aNumProperties; i++) {

    const BluetoothProperty& p = aProperties[i];

    if (p.mType == PROPERTY_BDADDR) {
      sAdapterBdAddress = p.mString;
      BT_APPEND_NAMED_VALUE(propertiesArray, "Address", sAdapterBdAddress);

    } else if (p.mType == PROPERTY_BDNAME) {
      sAdapterBdName = p.mString;
      BT_APPEND_NAMED_VALUE(propertiesArray, "Name", sAdapterBdName);

    } else if (p.mType == PROPERTY_ADAPTER_SCAN_MODE) {
      sAdapterDiscoverable =
        (p.mScanMode == SCAN_MODE_CONNECTABLE_DISCOVERABLE);
      BT_APPEND_NAMED_VALUE(propertiesArray, "Discoverable",
                            sAdapterDiscoverable);

    } else if (p.mType == PROPERTY_ADAPTER_BONDED_DEVICES) {
      
      
      
      BT_LOGD("Adapter property: BONDED_DEVICES. Count: %d",
              p.mStringArray.Length());

      
      sAdapterBondedAddressArray.Clear();
      sAdapterBondedAddressArray.AppendElements(p.mStringArray);

      BT_APPEND_NAMED_VALUE(propertiesArray, "PairedDevices",
                            sAdapterBondedAddressArray);
    } else if (p.mType == PROPERTY_UNKNOWN) {
      
    } else {
      BT_LOGD("Unhandled adapter property type: %d", p.mType);
      continue;
    }
  }

  NS_ENSURE_TRUE_VOID(propertiesArray.Length() > 0);

  DistributeSignal(NS_LITERAL_STRING("PropertyChanged"),
                   NS_LITERAL_STRING(KEY_ADAPTER),
                   BluetoothValue(propertiesArray));

  
  if (!sSetPropertyRunnableArray.IsEmpty()) {
    DispatchReplySuccess(sSetPropertyRunnableArray[0]);
    sSetPropertyRunnableArray.RemoveElementAt(0);
  }
#else
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothValue propertyValue;
  InfallibleTArray<BluetoothNamedValue> props;

  for (int i = 0; i < aNumProperties; i++) {

    const BluetoothProperty& p = aProperties[i];

    if (p.mType == PROPERTY_BDADDR) {
      sAdapterBdAddress = p.mString;
      propertyValue = sAdapterBdAddress;
      BT_APPEND_NAMED_VALUE(props, "Address", propertyValue);

    } else if (p.mType == PROPERTY_BDNAME) {
      sAdapterBdName = p.mString;
      propertyValue = sAdapterBdName;
      BT_APPEND_NAMED_VALUE(props, "Name", propertyValue);

    } else if (p.mType == PROPERTY_ADAPTER_SCAN_MODE) {
      BluetoothScanMode newMode = p.mScanMode;

      if (newMode == SCAN_MODE_CONNECTABLE_DISCOVERABLE) {
        propertyValue = sAdapterDiscoverable = true;
      } else {
        propertyValue = sAdapterDiscoverable = false;
      }

      BT_APPEND_NAMED_VALUE(props, "Discoverable", propertyValue);

    } else if (p.mType == PROPERTY_ADAPTER_DISCOVERY_TIMEOUT) {
      propertyValue = sAdapterDiscoverableTimeout = p.mUint32;
      BT_APPEND_NAMED_VALUE(props, "DiscoverableTimeout", propertyValue);

    } else if (p.mType == PROPERTY_ADAPTER_BONDED_DEVICES) {
      
      
      
      BT_LOGD("Adapter property: BONDED_DEVICES. Count: %d",
              p.mStringArray.Length());

      
      sAdapterBondedAddressArray.Clear();

      for (size_t index = 0; index < p.mStringArray.Length(); index++) {
        sAdapterBondedAddressArray.AppendElement(p.mStringArray[index]);
      }

      propertyValue = sAdapterBondedAddressArray;
      BT_APPEND_NAMED_VALUE(props, "Devices", propertyValue);

    } else if (p.mType == PROPERTY_UUIDS) {
      
      continue;
    } else if (p.mType == PROPERTY_UNKNOWN) {
      
      continue;
    } else {
      BT_LOGD("Unhandled adapter property type: %d", p.mType);
      continue;
    }
  }

  NS_ENSURE_TRUE_VOID(props.Length() > 0);

  DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("PropertyChanged"),
                                   NS_LITERAL_STRING(KEY_ADAPTER),
                                   BluetoothValue(props)));

  

  if (!sSetPropertyRunnableArray.IsEmpty()) {
    DispatchBluetoothReply(sSetPropertyRunnableArray[0],
                           BluetoothValue(true), EmptyString());
    sSetPropertyRunnableArray.RemoveElementAt(0);
  }
#endif
}








void
BluetoothServiceBluedroid::RemoteDevicePropertiesNotification(
  BluetoothStatus aStatus, const nsAString& aBdAddr,
  int aNumProperties, const BluetoothProperty* aProperties)
{
#ifdef MOZ_B2G_BT_API_V2
  MOZ_ASSERT(NS_IsMainThread());

  InfallibleTArray<BluetoothNamedValue> propertiesArray;

  BT_APPEND_NAMED_VALUE(propertiesArray, "Address", nsString(aBdAddr));

  for (int i = 0; i < aNumProperties; ++i) {

    const BluetoothProperty& p = aProperties[i];

    if (p.mType == PROPERTY_BDNAME) {
      BT_APPEND_NAMED_VALUE(propertiesArray, "Name", p.mString);

    } else if (p.mType == PROPERTY_CLASS_OF_DEVICE) {
      uint32_t cod = p.mUint32;
      BT_APPEND_NAMED_VALUE(propertiesArray, "Cod", cod);

    } else if (p.mType == PROPERTY_UUIDS) {
      nsTArray<nsString> uuids;

      
      for (uint32_t index = 0; index < p.mUuidArray.Length(); ++index) {
        nsAutoString uuid;
        UuidToString(p.mUuidArray[index], uuid);

        if (!uuids.Contains(uuid)) { 
          uuids.InsertElementSorted(uuid);
        }
      }
      BT_APPEND_NAMED_VALUE(propertiesArray, "UUIDs", uuids);

    } else if (p.mType == PROPERTY_TYPE_OF_DEVICE) {
      BT_APPEND_NAMED_VALUE(propertiesArray, "Type",
                            static_cast<uint32_t>(p.mTypeOfDevice));

    } else if (p.mType == PROPERTY_UNKNOWN) {
      
    } else {
      BT_LOGD("Other non-handled device properties. Type: %d", p.mType);
    }
  }

  
  
  
  
  
  
  
  
  
  

  
  BluetoothSignal signal(NS_LITERAL_STRING("PropertyChanged"),
                         nsString(aBdAddr), propertiesArray);

  
  if (!sFetchUuidsRunnableArray.IsEmpty()) {
    
    DispatchReplySuccess(sFetchUuidsRunnableArray[0],
                         propertiesArray[1].value()); 
    sFetchUuidsRunnableArray.RemoveElementAt(0);
    DistributeSignal(signal);
    return;
  }

  
  if (sRequestedDeviceCountArray.IsEmpty()) {
    
    
    DistributeSignal(signal);
    return;
  }

  
  sRemoteDevicesPack.AppendElement(
    BluetoothNamedValue(nsString(aBdAddr), propertiesArray));

  if (--sRequestedDeviceCountArray[0] == 0) {
    if (!sGetDeviceRunnableArray.IsEmpty()) {
      DispatchReplySuccess(sGetDeviceRunnableArray[0], sRemoteDevicesPack);
      sGetDeviceRunnableArray.RemoveElementAt(0);
    }

    sRequestedDeviceCountArray.RemoveElementAt(0);
    sRemoteDevicesPack.Clear();
  }

  DistributeSignal(signal);
#else
  MOZ_ASSERT(NS_IsMainThread());

  InfallibleTArray<BluetoothNamedValue> props;

  BT_APPEND_NAMED_VALUE(props, "Address", BluetoothValue(nsString(aBdAddr)));

  bool isCodInvalid = false;
  for (int i = 0; i < aNumProperties; ++i) {

    const BluetoothProperty& p = aProperties[i];

    if (p.mType == PROPERTY_BDNAME) {
      BT_APPEND_NAMED_VALUE(props, "Name", p.mString);
    } else if (p.mType == PROPERTY_CLASS_OF_DEVICE) {
      uint32_t cod = p.mUint32;
      nsString icon;
      ClassToIcon(cod, icon);
      if (!icon.IsEmpty()) {
        
        BT_APPEND_NAMED_VALUE(props, "Class", cod);
        BT_APPEND_NAMED_VALUE(props, "Icon", icon);
      } else {
        
        
        
        isCodInvalid = true;
      }
    } else if (p.mType == PROPERTY_UUIDS) {
      InfallibleTArray<nsString> uuidsArray;
      uint32_t cod = 0;

      for (size_t i = 0; i < p.mUuidArray.Length(); i++) {
        uint16_t uuidServiceClass = UuidToServiceClassInt(p.mUuidArray[i]);
        BluetoothServiceClass serviceClass =
          BluetoothUuidHelper::GetBluetoothServiceClass(uuidServiceClass);

        
        nsString uuid;
        BluetoothUuidHelper::GetString(serviceClass, uuid);
        uuidsArray.AppendElement(uuid);

        
        if (isCodInvalid) {
          if (serviceClass == BluetoothServiceClass::HANDSFREE ||
              serviceClass == BluetoothServiceClass::HEADSET) {
            BT_LOGD("Restore Class Of Device to Audio bit");
            SET_AUDIO_BIT(cod);
          } else if (serviceClass == BluetoothServiceClass::A2DP_SINK) {
            BT_LOGD("Restore Class of Device to Rendering bit");
            SET_RENDERING_BIT(cod);
          }
        }
      }

      if (isCodInvalid) {
        BT_APPEND_NAMED_VALUE(props, "Class", cod);
        
        BT_APPEND_NAMED_VALUE(props, "Icon", NS_LITERAL_STRING("audio-card"));
      }
      BT_APPEND_NAMED_VALUE(props, "UUIDS", uuidsArray);
    } else if (p.mType == PROPERTY_UNKNOWN) {
      
    } else {
      BT_LOGD("Other non-handled device properties. Type: %d", p.mType);
    }
  }

  
  
  BT_APPEND_NAMED_VALUE(props, "Connected", IsConnected(aBdAddr));

  if (sRequestedDeviceCountArray.IsEmpty()) {
    
    
    return;
  }

  
  sRemoteDevicesPack.AppendElement(
    BluetoothNamedValue(nsString(aBdAddr), props));

  if (--sRequestedDeviceCountArray[0] == 0) {
    if (!sGetDeviceRunnableArray.IsEmpty()) {
      DispatchBluetoothReply(sGetDeviceRunnableArray[0],
                             sRemoteDevicesPack, EmptyString());
      sGetDeviceRunnableArray.RemoveElementAt(0);
    }

    sRequestedDeviceCountArray.RemoveElementAt(0);
    sRemoteDevicesPack.Clear();
  }

  
  DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("PropertyChanged"),
                                   nsString(aBdAddr),
                                   BluetoothValue(props)));
#endif
}

void
BluetoothServiceBluedroid::DeviceFoundNotification(
  int aNumProperties, const BluetoothProperty* aProperties)
{
#ifdef MOZ_B2G_BT_API_V2
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothValue propertyValue;
  InfallibleTArray<BluetoothNamedValue> propertiesArray;

  for (int i = 0; i < aNumProperties; i++) {

    const BluetoothProperty& p = aProperties[i];

    if (p.mType == PROPERTY_BDADDR) {
      BT_APPEND_NAMED_VALUE(propertiesArray, "Address", p.mString);

    } else if (p.mType == PROPERTY_BDNAME) {
      BT_APPEND_NAMED_VALUE(propertiesArray, "Name", p.mString);

    } else if (p.mType == PROPERTY_CLASS_OF_DEVICE) {
      BT_APPEND_NAMED_VALUE(propertiesArray, "Cod", p.mUint32);

    } else if (p.mType == PROPERTY_UUIDS) {
      nsTArray<nsString> uuids;

      
      for (uint32_t index = 0; index < p.mUuidArray.Length(); ++index) {
        nsAutoString uuid;
        UuidToString(p.mUuidArray[index], uuid);

        if (!uuids.Contains(uuid)) { 
          uuids.InsertElementSorted(uuid);
        }
      }
      BT_APPEND_NAMED_VALUE(propertiesArray, "UUIDs", uuids);

    } else if (p.mType == PROPERTY_TYPE_OF_DEVICE) {
      BT_APPEND_NAMED_VALUE(propertiesArray, "Type",
                            static_cast<uint32_t>(p.mTypeOfDevice));

    } else if (p.mType == PROPERTY_UNKNOWN) {
      
    } else {
      BT_LOGD("Not handled remote device property: %d", p.mType);
    }
  }

  DistributeSignal(NS_LITERAL_STRING("DeviceFound"),
                   NS_LITERAL_STRING(KEY_ADAPTER),
                   BluetoothValue(propertiesArray));
#else
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothValue propertyValue;
  InfallibleTArray<BluetoothNamedValue> propertiesArray;

  for (int i = 0; i < aNumProperties; i++) {

    const BluetoothProperty& p = aProperties[i];

    if (p.mType == PROPERTY_BDADDR) {
      propertyValue = p.mString;

      BT_APPEND_NAMED_VALUE(propertiesArray, "Address", propertyValue);
    } else if (p.mType == PROPERTY_BDNAME) {
      BT_APPEND_NAMED_VALUE(propertiesArray, "Name", p.mString);
    } else if (p.mType == PROPERTY_CLASS_OF_DEVICE) {
      uint32_t cod = p.mUint32;
      propertyValue = cod;
      BT_APPEND_NAMED_VALUE(propertiesArray, "Class", propertyValue);

      nsString icon;
      ClassToIcon(cod, icon);
      propertyValue = icon;
      BT_APPEND_NAMED_VALUE(propertiesArray, "Icon", propertyValue);
    } else if (p.mType == PROPERTY_UNKNOWN) {
      
    } else {
      BT_LOGD("Not handled remote device property: %d", p.mType);
    }
  }

  DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("DeviceFound"),
                                   NS_LITERAL_STRING(KEY_ADAPTER),
                                   BluetoothValue(propertiesArray)));
#endif
}

void
BluetoothServiceBluedroid::DiscoveryStateChangedNotification(bool aState)
{
#ifdef MOZ_B2G_BT_API_V2
  MOZ_ASSERT(NS_IsMainThread());

  sAdapterDiscovering = aState;

  
  InfallibleTArray<BluetoothNamedValue> propertiesArray;
  BT_APPEND_NAMED_VALUE(propertiesArray, "Discovering", sAdapterDiscovering);

  DistributeSignal(NS_LITERAL_STRING("PropertyChanged"),
                   NS_LITERAL_STRING(KEY_ADAPTER),
                   BluetoothValue(propertiesArray));

  
  if (!sChangeDiscoveryRunnableArray.IsEmpty()) {
    DispatchReplySuccess(sChangeDiscoveryRunnableArray[0]);
    sChangeDiscoveryRunnableArray.RemoveElementAt(0);
  }
#else
  MOZ_ASSERT(NS_IsMainThread());

  bool isDiscovering = (aState == true);

  DistributeSignal(
    BluetoothSignal(NS_LITERAL_STRING(DISCOVERY_STATE_CHANGED_ID),
                    NS_LITERAL_STRING(KEY_ADAPTER), isDiscovering));

  
  
  InfallibleTArray<BluetoothNamedValue> props;
  BT_APPEND_NAMED_VALUE(props, "Discovering", BluetoothValue(isDiscovering));
  DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("PropertyChanged"),
                                   NS_LITERAL_STRING(KEY_ADAPTER),
                                   BluetoothValue(props)));
#endif
}

void
BluetoothServiceBluedroid::PinRequestNotification(const nsAString& aRemoteBdAddr,
                                                  const nsAString& aBdName,
                                                  uint32_t aCod)
{
#ifdef MOZ_B2G_BT_API_V2
  MOZ_ASSERT(NS_IsMainThread());

  InfallibleTArray<BluetoothNamedValue> propertiesArray;

  BT_APPEND_NAMED_VALUE(propertiesArray, "address", nsString(aRemoteBdAddr));
  BT_APPEND_NAMED_VALUE(propertiesArray, "name", nsString(aBdName));
  BT_APPEND_NAMED_VALUE(propertiesArray, "passkey", EmptyString());
  BT_APPEND_NAMED_VALUE(propertiesArray, "type",
                        NS_LITERAL_STRING(PAIRING_REQ_TYPE_ENTERPINCODE));

  sPairingNameTable.Put(nsString(aRemoteBdAddr), nsString(aBdName));

  DistributeSignal(NS_LITERAL_STRING("PairingRequest"),
                   NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                   BluetoothValue(propertiesArray));
#else
  MOZ_ASSERT(NS_IsMainThread());

  InfallibleTArray<BluetoothNamedValue> propertiesArray;

  BT_APPEND_NAMED_VALUE(propertiesArray, "address", nsString(aRemoteBdAddr));
  BT_APPEND_NAMED_VALUE(propertiesArray, "method",
                        NS_LITERAL_STRING("pincode"));
  BT_APPEND_NAMED_VALUE(propertiesArray, "name", nsString(aBdName));

  DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("RequestPinCode"),
                                   NS_LITERAL_STRING(KEY_LOCAL_AGENT),
                                   BluetoothValue(propertiesArray)));
#endif
}

void
BluetoothServiceBluedroid::SspRequestNotification(
  const nsAString& aRemoteBdAddr, const nsAString& aBdName, uint32_t aCod,
  BluetoothSspVariant aPairingVariant, uint32_t aPassKey)
{
  MOZ_ASSERT(NS_IsMainThread());

#ifdef MOZ_B2G_BT_API_V2
  InfallibleTArray<BluetoothNamedValue> propertiesArray;
  nsAutoString passkey;
  nsAutoString pairingType;

  







  switch (aPairingVariant) {
    case SSP_VARIANT_PASSKEY_CONFIRMATION:
      pairingType.AssignLiteral(PAIRING_REQ_TYPE_CONFIRMATION);
      passkey.AppendInt(aPassKey);
      break;
    case SSP_VARIANT_PASSKEY_NOTIFICATION:
      pairingType.AssignLiteral(PAIRING_REQ_TYPE_DISPLAYPASSKEY);
      passkey.AppendInt(aPassKey);
      break;
    case SSP_VARIANT_CONSENT:
      pairingType.AssignLiteral(PAIRING_REQ_TYPE_CONSENT);
      break;
    default:
      BT_WARNING("Unhandled SSP Bonding Variant: %d", aPairingVariant);
      return;
  }

  BT_APPEND_NAMED_VALUE(propertiesArray, "address", nsString(aRemoteBdAddr));
  BT_APPEND_NAMED_VALUE(propertiesArray, "name", nsString(aBdName));
  BT_APPEND_NAMED_VALUE(propertiesArray, "passkey", passkey);
  BT_APPEND_NAMED_VALUE(propertiesArray, "type", pairingType);

  sPairingNameTable.Put(nsString(aRemoteBdAddr), nsString(aBdName));

  DistributeSignal(NS_LITERAL_STRING("PairingRequest"),
                   NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                   BluetoothValue(propertiesArray));
#else
  InfallibleTArray<BluetoothNamedValue> propertiesArray;

  BT_APPEND_NAMED_VALUE(propertiesArray, "address", nsString(aRemoteBdAddr));
  BT_APPEND_NAMED_VALUE(propertiesArray, "method",
                        NS_LITERAL_STRING("confirmation"));
  BT_APPEND_NAMED_VALUE(propertiesArray, "name", nsString(aBdName));
  BT_APPEND_NAMED_VALUE(propertiesArray, "passkey", aPassKey);

  DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("RequestConfirmation"),
                                   NS_LITERAL_STRING(KEY_LOCAL_AGENT),
                                   BluetoothValue(propertiesArray)));
#endif
}

void
BluetoothServiceBluedroid::BondStateChangedNotification(
  BluetoothStatus aStatus, const nsAString& aRemoteBdAddr,
  BluetoothBondState aState)
{
#ifdef MOZ_B2G_BT_API_V2
  MOZ_ASSERT(NS_IsMainThread());

  if (aState == BOND_STATE_BONDING) {
    
    return;
  }

  BT_LOGR("Bond state: %d status: %d", aState, aStatus);

  bool bonded = (aState == BOND_STATE_BONDED);
  if (aStatus != STATUS_SUCCESS) {
    if (!bonded) { 
      BT_LOGR("Pair failed! Abort pairing.");

      
      DistributeSignal(NS_LITERAL_STRING(PAIRING_ABORTED_ID),
                       NS_LITERAL_STRING(KEY_ADAPTER));

      
      if (!sBondingRunnableArray.IsEmpty()) {
        DispatchReplyError(sBondingRunnableArray[0], aStatus);
        sBondingRunnableArray.RemoveElementAt(0);
      }
    } else if (!sUnbondingRunnableArray.IsEmpty()) { 
      
      DispatchReplyError(sUnbondingRunnableArray[0], aStatus);
      sUnbondingRunnableArray.RemoveElementAt(0);
    }

    return;
  }

  
  nsString deviceName;
  bool nameExists = sPairingNameTable.Get(aRemoteBdAddr, &deviceName);
  if (nameExists) {
    sPairingNameTable.Remove(aRemoteBdAddr);
  }

  
  InfallibleTArray<BluetoothNamedValue> propertiesArray;
  nsString remoteBdAddr = nsString(aRemoteBdAddr);
  if (!bonded) {
    sAdapterBondedAddressArray.RemoveElement(remoteBdAddr);
  } else {
    if (!sAdapterBondedAddressArray.Contains(remoteBdAddr)) {
      sAdapterBondedAddressArray.AppendElement(remoteBdAddr);
    }

    
    
    
    
    MOZ_ASSERT(nameExists);
    BT_APPEND_NAMED_VALUE(propertiesArray, "Name", deviceName);
  }

  
  BT_APPEND_NAMED_VALUE(propertiesArray, "Paired", bonded);
  DistributeSignal(NS_LITERAL_STRING("PropertyChanged"),
                   aRemoteBdAddr,
                   BluetoothValue(propertiesArray));

  
  BT_INSERT_NAMED_VALUE(propertiesArray, 0, "Address", remoteBdAddr);
  DistributeSignal(bonded ? NS_LITERAL_STRING(DEVICE_PAIRED_ID)
                          : NS_LITERAL_STRING(DEVICE_UNPAIRED_ID),
                   NS_LITERAL_STRING(KEY_ADAPTER),
                   BluetoothValue(propertiesArray));

  
  if (bonded && !sBondingRunnableArray.IsEmpty()) {
    DispatchReplySuccess(sBondingRunnableArray[0]);
    sBondingRunnableArray.RemoveElementAt(0);
  } else if (!bonded && !sUnbondingRunnableArray.IsEmpty()) {
    DispatchReplySuccess(sUnbondingRunnableArray[0]);
    sUnbondingRunnableArray.RemoveElementAt(0);
  }
#else
  MOZ_ASSERT(NS_IsMainThread());

  if (aState == BOND_STATE_BONDING) {
    
    return;
  }

  if (aState == BOND_STATE_BONDED &&
      sAdapterBondedAddressArray.Contains(aRemoteBdAddr)) {
    
    return;
  }

  switch (aStatus) {
    case STATUS_SUCCESS:
    {
      bool bonded;
      if (aState == BOND_STATE_NONE) {
        bonded = false;
        sAdapterBondedAddressArray.RemoveElement(aRemoteBdAddr);
      } else if (aState == BOND_STATE_BONDED) {
        bonded = true;
        sAdapterBondedAddressArray.AppendElement(aRemoteBdAddr);
      } else {
        return;
      }

      
      InfallibleTArray<BluetoothNamedValue> propertiesChangeArray;
      BT_APPEND_NAMED_VALUE(propertiesChangeArray, "Devices",
                            sAdapterBondedAddressArray);

      DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("PropertyChanged"),
                                       NS_LITERAL_STRING(KEY_ADAPTER),
                                       BluetoothValue(propertiesChangeArray)));

      if (bonded && !sBondingRunnableArray.IsEmpty()) {
        DispatchBluetoothReply(sBondingRunnableArray[0],
                               BluetoothValue(true), EmptyString());

        sBondingRunnableArray.RemoveElementAt(0);
      } else if (!bonded && !sUnbondingRunnableArray.IsEmpty()) {
        DispatchBluetoothReply(sUnbondingRunnableArray[0],
                               BluetoothValue(true), EmptyString());

        sUnbondingRunnableArray.RemoveElementAt(0);
      }

      
      InfallibleTArray<BluetoothNamedValue> propertiesArray;
      BT_APPEND_NAMED_VALUE(propertiesArray, "address", nsString(aRemoteBdAddr));
      BT_APPEND_NAMED_VALUE(propertiesArray, "status", bonded);

      DistributeSignal(
        BluetoothSignal(NS_LITERAL_STRING(PAIRED_STATUS_CHANGED_ID),
                        NS_LITERAL_STRING(KEY_ADAPTER),
                        BluetoothValue(propertiesArray)));
      break;
    }
    case STATUS_BUSY:
    case STATUS_AUTH_FAILURE:
    case STATUS_RMT_DEV_DOWN:
    {
      InfallibleTArray<BluetoothNamedValue> propertiesArray;
      DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("Cancel"),
                                 NS_LITERAL_STRING(KEY_LOCAL_AGENT),
                                 BluetoothValue(propertiesArray)));

      if (!sBondingRunnableArray.IsEmpty()) {
        DispatchBluetoothReply(sBondingRunnableArray[0],
                               BluetoothValue(true),
                               NS_LITERAL_STRING("Authentication failure"));
        sBondingRunnableArray.RemoveElementAt(0);
      }
      break;
    }
    default:
      BT_WARNING("Got an unhandled status of BondStateChangedCallback!");
      
      if (!sBondingRunnableArray.IsEmpty()) {
        DispatchBluetoothReply(sBondingRunnableArray[0],
                               BluetoothValue(true),
                               NS_LITERAL_STRING("Internal failure"));
        sBondingRunnableArray.RemoveElementAt(0);
      }
      break;
  }
#endif
}

void
BluetoothServiceBluedroid::AclStateChangedNotification(
  BluetoothStatus aStatus, const nsAString& aRemoteBdAddr, bool aState)
{
  MOZ_ASSERT(NS_IsMainThread());

  
}

void
BluetoothServiceBluedroid::DutModeRecvNotification(uint16_t aOpcode,
                                                   const uint8_t* aBuf,
                                                   uint8_t aLen)
{
  MOZ_ASSERT(NS_IsMainThread());

  
}

void
BluetoothServiceBluedroid::LeTestModeNotification(BluetoothStatus aStatus,
                                                  uint16_t aNumPackets)
{
  MOZ_ASSERT(NS_IsMainThread());

  
}

#ifdef MOZ_B2G_BT_API_V2

#else
void
BluetoothServiceBluedroid::EnergyInfoNotification(
  const BluetoothActivityEnergyInfo& aInfo)
{
  MOZ_ASSERT(NS_IsMainThread());

  
}
#endif

void
BluetoothServiceBluedroid::BackendErrorNotification(bool aCrashed)
{
  MOZ_ASSERT(NS_IsMainThread());
 
 if (aCrashed) {
  BT_LOGR("Set aRestart = true");
  sIsRestart = true;
  BT_LOGR("Reocvery step2: stop bluetooth");
#ifdef MOZ_B2G_BT_API_V2
  StopBluetooth(false, nullptr);
#else
  StopBluetooth(false);
#endif
 }
}

void
BluetoothServiceBluedroid::CompleteToggleBt(bool aEnabled)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (sIsRestart && !aEnabled && sIsFirstTimeToggleOffBt) {
    
    
  } else if (sIsRestart && !aEnabled && !sIsFirstTimeToggleOffBt) {
    
    BT_LOGR("CompleteToggleBt set sIsFirstTimeToggleOffBt = true");
    sIsFirstTimeToggleOffBt = true;
    BluetoothService::CompleteToggleBt(aEnabled);
    AdapterStateChangedNotification(false);
  } else {
    BluetoothService::CompleteToggleBt(aEnabled);
  }
}
