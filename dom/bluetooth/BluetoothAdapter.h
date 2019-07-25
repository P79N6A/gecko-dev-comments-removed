





#ifndef mozilla_dom_bluetooth_bluetoothadapter_h__
#define mozilla_dom_bluetooth_bluetoothadapter_h__

#include "BluetoothCommon.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMBluetoothAdapter.h"

class nsIEventTarget;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothAdapter : public nsIDOMBluetoothAdapter
                       , public nsDOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMBLUETOOTHADAPTER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothAdapter,
                                           nsDOMEventTargetHelper)

  BluetoothAdapter();

  nsresult FirePowered();

protected:
  bool mPower;

  NS_DECL_EVENT_HANDLER(powered)

private:
  nsCOMPtr<nsIEventTarget> mToggleBtThread;
  nsresult ToggleBluetoothAsync();
};

END_BLUETOOTH_NAMESPACE
#endif
