





#ifndef mozilla_dom_bluetooth_bluetoothpairinglistener_h
#define mozilla_dom_bluetooth_bluetoothpairinglistener_h

#include "BluetoothCommon.h"
#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSignal;

class BluetoothPairingListener final : public DOMEventTargetHelper
                                     , public BluetoothSignalObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  static already_AddRefed<BluetoothPairingListener>
    Create(nsPIDOMWindow* aWindow);

  void DispatchPairingEvent(const nsAString& aName,
                            const nsAString& aAddress,
                            const nsAString& aPasskey,
                            const nsAString& aType);

  void Notify(const BluetoothSignal& aParam); 

  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;
  virtual void DisconnectFromOwner() override;
  virtual void EventListenerAdded(nsIAtom* aType) override;

  IMPL_EVENT_HANDLER(displaypasskeyreq);
  IMPL_EVENT_HANDLER(enterpincodereq);
  IMPL_EVENT_HANDLER(pairingconfirmationreq);
  IMPL_EVENT_HANDLER(pairingconsentreq);

private:
  BluetoothPairingListener(nsPIDOMWindow* aWindow);
  ~BluetoothPairingListener();

  






  void TryListeningToBluetoothSignal();

  



  bool mHasListenedToSignal;
};

END_BLUETOOTH_NAMESPACE

#endif 
