





#ifndef mozilla_dom_bluetooth_bluetoothrillistener_h__
#define mozilla_dom_bluetooth_bluetoothrillistener_h__

#include "BluetoothCommon.h"

#include "nsCOMPtr.h"
#include "nsIRadioInterfaceLayer.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothRilListener
{
public:
  BluetoothRilListener();

  bool StartListening();
  bool StopListening();

  nsIRILTelephonyCallback* GetCallback();

private:
  nsCOMPtr<nsIRILTelephonyCallback> mRILTelephonyCallback;
};

END_BLUETOOTH_NAMESPACE

#endif
