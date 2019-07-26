

















#include "BluetoothServiceBluedroid.h"

#include <hardware/bluetooth.h>
#include <hardware/hardware.h>

#include "BluetoothReplyRunnable.h"
#include "BluetoothUtils.h"
#include "BluetoothUuid.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/ipc/UnixSocket.h"

using namespace mozilla;
using namespace mozilla::ipc;
USING_BLUETOOTH_NAMESPACE

typedef char bdstr_t[18];




class DistributeBluetoothSignalTask : public nsRunnable {
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
    bs->DistributeSignal(mSignal);

    return NS_OK;
  }

private:
  BluetoothSignal mSignal;
};




static bluetooth_device_t* sBtDevice;
static const bt_interface_t* sBtInterface;
static bool sAdapterDiscoverable = false;
static bool sIsBtEnabled = false;
static nsString sAdapterBdAddress;
static nsString sAdapterBdName;
static uint32_t sAdapterDiscoverableTimeout;
static InfallibleTArray<nsString> sAdapterBondedAddressArray;
static InfallibleTArray<BluetoothNamedValue> sRemoteDevicesPack;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sBondingRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sChangeDiscoveryRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sGetPairedDeviceRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sSetPropertyRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sUnbondingRunnableArray;
static nsTArray<int> sRequestedDeviceCountArray;




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
}

static void
AdapterStateChangeCallback(bt_state_t aStatus)
{
  MOZ_ASSERT(!NS_IsMainThread());

  BT_LOGD("%s, BT_STATE:%d", __FUNCTION__, aStatus);
  nsAutoString signalName;
  if (aStatus == BT_STATE_ON) {
    sIsBtEnabled = true;
    signalName = NS_LITERAL_STRING("AdapterAdded");
  } else {
    sIsBtEnabled = false;
    signalName = NS_LITERAL_STRING("Disabled");
  }

  BluetoothSignal signal(signalName, NS_LITERAL_STRING(KEY_MANAGER),
                         BluetoothValue(true));
  nsRefPtr<DistributeBluetoothSignalTask>
    t = new DistributeBluetoothSignalTask(signal);
  if (NS_FAILED(NS_DispatchToMainThread(t))) {
    NS_WARNING("Failed to dispatch to main thread!");
  }
}

static void
BdAddressTypeToString(bt_bdaddr_t* aBdAddressType, nsAString& aRetBdAddress)
{
  uint8_t* addr = aBdAddressType->address;
  bdstr_t bdstr;

  sprintf((char*)bdstr, "%02x:%02x:%02x:%02x:%02x:%02x",
          (int)addr[0],(int)addr[1],(int)addr[2],
          (int)addr[3],(int)addr[4],(int)addr[5]);

  aRetBdAddress = NS_ConvertUTF8toUTF16((char*)bdstr);
}

static bool
IsReady()
{
  if (!sBtInterface || !sIsBtEnabled) {
    BT_LOGR("Warning! Bluetooth Service is not ready");
    return false;
  }
  return true;
}

static void
StringToBdAddressType(const nsAString& aBdAddress,
                      bt_bdaddr_t *aRetBdAddressType)
{
  const char* str = NS_ConvertUTF16toUTF8(aBdAddress).get();

  for (int i = 0; i < 6; i++) {
    aRetBdAddressType->address[i] = (uint8_t) strtoul(str, (char **)&str, 16);
    str++;
  }
}







