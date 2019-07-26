





#include "BluetoothUuid.h"

USING_BLUETOOTH_NAMESPACE

void
BluetoothUuidHelper::GetString(BluetoothServiceClass aServiceClassUuid,
                               nsAString& aRetUuidStr)
{
  aRetUuidStr.Truncate();

  aRetUuidStr.AppendLiteral("0000");
  aRetUuidStr.AppendInt(aServiceClassUuid, 16);
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

  switch (integer) {
    case BluetoothServiceClass::A2DP:
    case BluetoothServiceClass::HANDSFREE:
    case BluetoothServiceClass::HANDSFREE_AG:
    case BluetoothServiceClass::HEADSET:
    case BluetoothServiceClass::HEADSET_AG:
    case BluetoothServiceClass::HID:
    case BluetoothServiceClass::OBJECT_PUSH:
      retValue = (BluetoothServiceClass)integer;
  }
  return retValue;
}
