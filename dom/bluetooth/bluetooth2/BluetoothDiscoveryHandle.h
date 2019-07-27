





#ifndef mozilla_dom_bluetooth_bluetoothdiscoveryhandle_h
#define mozilla_dom_bluetooth_bluetoothdiscoveryhandle_h

#include "BluetoothCommon.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsISupportsImpl.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothDevice;

class BluetoothDiscoveryHandle final : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  static already_AddRefed<BluetoothDiscoveryHandle>
    Create(nsPIDOMWindow* aWindow);

  void DispatchDeviceEvent(BluetoothDevice* aDevice);

  IMPL_EVENT_HANDLER(devicefound);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

private:
  BluetoothDiscoveryHandle(nsPIDOMWindow* aWindow);
  ~BluetoothDiscoveryHandle();
};

END_BLUETOOTH_NAMESPACE

#endif 