static void
AdapterPropertiesChangeCallback(bt_status_t aStatus, int aNumProperties,
                                bt_property_t *aProperties)
{
  MOZ_ASSERT(!NS_IsMainThread());

  BluetoothValue propertyValue;
  InfallibleTArray<BluetoothNamedValue> propertiesArray;

  for (int i = 0; i < aNumProperties; i++) {
    bt_property_t p = aProperties[i];

    if (p.type == BT_PROPERTY_BDADDR) {
      BdAddressTypeToString((bt_bdaddr_t*)p.val, sAdapterBdAddress);
      propertyValue = sAdapterBdAddress;
      propertiesArray.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("Address"), propertyValue));
    } else if (p.type == BT_PROPERTY_BDNAME) {
      
      
      propertyValue = sAdapterBdName = NS_ConvertUTF8toUTF16(
        nsCString((char*)p.val, p.len));
      propertiesArray.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("Name"), propertyValue));
    } else if (p.type == BT_PROPERTY_ADAPTER_SCAN_MODE) {
      propertyValue = sAdapterDiscoverable = *(uint32_t*)p.val;
      propertiesArray.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("Discoverable"), propertyValue));
    } else if (p.type == BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT) {
      propertyValue = sAdapterDiscoverableTimeout = *(uint32_t*)p.val;
      propertiesArray.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("DiscoverableTimeout"),
                            propertyValue));
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
      propertiesArray.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("Devices"), propertyValue));
    } else if (p.type == BT_PROPERTY_UUIDS) {
      
      return;
    } else {
      BT_LOGD("Unhandled adapter property type: %d", p.type);
      return;
    }

    BluetoothValue value(propertiesArray);
    BluetoothSignal signal(NS_LITERAL_STRING("PropertyChanged"),
                           NS_LITERAL_STRING(KEY_ADAPTER), value);
    nsRefPtr<DistributeBluetoothSignalTask>
      t = new DistributeBluetoothSignalTask(signal);
    if (NS_FAILED(NS_DispatchToMainThread(t))) {
      NS_WARNING("Failed to dispatch to main thread!");
    }

    
    if (!sSetPropertyRunnableArray.IsEmpty()) {
      DispatchBluetoothReply(sSetPropertyRunnableArray[0], BluetoothValue(true),
                             EmptyString());
      sSetPropertyRunnableArray.RemoveElementAt(0);
    }
  }
}







static void
RemoteDevicePropertiesChangeCallback(bt_status_t aStatus,
                                     bt_bdaddr_t *aBdAddress,
                                     int aNumProperties,
                                     bt_property_t *aProperties)
{
  MOZ_ASSERT(!NS_IsMainThread());

  if (sRequestedDeviceCountArray.IsEmpty()) {
    MOZ_ASSERT(sGetPairedDeviceRunnableArray.IsEmpty());
    return;
  }

  sRequestedDeviceCountArray[0]--;

  InfallibleTArray<BluetoothNamedValue> props;

  nsString remoteDeviceBdAddress;
  BdAddressTypeToString(aBdAddress, remoteDeviceBdAddress);
  props.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("Address"), remoteDeviceBdAddress));

  for (int i = 0; i < aNumProperties; ++i) {
    bt_property_t p = aProperties[i];

    if (p.type == BT_PROPERTY_BDNAME) {
      BluetoothValue propertyValue = NS_ConvertUTF8toUTF16((char*)p.val);
      props.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("Name"), propertyValue));
    } else if (p.type == BT_PROPERTY_CLASS_OF_DEVICE) {
      uint32_t cod = *(uint32_t*)p.val;
      props.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("Class"), BluetoothValue(cod)));

      nsString icon;
      ClassToIcon(cod, icon);
      props.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("Icon"), BluetoothValue(icon)));
    } else {
      BT_LOGD("Other non-handled device properties. Type: %d", p.type);
    }
  }

  
  sRemoteDevicesPack.AppendElement(
    BluetoothNamedValue(remoteDeviceBdAddress, props));

  if (sRequestedDeviceCountArray[0] == 0) {
    MOZ_ASSERT(!sGetPairedDeviceRunnableArray.IsEmpty());

    if (sGetPairedDeviceRunnableArray.IsEmpty()) {
      BT_LOGR("No runnable to return");
      return;
    }

    DispatchBluetoothReply(sGetPairedDeviceRunnableArray[0],
                           sRemoteDevicesPack, EmptyString());

    
    sRemoteDevicesPack.Clear();

    sRequestedDeviceCountArray.RemoveElementAt(0);
    sGetPairedDeviceRunnableArray.RemoveElementAt(0);
  }
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
      propertiesArray.AppendElement(
          BluetoothNamedValue(NS_LITERAL_STRING("Address"), propertyValue));
    } else if (p.type == BT_PROPERTY_BDNAME) {
      propertyValue = NS_ConvertUTF8toUTF16((char*)p.val);
      propertiesArray.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("Name"), propertyValue));
    } else if (p.type == BT_PROPERTY_CLASS_OF_DEVICE) {
      uint32_t cod = *(uint32_t*)p.val;
      propertyValue = cod;
      propertiesArray.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("Class"), propertyValue));
      nsString icon;
      ClassToIcon(cod, icon);
      propertyValue = icon;
      propertiesArray.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("Icon"), propertyValue));
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
    NS_WARNING("Failed to dispatch to main thread!");
  }
}

