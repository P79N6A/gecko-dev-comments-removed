
















#include "BluetoothGonkService.h"

USING_BLUETOOTH_NAMESPACE

BluetoothService*
BluetoothService::Create()
{
  return new BluetoothGonkService();
}
