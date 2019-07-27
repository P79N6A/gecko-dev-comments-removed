

















#include "BluetoothServiceBluedroid.h"

#include "BluetoothA2dpManager.h"
#include "BluetoothGattManager.h"
#include "BluetoothHfpManager.h"
#include "BluetoothHidManager.h"
#include "BluetoothOppManager.h"
#include "BluetoothProfileController.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothUtils.h"
#include "BluetoothUuid.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/ipc/UnixSocket.h"
#include "mozilla/StaticMutex.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "nsDataHashtable.h"

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

class BluetoothServiceBluedroid::EnableResultHandler MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    BT_LOGR("BluetoothInterface::Enable failed: %d", aStatus);

    BluetoothService::AcknowledgeToggleBt(false);
  }
};





class BluetoothServiceBluedroid::ProfileInitResultHandler MOZ_FINAL
  : public BluetoothProfileResultHandler
{
public:
  ProfileInitResultHandler(unsigned char aNumProfiles)
    : mNumProfiles(aNumProfiles)
  {
    MOZ_ASSERT(mNumProfiles);
  }

  void Init() MOZ_OVERRIDE
  {
    if (!(--mNumProfiles)) {
      Proceed();
    }
  }

  void OnError(nsresult aResult) MOZ_OVERRIDE
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

class BluetoothServiceBluedroid::InitResultHandler MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  void Init() MOZ_OVERRIDE
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

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
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

class BluetoothServiceBluedroid::DisableResultHandler MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
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

class BluetoothServiceBluedroid::GetRemoteDevicePropertiesResultHandler
  MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  GetRemoteDevicePropertiesResultHandler(const nsAString& aDeviceAddress)
  : mDeviceAddress(aDeviceAddress)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

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

  int requestedDeviceCount = aDeviceAddress.Length();
  if (requestedDeviceCount == 0) {
    DispatchReplySuccess(aRunnable);
    return NS_OK;
  }

  for (int i = 0; i < requestedDeviceCount; i++) {
    
    sBtInterface->GetRemoteDeviceProperties(aDeviceAddress[i],
      new GetRemoteDevicePropertiesResultHandler(aDeviceAddress[i]));
  }

  return NS_OK;
}

class BluetoothServiceBluedroid::StartDiscoveryResultHandler MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  StartDiscoveryResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    sChangeDiscoveryRunnableArray.RemoveElement(mRunnable);
    DispatchReplyError(mRunnable, aStatus);
  }

private:
  BluetoothReplyRunnable* mRunnable;
};

nsresult
BluetoothServiceBluedroid::StartDiscoveryInternal(
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY(aRunnable, NS_OK);

  sChangeDiscoveryRunnableArray.AppendElement(aRunnable);
  sBtInterface->StartDiscovery(new StartDiscoveryResultHandler(aRunnable));

  return NS_OK;
}

class BluetoothServiceBluedroid::CancelDiscoveryResultHandler MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  CancelDiscoveryResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    sChangeDiscoveryRunnableArray.RemoveElement(mRunnable);
    DispatchReplyError(mRunnable, aStatus);
  }

private:
  BluetoothReplyRunnable* mRunnable;
};

nsresult
BluetoothServiceBluedroid::StopDiscoveryInternal(
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY(aRunnable, NS_OK);

  sChangeDiscoveryRunnableArray.AppendElement(aRunnable);
  sBtInterface->CancelDiscovery(new CancelDiscoveryResultHandler(aRunnable));

  return NS_OK;
}

class BluetoothServiceBluedroid::GetRemoteServicesResultHandler MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  GetRemoteServicesResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
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

class BluetoothServiceBluedroid::SetAdapterPropertyResultHandler MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  SetAdapterPropertyResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    sSetPropertyRunnableArray.RemoveElement(mRunnable);
    DispatchReplyError(mRunnable, aStatus);
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

class BluetoothServiceBluedroid::CreateBondResultHandler MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  CreateBondResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  {
    MOZ_ASSERT(mRunnable);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    sBondingRunnableArray.RemoveElement(mRunnable);
    DispatchReplyError(mRunnable, aStatus);
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

  sBtInterface->CreateBond(aDeviceAddress,
                           new CreateBondResultHandler(aRunnable));
  return NS_OK;
}