static void
DiscoveryStateChangedCallback(bt_discovery_state_t aState)
{
  MOZ_ASSERT(!NS_IsMainThread());

  if (!sChangeDiscoveryRunnableArray.IsEmpty()) {
    BluetoothValue values(true);
    DispatchBluetoothReply(sChangeDiscoveryRunnableArray[0],
                           values, EmptyString());

    sChangeDiscoveryRunnableArray.RemoveElementAt(0);
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

  propertiesArray.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("address"), remoteAddress));
  propertiesArray.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("method"),
                        NS_LITERAL_STRING("pincode")));
  propertiesArray.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("name"),
                        NS_ConvertUTF8toUTF16(
                          (const char*)aRemoteBdName->name)));

  BluetoothValue value = propertiesArray;
  BluetoothSignal signal(NS_LITERAL_STRING("RequestPinCode"),
                         NS_LITERAL_STRING(KEY_LOCAL_AGENT), value);
  nsRefPtr<DistributeBluetoothSignalTask>
    t = new DistributeBluetoothSignalTask(signal);
  if (NS_FAILED(NS_DispatchToMainThread(t))) {
    NS_WARNING("Failed to dispatch to main thread!");
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

  propertiesArray.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("address"), remoteAddress));
  propertiesArray.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("method"),
                        NS_LITERAL_STRING("confirmation")));
  propertiesArray.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("name"),
                        NS_ConvertUTF8toUTF16(
                          (const char*)aRemoteBdName->name)));
  propertiesArray.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("passkey"), aPasskey));

  BluetoothValue value = propertiesArray;
  BluetoothSignal signal(NS_LITERAL_STRING("RequestConfirmation"),
                         NS_LITERAL_STRING(KEY_LOCAL_AGENT), value);
  nsRefPtr<DistributeBluetoothSignalTask>
    t = new DistributeBluetoothSignalTask(signal);
  if (NS_FAILED(NS_DispatchToMainThread(t))) {
    NS_WARNING("Failed to dispatch to main thread!");
  }
}

