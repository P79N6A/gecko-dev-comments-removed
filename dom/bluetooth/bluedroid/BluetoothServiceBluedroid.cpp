

















#include "BluetoothServiceBluedroid.h"

#include "BluetoothA2dpManager.h"
#include "BluetoothHfpManager.h"
#include "BluetoothOppManager.h"
#include "BluetoothInterface.h"
#include "BluetoothProfileController.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothUtils.h"
#include "BluetoothUuid.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/ipc/UnixSocket.h"
#include "mozilla/StaticMutex.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"

#define ENSURE_BLUETOOTH_IS_READY(runnable, result)                    \
  do {                                                                 \
    if (!sBtInterface || !IsEnabled()) {                               \
      NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth is not ready");     \
      DispatchBluetoothReply(runnable, BluetoothValue(), errorStr);    \
      return result;                                                   \
    }                                                                  \
  } while(0)

#define MAX_UUID_SIZE 16

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





static Atomic<bool> sAdapterDiscoverable(false);
static Atomic<uint32_t> sAdapterDiscoverableTimeout(0);




class DistributeBluetoothSignalTask MOZ_FINAL : public nsRunnable
{
public:
  DistributeBluetoothSignalTask(const BluetoothSignal& aSignal) :
    mSignal(aSignal)
  {
  }

  NS_IMETHOD
  Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE(bs, NS_ERROR_FAILURE);

    bs->DistributeSignal(mSignal);

    return NS_OK;
  }

private:
  BluetoothSignal mSignal;
};

class SetupAfterEnabledTask MOZ_FINAL : public nsRunnable
{
public:
  class SetAdapterPropertyResultHandler MOZ_FINAL
  : public BluetoothResultHandler
  {
  public:
    void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
    {
      BT_LOGR("Fail to set: BT_SCAN_MODE_CONNECTABLE");
    }
  };

  NS_IMETHOD
  Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    sControllerArray.Clear();
    sBondingRunnableArray.Clear();
    sGetDeviceRunnableArray.Clear();
    sSetPropertyRunnableArray.Clear();
    sUnbondingRunnableArray.Clear();

    
    NS_ENSURE_TRUE(sBtInterface, NS_ERROR_FAILURE);
    sBtInterface->SetAdapterProperty(
      BluetoothNamedValue(NS_ConvertUTF8toUTF16("Discoverable"), true),
      new SetAdapterPropertyResultHandler());

    
    
    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE(bs, NS_ERROR_FAILURE);

    bs->AdapterAddedReceived();
    bs->TryFiringAdapterAdded();

    
    BluetoothOppManager* opp = BluetoothOppManager::Get();
    if (!opp || !opp->Listen()) {
      BT_LOGR("Fail to start BluetoothOppManager listening");
    }

    return NS_OK;
  }
};





class ProfileDeinitResultHandler MOZ_FINAL
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

class CleanupTask MOZ_FINAL : public nsRunnable
{
public:
  NS_IMETHOD
  Run()
  {
    static void (* const sDeinitManager[])(BluetoothProfileResultHandler*) = {
      BluetoothHfpManager::DeinitHfpInterface,
      BluetoothA2dpManager::DeinitA2dpInterface
    };

    MOZ_ASSERT(NS_IsMainThread());

    
    nsRefPtr<ProfileDeinitResultHandler> res =
      new ProfileDeinitResultHandler(MOZ_ARRAY_LENGTH(sDeinitManager));

    for (size_t i = 0; i < MOZ_ARRAY_LENGTH(sDeinitManager); ++i) {
      sDeinitManager[i](res);
    }

    return NS_OK;
  }
};




static void
ClassToIcon(uint32_t aClass, nsAString& aRetIcon)
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

static ControlPlayStatus
PlayStatusStringToControlPlayStatus(const nsAString& aPlayStatus)
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