class BluetoothServiceBluedroid::RemoveBondResultHandler MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  RemoveBondResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  {
    MOZ_ASSERT(mRunnable);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    sUnbondingRunnableArray.RemoveElement(mRunnable);
    DispatchReplyError(mRunnable, aStatus);
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

class BluetoothServiceBluedroid::PinReplyResultHandler MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  PinReplyResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void PinReply() MOZ_OVERRIDE
  {
    DispatchReplySuccess(mRunnable);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    DispatchReplyError(mRunnable, aStatus);
  }

private:
  BluetoothReplyRunnable* mRunnable;
};

void
BluetoothServiceBluedroid::SetPinCodeInternal(
  const nsAString& aDeviceAddress, const nsAString& aPinCode,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  sBtInterface->PinReply(aDeviceAddress, true, aPinCode,
    new PinReplyResultHandler(aRunnable));
}

void
BluetoothServiceBluedroid::SetPasskeyInternal(
  const nsAString& aDeviceAddress, uint32_t aPasskey,
  BluetoothReplyRunnable* aRunnable)
{
  return;
}

class BluetoothServiceBluedroid::SspReplyResultHandler MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  SspReplyResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void SspReply() MOZ_OVERRIDE
  {
    DispatchReplySuccess(mRunnable);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    DispatchReplyError(mRunnable, aStatus);
  }

private:
  BluetoothReplyRunnable* mRunnable;
};

void
BluetoothServiceBluedroid::SetPairingConfirmationInternal(
  const nsAString& aDeviceAddress, bool aConfirm,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  sBtInterface->SspReply(aDeviceAddress,
                         NS_ConvertUTF8toUTF16("PasskeyConfirmation"),
                         aConfirm, 0, new SspReplyResultHandler(aRunnable));
}

void
BluetoothServiceBluedroid::NextBluetoothProfileController()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  NS_ENSURE_FALSE_VOID(sControllerArray.IsEmpty());
  sControllerArray.RemoveElementAt(0);

  
  if (!sControllerArray.IsEmpty()) {
    sControllerArray[0]->StartSession();
  }
}

void
BluetoothServiceBluedroid::ConnectDisconnect(
  bool aConnect, const nsAString& aDeviceAddress,
  BluetoothReplyRunnable* aRunnable,
  uint16_t aServiceUuid, uint32_t aCod)
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

bool
BluetoothServiceBluedroid::IsConnected(uint16_t aProfileId)
{
  return true;
}

void
BluetoothServiceBluedroid::Disconnect(
  const nsAString& aDeviceAddress, uint16_t aServiceUuid,
  BluetoothReplyRunnable* aRunnable)
{
  ConnectDisconnect(false, aDeviceAddress, aRunnable, aServiceUuid);
}

void
BluetoothServiceBluedroid::SendFile(const nsAString& aDeviceAddress,
                                    BlobParent* aBlobParent,
                                    BlobChild* aBlobChild,
                                    BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  if (!opp || !opp->SendFile(aDeviceAddress, aBlobParent)) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("SendFile failed"));
    return;
  }

  DispatchReplySuccess(aRunnable);
}

void
BluetoothServiceBluedroid::SendFile(const nsAString& aDeviceAddress,
                                    nsIDOMBlob* aBlob,
                                    BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  if (!opp || !opp->SendFile(aDeviceAddress, aBlob)) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("SendFile failed"));
    return;
  }

  DispatchReplySuccess(aRunnable);
}

void
BluetoothServiceBluedroid::StopSendingFile(const nsAString& aDeviceAddress,
                                           BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  nsAutoString errorStr;
  if (!opp || !opp->StopSendingFile()) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("StopSendingFile failed"));
    return;
  }

  DispatchReplySuccess(aRunnable);
}

void
BluetoothServiceBluedroid::ConfirmReceivingFile(
  const nsAString& aDeviceAddress, bool aConfirm,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  nsAutoString errorStr;
  if (!opp || !opp->ConfirmReceivingFile(aConfirm)) {
    DispatchReplyError(aRunnable,
                       NS_LITERAL_STRING("ConfirmReceivingFile failed"));
    return;
  }

  DispatchReplySuccess(aRunnable);
}

void
BluetoothServiceBluedroid::ConnectSco(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  if (!hfp || !hfp->ConnectSco()) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("ConnectSco failed"));
    return;
  }

  DispatchReplySuccess(aRunnable);
}

void
BluetoothServiceBluedroid::DisconnectSco(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  if (!hfp || !hfp->DisconnectSco()) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("DisconnectSco failed"));
    return;
  }

  DispatchReplySuccess(aRunnable);
}

