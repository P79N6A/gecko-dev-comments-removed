





#include "BluetoothService.h"
#include "BluetoothUtils.h"
#include "mozilla/dom/BluetoothGattCharacteristicBinding.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "mozilla/dom/bluetooth/BluetoothGattCharacteristic.h"
#include "mozilla/dom/bluetooth/BluetoothGattDescriptor.h"
#include "mozilla/dom/bluetooth/BluetoothGattService.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"

using namespace mozilla;
using namespace mozilla::dom;

USING_BLUETOOTH_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(
  BluetoothGattCharacteristic, mOwner, mService, mDescriptors)
NS_IMPL_CYCLE_COLLECTING_ADDREF(BluetoothGattCharacteristic)
NS_IMPL_CYCLE_COLLECTING_RELEASE(BluetoothGattCharacteristic)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BluetoothGattCharacteristic)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

BluetoothGattCharacteristic::BluetoothGattCharacteristic(
  nsPIDOMWindow* aOwner,
  BluetoothGattService* aService,
  const BluetoothGattId& aCharId)
  : mOwner(aOwner)
  , mService(aService)
  , mCharId(aCharId)
{
  MOZ_ASSERT(aOwner);
  MOZ_ASSERT(mService);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  
  
  nsString path;
  GeneratePathFromGattId(mCharId, path, mUuidStr);
  bs->RegisterBluetoothSignalHandler(path, this);
}

BluetoothGattCharacteristic::~BluetoothGattCharacteristic()
{
  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  nsString path;
  GeneratePathFromGattId(mCharId, path);
  bs->UnregisterBluetoothSignalHandler(path, this);
}

void
BluetoothGattCharacteristic::HandleDescriptorsDiscovered(
  const BluetoothValue& aValue)
{
  MOZ_ASSERT(aValue.type() == BluetoothValue::TArrayOfBluetoothGattId);

  const InfallibleTArray<BluetoothGattId>& descriptorIds =
    aValue.get_ArrayOfBluetoothGattId();

  for (uint32_t i = 0; i < descriptorIds.Length(); i++) {
    mDescriptors.AppendElement(new BluetoothGattDescriptor(
      GetParentObject(), this, descriptorIds[i]));
  }

  BluetoothGattCharacteristicBinding::ClearCachedDescriptorsValue(this);
}

void
BluetoothGattCharacteristic::Notify(const BluetoothSignal& aData)
{
  BT_LOGD("[D] %s", NS_ConvertUTF16toUTF8(aData.name()).get());

  BluetoothValue v = aData.value();
  if (aData.name().EqualsLiteral("DescriptorsDiscovered")) {
    HandleDescriptorsDiscovered(v);
  } else {
    BT_WARNING("Not handling GATT Characteristic signal: %s",
               NS_ConvertUTF16toUTF8(aData.name()).get());
  }
}

JSObject*
BluetoothGattCharacteristic::WrapObject(JSContext* aContext,
                                        JS::Handle<JSObject*> aGivenProto)
{
  return BluetoothGattCharacteristicBinding::Wrap(aContext, this, aGivenProto);
}
