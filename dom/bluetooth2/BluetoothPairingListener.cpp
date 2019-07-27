





#include "mozilla/dom/bluetooth/BluetoothPairingListener.h"
#include "mozilla/dom/bluetooth/BluetoothPairingHandle.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/BluetoothPairingEvent.h"
#include "mozilla/dom/BluetoothPairingListenerBinding.h"
#include "BluetoothService.h"

USING_BLUETOOTH_NAMESPACE

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BluetoothPairingListener)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BluetoothPairingListener, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BluetoothPairingListener, DOMEventTargetHelper)

BluetoothPairingListener::BluetoothPairingListener(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
{
  MOZ_ASSERT(aWindow);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);
  bs->RegisterBluetoothSignalHandler(NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                                     this);
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
  BluetoothService* bs = BluetoothService::Get();
  
  NS_ENSURE_TRUE_VOID(bs);
  bs->UnregisterBluetoothSignalHandler(NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                                       this);
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

void
BluetoothPairingListener::Notify(const BluetoothSignal& aData)
{
  InfallibleTArray<BluetoothNamedValue> arr;

  BluetoothValue value = aData.value();
  if (aData.name().EqualsLiteral("PairingRequest")) {

    MOZ_ASSERT(value.type() == BluetoothValue::TArrayOfBluetoothNamedValue);

    const InfallibleTArray<BluetoothNamedValue>& arr =
      value.get_ArrayOfBluetoothNamedValue();

    MOZ_ASSERT(arr.Length() == 3 &&
               arr[0].value().type() == BluetoothValue::TnsString && 
               arr[1].value().type() == BluetoothValue::TnsString && 
               arr[2].value().type() == BluetoothValue::TnsString);  

    nsString deviceAddress = arr[0].value().get_nsString();
    nsString passkey = arr[1].value().get_nsString();
    nsString type = arr[2].value().get_nsString();

    
    InfallibleTArray<BluetoothNamedValue> props;
    BT_APPEND_NAMED_VALUE(props, "Address", deviceAddress);
    nsRefPtr<BluetoothDevice> device =
      BluetoothDevice::Create(GetOwner(), props);

    
    DispatchPairingEvent(device, passkey, type);
  } else {
    BT_WARNING("Not handling pairing listener signal: %s",
               NS_ConvertUTF16toUTF8(aData.name()).get());
  }
}

JSObject*
BluetoothPairingListener::WrapObject(JSContext* aCx)
{
  return BluetoothPairingListenerBinding::Wrap(aCx, this);
}

void
BluetoothPairingListener::DisconnectFromOwner()
{
  DOMEventTargetHelper::DisconnectFromOwner();

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);
  bs->UnregisterBluetoothSignalHandler(NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                                       this);
}
