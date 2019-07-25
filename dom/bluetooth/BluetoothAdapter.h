





#ifndef mozilla_dom_bluetooth_bluetoothadapter_h__
#define mozilla_dom_bluetooth_bluetoothadapter_h__

#include "BluetoothCommon.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMBluetoothAdapter.h"
#include "nsIDOMDOMRequest.h"

class nsIEventTarget;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothAdapter : public nsDOMEventTargetHelper
                       , public nsIDOMBluetoothAdapter
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMBLUETOOTHADAPTER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothAdapter,
                                           nsDOMEventTargetHelper)

};

END_BLUETOOTH_NAMESPACE

#endif
