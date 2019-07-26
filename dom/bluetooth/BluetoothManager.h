





#ifndef mozilla_dom_bluetooth_bluetoothmanager_h__
#define mozilla_dom_bluetooth_bluetoothmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothPropertyContainer.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMBluetoothManager.h"
#include "mozilla/Observer.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothNamedValue;

class BluetoothManager : public nsDOMEventTargetHelper
                       , public nsIDOMBluetoothManager
                       , public BluetoothSignalObserver
                       , public BluetoothPropertyContainer
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMBLUETOOTHMANAGER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  static already_AddRefed<BluetoothManager>
  Create(nsPIDOMWindow* aWindow);
  void Notify(const BluetoothSignal& aData);
  virtual void SetPropertyByValue(const BluetoothNamedValue& aValue);
private:
  BluetoothManager(nsPIDOMWindow* aWindow);
  ~BluetoothManager();
};

END_BLUETOOTH_NAMESPACE

nsresult NS_NewBluetoothManager(nsPIDOMWindow* aWindow,
                                nsIDOMBluetoothManager** aBluetoothManager);

#endif
