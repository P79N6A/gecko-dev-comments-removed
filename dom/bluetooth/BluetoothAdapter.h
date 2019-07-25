





#ifndef mozilla_dom_bluetooth_bluetoothadapter_h__
#define mozilla_dom_bluetooth_bluetoothadapter_h__

#include "BluetoothCommon.h"
#include "nsIDOMBluetoothAdapter.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothAdapter : public nsIDOMBluetoothAdapter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMBLUETOOTHADAPTER

  BluetoothAdapter();

protected:
  bool mPower;
};

END_BLUETOOTH_NAMESPACE
#endif