static void
BondStateChangedCallback(bt_status_t aStatus, bt_bdaddr_t* aRemoteBdAddress,
                         bt_bond_state_t aState)
{
  MOZ_ASSERT(!NS_IsMainThread());

  nsAutoString remoteAddress;
  BdAddressTypeToString(aRemoteBdAddress, remoteAddress);
  bool bonded;

  if (aState == BT_BOND_STATE_BONDING) {
    
    return;
  } else if (aState == BT_BOND_STATE_NONE) {
    bonded = false;
    sAdapterBondedAddressArray.RemoveElement(remoteAddress);
  } else if (aState == BT_BOND_STATE_BONDED) {
    bonded = true;
    sAdapterBondedAddressArray.AppendElement(remoteAddress);
  }

  
  InfallibleTArray<BluetoothNamedValue> propertiesChangeArray;
  propertiesChangeArray.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("Devices"),
                        sAdapterBondedAddressArray));
  BluetoothValue value(propertiesChangeArray);
  BluetoothSignal signal(NS_LITERAL_STRING("PropertyChanged"),
                         NS_LITERAL_STRING(KEY_ADAPTER),
                         BluetoothValue(propertiesChangeArray));
  NS_DispatchToMainThread(new DistributeBluetoothSignalTask(signal));

  
  InfallibleTArray<BluetoothNamedValue> propertiesArray;
  propertiesArray.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("address"), remoteAddress));
  propertiesArray.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("status"), bonded));
  BluetoothSignal newSignal(NS_LITERAL_STRING(PAIRED_STATUS_CHANGED_ID),
                            NS_LITERAL_STRING(KEY_ADAPTER),
                            BluetoothValue(propertiesArray));
  NS_DispatchToMainThread(new DistributeBluetoothSignalTask(newSignal));

  if (bonded && !sBondingRunnableArray.IsEmpty()) {
    DispatchBluetoothReply(sBondingRunnableArray[0],
                           BluetoothValue(true), EmptyString());

    sBondingRunnableArray.RemoveElementAt(0);
  } else if (!bonded && !sUnbondingRunnableArray.IsEmpty()) {
    DispatchBluetoothReply(sUnbondingRunnableArray[0],
                           BluetoothValue(true), EmptyString());

    sUnbondingRunnableArray.RemoveElementAt(0);
  }
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
  AdapterPropertiesChangeCallback,
  RemoteDevicePropertiesChangeCallback,
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
  hw_module_t* module;
  hw_device_t* device;
  int err = hw_get_module(BT_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
  if (err != 0) {
    BT_LOGR("Error: %s ", strerror(err));
    return false;
  }
  module->methods->open(module, BT_HARDWARE_MODULE_ID, &device);
  sBtDevice = (bluetooth_device_t *)device;
  sBtInterface = sBtDevice->get_bluetooth_interface();
  BT_LOGD("Bluetooth HAL loaded");

  return true;
}

static nsresult
StartStopGonkBluetooth(bool aShouldEnable)
{
  MOZ_ASSERT(!NS_IsMainThread());

  static bool sIsBtInterfaceInitialized = false;

  if (!EnsureBluetoothHalLoad()) {
    BT_LOGR("Failed to load bluedroid library.\n");
    return NS_ERROR_FAILURE;
  }

  if (sIsBtEnabled == aShouldEnable)
    return NS_OK;

  if (sBtInterface && !sIsBtInterfaceInitialized) {
    int ret = sBtInterface->init(&sBluetoothCallbacks);
    if (ret != BT_STATUS_SUCCESS) {
      BT_LOGR("Error while setting the callbacks %s", __FUNCTION__);
      sBtInterface = nullptr;
      return NS_ERROR_FAILURE;
    }
    sIsBtInterfaceInitialized = true;
  }
  int ret = aShouldEnable ? sBtInterface->enable() : sBtInterface->disable();

  return (ret == BT_STATUS_SUCCESS) ? NS_OK : NS_ERROR_FAILURE;
}

static void
ReplyStatusError(BluetoothReplyRunnable* aBluetoothReplyRunnable,
                 int aStatusCode, const nsAString& aCustomMsg)
{
  MOZ_ASSERT(aBluetoothReplyRunnable, "Reply runnable is nullptr");
  nsAutoString replyError;

  replyError.Assign(aCustomMsg);
  if (aStatusCode == BT_STATUS_BUSY) {
    replyError.AppendLiteral(":BT_STATUS_BUSY");
  } else if (aStatusCode == BT_STATUS_NOT_READY) {
    replyError.AppendLiteral(":BT_STATUS_NOT_READY");
  } else if (aStatusCode == BT_STATUS_DONE) {
    replyError.AppendLiteral(":BT_STATUS_DONE");
  } else if (aStatusCode == BT_STATUS_AUTH_FAILURE) {
    replyError.AppendLiteral(":BT_STATUS_AUTH_FAILURE");
  } else if (aStatusCode == BT_STATUS_RMT_DEV_DOWN) {
    replyError.AppendLiteral(":BT_STATUS_RMT_DEV_DOWN");
  } else if (aStatusCode == BT_STATUS_FAIL) {
    replyError.AppendLiteral(":BT_STATUS_FAIL");
  }

  DispatchBluetoothReply(aBluetoothReplyRunnable, BluetoothValue(true),
                         replyError);
}




