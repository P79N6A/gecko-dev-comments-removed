





#include "base/basictypes.h"
#include "BluetoothDevice.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothUtils.h"

#include "nsDOMClassInfo.h"
#include "nsTArrayHelpers.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/BluetoothDeviceBinding.h"

USING_BLUETOOTH_NAMESPACE

DOMCI_DATA(BluetoothDevice, BluetoothDevice)

NS_IMPL_CYCLE_COLLECTION_CLASS(BluetoothDevice)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(BluetoothDevice,
                                               nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mJsUuids)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mJsServices)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(BluetoothDevice,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(BluetoothDevice,
                                                nsDOMEventTargetHelper)
  tmp->Unroot();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BluetoothDevice)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBluetoothDevice)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BluetoothDevice)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BluetoothDevice, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BluetoothDevice, nsDOMEventTargetHelper)

BluetoothDevice::BluetoothDevice(nsPIDOMWindow* aWindow,
                                 const nsAString& aAdapterPath,
                                 const BluetoothValue& aValue)
  : nsDOMEventTargetHelper(aWindow)
  , BluetoothPropertyContainer(BluetoothObjectType::TYPE_DEVICE)
  , mJsUuids(nullptr)
  , mJsServices(nullptr)
  , mAdapterPath(aAdapterPath)
  , mIsRooted(false)
{
  MOZ_ASSERT(aWindow);
  MOZ_ASSERT(IsDOMBinding());

  BindToOwner(aWindow);
  const InfallibleTArray<BluetoothNamedValue>& values =
    aValue.get_ArrayOfBluetoothNamedValue();
  for (uint32_t i = 0; i < values.Length(); ++i) {
    SetPropertyByValue(values[i]);
  }

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);
  bs->RegisterBluetoothSignalHandler(mAddress, this);
}

BluetoothDevice::~BluetoothDevice()
{
  BluetoothService* bs = BluetoothService::Get();
  
  NS_ENSURE_TRUE_VOID(bs);
  bs->UnregisterBluetoothSignalHandler(mAddress, this);
  Unroot();
}

void
BluetoothDevice::Root()
{
  if (!mIsRooted) {
    mozilla::HoldJSObjects(this);
    mIsRooted = true;
  }
}

void
BluetoothDevice::Unroot()
{
  if (mIsRooted) {
    mJsUuids = nullptr;
    mJsServices = nullptr;
    mozilla::DropJSObjects(this);
    mIsRooted = false;
  }
}

void
BluetoothDevice::SetPropertyByValue(const BluetoothNamedValue& aValue)
{
  const nsString& name = aValue.name();
  const BluetoothValue& value = aValue.value();
  if (name.EqualsLiteral("Name")) {
    mName = value.get_nsString();
  } else if (name.EqualsLiteral("Path")) {
    MOZ_ASSERT(value.get_nsString().Length() > 0);
    mPath = value.get_nsString();
  } else if (name.EqualsLiteral("Address")) {
    mAddress = value.get_nsString();
  } else if (name.EqualsLiteral("Class")) {
    mClass = value.get_uint32_t();
  } else if (name.EqualsLiteral("Icon")) {
    mIcon = value.get_nsString();
  } else if (name.EqualsLiteral("Connected")) {
    mConnected = value.get_bool();
  } else if (name.EqualsLiteral("Paired")) {
    mPaired = value.get_bool();
  } else if (name.EqualsLiteral("UUIDs")) {
    mUuids = value.get_ArrayOfnsString();
    nsresult rv;
    nsIScriptContext* sc = GetContextForEventHandlers(&rv);
    NS_ENSURE_SUCCESS_VOID(rv);

    AutoPushJSContext cx(sc->GetNativeContext());

    JS::Rooted<JSObject*> uuids(cx);
    if (NS_FAILED(nsTArrayToJSArray(cx, mUuids, uuids.address()))) {
      NS_WARNING("Cannot set JS UUIDs object!");
      return;
    }
    mJsUuids = uuids;
    Root();
  } else if (name.EqualsLiteral("Services")) {
    mServices = value.get_ArrayOfnsString();
    nsresult rv;
    nsIScriptContext* sc = GetContextForEventHandlers(&rv);
    NS_ENSURE_SUCCESS_VOID(rv);

    AutoPushJSContext cx(sc->GetNativeContext());

    JS::Rooted<JSObject*> services(cx);
    if (NS_FAILED(nsTArrayToJSArray(cx, mServices, services.address()))) {
      NS_WARNING("Cannot set JS Services object!");
      return;
    }
    mJsServices = services;
    Root();
  } else {
    nsCString warningMsg;
    warningMsg.AssignLiteral("Not handling device property: ");
    warningMsg.Append(NS_ConvertUTF16toUTF8(name));
    NS_WARNING(warningMsg.get());
  }
}


already_AddRefed<BluetoothDevice>
BluetoothDevice::Create(nsPIDOMWindow* aWindow,
                        const nsAString& aAdapterPath,
                        const BluetoothValue& aValue)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWindow);

  nsRefPtr<BluetoothDevice> device =
    new BluetoothDevice(aWindow, aAdapterPath, aValue);
  return device.forget();
}

void
BluetoothDevice::Notify(const BluetoothSignal& aData)
{
  BT_LOG("[D] %s: %s", __FUNCTION__, NS_ConvertUTF16toUTF8(aData.name()).get());

  BluetoothValue v = aData.value();
  if (aData.name().EqualsLiteral("PropertyChanged")) {
    NS_ASSERTION(v.type() == BluetoothValue::TArrayOfBluetoothNamedValue,
                 "PropertyChanged: Invalid value type");
    const InfallibleTArray<BluetoothNamedValue>& arr =
      v.get_ArrayOfBluetoothNamedValue();

    NS_ASSERTION(arr.Length() == 1,
                 "Got more than one property in a change message!");
    SetPropertyByValue(arr[0]);
  } else {
#ifdef DEBUG
    nsCString warningMsg;
    warningMsg.AssignLiteral("Not handling device signal: ");
    warningMsg.Append(NS_ConvertUTF16toUTF8(aData.name()));
    NS_WARNING(warningMsg.get());
#endif
  }
}

JS::Value
BluetoothDevice::GetUuids(JSContext* aCx, ErrorResult& aRv)
{
  if (!mJsUuids) {
    NS_WARNING("UUIDs not yet set!\n");
    aRv.Throw(NS_ERROR_FAILURE);
    return JS::NullValue();
  }

  JS::ExposeObjectToActiveJS(mJsUuids);
  return JS::ObjectValue(*mJsUuids);
}

JS::Value
BluetoothDevice::GetServices(JSContext* aCx, ErrorResult& aRv)
{
  if (!mJsServices) {
    NS_WARNING("Services not yet set!\n");
    aRv.Throw(NS_ERROR_FAILURE);
    return JS::Value(JSVAL_NULL);
  }

  JS::ExposeObjectToActiveJS(mJsServices);
  return JS::ObjectValue(*mJsServices);
}

JSObject*
BluetoothDevice::WrapObject(JSContext* aContext, JS::Handle<JSObject*> aScope)
{
  return BluetoothDeviceBinding::Wrap(aContext, aScope, this);
}
