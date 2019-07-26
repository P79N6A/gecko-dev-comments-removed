





#ifndef mozilla_dom_bluetooth_bluetoothrillistener_h__
#define mozilla_dom_bluetooth_bluetoothrillistener_h__

#include "BluetoothCommon.h"

#include "nsCOMPtr.h"

class nsIIccListener;
class nsIMobileConnectionListener;
class nsITelephonyListener;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothRilListener
{
public:
  BluetoothRilListener();

  bool StartListening();
  bool StopListening();

  void EnumerateCalls();

private:
  bool StartIccListening();
  bool StopIccListening();

  bool StartMobileConnectionListening();
  bool StopMobileConnectionListening();

  bool StartTelephonyListening();
  bool StopTelephonyListening();

  nsCOMPtr<nsIIccListener> mIccListener;
  nsCOMPtr<nsIMobileConnectionListener> mMobileConnectionListener;
  nsCOMPtr<nsITelephonyListener> mTelephonyListener;
};

END_BLUETOOTH_NAMESPACE

#endif
