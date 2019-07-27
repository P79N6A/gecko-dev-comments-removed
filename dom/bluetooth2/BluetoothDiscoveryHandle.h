





#ifndef mozilla_dom_bluetooth_bluetoothdiscoveryhandle_h
#define mozilla_dom_bluetooth_bluetoothdiscoveryhandle_h

#include "BluetoothCommon.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/Observer.h"
#include "nsISupportsImpl.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothValue;

class BluetoothDiscoveryHandle MOZ_FINAL : public DOMEventTargetHelper
                                         , public BluetoothSignalObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  static already_AddRefed<BluetoothDiscoveryHandle>
    Create(nsPIDOMWindow* aWindow);

  IMPL_EVENT_HANDLER(devicefound);

  void Notify(const BluetoothSignal& aData); 

  virtual void DisconnectFromOwner() MOZ_OVERRIDE;
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

private:
  BluetoothDiscoveryHandle(nsPIDOMWindow* aWindow);
  ~BluetoothDiscoveryHandle();

  




  void ListenToBluetoothSignal(bool aStart);

  




  void DispatchDeviceEvent(const BluetoothValue& aValue);
};

END_BLUETOOTH_NAMESPACE

#endif 
