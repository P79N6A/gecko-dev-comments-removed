





#ifndef mozilla_dom_bluetooth_bluetoothutils_h__
#define mozilla_dom_bluetooth_bluetoothutils_h__

#include "BluetoothCommon.h"

class JSContext;
class JSObject;

BEGIN_BLUETOOTH_NAMESPACE

nsresult
StringArrayToJSArray(JSContext* aCx, JSObject* aGlobal,
                     const nsTArray<nsString>& aSourceArray,
                     JSObject** aResultArray);

END_BLUETOOTH_NAMESPACE

#endif