static void
AdapterStateChangeCallback(bt_state_t aStatus)
{
  MOZ_ASSERT(!NS_IsMainThread());

  BT_LOGR("BT_STATE: %d", aStatus);

  bool isBtEnabled = (aStatus == BT_STATE_ON);

  if (!isBtEnabled &&
      NS_FAILED(NS_DispatchToMainThread(new CleanupTask()))) {
    BT_WARNING("Failed to dispatch to main thread!");
    return;
  }

  nsRefPtr<nsRunnable> runnable =
    new BluetoothService::ToggleBtAck(isBtEnabled);
  if (NS_FAILED(NS_DispatchToMainThread(runnable))) {
    BT_WARNING("Failed to dispatch to main thread!");
    return;
  }

  if (isBtEnabled &&
      NS_FAILED(NS_DispatchToMainThread(new SetupAfterEnabledTask()))) {
    BT_WARNING("Failed to dispatch to main thread!");
    return;
  }
}

class AdapterPropertiesCallbackTask MOZ_FINAL : public nsRunnable
{
public:
  NS_IMETHOD
  Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (!sSetPropertyRunnableArray.IsEmpty()) {
      DispatchBluetoothReply(sSetPropertyRunnableArray[0],
                             BluetoothValue(true), EmptyString());
      sSetPropertyRunnableArray.RemoveElementAt(0);
    }

    return NS_OK;
  }
};






static void
AdapterPropertiesCallback(bt_status_t aStatus, int aNumProperties,
                          bt_property_t *aProperties)
{
  MOZ_ASSERT(!NS_IsMainThread());

  BluetoothValue propertyValue;
  InfallibleTArray<BluetoothNamedValue> props;

  for (int i = 0; i < aNumProperties; i++) {
    bt_property_t p;
    
    memcpy(&p, &aProperties[i], sizeof(p));

    if (p.type == BT_PROPERTY_BDADDR) {
      BdAddressTypeToString((bt_bdaddr_t*)p.val, sAdapterBdAddress);
      propertyValue = sAdapterBdAddress;
      BT_APPEND_NAMED_VALUE(props, "Address", propertyValue);
    } else if (p.type == BT_PROPERTY_BDNAME) {
      
      
      propertyValue = sAdapterBdName = NS_ConvertUTF8toUTF16(
        nsCString((char*)p.val, p.len));
      BT_APPEND_NAMED_VALUE(props, "Name", propertyValue);
    } else if (p.type == BT_PROPERTY_ADAPTER_SCAN_MODE) {
      bt_scan_mode_t newMode = *(bt_scan_mode_t*)p.val;

      if (newMode == BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE) {
        propertyValue = sAdapterDiscoverable = true;
      } else {
        propertyValue = sAdapterDiscoverable = false;
      }

      BT_APPEND_NAMED_VALUE(props, "Discoverable", propertyValue);
    } else if (p.type == BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT) {
      propertyValue = sAdapterDiscoverableTimeout = *(uint32_t*)p.val;
      BT_APPEND_NAMED_VALUE(props, "DiscoverableTimeout", propertyValue);
    } else if (p.type == BT_PROPERTY_ADAPTER_BONDED_DEVICES) {
      
      
      
      bt_bdaddr_t* deviceBdAddressTypes = (bt_bdaddr_t*)p.val;
      int numOfAddresses = p.len / BLUETOOTH_ADDRESS_BYTES;
      BT_LOGD("Adapter property: BONDED_DEVICES. Count: %d", numOfAddresses);

      
      sAdapterBondedAddressArray.Clear();

      for (int index = 0; index < numOfAddresses; index++) {
        nsAutoString deviceBdAddress;
        BdAddressTypeToString(deviceBdAddressTypes + index, deviceBdAddress);
        sAdapterBondedAddressArray.AppendElement(deviceBdAddress);
      }

      propertyValue = sAdapterBondedAddressArray;
      BT_APPEND_NAMED_VALUE(props, "Devices", propertyValue);
    } else if (p.type == BT_PROPERTY_UUIDS) {
      
      continue;
    } else {
      BT_LOGD("Unhandled adapter property type: %d", p.type);
      continue;
    }
  }

  NS_ENSURE_TRUE_VOID(props.Length() > 0);

  BluetoothValue value(props);
  BluetoothSignal signal(NS_LITERAL_STRING("PropertyChanged"),
                         NS_LITERAL_STRING(KEY_ADAPTER), value);
  nsRefPtr<DistributeBluetoothSignalTask>
    t = new DistributeBluetoothSignalTask(signal);
  if (NS_FAILED(NS_DispatchToMainThread(t))) {
    BT_WARNING("Failed to dispatch to main thread!");
  }

  
  NS_DispatchToMainThread(new AdapterPropertiesCallbackTask());
}

