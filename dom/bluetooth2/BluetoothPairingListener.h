





#ifndef mozilla_dom_bluetooth_bluetoothpairinglistener_h
#define mozilla_dom_bluetooth_bluetoothpairinglistener_h

#include "BluetoothCommon.h"
#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothDevice;

class BluetoothPairingListener MOZ_FINAL : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  static already_AddRefed<BluetoothPairingListener>
    Create(nsPIDOMWindow* aWindow);

  void DispatchPairingEvent(BluetoothDevice* aDevice,
                            const nsAString& aPasskey,
                            const nsAString& aType);

  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  IMPL_EVENT_HANDLER(displaypasskeyreq);
  IMPL_EVENT_HANDLER(enterpincodereq);
  IMPL_EVENT_HANDLER(pairingconfirmationreq);
  IMPL_EVENT_HANDLER(pairingconsentreq);

private:
  BluetoothPairingListener(nsPIDOMWindow* aWindow);
  ~BluetoothPairingListener();
};

END_BLUETOOTH_NAMESPACE

#endif 
