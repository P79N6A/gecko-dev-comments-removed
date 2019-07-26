





#ifndef mozilla_dom_bluetooth_bluetoothtelephonylistener_h__
#define mozilla_dom_bluetooth_bluetoothtelephonylistener_h__

#include "BluetoothCommon.h"

#include "nsCOMPtr.h"
#include "nsITelephonyProvider.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothTelephonyListener
{
public:
  BluetoothTelephonyListener();

  bool StartListening();
  bool StopListening();

  nsITelephonyListener* GetListener();

private:
  nsCOMPtr<nsITelephonyListener> mTelephonyListener;
};

END_BLUETOOTH_NAMESPACE

#endif