class RemoteDevicePropertiesCallbackTask : public nsRunnable
{
  const InfallibleTArray<BluetoothNamedValue> mProps;
  nsString mRemoteDeviceBdAddress;
public:
  RemoteDevicePropertiesCallbackTask(
    const InfallibleTArray<BluetoothNamedValue>& aProps,
    const nsAString& aRemoteDeviceBdAddress)
  : mProps(aProps)
  , mRemoteDeviceBdAddress(aRemoteDeviceBdAddress)
  { }

  NS_IMETHOD
  Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (sRequestedDeviceCountArray.IsEmpty()) {
      
      
      return NS_OK;
    }

    
    BluetoothSignal signal(NS_LITERAL_STRING("PropertyChanged"),
                           mRemoteDeviceBdAddress, mProps);
    nsRefPtr<DistributeBluetoothSignalTask>
      t = new DistributeBluetoothSignalTask(signal);
    if (NS_FAILED(NS_DispatchToMainThread(t))) {
      BT_WARNING("Failed to dispatch to main thread!");
      return NS_OK;
    }

    
    sRemoteDevicesPack.AppendElement(
      BluetoothNamedValue(mRemoteDeviceBdAddress, mProps));

    if (--sRequestedDeviceCountArray[0] == 0) {
      if (!sGetDeviceRunnableArray.IsEmpty()) {
        DispatchBluetoothReply(sGetDeviceRunnableArray[0],
                               sRemoteDevicesPack, EmptyString());
        sGetDeviceRunnableArray.RemoveElementAt(0);
      }

      sRequestedDeviceCountArray.RemoveElementAt(0);
      sRemoteDevicesPack.Clear();
    }

    return NS_OK;
  }
};






