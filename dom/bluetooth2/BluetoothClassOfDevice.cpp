





#include "BluetoothClassOfDevice.h"

#include "mozilla/dom/BluetoothClassOfDeviceBinding.h"
#include "nsThreadUtils.h"

USING_BLUETOOTH_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(BluetoothClassOfDevice, mOwnerWindow)
NS_IMPL_CYCLE_COLLECTING_ADDREF(BluetoothClassOfDevice)
NS_IMPL_CYCLE_COLLECTING_RELEASE(BluetoothClassOfDevice)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BluetoothClassOfDevice)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END














#define GET_MAJOR_SERVICE_CLASS(cod) (((cod) & 0xffe000) >> 13)


#define GET_MAJOR_DEVICE_CLASS(cod)  (((cod) & 0x1f00) >> 8)


#define GET_MINOR_DEVICE_CLASS(cod)  (((cod) & 0xfc) >> 2)

BluetoothClassOfDevice::BluetoothClassOfDevice(nsPIDOMWindow* aOwner)
  : mOwnerWindow(aOwner)
{
  MOZ_ASSERT(aOwner);

  Reset();
}

BluetoothClassOfDevice::~BluetoothClassOfDevice()
{}

void
BluetoothClassOfDevice::Reset()
{
  mMajorServiceClass = 0x1; 
  mMajorDeviceClass = 0x1F; 
  mMinorDeviceClass = 0;
}

bool
BluetoothClassOfDevice::Equals(const uint32_t aValue)
{
  return (mMajorServiceClass == GET_MAJOR_SERVICE_CLASS(aValue) &&
          mMajorDeviceClass == GET_MAJOR_DEVICE_CLASS(aValue) &&
          mMinorDeviceClass == GET_MINOR_DEVICE_CLASS(aValue));
}

uint32_t
BluetoothClassOfDevice::ToUint32()
{
  return (mMajorServiceClass & 0x7ff) << 13 |
         (mMajorDeviceClass & 0x1f) << 8 |
         (mMinorDeviceClass & 0x3f) << 2;
}

void
BluetoothClassOfDevice::Update(const uint32_t aValue)
{
  mMajorServiceClass = GET_MAJOR_SERVICE_CLASS(aValue);
  mMajorDeviceClass = GET_MAJOR_DEVICE_CLASS(aValue);
  mMinorDeviceClass = GET_MINOR_DEVICE_CLASS(aValue);
}


already_AddRefed<BluetoothClassOfDevice>
BluetoothClassOfDevice::Create(nsPIDOMWindow* aOwner)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aOwner);

  nsRefPtr<BluetoothClassOfDevice> cod = new BluetoothClassOfDevice(aOwner);
  return cod.forget();
}

JSObject*
BluetoothClassOfDevice::WrapObject(JSContext* aCx,
                                   JS::Handle<JSObject*> aGivenProto)
{
  return BluetoothClassOfDeviceBinding::Wrap(aCx, this, aGivenProto);
}
