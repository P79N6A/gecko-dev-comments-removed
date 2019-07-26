





#include "base/basictypes.h"

#include "BluetoothDevice.h"
#include "BluetoothUtils.h"
#include "jsapi.h"
#include "mozilla/Scoped.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "nsContentUtils.h"
#include "nsISystemMessagesInternal.h"
#include "nsTArray.h"
#include "nsString.h"

USING_BLUETOOTH_NAMESPACE

nsresult
mozilla::dom::bluetooth::StringArrayToJSArray(JSContext* aCx, JSObject* aGlobal,
                                              const nsTArray<nsString>& aSourceArray,
                                              JSObject** aResultArray)
{
  NS_ASSERTION(aCx, "Null context!");
  NS_ASSERTION(aGlobal, "Null global!");

  JSAutoRequest ar(aCx);
  JSAutoCompartment ac(aCx, aGlobal);

  JSObject* arrayObj;

  if (aSourceArray.IsEmpty()) {
    arrayObj = JS_NewArrayObject(aCx, 0, nullptr);
  } else {
    uint32_t valLength = aSourceArray.Length();
    mozilla::ScopedDeleteArray<jsval> valArray(new jsval[valLength]);
    JS::AutoArrayRooter tvr(aCx, 0, valArray);
    for (uint32_t index = 0; index < valLength; index++) {
      JSString* s = JS_NewUCStringCopyN(aCx, aSourceArray[index].BeginReading(),
                                        aSourceArray[index].Length());
      if(!s) {
        NS_WARNING("Memory allocation error!");
        return NS_ERROR_OUT_OF_MEMORY;
      }
      valArray[index] = STRING_TO_JSVAL(s);
      tvr.changeLength(index + 1);
    }
    arrayObj = JS_NewArrayObject(aCx, valLength, valArray);
  }

  if (!arrayObj) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  if (!JS_FreezeObject(aCx, arrayObj)) {
    return NS_ERROR_FAILURE;
  }

  *aResultArray = arrayObj;
  return NS_OK;
}

nsresult
mozilla::dom::bluetooth::BluetoothDeviceArrayToJSArray(JSContext* aCx, JSObject* aGlobal,
                                                       const nsTArray<nsRefPtr<BluetoothDevice> >& aSourceArray,
                                                       JSObject** aResultArray)
{
  NS_ASSERTION(aCx, "Null context!");
  NS_ASSERTION(aGlobal, "Null global!");

  JSAutoRequest ar(aCx);
  JSAutoCompartment ac(aCx, aGlobal);

  JSObject* arrayObj;

  if (aSourceArray.IsEmpty()) {
    arrayObj = JS_NewArrayObject(aCx, 0, nullptr);
  } else {
    uint32_t valLength = aSourceArray.Length();
    mozilla::ScopedDeleteArray<jsval> valArray(new jsval[valLength]);
    JS::AutoArrayRooter tvr(aCx, 0, valArray);
    for (uint32_t index = 0; index < valLength; index++) {
      nsISupports* obj = aSourceArray[index]->ToISupports();
      nsresult rv =
        nsContentUtils::WrapNative(aCx, aGlobal, obj, &valArray[index]);
      NS_ENSURE_SUCCESS(rv, rv);
      tvr.changeLength(index + 1);
    }
    arrayObj = JS_NewArrayObject(aCx, valLength, valArray);
  }

  if (!arrayObj) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!JS_FreezeObject(aCx, arrayObj)) {
    return NS_ERROR_FAILURE;
  }

  *aResultArray = arrayObj;
  return NS_OK;
}

bool
mozilla::dom::bluetooth::SetJsObject(JSContext* aContext,
                                     JSObject* aObj,
                                     const InfallibleTArray<BluetoothNamedValue>& aData)
{
  for (uint32_t i = 0; i < aData.Length(); i++) {
    jsval v;
    if (aData[i].value().type() == BluetoothValue::TnsString) {
      nsString data = aData[i].value().get_nsString();
      JSString* JsData = JS_NewStringCopyN(aContext,
                                           NS_ConvertUTF16toUTF8(data).get(),
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
mozilla::dom::bluetooth::GetObjectPathFromAddress(const nsAString& aAdapterPath,
                                                  const nsAString& aDeviceAddress)
{
  
  
  
  nsString devicePath(aAdapterPath);
  devicePath.AppendLiteral("/dev_");
  devicePath.Append(aDeviceAddress);
  devicePath.ReplaceChar(':', '_');
  return devicePath;
}

nsString
mozilla::dom::bluetooth::GetAddressFromObjectPath(const nsAString& aObjectPath)
{
  
  
  
  nsString address(aObjectPath);
  int addressHead = address.RFind("/") + 5;

  MOZ_ASSERT(addressHead + BLUETOOTH_ADDRESS_LENGTH == address.Length());

  address.Cut(0, addressHead);
  address.ReplaceChar('_', ':');

  return address;
}

bool
mozilla::dom::bluetooth::BroadcastSystemMessage(
  const nsAString& aType,
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