static void
RemoteDevicePropertiesCallback(bt_status_t aStatus, bt_bdaddr_t *aBdAddress,
                               int aNumProperties, bt_property_t *aProperties)
{
  MOZ_ASSERT(!NS_IsMainThread());

  InfallibleTArray<BluetoothNamedValue> props;

  nsString remoteDeviceBdAddress;
  BdAddressTypeToString(aBdAddress, remoteDeviceBdAddress);
  BT_APPEND_NAMED_VALUE(props, "Address", remoteDeviceBdAddress);

  bool isCodInvalid = false;
  for (int i = 0; i < aNumProperties; ++i) {
    bt_property_t p = aProperties[i];

    if (p.type == BT_PROPERTY_BDNAME) {
      BluetoothValue propertyValue = NS_ConvertUTF8toUTF16((char*)p.val);
      BT_APPEND_NAMED_VALUE(props, "Name", propertyValue);
    } else if (p.type == BT_PROPERTY_CLASS_OF_DEVICE) {
      uint32_t cod = *(uint32_t*)p.val;
      nsString icon;
      ClassToIcon(cod, icon);
      if (!icon.IsEmpty()) {
        
        BT_APPEND_NAMED_VALUE(props, "Class", cod);
        BT_APPEND_NAMED_VALUE(props, "Icon", icon);
      } else {
        
        
        
        isCodInvalid = true;
      }
    } else if (p.type == BT_PROPERTY_UUIDS) {
      InfallibleTArray<nsString> uuidsArray;
      int uuidListLength = p.len / MAX_UUID_SIZE;
      uint32_t cod = 0;

      for (int i = 0; i < uuidListLength; i++) {
        uint16_t uuidServiceClass = UuidToServiceClassInt((bt_uuid_t*)(p.val +
          (i * MAX_UUID_SIZE)));
        BluetoothServiceClass serviceClass = BluetoothUuidHelper::
          GetBluetoothServiceClass(uuidServiceClass);

        
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
    } else {
      BT_LOGD("Other non-handled device properties. Type: %d", p.type);
    }
  }

  
  NS_DispatchToMainThread(
    new RemoteDevicePropertiesCallbackTask(props, remoteDeviceBdAddress));
}

static void
DeviceFoundCallback(int aNumProperties, bt_property_t *aProperties)
{
  MOZ_ASSERT(!NS_IsMainThread());

  BluetoothValue propertyValue;
  InfallibleTArray<BluetoothNamedValue> propertiesArray;

  for (int i = 0; i < aNumProperties; i++) {
    bt_property_t p = aProperties[i];

    if (p.type == BT_PROPERTY_BDADDR) {
      nsString remoteDeviceBdAddress;
      BdAddressTypeToString((bt_bdaddr_t*)p.val, remoteDeviceBdAddress);
      propertyValue = remoteDeviceBdAddress;

      BT_APPEND_NAMED_VALUE(propertiesArray, "Address", propertyValue);
    } else if (p.type == BT_PROPERTY_BDNAME) {
      propertyValue = NS_ConvertUTF8toUTF16((char*)p.val);
      BT_APPEND_NAMED_VALUE(propertiesArray, "Name", propertyValue);
    } else if (p.type == BT_PROPERTY_CLASS_OF_DEVICE) {
      uint32_t cod = *(uint32_t*)p.val;
      propertyValue = cod;
      BT_APPEND_NAMED_VALUE(propertiesArray, "Class", propertyValue);

      nsString icon;
      ClassToIcon(cod, icon);
      propertyValue = icon;
      BT_APPEND_NAMED_VALUE(propertiesArray, "Icon", propertyValue);
    } else {
      BT_LOGD("Not handled remote device property: %d", p.type);
    }
  }

  BluetoothValue value = propertiesArray;
  BluetoothSignal signal(NS_LITERAL_STRING("DeviceFound"),
                         NS_LITERAL_STRING(KEY_ADAPTER), value);
  nsRefPtr<DistributeBluetoothSignalTask>
    t = new DistributeBluetoothSignalTask(signal);
  if (NS_FAILED(NS_DispatchToMainThread(t))) {
    BT_WARNING("Failed to dispatch to main thread!");
  }
}

static void
DiscoveryStateChangedCallback(bt_discovery_state_t aState)
{
  MOZ_ASSERT(!NS_IsMainThread());

  bool isDiscovering = (aState == BT_DISCOVERY_STARTED);
  BluetoothSignal signal(NS_LITERAL_STRING(DISCOVERY_STATE_CHANGED_ID),
                         NS_LITERAL_STRING(KEY_ADAPTER), isDiscovering);

  nsRefPtr<DistributeBluetoothSignalTask> t =
    new DistributeBluetoothSignalTask(signal);
  if (NS_FAILED(NS_DispatchToMainThread(t))) {
    BT_WARNING("Failed to dispatch to main thread!");
  }
}

static void
PinRequestCallback(bt_bdaddr_t* aRemoteBdAddress,
                   bt_bdname_t* aRemoteBdName, uint32_t aRemoteClass)
{
  MOZ_ASSERT(!NS_IsMainThread());

  InfallibleTArray<BluetoothNamedValue> propertiesArray;
  nsAutoString remoteAddress;
  BdAddressTypeToString(aRemoteBdAddress, remoteAddress);

  BT_APPEND_NAMED_VALUE(propertiesArray, "address", remoteAddress);
  BT_APPEND_NAMED_VALUE(propertiesArray, "method",
                        NS_LITERAL_STRING("pincode"));
  BT_APPEND_NAMED_VALUE(propertiesArray, "name",
                        NS_ConvertUTF8toUTF16(
                          (const char*)aRemoteBdName->name));

  BluetoothValue value = propertiesArray;
  BluetoothSignal signal(NS_LITERAL_STRING("RequestPinCode"),
                         NS_LITERAL_STRING(KEY_LOCAL_AGENT), value);
  nsRefPtr<DistributeBluetoothSignalTask>
    t = new DistributeBluetoothSignalTask(signal);
  if (NS_FAILED(NS_DispatchToMainThread(t))) {
    BT_WARNING("Failed to dispatch to main thread!");
  }
}

static void
SspRequestCallback(bt_bdaddr_t* aRemoteBdAddress, bt_bdname_t* aRemoteBdName,
                   uint32_t aRemoteClass, bt_ssp_variant_t aPairingVariant,
                   uint32_t aPasskey)
{
  MOZ_ASSERT(!NS_IsMainThread());

  InfallibleTArray<BluetoothNamedValue> propertiesArray;
  nsAutoString remoteAddress;
  BdAddressTypeToString(aRemoteBdAddress, remoteAddress);

  BT_APPEND_NAMED_VALUE(propertiesArray, "address", remoteAddress);
  BT_APPEND_NAMED_VALUE(propertiesArray, "method",
                        NS_LITERAL_STRING("confirmation"));
  BT_APPEND_NAMED_VALUE(propertiesArray, "name",
                        NS_ConvertUTF8toUTF16(
                          (const char*)aRemoteBdName->name));
  BT_APPEND_NAMED_VALUE(propertiesArray, "passkey", aPasskey);

  BluetoothValue value = propertiesArray;
  BluetoothSignal signal(NS_LITERAL_STRING("RequestConfirmation"),
                         NS_LITERAL_STRING(KEY_LOCAL_AGENT), value);
  nsRefPtr<DistributeBluetoothSignalTask>
    t = new DistributeBluetoothSignalTask(signal);
  if (NS_FAILED(NS_DispatchToMainThread(t))) {
    BT_WARNING("Failed to dispatch to main thread!");
  }
}

class BondStateChangedCallbackTask : public nsRunnable
{
  nsString mRemoteDeviceBdAddress;
  bool mBonded;
public:
  BondStateChangedCallbackTask(const nsAString& aRemoteDeviceBdAddress,
                               bool aBonded)
  : mRemoteDeviceBdAddress(aRemoteDeviceBdAddress)
  , mBonded(aBonded)
  { }

  NS_IMETHOD
  Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mBonded && !sBondingRunnableArray.IsEmpty()) {
      DispatchBluetoothReply(sBondingRunnableArray[0],
                             BluetoothValue(true), EmptyString());

      sBondingRunnableArray.RemoveElementAt(0);
    } else if (!mBonded && !sUnbondingRunnableArray.IsEmpty()) {
      DispatchBluetoothReply(sUnbondingRunnableArray[0],
                             BluetoothValue(true), EmptyString());

      sUnbondingRunnableArray.RemoveElementAt(0);
    }

    
    InfallibleTArray<BluetoothNamedValue> propertiesArray;
    BT_APPEND_NAMED_VALUE(propertiesArray, "address", mRemoteDeviceBdAddress);
    BT_APPEND_NAMED_VALUE(propertiesArray, "status", mBonded);

    BluetoothSignal signal(NS_LITERAL_STRING(PAIRED_STATUS_CHANGED_ID),
                           NS_LITERAL_STRING(KEY_ADAPTER),
                           BluetoothValue(propertiesArray));
    NS_DispatchToMainThread(new DistributeBluetoothSignalTask(signal));

    return NS_OK;
  }
};