void
BluetoothServiceBluedroid::IsScoConnected(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  if (!hfp) {
    DispatchReplyError(aRunnable, NS_LITERAL_STRING("IsScoConnected failed"));
    return;
  }

  DispatchReplySuccess(aRunnable, BluetoothValue(hfp->IsScoConnected()));
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
  DispatchReplySuccess(aRunnable);
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
  DispatchReplySuccess(aRunnable);
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
BluetoothServiceBluedroid::UnregisterGattClientInternal(
  int aClientIf, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  ENSURE_BLUETOOTH_IS_READY_VOID(aRunnable);

  BluetoothGattManager* gatt = BluetoothGattManager::Get();
  ENSURE_GATT_MGR_IS_READY_VOID(gatt, aRunnable);

  gatt->UnregisterClient(aClientIf, aRunnable);
}









class BluetoothServiceBluedroid::ProfileDeinitResultHandler MOZ_FINAL
  : public BluetoothProfileResultHandler
{
public:
  ProfileDeinitResultHandler(unsigned char aNumProfiles)
    : mNumProfiles(aNumProfiles)
  {
    MOZ_ASSERT(mNumProfiles);
  }

  void Deinit() MOZ_OVERRIDE
  {
    if (!(--mNumProfiles)) {
      Proceed();
    }
  }

  void OnError(nsresult aResult) MOZ_OVERRIDE
  {
    if (!(--mNumProfiles)) {
      Proceed();
    }
  }

private:
  void Proceed() const
  {
    sBtInterface->Cleanup(nullptr);
  }

  unsigned char mNumProfiles;
};

class BluetoothServiceBluedroid::SetAdapterPropertyDiscoverableResultHandler
  MOZ_FINAL
  : public BluetoothResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_LOGR("Fail to set: BT_SCAN_MODE_CONNECTABLE");
  }
};

void
BluetoothServiceBluedroid::AdapterStateChangedNotification(bool aState)
{
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

    BluetoothSignal signal(NS_LITERAL_STRING("PropertyChanged"),
                           NS_LITERAL_STRING(KEY_ADAPTER), props);
    bs->DistributeSignal(signal);

    
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
}







void
BluetoothServiceBluedroid::AdapterPropertiesNotification(
  BluetoothStatus aStatus, int aNumProperties,
  const BluetoothProperty* aProperties)
{
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

  DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("PropertyChanged"),
                                   NS_LITERAL_STRING(KEY_ADAPTER),
                                   BluetoothValue(propertiesArray)));

  
  if (!sSetPropertyRunnableArray.IsEmpty()) {
    DispatchReplySuccess(sSetPropertyRunnableArray[0]);
    sSetPropertyRunnableArray.RemoveElementAt(0);
  }
}








void
BluetoothServiceBluedroid::RemoteDevicePropertiesNotification(
  BluetoothStatus aStatus, const nsAString& aBdAddr,
  int aNumProperties, const BluetoothProperty* aProperties)
{
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
}

void
BluetoothServiceBluedroid::DeviceFoundNotification(
  int aNumProperties, const BluetoothProperty* aProperties)
{
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

  DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("DeviceFound"),
                                   NS_LITERAL_STRING(KEY_ADAPTER),
                                   BluetoothValue(propertiesArray)));
}

void
BluetoothServiceBluedroid::DiscoveryStateChangedNotification(bool aState)
{
  MOZ_ASSERT(NS_IsMainThread());

  sAdapterDiscovering = aState;

  
  InfallibleTArray<BluetoothNamedValue> propertiesArray;
  BT_APPEND_NAMED_VALUE(propertiesArray, "Discovering", sAdapterDiscovering);

  DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("PropertyChanged"),
                                   NS_LITERAL_STRING(KEY_ADAPTER),
                                   BluetoothValue(propertiesArray)));

  
  if (!sChangeDiscoveryRunnableArray.IsEmpty()) {
    DispatchReplySuccess(sChangeDiscoveryRunnableArray[0]);
    sChangeDiscoveryRunnableArray.RemoveElementAt(0);
  }
}

void
BluetoothServiceBluedroid::PinRequestNotification(const nsAString& aRemoteBdAddr,
                                                  const nsAString& aBdName,
                                                  uint32_t aCod)
{
  MOZ_ASSERT(NS_IsMainThread());

  InfallibleTArray<BluetoothNamedValue> propertiesArray;

  BT_APPEND_NAMED_VALUE(propertiesArray, "address", nsString(aRemoteBdAddr));
  BT_APPEND_NAMED_VALUE(propertiesArray, "name", nsString(aBdName));
  BT_APPEND_NAMED_VALUE(propertiesArray, "passkey", EmptyString());
  BT_APPEND_NAMED_VALUE(propertiesArray, "type",
                        NS_LITERAL_STRING(PAIRING_REQ_TYPE_ENTERPINCODE));

  sPairingNameTable.Put(nsString(aRemoteBdAddr), nsString(aBdName));

  DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("PairingRequest"),
                                   NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                                   BluetoothValue(propertiesArray)));
}

