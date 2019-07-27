





#include "BluetoothService.h"
#include "BluetoothUtils.h"
#include "mozilla/dom/bluetooth/BluetoothPairingListener.h"
#include "mozilla/dom/bluetooth/BluetoothPairingHandle.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/BluetoothPairingEvent.h"
#include "mozilla/dom/BluetoothPairingListenerBinding.h"

USING_BLUETOOTH_NAMESPACE

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BluetoothPairingListener)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BluetoothPairingListener, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BluetoothPairingListener, DOMEventTargetHelper)

BluetoothPairingListener::BluetoothPairingListener(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
  , mHasListenedToSignal(false)
{
  MOZ_ASSERT(aWindow);

  TryListeningToBluetoothSignal();
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
  UnregisterBluetoothSignalHandler(NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                                   this);
}

void
BluetoothPairingListener::DispatchPairingEvent(const nsAString& aName,
                                               const nsAString& aAddress,
                                               const nsAString& aPasskey,
                                               const nsAString& aType)
{
  MOZ_ASSERT(!aName.IsEmpty() && !aAddress.IsEmpty() && !aType.IsEmpty());

  nsRefPtr<BluetoothPairingHandle> handle =
    BluetoothPairingHandle::Create(GetOwner(),
                                   aAddress,
                                   aType,
                                   aPasskey);

  BluetoothPairingEventInit init;
  init.mDeviceName = aName;
  init.mHandle = handle;

  nsRefPtr<BluetoothPairingEvent> event =
    BluetoothPairingEvent::Constructor(this,
                                       aType,
                                       init);

  DispatchTrustedEvent(event);
}

void
BluetoothPairingListener::Notify(const BluetoothSignal& aData)
{
  InfallibleTArray<BluetoothNamedValue> arr;

  BluetoothValue value = aData.value();
  if (aData.name().EqualsLiteral("PairingRequest")) {

    MOZ_ASSERT(value.type() == BluetoothValue::TArrayOfBluetoothNamedValue);

    const InfallibleTArray<BluetoothNamedValue>& arr =
      value.get_ArrayOfBluetoothNamedValue();

    MOZ_ASSERT(arr.Length() == 4 &&
               arr[0].value().type() == BluetoothValue::TnsString && 
               arr[1].value().type() == BluetoothValue::TnsString && 
               arr[2].value().type() == BluetoothValue::TnsString && 
               arr[3].value().type() == BluetoothValue::TnsString);  

    nsString address = arr[0].value().get_nsString();
    nsString name = arr[1].value().get_nsString();
    nsString passkey = arr[2].value().get_nsString();
    nsString type = arr[3].value().get_nsString();

    
    DispatchPairingEvent(name, address, passkey, type);
  } else {
    BT_WARNING("Not handling pairing listener signal: %s",
               NS_ConvertUTF16toUTF8(aData.name()).get());
  }
}

JSObject*
BluetoothPairingListener::WrapObject(JSContext* aCx,
                                     JS::Handle<JSObject*> aGivenProto)
{
  return BluetoothPairingListenerBinding::Wrap(aCx, this, aGivenProto);
}

void
BluetoothPairingListener::DisconnectFromOwner()
{
  DOMEventTargetHelper::DisconnectFromOwner();
  UnregisterBluetoothSignalHandler(NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                                   this);
}

void
BluetoothPairingListener::EventListenerAdded(nsIAtom* aType)
{
  DOMEventTargetHelper::EventListenerAdded(aType);

  TryListeningToBluetoothSignal();
}

void
BluetoothPairingListener::TryListeningToBluetoothSignal()
{
  if (mHasListenedToSignal) {
    
    return;
  }

  
  
  
  if (!HasListenersFor(nsGkAtoms::ondisplaypasskeyreq) ||
      !HasListenersFor(nsGkAtoms::onenterpincodereq) ||
      !HasListenersFor(nsGkAtoms::onpairingconfirmationreq) ||
      !HasListenersFor(nsGkAtoms::onpairingconsentreq)) {
    BT_LOGR("Pairing listener is not ready to handle pairing requests!");
    return;
  }

  
  RegisterBluetoothSignalHandler(NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                                 this);

  mHasListenedToSignal = true;
}
