





#include "base/basictypes.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothUtils.h"
#include "jsapi.h"
#include "mozilla/Scoped.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "nsContentUtils.h"
#include "nsISystemMessagesInternal.h"
#include "nsString.h"
#include "nsTArray.h"

BEGIN_BLUETOOTH_NAMESPACE

bool
SetJsObject(JSContext* aContext,
            JSObject* aObj,
            const InfallibleTArray<BluetoothNamedValue>& aData)
{
  for (uint32_t i = 0; i < aData.Length(); i++) {
    jsval v;
    if (aData[i].value().type() == BluetoothValue::TnsString) {
      nsString data = aData[i].value().get_nsString();
      JSString* JsData = JS_NewUCStringCopyN(aContext,
                                             data.BeginReading(),
                                             data.Length());
      NS_ENSURE_TRUE(JsData, false);
      v = STRING_TO_JSVAL(JsData);
    } else if (aData[i].value().type() == BluetoothValue::Tuint32_t) {
      int data = aData[i].value().get_uint32_t();
      v = INT_TO_JSVAL(data);
    } else if (aData[i].value().type() == BluetoothValue::Tbool) {
      bool data = aData[i].value().get_bool();
      v = BOOLEAN_TO_JSVAL(data);
    } else {
      NS_WARNING("SetJsObject: Parameter is not handled");
    }

    if (!JS_SetProperty(aContext, aObj,
                        NS_ConvertUTF16toUTF8(aData[i].name()).get(),
                        &v)) {
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

  if (!SetJsObject(cx, obj, aData)) {
    NS_WARNING("Failed to set properties of system message!");
    return false;
  }

  nsCOMPtr<nsISystemMessagesInternal> systemMessenger =
    do_GetService("@mozilla.org/system-message-internal;1");

  if (!systemMessenger) {
    NS_WARNING("Failed to get SystemMessenger service!");
    return false;
  }

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