nsresult
BluetoothServiceBluedroid::StartInternal()
{
  MOZ_ASSERT(!NS_IsMainThread());

  nsresult ret = StartStopGonkBluetooth(true);
  if (NS_FAILED(ret)) {
    BT_LOGR("Error: %s", __FUNCTION__);
  }

  return ret;
}

nsresult
BluetoothServiceBluedroid::StopInternal()
{
  MOZ_ASSERT(!NS_IsMainThread());

  nsresult ret = StartStopGonkBluetooth(false);
  if (NS_FAILED(ret)) {
    BT_LOGR("Error: %s", __FUNCTION__);
  }

  return ret;
}

bool
BluetoothServiceBluedroid::IsEnabledInternal()
{
  MOZ_ASSERT(!NS_IsMainThread());

  if (!EnsureBluetoothHalLoad()) {
    NS_ERROR("Failed to load bluedroid library.\n");
    return false;
  }

  return sIsBtEnabled;
}

nsresult
BluetoothServiceBluedroid::GetDefaultAdapterPathInternal(
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<BluetoothReplyRunnable> runnable(aRunnable);

  BluetoothValue v = InfallibleTArray<BluetoothNamedValue>();
  v.get_ArrayOfBluetoothNamedValue().AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("Name"), sAdapterBdName));

  v.get_ArrayOfBluetoothNamedValue().AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("Devices"),
                        sAdapterBondedAddressArray));

  nsAutoString replyError;
  DispatchBluetoothReply(runnable.get(), v, replyError);

  runnable.forget();

  return NS_OK;
}

nsresult
BluetoothServiceBluedroid::GetConnectedDevicePropertiesInternal(
  uint16_t aProfileId, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  DispatchBluetoothReply(aRunnable, BluetoothValue(true), EmptyString());

  return NS_OK;
}

nsresult
BluetoothServiceBluedroid::GetPairedDevicePropertiesInternal(
  const nsTArray<nsString>& aDeviceAddress, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return NS_OK;
  }

  int requestedDeviceCount = aDeviceAddress.Length();
  if (requestedDeviceCount == 0) {
    InfallibleTArray<BluetoothNamedValue> emptyArr;
    DispatchBluetoothReply(aRunnable, BluetoothValue(emptyArr), EmptyString());
    return NS_OK;
  }

  for (int i = 0; i < requestedDeviceCount; i++) {
    
    bt_bdaddr_t addressType;
    StringToBdAddressType(aDeviceAddress[i], &addressType);
    int ret = sBtInterface->get_remote_device_properties(&addressType);
    if (ret != BT_STATUS_SUCCESS) {
      DispatchBluetoothReply(aRunnable, BluetoothValue(true),
                             NS_LITERAL_STRING("GetPairedDeviceFailed"));
      return NS_OK;
    }
  }

  sRequestedDeviceCountArray.AppendElement(requestedDeviceCount);
  sGetPairedDeviceRunnableArray.AppendElement(aRunnable);

  return NS_OK;
}

nsresult
BluetoothServiceBluedroid::StartDiscoveryInternal(
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);

    return NS_OK;
  }
  int ret = sBtInterface->start_discovery();
  if (ret != BT_STATUS_SUCCESS) {
    ReplyStatusError(aRunnable, ret, NS_LITERAL_STRING("StartDiscovery"));

    return NS_OK;
  }

  sChangeDiscoveryRunnableArray.AppendElement(aRunnable);
  return NS_OK;
}

nsresult
BluetoothServiceBluedroid::StopDiscoveryInternal(
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return NS_OK;
  }
  int ret = sBtInterface->cancel_discovery();
  if (ret != BT_STATUS_SUCCESS) {
    ReplyStatusError(aRunnable, ret, NS_LITERAL_STRING("StopDiscovery"));
    return NS_OK;
  }

  sChangeDiscoveryRunnableArray.AppendElement(aRunnable);
  return NS_OK;
}

