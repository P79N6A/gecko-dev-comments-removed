





#include "base/basictypes.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothUtils.h"
#include "jsapi.h"
#include "mozilla/Scoped.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "nsContentUtils.h"
#include "nsIScriptContext.h"
#include "nsISystemMessagesInternal.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsServiceManagerUtils.h"

BEGIN_BLUETOOTH_NAMESPACE

bool
SetJsObject(JSContext* aContext,
            const BluetoothValue& aValue,
            JS::Handle<JSObject*> aObj)
{
  MOZ_ASSERT(aContext && aObj);

  if (aValue.type() != BluetoothValue::TArrayOfBluetoothNamedValue) {
    BT_WARNING("SetJsObject: Invalid parameter type");
    return false;
  }

  const nsTArray<BluetoothNamedValue>& arr =
    aValue.get_ArrayOfBluetoothNamedValue();

  for (uint32_t i = 0; i < arr.Length(); i++) {
    JS::Rooted<JS::Value> val(aContext);
    const BluetoothValue& v = arr[i].value();

    switch(v.type()) {
       case BluetoothValue::TnsString: {
        JSString* jsData = JS_NewUCStringCopyN(aContext,
                                     v.get_nsString().BeginReading(),
                                     v.get_nsString().Length());
        NS_ENSURE_TRUE(jsData, false);
        val = STRING_TO_JSVAL(jsData);
        break;
      }
      case BluetoothValue::Tuint32_t:
        val = INT_TO_JSVAL(v.get_uint32_t());
        break;
      case BluetoothValue::Tbool:
        val = BOOLEAN_TO_JSVAL(v.get_bool());
        break;
      default:
        BT_WARNING("SetJsObject: Parameter is not handled");
        break;
    }

    if (!JS_SetProperty(aContext, aObj,
                        NS_ConvertUTF16toUTF8(arr[i].name()).get(),
                        val)) {
      BT_WARNING("Failed to set property");
      return false;
    }
  }

  return true;
}

nsString
GetObjectPathFromAddress(const nsAString& aAdapterPath,
                         const nsAString& aDeviceAddress)
{
  
  
  
  nsString devicePath(aAdapterPath);
  devicePath.AppendLiteral("/dev_");
  devicePath.Append(aDeviceAddress);
  devicePath.ReplaceChar(':', '_');
  return devicePath;
}

nsString
GetAddressFromObjectPath(const nsAString& aObjectPath)
{
  
  
  
  nsString address(aObjectPath);
  int addressHead = address.RFind("/") + 5;

  MOZ_ASSERT(addressHead + BLUETOOTH_ADDRESS_LENGTH == (int)address.Length());

  address.Cut(0, addressHead);
  address.ReplaceChar('_', ':');

  return address;
}

bool
BroadcastSystemMessage(const nsAString& aType,
                       const InfallibleTArray<BluetoothNamedValue>& aData)
{
  mozilla::AutoSafeJSContext cx;
  NS_ASSERTION(!::JS_IsExceptionPending(cx),
      "Shouldn't get here when an exception is pending!");

  JS::Rooted<JSObject*> obj(cx, JS_NewPlainObject(cx));
  if (!obj) {
    BT_WARNING("Failed to new JSObject for system message!");
    return false;
  }

  if (!SetJsObject(cx, aData, obj)) {
    BT_WARNING("Failed to set properties of system message!");
    return false;
  }

  nsCOMPtr<nsISystemMessagesInternal> systemMessenger =
    do_GetService("@mozilla.org/system-message-internal;1");
  NS_ENSURE_TRUE(systemMessenger, false);

  JS::Rooted<JS::Value> value(cx, JS::ObjectValue(*obj));
  systemMessenger->BroadcastMessage(aType, value,
                                    JS::UndefinedHandleValue);

  return true;
}

void
DispatchBluetoothReply(BluetoothReplyRunnable* aRunnable,
                       const BluetoothValue& aValue,
                       const nsAString& aErrorStr)
{
  
  BluetoothReply* reply;
  if (!aErrorStr.IsEmpty()) {
    nsString err(aErrorStr);
    reply = new BluetoothReply(BluetoothReplyError(err));
  } else {
    MOZ_ASSERT(aValue.type() != BluetoothValue::T__None);
    reply = new BluetoothReply(BluetoothReplySuccess(aValue));
  }

  aRunnable->SetReply(reply);
  if (NS_FAILED(NS_DispatchToMainThread(aRunnable))) {
    BT_WARNING("Failed to dispatch to main thread!");
  }
}

void
ParseAtCommand(const nsACString& aAtCommand, const int aStart,
               nsTArray<nsCString>& aRetValues)
{
  int length = aAtCommand.Length();
  int begin = aStart;

  for (int i = aStart; i < length; ++i) {
    
    if (aAtCommand[i] == ',') {
      nsCString tmp(nsDependentCSubstring(aAtCommand, begin, i - begin));
      aRetValues.AppendElement(tmp);

      begin = i + 1;
    }
  }

  nsCString tmp(nsDependentCSubstring(aAtCommand, begin));
  aRetValues.AppendElement(tmp);
}

void
DispatchStatusChangedEvent(const nsAString& aType,
                           const nsAString& aAddress,
                           bool aStatus)
{
  MOZ_ASSERT(NS_IsMainThread());

  InfallibleTArray<BluetoothNamedValue> data;
  data.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("address"), nsString(aAddress)));
  data.AppendElement(
    BluetoothNamedValue(NS_LITERAL_STRING("status"), aStatus));

  BluetoothSignal signal(nsString(aType), NS_LITERAL_STRING(KEY_ADAPTER), data);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);
  bs->DistributeSignal(signal);
}

END_BLUETOOTH_NAMESPACE
