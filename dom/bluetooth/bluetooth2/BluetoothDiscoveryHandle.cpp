





#include "BluetoothService.h"
#include "mozilla/dom/BluetoothDeviceEvent.h"
#include "mozilla/dom/BluetoothDiscoveryHandleBinding.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "mozilla/dom/bluetooth/BluetoothDiscoveryHandle.h"
#include "mozilla/dom/bluetooth/BluetoothLeDeviceEvent.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "nsThreadUtils.h"

USING_BLUETOOTH_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_INHERITED(BluetoothDiscoveryHandle,
                                   DOMEventTargetHelper,
                                   mAdapter)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BluetoothDiscoveryHandle)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BluetoothDiscoveryHandle, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BluetoothDiscoveryHandle, DOMEventTargetHelper)

BluetoothDiscoveryHandle::BluetoothDiscoveryHandle(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
  , mLeScanUuid(EmptyString())
{
  MOZ_ASSERT(aWindow);
}

BluetoothDiscoveryHandle::BluetoothDiscoveryHandle(
  nsPIDOMWindow* aWindow,
  const nsTArray<nsString>& aServiceUuids,
  const nsAString& aLeScanUuid,
  BluetoothAdapter* aAdapter)
  : DOMEventTargetHelper(aWindow)
  , mLeScanUuid(aLeScanUuid)
  , mServiceUuids(aServiceUuids)
  , mAdapter(aAdapter)
{
  MOZ_ASSERT(aWindow);
}

BluetoothDiscoveryHandle::~BluetoothDiscoveryHandle()
{
  
  if (!mLeScanUuid.IsEmpty() && mAdapter != nullptr) {
    mAdapter->RemoveLeScanHandle(mLeScanUuid);
  }
}

void
BluetoothDiscoveryHandle::DisconnectFromOwner()
{
  DOMEventTargetHelper::DisconnectFromOwner();

  
  if (!mLeScanUuid.IsEmpty() && mAdapter != nullptr) {
    mAdapter->RemoveLeScanHandle(mLeScanUuid);
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

already_AddRefed<BluetoothDiscoveryHandle>
BluetoothDiscoveryHandle::Create(
  nsPIDOMWindow* aWindow,
  const nsTArray<nsString>& aServiceUuids,
  const nsAString& aLeScanUuid,
  BluetoothAdapter* aAdapter)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWindow);
  MOZ_ASSERT(aAdapter);

  nsRefPtr<BluetoothDiscoveryHandle> handle =
    new BluetoothDiscoveryHandle(aWindow, aServiceUuids, aLeScanUuid, aAdapter);
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

void
BluetoothDiscoveryHandle::DispatchLeDeviceEvent(BluetoothDevice* aLeDevice,
  int32_t aRssi, nsTArray<uint8_t>& aScanRecord)
{
  MOZ_ASSERT(aLeDevice);

  nsTArray<nsString> remoteUuids;
  aLeDevice->GetUuids(remoteUuids);

  bool hasUuidsFilter = !mServiceUuids.IsEmpty();
  bool noAdvertisingUuid  = remoteUuids.IsEmpty();
  
  
  if (hasUuidsFilter && noAdvertisingUuid) {
    return;
  }

  
  
  
  
  bool matched = false;
  for (size_t index = 0; index < remoteUuids.Length(); ++index) {
    if (mServiceUuids.Contains(remoteUuids[index])) {
      matched = true;
      break;
    }
  }

  
  
  
  if (matched || mServiceUuids.IsEmpty()) {
    nsRefPtr<BluetoothLeDeviceEvent> event =
      BluetoothLeDeviceEvent::Constructor(this,
                                          NS_LITERAL_STRING("devicefound"),
                                          aLeDevice,
                                          aRssi,
                                          aScanRecord);
    DispatchTrustedEvent(event);
  }
}

JSObject*
BluetoothDiscoveryHandle::WrapObject(JSContext* aCx,
                                     JS::Handle<JSObject*> aGivenProto)
{
  return BluetoothDiscoveryHandleBinding::Wrap(aCx, this, aGivenProto);
}