nsresult
BluetoothServiceBluedroid::GetDevicePropertiesInternal(
  const BluetoothSignal& aSignal)
{
  return NS_OK;
}

nsresult
BluetoothServiceBluedroid::SetProperty(BluetoothObjectType aType,
                                       const BluetoothNamedValue& aValue,
                                       BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);

    return NS_OK;
  }

  const nsString propName = aValue.name();
  bt_property_t prop;
  nsString str;

  
  if (propName.EqualsLiteral("Name")) {
    prop.type = BT_PROPERTY_BDNAME;
  } else if (propName.EqualsLiteral("Discoverable")) {
    prop.type = BT_PROPERTY_ADAPTER_SCAN_MODE;
  } else if (propName.EqualsLiteral("DiscoverableTimeout")) {
    prop.type = BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT;
  } else {
    BT_LOGR("Warning: Property type is not supported yet, type: %d", prop.type);
  }

  if (aValue.value().type() == BluetoothValue::Tuint32_t) {
    
    prop.val = (void*)aValue.value().get_uint32_t();
  } else if (aValue.value().type() == BluetoothValue::TnsString) {
    
    str = aValue.value().get_nsString();
    const char* name = NS_ConvertUTF16toUTF8(str).get();
    prop.val = (void*)name;
    prop.len = strlen(name);
  } else if (aValue.value().type() == BluetoothValue::Tbool) {
    bt_scan_mode_t mode = aValue.value().get_bool() ?
                            BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE :
                            BT_SCAN_MODE_CONNECTABLE;
    bt_scan_mode_t* sss = &mode;
    prop.val = (void*)sss;
    prop.len = sizeof(sss);
  } else {
    BT_LOGR("SetProperty but the property cannot be recognized correctly.");
    return NS_OK;
  }

  sSetPropertyRunnableArray.AppendElement(aRunnable);
  int ret = sBtInterface->set_adapter_property(&prop);

  if (ret != BT_STATUS_SUCCESS)
    ReplyStatusError(aRunnable, ret, NS_LITERAL_STRING("SetProperty"));

  return NS_OK;
}

bool
BluetoothServiceBluedroid::GetDevicePath(const nsAString& aAdapterPath,
                                         const nsAString& aDeviceAddress,
                                         nsAString& aDevicePath)
{
  return true;
}

bool
BluetoothServiceBluedroid::AddServiceRecords(const char* serviceName,
                                             unsigned long long uuidMsb,
                                             unsigned long long uuidLsb,
                                             int channel)
{
  return true;
}

bool
BluetoothServiceBluedroid::RemoveServiceRecords(const char* serviceName,
                                                unsigned long long uuidMsb,
                                                unsigned long long uuidLsb,
                                                int channel)
{
  return true;
}

bool
BluetoothServiceBluedroid::AddReservedServicesInternal(
  const nsTArray<uint32_t>& aServices,
  nsTArray<uint32_t>& aServiceHandlesContainer)
{
  return true;

}

bool
BluetoothServiceBluedroid::RemoveReservedServicesInternal(
  const nsTArray<uint32_t>& aServiceHandles)
{
  return true;
}

nsresult
BluetoothServiceBluedroid::GetScoSocket(
  const nsAString& aObjectPath, bool aAuth, bool aEncrypt,
  mozilla::ipc::UnixSocketConsumer* aConsumer)
{
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

nsresult
BluetoothServiceBluedroid::CreatePairedDeviceInternal(
  const nsAString& aDeviceAddress, int aTimeout,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return NS_OK;
  }

  bt_bdaddr_t remoteAddress;
  StringToBdAddressType(aDeviceAddress, &remoteAddress);

  int ret = sBtInterface->create_bond(&remoteAddress);
  if (ret != BT_STATUS_SUCCESS) {
    ReplyStatusError(aRunnable, ret, NS_LITERAL_STRING("CreatedPairedDevice"));
  } else {
    sBondingRunnableArray.AppendElement(aRunnable);
  }

  return NS_OK;
}