static void
BondStateChangedCallback(bt_status_t aStatus, bt_bdaddr_t* aRemoteBdAddress,
                         bt_bond_state_t aState)
{
  MOZ_ASSERT(!NS_IsMainThread());

  if (aState == BT_BOND_STATE_BONDING) {
    
    return;
  }

  nsAutoString remoteBdAddress;
  BdAddressTypeToString(aRemoteBdAddress, remoteBdAddress);

  if (aState == BT_BOND_STATE_BONDED &&
      sAdapterBondedAddressArray.Contains(remoteBdAddress)) {
    
    return;
  }

  bool bonded;
  if (aState == BT_BOND_STATE_NONE) {
    bonded = false;
    sAdapterBondedAddressArray.RemoveElement(remoteBdAddress);
  } else if (aState == BT_BOND_STATE_BONDED) {
    bonded = true;
    sAdapterBondedAddressArray.AppendElement(remoteBdAddress);
  }

  
  InfallibleTArray<BluetoothNamedValue> propertiesChangeArray;
  BT_APPEND_NAMED_VALUE(propertiesChangeArray, "Devices",
                        sAdapterBondedAddressArray);

  BluetoothValue value(propertiesChangeArray);
  BluetoothSignal signal(NS_LITERAL_STRING("PropertyChanged"),
                         NS_LITERAL_STRING(KEY_ADAPTER),
                         BluetoothValue(propertiesChangeArray));
  NS_DispatchToMainThread(new DistributeBluetoothSignalTask(signal));

  
  NS_DispatchToMainThread(
    new BondStateChangedCallbackTask(remoteBdAddress, bonded));
}

