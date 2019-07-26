
















#ifndef mozilla_dom_bluetooth_bluetoothgonkservice_h__
#define mozilla_dom_bluetooth_bluetoothgonkservice_h__

#include "mozilla/Attributes.h"
#include "BluetoothCommon.h"
#include "BluetoothDBusService.h"

BEGIN_BLUETOOTH_NAMESPACE










class BluetoothGonkService : public BluetoothDBusService
{
public:
  





  virtual nsresult StartInternal() MOZ_OVERRIDE;

  




  virtual nsresult StopInternal() MOZ_OVERRIDE;

  




  virtual bool IsEnabledInternal() MOZ_OVERRIDE;
};

END_BLUETOOTH_NAMESPACE

#endif
