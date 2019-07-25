





#ifndef mozilla_dom_bluetooth_bluetoothmanager_h__
#define mozilla_dom_bluetooth_bluetoothmanager_h__

#include "BluetoothCommon.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMBluetoothManager.h"
#include "mozilla/Observer.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothManager : public nsDOMEventTargetHelper
                       , public nsIDOMBluetoothManager
                       , public BluetoothSignalObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMBLUETOOTHMANAGER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothManager,
                                           nsDOMEventTargetHelper)


  inline void SetEnabledInternal(bool aEnabled) {mEnabled = aEnabled;}

  static already_AddRefed<BluetoothManager>
  Create(nsPIDOMWindow* aWindow);
  void Notify(const BluetoothSignal& aData);
private:
  BluetoothManager() {}
  BluetoothManager(nsPIDOMWindow* aWindow);
  ~BluetoothManager();
  bool mEnabled;
  nsString mName;

  NS_DECL_EVENT_HANDLER(enabled)
};

END_BLUETOOTH_NAMESPACE

nsresult NS_NewBluetoothManager(nsPIDOMWindow* aWindow,
                                nsIDOMBluetoothManager** aBluetoothManager);

#endif