static void
AclStateChangedCallback(bt_status_t aStatus, bt_bdaddr_t* aRemoteBdAddress,
                        bt_acl_state_t aState)
{
  
}

static void
CallbackThreadEvent(bt_cb_thread_evt evt)
{
  
}

bt_callbacks_t sBluetoothCallbacks =
{
  sizeof(sBluetoothCallbacks),
  AdapterStateChangeCallback,
  AdapterPropertiesCallback,
  RemoteDevicePropertiesCallback,
  DeviceFoundCallback,
  DiscoveryStateChangedCallback,
  PinRequestCallback,
  SspRequestCallback,
  BondStateChangedCallback,
  AclStateChangedCallback,
  CallbackThreadEvent
};




static bool
EnsureBluetoothHalLoad()
{
  sBtInterface = BluetoothInterface::GetInstance();
  NS_ENSURE_TRUE(sBtInterface, false);

  return true;
}

class EnableResultHandler MOZ_FINAL : public BluetoothResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    BT_LOGR("BluetoothInterface::Enable failed: %d", aStatus);

    nsRefPtr<nsRunnable> runnable = new BluetoothService::ToggleBtAck(false);
    if (NS_FAILED(NS_DispatchToMainThread(runnable))) {
      BT_WARNING("Failed to dispatch to main thread!");
    }
  }
};





class ProfileInitResultHandler MOZ_FINAL
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

class InitResultHandler MOZ_FINAL : public BluetoothResultHandler
{
public:
  void Init() MOZ_OVERRIDE
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

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    BT_LOGR("BluetoothInterface::Init failed: %d", aStatus);

    sBtInterface = nullptr;

    nsRefPtr<nsRunnable> runnable = new BluetoothService::ToggleBtAck(false);
    if (NS_FAILED(NS_DispatchToMainThread(runnable))) {
      BT_WARNING("Failed to dispatch to main thread!");
    }
  }
};

static nsresult
StartGonkBluetooth()
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE(sBtInterface, NS_ERROR_FAILURE);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE(bs, NS_ERROR_FAILURE);

  if (bs->IsEnabled()) {
    
    nsRefPtr<nsRunnable> runnable = new BluetoothService::ToggleBtAck(true);
    if (NS_FAILED(NS_DispatchToMainThread(runnable))) {
      BT_WARNING("Failed to dispatch to main thread!");
    }
    return NS_OK;
  }

  sBtInterface->Init(&sBluetoothCallbacks, new InitResultHandler());

  return NS_OK;
}

