





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
}

BluetoothDiscoveryHandle::~BluetoothDiscoveryHandle()
{
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
BluetoothDiscoveryHandle::DispatchDeviceEvent(BluetoothDevice* aDevice)
{
  MOZ_ASSERT(aDevice);

  BluetoothDeviceEventInit init;
  init.mDevice = aDevice;

  nsRefPtr<BluetoothDeviceEvent> event =
    BluetoothDeviceEvent::Constructor(this,
                                      NS_LITERAL_STRING("devicefound"),
                                      init);
  DispatchTrustedEvent(event);
}

JSObject*
BluetoothDiscoveryHandle::WrapObject(JSContext* aCx,
                                     JS::Handle<JSObject*> aGivenProto)
{
  return BluetoothDiscoveryHandleBinding::Wrap(aCx, this, aGivenProto);
}
