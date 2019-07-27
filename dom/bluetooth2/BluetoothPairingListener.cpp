





#include "mozilla/dom/bluetooth/BluetoothPairingListener.h"
#include "mozilla/dom/bluetooth/BluetoothPairingHandle.h"
#include "mozilla/dom/BluetoothPairingEvent.h"
#include "mozilla/dom/BluetoothPairingListenerBinding.h"

USING_BLUETOOTH_NAMESPACE

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BluetoothPairingListener)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BluetoothPairingListener, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BluetoothPairingListener, DOMEventTargetHelper)

BluetoothPairingListener::BluetoothPairingListener(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
{
  MOZ_ASSERT(aWindow);
  MOZ_ASSERT(IsDOMBinding());
}

already_AddRefed<BluetoothPairingListener>
BluetoothPairingListener::Create(nsPIDOMWindow* aWindow)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWindow);

  nsRefPtr<BluetoothPairingListener> handle =
    new BluetoothPairingListener(aWindow);

  return handle.forget();
}

BluetoothPairingListener::~BluetoothPairingListener()
{
}

void
BluetoothPairingListener::DispatchPairingEvent(BluetoothDevice* aDevice,
                                               const nsAString& aPasskey,
                                               const nsAString& aType)
{
  MOZ_ASSERT(aDevice && !aType.IsEmpty());

  nsString address;
  aDevice->GetAddress(address);

  nsRefPtr<BluetoothPairingHandle> handle =
    BluetoothPairingHandle::Create(GetOwner(),
                                   address,
                                   aType,
                                   aPasskey);

  BluetoothPairingEventInit init;
  init.mDevice = aDevice;
  init.mHandle = handle;

  nsRefPtr<BluetoothPairingEvent> event =
    BluetoothPairingEvent::Constructor(this,
                                       aType,
                                       init);

  DispatchTrustedEvent(event);
}

JSObject*
BluetoothPairingListener::WrapObject(JSContext* aCx)
{
  return BluetoothPairingListenerBinding::Wrap(aCx, this);
}