void
BluetoothServiceBluedroid::SspRequestNotification(
  const nsAString& aRemoteBdAddr, const nsAString& aBdName, uint32_t aCod,
  BluetoothSspVariant aPairingVariant, uint32_t aPasskey)
{
  MOZ_ASSERT(NS_IsMainThread());

  InfallibleTArray<BluetoothNamedValue> propertiesArray;
  nsAutoString passkey;
  nsAutoString pairingType;

  







  switch (aPairingVariant) {
    case SSP_VARIANT_PASSKEY_CONFIRMATION:
      pairingType.AssignLiteral(PAIRING_REQ_TYPE_CONFIRMATION);
      passkey.AppendInt(aPasskey);
      break;
    case SSP_VARIANT_PASSKEY_NOTIFICATION:
      pairingType.AssignLiteral(PAIRING_REQ_TYPE_DISPLAYPASSKEY);
      passkey.AppendInt(aPasskey);
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

  DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("PairingRequest"),
                                   NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                                   BluetoothValue(propertiesArray)));
}

void
BluetoothServiceBluedroid::BondStateChangedNotification(
  BluetoothStatus aStatus, const nsAString& aRemoteBdAddr,
  BluetoothBondState aState)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (aState == BOND_STATE_BONDING) {
    
    return;
  }

  bool bonded = (aState == BOND_STATE_BONDED);

  
  nsString remoteBdAddr = nsString(aRemoteBdAddr);
  if (bonded) {
    if (!sAdapterBondedAddressArray.Contains(remoteBdAddr)) {
      sAdapterBondedAddressArray.AppendElement(remoteBdAddr);
    }
  } else {
    sAdapterBondedAddressArray.RemoveElement(remoteBdAddr);
  }

  
  InfallibleTArray<BluetoothNamedValue> propertiesArray;
  BT_APPEND_NAMED_VALUE(propertiesArray, "Paired", bonded);

  
  nsString deviceName = EmptyString();
  bool nameExists = sPairingNameTable.Get(aRemoteBdAddr, &deviceName);
  if (nameExists) {
    sPairingNameTable.Remove(aRemoteBdAddr);
  }

  
  if (bonded && STATUS_SUCCESS) {
    MOZ_ASSERT(nameExists);
    BT_APPEND_NAMED_VALUE(propertiesArray, "Name", deviceName);
  }

  DistributeSignal(BluetoothSignal(NS_LITERAL_STRING("PropertyChanged"),
                                   nsString(aRemoteBdAddr),
                                   BluetoothValue(propertiesArray)));

  
  BT_INSERT_NAMED_VALUE(propertiesArray, 0, "Address", nsString(aRemoteBdAddr));

  nsString signalName = bonded ? NS_LITERAL_STRING(DEVICE_PAIRED_ID)
                               : NS_LITERAL_STRING(DEVICE_UNPAIRED_ID);

  DistributeSignal(BluetoothSignal(signalName,
                                   NS_LITERAL_STRING(KEY_ADAPTER),
                                   BluetoothValue(propertiesArray)));

  if (aStatus == STATUS_SUCCESS) {
    
    if (bonded && !sBondingRunnableArray.IsEmpty()) {
      DispatchReplySuccess(sBondingRunnableArray[0]);
      sBondingRunnableArray.RemoveElementAt(0);
    } else if (!bonded && !sUnbondingRunnableArray.IsEmpty()) {
      DispatchReplySuccess(sUnbondingRunnableArray[0]);
      sUnbondingRunnableArray.RemoveElementAt(0);
    }
  } else {
    
    if (!bonded && !sBondingRunnableArray.IsEmpty()) {
      DispatchReplyError(sBondingRunnableArray[0],
                         NS_LITERAL_STRING("Pair failed"));
      sBondingRunnableArray.RemoveElementAt(0);
    } else if (bonded && !sUnbondingRunnableArray.IsEmpty()) {
      DispatchReplyError(sUnbondingRunnableArray[0],
                         NS_LITERAL_STRING("Unpair failed"));
      sUnbondingRunnableArray.RemoveElementAt(0);
    }
  }
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
