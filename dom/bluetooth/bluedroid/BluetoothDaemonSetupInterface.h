





#ifndef mozilla_dom_bluetooth_bluedroid_bluetoothdaemonsetupinterface_h__
#define mozilla_dom_bluetooth_bluedroid_bluetoothdaemonsetupinterface_h__

#include "BluetoothCommon.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSetupResultHandler
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(BluetoothSetupResultHandler)

  virtual ~BluetoothSetupResultHandler();

  virtual void OnError(BluetoothStatus aStatus);
  virtual void RegisterModule();
  virtual void UnregisterModule();
  virtual void Configuration();
};

END_BLUETOOTH_NAMESPACE

#endif