class DisableResultHandler MOZ_FINAL : public BluetoothResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    BT_LOGR("BluetoothInterface::Disable failed: %d", aStatus);

    nsRefPtr<nsRunnable> runnable = new BluetoothService::ToggleBtAck(true);
    if (NS_FAILED(NS_DispatchToMainThread(runnable))) {
      BT_WARNING("Failed to dispatch to main thread!");
    }
  }
};

static nsresult
StopGonkBluetooth()
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE(sBtInterface, NS_ERROR_FAILURE);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE(bs, NS_ERROR_FAILURE);

  if (!bs->IsEnabled()) {
    
    nsRefPtr<nsRunnable> runnable = new BluetoothService::ToggleBtAck(false);
    if (NS_FAILED(NS_DispatchToMainThread(runnable))) {
      BT_WARNING("Failed to dispatch to main thread!");
    }
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
    nsRefPtr<nsRunnable> runnable =
      new BluetoothService::ToggleBtAck(false);
    if (NS_FAILED(NS_DispatchToMainThread(runnable))) {
      BT_WARNING("Failed to dispatch to main thread!");
    }
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
    nsRefPtr<nsRunnable> runnable =
      new BluetoothService::ToggleBtAck(true);
    if (NS_FAILED(NS_DispatchToMainThread(runnable))) {
      BT_WARNING("Failed to dispatch to main thread!");
    }
    BT_LOGR("Error");
  }

  return ret;
}

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

class GetRemoteDevicePropertiesResultHandler MOZ_FINAL
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
    InfallibleTArray<BluetoothNamedValue> emptyArr;
    DispatchBluetoothReply(aRunnable, BluetoothValue(emptyArr), EmptyString());
    return NS_OK;
  }

  sRequestedDeviceCountArray.AppendElement(requestedDeviceCount);
  sGetDeviceRunnableArray.AppendElement(aRunnable);

  for (int i = 0; i < requestedDeviceCount; i++) {
    
    sBtInterface->GetRemoteDeviceProperties(aDeviceAddress[i],
      new GetRemoteDevicePropertiesResultHandler(aDeviceAddress[i]));
  }

  return NS_OK;
}

class StartDiscoveryResultHandler MOZ_FINAL : public BluetoothResultHandler
{
public:
  StartDiscoveryResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void StartDiscovery() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    DispatchBluetoothReply(mRunnable, true, EmptyString());
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    ReplyStatusError(mRunnable, aStatus, NS_LITERAL_STRING("StartDiscovery"));
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
  sBtInterface->StartDiscovery(new StartDiscoveryResultHandler(aRunnable));

  return NS_OK;
}

class CancelDiscoveryResultHandler MOZ_FINAL : public BluetoothResultHandler
{
public:
  CancelDiscoveryResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void CancelDiscovery() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    DispatchBluetoothReply(mRunnable, true, EmptyString());
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    ReplyStatusError(mRunnable, aStatus, NS_LITERAL_STRING("StopDiscovery"));
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
  sBtInterface->CancelDiscovery(new CancelDiscoveryResultHandler(aRunnable));

  return NS_OK;
}

