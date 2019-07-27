





#include "BluetoothDiscoveryHandle.h"
#include "BluetoothService.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/BluetoothDeviceEvent.h"
#include "mozilla/dom/BluetoothDiscoveryHandleBinding.h"
#include "nsThreadUtils.h"

USING_BLUETOOTH_NAMESPACE

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BluetoothDiscoveryHandle)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BluetoothDiscoveryHandle, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BluetoothDiscoveryHandle, DOMEventTargetHelper)

BluetoothDiscoveryHandle::BluetoothDiscoveryHandle(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
{
  MOZ_ASSERT(aWindow);
  MOZ_ASSERT(IsDOMBinding());

  ListenToBluetoothSignal(true);
}

BluetoothDiscoveryHandle::~BluetoothDiscoveryHandle()
{
  ListenToBluetoothSignal(false);
}

void
BluetoothDiscoveryHandle::ListenToBluetoothSignal(bool aStart)
{
  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  if (aStart) {
    bs->RegisterBluetoothSignalHandler(
      NS_LITERAL_STRING(KEY_DISCOVERY_HANDLE), this);
  } else {
    bs->UnregisterBluetoothSignalHandler(
      NS_LITERAL_STRING(KEY_DISCOVERY_HANDLE), this);
  }
}


already_AddRefed<BluetoothDiscoveryHandle>
BluetoothDiscoveryHandle::Create(nsPIDOMWindow* aWindow)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWindow);

  nsRefPtr<BluetoothDiscoveryHandle> handle =
    new BluetoothDiscoveryHandle(aWindow);
  return handle.forget();
}

void
BluetoothDiscoveryHandle::DispatchDeviceEvent(const BluetoothValue& aValue)
{
  
  nsRefPtr<BluetoothDevice> device =
    BluetoothDevice::Create(GetOwner(), aValue);

  
  BluetoothDeviceEventInit init;
  init.mDevice = device;
  nsRefPtr<BluetoothDeviceEvent> event =
    BluetoothDeviceEvent::Constructor(this,
                                      NS_LITERAL_STRING("devicefound"),
                                      init);
  DispatchTrustedEvent(event);
}

void
BluetoothDiscoveryHandle::Notify(const BluetoothSignal& aData)
{
  BT_LOGD("[DH] %s", NS_ConvertUTF16toUTF8(aData.name()).get());

  if (aData.name().EqualsLiteral("DeviceFound")) {
    DispatchDeviceEvent(aData.value());
  } else {
    BT_WARNING("Not handling discovery handle signal: %s",
               NS_ConvertUTF16toUTF8(aData.name()).get());
  }
}

JSObject*
BluetoothDiscoveryHandle::WrapObject(JSContext* aCx)
{
  return BluetoothDiscoveryHandleBinding::Wrap(aCx, this);
}
