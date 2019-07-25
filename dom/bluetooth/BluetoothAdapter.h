





#ifndef mozilla_dom_bluetooth_bluetoothadapter_h__
#define mozilla_dom_bluetooth_bluetoothadapter_h__

#include "BluetoothCommon.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMBluetoothAdapter.h"
#include "nsIDOMDOMRequest.h"
#include "mozilla/Observer.h"

class nsIEventTarget;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothAdapter : public nsDOMEventTargetHelper
                       , public nsIDOMBluetoothAdapter
                       , public BluetoothEventObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMBLUETOOTHADAPTER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothAdapter,
                                           nsDOMEventTargetHelper)

  static already_AddRefed<BluetoothAdapter>
  Create(const nsCString& name);

  void Notify(const BluetoothEvent& aParam);
protected:
  nsCString mName;
private:
  BluetoothAdapter() {}
  BluetoothAdapter(const nsCString& name);
  ~BluetoothAdapter();
};

END_BLUETOOTH_NAMESPACE

#endif