class SetAdapterPropertyResultHandler MOZ_FINAL : public BluetoothResultHandler
{
public:
  SetAdapterPropertyResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    ReplyStatusError(mRunnable, aStatus, NS_LITERAL_STRING("SetProperty"));
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

class CreateBondResultHandler MOZ_FINAL : public BluetoothResultHandler
{
public:
  CreateBondResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    sBondingRunnableArray.RemoveElement(mRunnable);
    ReplyStatusError(mRunnable, aStatus, NS_LITERAL_STRING("CreatedPairedDevice"));
  }

private:
  BluetoothReplyRunnable* mRunnable;
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

class RemoveBondResultHandler MOZ_FINAL : public BluetoothResultHandler
{
public:
  RemoveBondResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    sUnbondingRunnableArray.RemoveElement(mRunnable);
    ReplyStatusError(mRunnable, aStatus, NS_LITERAL_STRING("RemoveDevice"));
  }

private:
  BluetoothReplyRunnable* mRunnable;
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

class PinReplyResultHandler MOZ_FINAL : public BluetoothResultHandler
{
public:
  PinReplyResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void PinReply() MOZ_OVERRIDE
  {
    DispatchBluetoothReply(mRunnable, BluetoothValue(true), EmptyString());
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
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

bool
BluetoothServiceBluedroid::SetPasskeyInternal(
  const nsAString& aDeviceAddress, uint32_t aPasskey,
  BluetoothReplyRunnable* aRunnable)
{
  return true;
}

class SspReplyResultHandler MOZ_FINAL : public BluetoothResultHandler
{
public:
  SspReplyResultHandler(BluetoothReplyRunnable* aRunnable)
  : mRunnable(aRunnable)
  { }

  void SspReply() MOZ_OVERRIDE
  {
    DispatchBluetoothReply(mRunnable, BluetoothValue(true), EmptyString());
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
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
                         NS_ConvertUTF8toUTF16("PasskeyConfirmation"),
                         aConfirm, 0, new SspReplyResultHandler(aRunnable));
  return true;
}

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

static void
NextBluetoothProfileController()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  NS_ENSURE_FALSE_VOID(sControllerArray.IsEmpty());
  sControllerArray.RemoveElementAt(0);
  
  if (!sControllerArray.IsEmpty()) {
    sControllerArray[0]->StartSession();
  }
}

static void
ConnectDisconnect(bool aConnect, const nsAString& aDeviceAddress,
                  BluetoothReplyRunnable* aRunnable,
                  uint16_t aServiceUuid, uint32_t aCod = 0)
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
  nsAutoString errorStr;
  if (!opp || !opp->SendFile(aDeviceAddress, aBlobParent)) {
    errorStr.AssignLiteral("Calling SendFile() failed");
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), errorStr);
}

void
BluetoothServiceBluedroid::SendFile(const nsAString& aDeviceAddress,
                                    nsIDOMBlob* aBlob,
                                    BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  nsAutoString errorStr;
  if (!opp || !opp->SendFile(aDeviceAddress, aBlob)) {
    errorStr.AssignLiteral("Calling SendFile() failed");
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), errorStr);
}

void
BluetoothServiceBluedroid::StopSendingFile(const nsAString& aDeviceAddress,
                                           BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  nsAutoString errorStr;
  if (!opp || !opp->StopSendingFile()) {
    errorStr.AssignLiteral("Calling StopSendingFile() failed");
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), errorStr);
}

void
BluetoothServiceBluedroid::ConfirmReceivingFile(
  const nsAString& aDeviceAddress, bool aConfirm,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be called from main thread!");

  
  
  
  
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  nsAutoString errorStr;
  if (!opp || !opp->ConfirmReceivingFile(aConfirm)) {
    errorStr.AssignLiteral("Calling ConfirmReceivingFile() failed");
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), errorStr);
}

void
BluetoothServiceBluedroid::ConnectSco(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  if (!hfp || !hfp->ConnectSco()) {
    NS_NAMED_LITERAL_STRING(replyError, "Calling ConnectSco() failed");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), replyError);
    return;
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), EmptyString());
}

void
BluetoothServiceBluedroid::DisconnectSco(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  if (!hfp || !hfp->DisconnectSco()) {
    NS_NAMED_LITERAL_STRING(replyError, "Calling DisconnectSco() failed");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), replyError);
    return;
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), EmptyString());
}

void
BluetoothServiceBluedroid::IsScoConnected(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  if (!hfp) {
    NS_NAMED_LITERAL_STRING(replyError, "Fail to get BluetoothHfpManager");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), replyError);
    return;
  }

  DispatchBluetoothReply(aRunnable, hfp->IsScoConnected(), EmptyString());
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
  DispatchBluetoothReply(aRunnable, BluetoothValue(true), EmptyString());
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
  DispatchBluetoothReply(aRunnable, BluetoothValue(true), EmptyString());
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

