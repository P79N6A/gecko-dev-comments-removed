





#include "BluetoothDBusService.h"

USING_BLUETOOTH_NAMESPACE

BluetoothService*
BluetoothService::Create()
{
  return new BluetoothDBusService();
}
