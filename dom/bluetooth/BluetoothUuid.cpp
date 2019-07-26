





#include "BluetoothUuid.h"

#include "BluetoothA2dpManager.h"
#include "BluetoothHfpManager.h"
#include "BluetoothHidManager.h"
#include "BluetoothOppManager.h"

USING_BLUETOOTH_NAMESPACE

void
BluetoothUuidHelper::GetString(BluetoothServiceClass aServiceClass,
                               nsAString& aRetUuidStr)
{
  aRetUuidStr.Truncate();

  aRetUuidStr.AppendLiteral("0000");
  aRetUuidStr.AppendInt(aServiceClass, 16);
  aRetUuidStr.AppendLiteral("-0000-1000-8000-00805F9B34FB");
}

BluetoothServiceClass
BluetoothUuidHelper::GetBluetoothServiceClass(const nsAString& aUuidStr)
{
  
  MOZ_ASSERT(aUuidStr.Length() == 36);

  




  BluetoothServiceClass retValue = BluetoothServiceClass::UNKNOWN;
  nsString uuid(Substring(aUuidStr, 4, 4));

  nsresult rv;
  int32_t integer = uuid.ToInteger(&rv, 16);
  NS_ENSURE_SUCCESS(rv, retValue);

  return GetBluetoothServiceClass(integer);
}

BluetoothServiceClass
BluetoothUuidHelper::GetBluetoothServiceClass(uint16_t aServiceUuid)
{
  BluetoothServiceClass retValue = BluetoothServiceClass::UNKNOWN;
  switch (aServiceUuid) {
    case BluetoothServiceClass::A2DP:
    case BluetoothServiceClass::A2DP_SINK:
    case BluetoothServiceClass::HANDSFREE:
    case BluetoothServiceClass::HANDSFREE_AG:
    case BluetoothServiceClass::HEADSET:
    case BluetoothServiceClass::HEADSET_AG:
    case BluetoothServiceClass::HID:
    case BluetoothServiceClass::OBJECT_PUSH:
      retValue = (BluetoothServiceClass)aServiceUuid;
  }
  return retValue;
}

BluetoothProfileManagerBase*
BluetoothUuidHelper::GetBluetoothProfileManager(uint16_t aServiceUuid)
{
  BluetoothProfileManagerBase* profile;
  BluetoothServiceClass serviceClass = GetBluetoothServiceClass(aServiceUuid);
  switch (serviceClass) {
    case BluetoothServiceClass::HANDSFREE:
    case BluetoothServiceClass::HEADSET:
      profile = BluetoothHfpManager::Get();
      break;
    case BluetoothServiceClass::HID:
      profile = BluetoothHidManager::Get();
      break;
    case BluetoothServiceClass::A2DP:
      profile = BluetoothA2dpManager::Get();
      break;
    case BluetoothServiceClass::OBJECT_PUSH:
      profile = BluetoothOppManager::Get();
      break;
    default:
      profile = nullptr;
  }
  return profile;
}
