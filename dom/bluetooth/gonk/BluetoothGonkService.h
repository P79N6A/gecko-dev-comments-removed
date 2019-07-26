
















#ifndef mozilla_dom_bluetooth_bluetoothgonkservice_h__
#define mozilla_dom_bluetooth_bluetoothgonkservice_h__

#include "BluetoothCommon.h"
#include "BluetoothDBusService.h"

BEGIN_BLUETOOTH_NAMESPACE










class BluetoothGonkService : public BluetoothDBusService
{
public:
  






  virtual nsresult StartInternal();

  






  virtual nsresult StopInternal();
};

END_BLUETOOTH_NAMESPACE

#endif
