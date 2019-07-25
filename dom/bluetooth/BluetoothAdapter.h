





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
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMBLUETOOTHADAPTER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothAdapter,
                                           nsDOMEventTargetHelper)

  BluetoothAdapter(nsPIDOMWindow*);

  inline void SetEnabledInternal(bool aEnabled) {mEnabled = aEnabled;}

protected:
  bool mEnabled;

  NS_DECL_EVENT_HANDLER(enabled)

private:
  nsCOMPtr<nsIEventTarget> mToggleBtThread;
};

END_BLUETOOTH_NAMESPACE

#endif
