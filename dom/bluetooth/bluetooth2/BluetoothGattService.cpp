





#include "BluetoothService.h"
#include "BluetoothUtils.h"
#include "mozilla/dom/BluetoothGattServiceBinding.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "mozilla/dom/bluetooth/BluetoothGattCharacteristic.h"
#include "mozilla/dom/bluetooth/BluetoothGattService.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"

using namespace mozilla;
using namespace mozilla::dom;

USING_BLUETOOTH_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_CLASS(BluetoothGattService)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(BluetoothGattService)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOwner)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mIncludedServices)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mCharacteristics)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER

  






  nsString path;
  GeneratePathFromGattId(tmp->mServiceId.mId, path);
  UnregisterBluetoothSignalHandler(path, tmp);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(BluetoothGattService)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOwner)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mIncludedServices)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCharacteristics)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(BluetoothGattService)

NS_IMPL_CYCLE_COLLECTING_ADDREF(BluetoothGattService)
NS_IMPL_CYCLE_COLLECTING_RELEASE(BluetoothGattService)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BluetoothGattService)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

BluetoothGattService::BluetoothGattService(
  nsPIDOMWindow* aOwner, const nsAString& aAppUuid,
  const BluetoothGattServiceId& aServiceId)
  : mOwner(aOwner)
  , mAppUuid(aAppUuid)
  , mServiceId(aServiceId)
{
  MOZ_ASSERT(aOwner);
  MOZ_ASSERT(!mAppUuid.IsEmpty());

  
  
  nsString path;
  GeneratePathFromGattId(mServiceId.mId, path, mUuidStr);
  RegisterBluetoothSignalHandler(path, this);
}

BluetoothGattService::~BluetoothGattService()
{
  nsString path;
  GeneratePathFromGattId(mServiceId.mId, path);
  UnregisterBluetoothSignalHandler(path, this);
}

void
BluetoothGattService::HandleIncludedServicesDiscovered(
  const BluetoothValue& aValue)
{
  MOZ_ASSERT(aValue.type() == BluetoothValue::TArrayOfBluetoothGattServiceId);

  const InfallibleTArray<BluetoothGattServiceId>& includedServIds =
    aValue.get_ArrayOfBluetoothGattServiceId();

  mIncludedServices.Clear();
  for (uint32_t i = 0; i < includedServIds.Length(); i++) {
    mIncludedServices.AppendElement(new BluetoothGattService(
      GetParentObject(), mAppUuid, includedServIds[i]));
  }

  BluetoothGattServiceBinding::ClearCachedIncludedServicesValue(this);
}

void
BluetoothGattService::HandleCharacteristicsDiscovered(
  const BluetoothValue& aValue)
{
  MOZ_ASSERT(aValue.type() ==
             BluetoothValue::TArrayOfBluetoothGattCharAttribute);

  const InfallibleTArray<BluetoothGattCharAttribute>& characteristics =
    aValue.get_ArrayOfBluetoothGattCharAttribute();

  mCharacteristics.Clear();
  for (uint32_t i = 0; i < characteristics.Length(); i++) {
    mCharacteristics.AppendElement(new BluetoothGattCharacteristic(
      GetParentObject(), this, characteristics[i]));
  }

  BluetoothGattServiceBinding::ClearCachedCharacteristicsValue(this);
}

void
BluetoothGattService::Notify(const BluetoothSignal& aData)
{
  BT_LOGD("[D] %s", NS_ConvertUTF16toUTF8(aData.name()).get());
  NS_ENSURE_TRUE_VOID(mSignalRegistered);

  BluetoothValue v = aData.value();
  if (aData.name().EqualsLiteral("IncludedServicesDiscovered")) {
    HandleIncludedServicesDiscovered(v);
  } else if (aData.name().EqualsLiteral("CharacteristicsDiscovered")) {
    HandleCharacteristicsDiscovered(v);
  } else {
    BT_WARNING("Not handling GATT Service signal: %s",
               NS_ConvertUTF16toUTF8(aData.name()).get());
  }
}

JSObject*
BluetoothGattService::WrapObject(JSContext* aContext,
                                 JS::Handle<JSObject*> aGivenProto)
{
  return BluetoothGattServiceBinding::Wrap(aContext, this, aGivenProto);
}
