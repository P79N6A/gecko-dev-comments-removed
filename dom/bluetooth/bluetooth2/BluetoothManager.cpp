





#include "base/basictypes.h"
#include "BluetoothManager.h"
#include "BluetoothAdapter.h"
#include "BluetoothService.h"
#include "BluetoothReplyRunnable.h"

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

class GetAdaptersTask : public BluetoothReplyRunnable
{
 public:
  GetAdaptersTask(BluetoothManager* aManager)
    : BluetoothReplyRunnable(nullptr)
    , mManager(aManager)
  { }

  bool
  ParseSuccessfulReply(JS::MutableHandle<JS::Value> aValue)
  {
    











    
    const BluetoothValue& adaptersProperties =
      mReply->get_BluetoothReplySuccess().value();
    NS_ENSURE_TRUE(adaptersProperties.type() ==
                   BluetoothValue::TArrayOfBluetoothNamedValue, false);

    const InfallibleTArray<BluetoothNamedValue>& adaptersPropertiesArray =
      adaptersProperties.get_ArrayOfBluetoothNamedValue();
    BT_API2_LOGR("GetAdaptersTask: len[%d]", adaptersPropertiesArray.Length());

    
    uint32_t numAdapters = adaptersPropertiesArray.Length();
    for (uint32_t i = 0; i < numAdapters; i++) {
      MOZ_ASSERT(adaptersPropertiesArray[i].name().EqualsLiteral("Adapter"));

      const BluetoothValue& properties = adaptersPropertiesArray[i].value();
      mManager->AppendAdapter(properties);
    }

    aValue.setUndefined();
    return true;
  }

  virtual void
  ReleaseMembers() override
  {
    BluetoothReplyRunnable::ReleaseMembers();
    mManager = nullptr;
  }

private:
  nsRefPtr<BluetoothManager> mManager;
};

BluetoothManager::BluetoothManager(nsPIDOMWindow *aWindow)
  : DOMEventTargetHelper(aWindow)
  , mDefaultAdapterIndex(-1)
{
  MOZ_ASSERT(aWindow);

  ListenToBluetoothSignal(true);
  BT_API2_LOGR("aWindow %p", aWindow);

  
  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  nsRefPtr<BluetoothReplyRunnable> result = new GetAdaptersTask(this);
  NS_ENSURE_SUCCESS_VOID(bs->GetAdaptersInternal(result));
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
  BT_API2_LOGR("mDefaultAdapterIndex: %d", mDefaultAdapterIndex);

  return DefaultAdapterExists() ? mAdapters[mDefaultAdapterIndex] : nullptr;
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
  BT_API2_LOGR();

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

  uint32_t i;
  for (i = 0; i < mAdapters.Length(); i++) {
    nsString address;
    mAdapters[i]->GetAddress(address);
    if (address.Equals(addressToRemove)) {
      mAdapters.RemoveElementAt(i);
      break;
    }
  }

  
  BluetoothAdapterEventInit init;
  init.mAddress = addressToRemove;
  DispatchAdapterEvent(NS_LITERAL_STRING("adapterremoved"), init);

  
  if (mDefaultAdapterIndex == (int)i) {
    ReselectDefaultAdapter();
  }
}

void
BluetoothManager::ReselectDefaultAdapter()
{
  
  mDefaultAdapterIndex = mAdapters.IsEmpty() ? -1 : 0;
  BT_API2_LOGR("mAdapters length: %d => NEW mDefaultAdapterIndex: %d",
               mAdapters.Length(), mDefaultAdapterIndex);

  
  DispatchAttributeEvent();
}

void
BluetoothManager::DispatchAdapterEvent(const nsAString& aType,
                                       const BluetoothAdapterEventInit& aInit)
{
  BT_API2_LOGR("aType (%s)", NS_ConvertUTF16toUTF8(aType).get());

  nsRefPtr<BluetoothAdapterEvent> event =
    BluetoothAdapterEvent::Constructor(this, aType, aInit);
  DispatchTrustedEvent(event);
}

void
BluetoothManager::DispatchAttributeEvent()
{
  MOZ_ASSERT(NS_IsMainThread());
  BT_API2_LOGR();

  AutoJSAPI jsapi;
  NS_ENSURE_TRUE_VOID(jsapi.Init(GetOwner()));
  JSContext* cx = jsapi.cx();
  JS::Rooted<JS::Value> value(cx);

  nsTArray<nsString> types;
  BT_APPEND_ENUM_STRING(types,
                        BluetoothManagerAttribute,
                        BluetoothManagerAttribute::DefaultAdapter);

  if (!ToJSValue(cx, types, &value)) {
    JS_ClearPendingException(cx);
    return;
  }

  
  RootedDictionary<BluetoothAttributeEventInit> init(cx);
  init.mAttrs = value;
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
BluetoothManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return BluetoothManagerBinding::Wrap(aCx, this, aGivenProto);
}
