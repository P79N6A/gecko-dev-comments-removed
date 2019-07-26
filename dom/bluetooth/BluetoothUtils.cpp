





#include "base/basictypes.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothUtils.h"
#include "jsapi.h"
#include "mozilla/Scoped.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "nsContentUtils.h"
#include "nsIScriptContext.h"
#include "nsISystemMessagesInternal.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsTArrayHelpers.h"

BEGIN_BLUETOOTH_NAMESPACE

bool
SetJsObject(JSContext* aContext,
            const BluetoothValue& aValue,
            JSObject* aObj)
{
  MOZ_ASSERT(aContext && aObj);

  if (aValue.type() == BluetoothValue::TArrayOfnsString) {
    const nsTArray<nsString>& sourceArray = aValue.get_ArrayOfnsString();
    if (NS_FAILED(nsTArrayToJSArray(aContext, sourceArray, &aObj))) {
      NS_WARNING("Cannot set JS UUIDs object!");
      return false;
    }
  } else if (aValue.type() == BluetoothValue::TArrayOfBluetoothNamedValue) {
    const nsTArray<BluetoothNamedValue>& arr =
      aValue.get_ArrayOfBluetoothNamedValue();

    for (uint32_t i = 0; i < arr.Length(); i++) {
      jsval val;
      const BluetoothValue& v = arr[i].value();
      JSString* JsData;

      switch(v.type()) {
        case BluetoothValue::TnsString:
          JsData =
            JS_NewStringCopyN(aContext,
                              NS_ConvertUTF16toUTF8(v.get_nsString()).get(),
                              v.get_nsString().Length());
          NS_ENSURE_TRUE(JsData, NS_ERROR_FAILURE);
          val = STRING_TO_JSVAL(JsData);
          break;
        case BluetoothValue::Tuint32_t:
          val = INT_TO_JSVAL(v.get_uint32_t());
          break;
        case BluetoothValue::Tbool:
          val = BOOLEAN_TO_JSVAL(v.get_bool());
          break;
        default:
          NS_WARNING("SetJsObject: Parameter is not handled");
          break;
      }

      if (!JS_SetProperty(aContext, aObj,
                          NS_ConvertUTF16toUTF8(arr[i].name()).get(),
                          &val)) {
        NS_WARNING("Failed to set property");
        return NS_ERROR_FAILURE;
      }
    }
  } else {
    NS_WARNING("Not handle the type of BluetoothValue!");
    return false;
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

  MOZ_ASSERT(addressHead + BLUETOOTH_ADDRESS_LENGTH == address.Length());

  address.Cut(0, addressHead);
  address.ReplaceChar('_', ':');

  return address;
}

bool
BroadcastSystemMessage(const nsAString& aType,
                       const InfallibleTArray<BluetoothNamedValue>& aData)
{
  JSContext* cx = nsContentUtils::GetSafeJSContext();
  NS_ASSERTION(!::JS_IsExceptionPending(cx),
      "Shouldn't get here when an exception is pending!");

  JSAutoRequest jsar(cx);
  JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
  if (!obj) {
    NS_WARNING("Failed to new JSObject for system message!");
    return false;
  }

  if (!SetJsObject(cx, aData, obj)) {
    NS_WARNING("Failed to set properties of system message!");
    return false;
  }

  nsCOMPtr<nsISystemMessagesInternal> systemMessenger =
    do_GetService("@mozilla.org/system-message-internal;1");
  NS_ENSURE_TRUE(systemMessenger, false);

  systemMessenger->BroadcastMessage(aType, OBJECT_TO_JSVAL(obj));

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
    NS_WARNING("Failed to dispatch to main thread!");
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

END_BLUETOOTH_NAMESPACE