nsresult
BluetoothServiceBluedroid::RemoveDeviceInternal(
  const nsAString& aDeviceAddress, BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return NS_OK;
  }

  bt_bdaddr_t remoteAddress;
  StringToBdAddressType(aDeviceAddress, &remoteAddress);

  int ret = sBtInterface->remove_bond(&remoteAddress);
  if (ret != BT_STATUS_SUCCESS) {
    ReplyStatusError(aRunnable, ret,
                     NS_LITERAL_STRING("RemoveDevice"));
  } else {
    sUnbondingRunnableArray.AppendElement(aRunnable);
  }

  return NS_OK;
}

bool
BluetoothServiceBluedroid::SetPinCodeInternal(
  const nsAString& aDeviceAddress, const nsAString& aPinCode,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return false;
  }

  bt_bdaddr_t remoteAddress;
  StringToBdAddressType(aDeviceAddress, &remoteAddress);

  int ret = sBtInterface->pin_reply(
      &remoteAddress, true, aPinCode.Length(),
      (bt_pin_code_t*)NS_ConvertUTF16toUTF8(aPinCode).get());

  if (ret != BT_STATUS_SUCCESS) {
    ReplyStatusError(aRunnable, ret, NS_LITERAL_STRING("SetPinCode"));
  } else {
    DispatchBluetoothReply(aRunnable, BluetoothValue(true), EmptyString());
  }

  return true;
}

bool
BluetoothServiceBluedroid::SetPasskeyInternal(
  const nsAString& aDeviceAddress, uint32_t aPasskey,
  BluetoothReplyRunnable* aRunnable)
{
  return true;
}

bool
BluetoothServiceBluedroid::SetPairingConfirmationInternal(
  const nsAString& aDeviceAddress, bool aConfirm,
  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return false;
  }

  bt_bdaddr_t remoteAddress;
  StringToBdAddressType(aDeviceAddress, &remoteAddress);

  int ret = sBtInterface->ssp_reply(&remoteAddress, (bt_ssp_variant_t)0,
                                    aConfirm, 0);
  if (ret != BT_STATUS_SUCCESS) {
    ReplyStatusError(aRunnable, ret,
                     NS_LITERAL_STRING("SetPairingConfirmation"));
  } else {
    DispatchBluetoothReply(aRunnable, BluetoothValue(true), EmptyString());
  }

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

void
BluetoothServiceBluedroid::Connect(const nsAString& aDeviceAddress,
                                   uint32_t aCod,
                                   uint16_t aServiceUuid,
                                   BluetoothReplyRunnable* aRunnable)
{

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

}

void
BluetoothServiceBluedroid::SendFile(const nsAString& aDeviceAddress,
                                    BlobParent* aBlobParent,
                                    BlobChild* aBlobChild,
                                    BluetoothReplyRunnable* aRunnable)
{

}

void
BluetoothServiceBluedroid::StopSendingFile(const nsAString& aDeviceAddress,
                                           BluetoothReplyRunnable* aRunnable)
{

}

void
BluetoothServiceBluedroid::ConfirmReceivingFile(
  const nsAString& aDeviceAddress, bool aConfirm,
  BluetoothReplyRunnable* aRunnable)
{

}

void
BluetoothServiceBluedroid::ConnectSco(BluetoothReplyRunnable* aRunnable)
{

}

void
BluetoothServiceBluedroid::DisconnectSco(BluetoothReplyRunnable* aRunnable)
{

}

void
BluetoothServiceBluedroid::IsScoConnected(BluetoothReplyRunnable* aRunnable)
{

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

}

void
BluetoothServiceBluedroid::SendPlayStatus(
  int64_t aDuration, int64_t aPosition,
  const nsAString& aPlayStatus,
  BluetoothReplyRunnable* aRunnable)
{

}

void
BluetoothServiceBluedroid::UpdatePlayStatus(
  uint32_t aDuration, uint32_t aPosition, ControlPlayStatus aPlayStatus)
{

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

