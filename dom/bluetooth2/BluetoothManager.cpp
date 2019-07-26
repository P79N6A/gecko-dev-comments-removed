





#include "base/basictypes.h"
#include "BluetoothManager.h"
#include "BluetoothCommon.h"
#include "BluetoothAdapter.h"
#include "BluetoothService.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/BluetoothManager2Binding.h"
#include "mozilla/Services.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfo.h"
#include "nsIPermissionManager.h"
#include "nsThreadUtils.h"

using namespace mozilla;

USING_BLUETOOTH_NAMESPACE


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BluetoothManager)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BluetoothManager, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BluetoothManager, DOMEventTargetHelper)

BluetoothManager::BluetoothManager(nsPIDOMWindow *aWindow)
  : DOMEventTargetHelper(aWindow)
  , mDefaultAdapterIndex(-1)
{
  MOZ_ASSERT(aWindow);
  MOZ_ASSERT(IsDOMBinding());

  ListenToBluetoothSignal(true);
}

BluetoothManager::~BluetoothManager()
{
  ListenToBluetoothSignal(false);
}

void
BluetoothManager::DisconnectFromOwner()
{
  DOMEventTargetHelper::DisconnectFromOwner();
  ListenToBluetoothSignal(false);
}

void
BluetoothManager::ListenToBluetoothSignal(bool aStart)
{
  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  if (aStart) {
    bs->RegisterBluetoothSignalHandler(NS_LITERAL_STRING(KEY_MANAGER), this);
  } else {
    bs->UnregisterBluetoothSignalHandler(NS_LITERAL_STRING(KEY_MANAGER), this);
  }
}

BluetoothAdapter*
BluetoothManager::GetDefaultAdapter()
{
  return (DefaultAdapterExists()) ? mAdapters[mDefaultAdapterIndex] : nullptr;
}

void
BluetoothManager::AppendAdapter(const BluetoothValue& aValue)
{
  MOZ_ASSERT(aValue.type() == BluetoothValue::TArrayOfBluetoothNamedValue);

  
  const InfallibleTArray<BluetoothNamedValue>& values =
    aValue.get_ArrayOfBluetoothNamedValue();
  nsRefPtr<BluetoothAdapter> adapter =
    BluetoothAdapter::Create(GetOwner(), values);

  mAdapters.AppendElement(adapter);

  
  if (!DefaultAdapterExists()) {
    MOZ_ASSERT(mAdapters.Length() == 1);
    ReselectDefaultAdapter();
  }
}

void
BluetoothManager::GetAdapters(nsTArray<nsRefPtr<BluetoothAdapter> >& aAdapters)
{
  aAdapters = mAdapters;
}


already_AddRefed<BluetoothManager>
BluetoothManager::Create(nsPIDOMWindow* aWindow)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWindow);

  nsRefPtr<BluetoothManager> manager = new BluetoothManager(aWindow);
  return manager.forget();
}

void
BluetoothManager::HandleAdapterAdded(const BluetoothValue& aValue)
{
  MOZ_ASSERT(aValue.type() == BluetoothValue::TArrayOfBluetoothNamedValue);

  AppendAdapter(aValue);

  
  BluetoothAdapterEventInit init;
  init.mAdapter = mAdapters.LastElement();
  DispatchAdapterEvent(NS_LITERAL_STRING("adapteradded"), init);
}

void
BluetoothManager::HandleAdapterRemoved(const BluetoothValue& aValue)
{
  MOZ_ASSERT(aValue.type() == BluetoothValue::TnsString);
  MOZ_ASSERT(DefaultAdapterExists());

  
  nsString addressToRemove = aValue.get_nsString();

  uint32_t numAdapters = mAdapters.Length();
  for (uint32_t i = 0; i < numAdapters; i++) {
    nsString address;
    mAdapters[i]->GetAddress(address);
    if (address.Equals(addressToRemove)) {
      mAdapters.RemoveElementAt(i);

      if (mDefaultAdapterIndex == (int)i) {
        ReselectDefaultAdapter();
      }
      break;
    }
  }

  
  BluetoothAdapterEventInit init;
  init.mAddress = addressToRemove;
  DispatchAdapterEvent(NS_LITERAL_STRING("adapterremoved"), init);
}

void
BluetoothManager::ReselectDefaultAdapter()
{
  
  mDefaultAdapterIndex = mAdapters.IsEmpty() ? -1 : 0;

  
  DispatchAttributeEvent();
}

void
BluetoothManager::DispatchAdapterEvent(const nsAString& aType,
                                       const BluetoothAdapterEventInit& aInit)
{
  nsRefPtr<BluetoothAdapterEvent> event =
    BluetoothAdapterEvent::Constructor(this, aType, aInit);

  DispatchTrustedEvent(event);
}

void
BluetoothManager::DispatchAttributeEvent()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  AutoJSContext cx;
  JS::Rooted<JS::Value> value(cx, JS::NullValue());
  if (DefaultAdapterExists()) {
    BluetoothAdapter* adapter = mAdapters[mDefaultAdapterIndex];
    nsCOMPtr<nsIGlobalObject> global =
      do_QueryInterface(adapter->GetParentObject());
    NS_ENSURE_TRUE_VOID(global);

    JS::Rooted<JSObject*> scope(cx, global->GetGlobalJSObject());
    NS_ENSURE_TRUE_VOID(scope);

    JSAutoCompartment ac(cx, scope);
    if (!ToJSValue(cx, adapter, &value)) {
      JS_ClearPendingException(cx);
      return;
    }
  }

  
  RootedDictionary<BluetoothAttributeEventInit> init(cx);
  init.mAttr = (uint16_t)BluetoothManagerAttribute::DefaultAdapter;
  init.mValue = value;
  nsRefPtr<BluetoothAttributeEvent> event =
    BluetoothAttributeEvent::Constructor(this,
                                         NS_LITERAL_STRING("attributechanged"),
                                         init);
  DispatchTrustedEvent(event);
}

void
BluetoothManager::Notify(const BluetoothSignal& aData)
{
  BT_LOGD("[M] %s", NS_ConvertUTF16toUTF8(aData.name()).get());

  if (aData.name().EqualsLiteral("AdapterAdded")) {
    HandleAdapterAdded(aData.value());
  } else if (aData.name().EqualsLiteral("AdapterRemoved")) {
    HandleAdapterRemoved(aData.value());
  } else {
    BT_WARNING("Not handling manager signal: %s",
               NS_ConvertUTF16toUTF8(aData.name()).get());
  }
}

JSObject*
BluetoothManager::WrapObject(JSContext* aCx)
{
  return BluetoothManagerBinding::Wrap(aCx, this);
}
