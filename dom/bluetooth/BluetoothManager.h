





#ifndef mozilla_dom_bluetooth_bluetoothmanager_h__
#define mozilla_dom_bluetooth_bluetoothmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothPropertyContainer.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMBluetoothManager.h"
#include "nsIEventTarget.h"
#include "nsIObserver.h"
#include "mozilla/Observer.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothNamedValue;

class BluetoothManager : public nsDOMEventTargetHelper
                       , public nsIDOMBluetoothManager
                       , public BluetoothSignalObserver
                       , public BluetoothPropertyContainer
                       , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMBLUETOOTHMANAGER
  NS_DECL_NSIOBSERVER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothManager,
                                           nsDOMEventTargetHelper)


  inline void SetEnabledInternal(bool aEnabled) {mEnabled = aEnabled;}

  static already_AddRefed<BluetoothManager>
  Create(nsPIDOMWindow* aWindow);
  void Notify(const BluetoothSignal& aData);
  virtual void SetPropertyByValue(const BluetoothNamedValue& aValue);
  nsresult FireEnabledDisabledEvent();
private:
  BluetoothManager(nsPIDOMWindow* aWindow);
  ~BluetoothManager();

  nsresult HandleMozsettingChanged(const PRUnichar* aData);

  bool mEnabled;

  NS_DECL_EVENT_HANDLER(enabled)
  NS_DECL_EVENT_HANDLER(disabled)
};

END_BLUETOOTH_NAMESPACE

nsresult NS_NewBluetoothManager(nsPIDOMWindow* aWindow,
                                nsIDOMBluetoothManager** aBluetoothManager);

#endif
