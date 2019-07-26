





#ifndef mozilla_dom_bluetooth_bluetoothutils_h__
#define mozilla_dom_bluetooth_bluetoothutils_h__

#include "BluetoothCommon.h"

class JSContext;
class JSObject;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothDevice;
class BluetoothNamedValue;

nsresult
StringArrayToJSArray(JSContext* aCx, JSObject* aGlobal,
                     const nsTArray<nsString>& aSourceArray,
                     JSObject** aResultArray);

nsresult
BluetoothDeviceArrayToJSArray(JSContext* aCx, JSObject* aGlobal,
                              const nsTArray<nsRefPtr<BluetoothDevice> >& aSourceArray,
                              JSObject** aResultArray);

bool
SetJsObject(JSContext* aContext,
            JSObject* aObj,
            const InfallibleTArray<BluetoothNamedValue>& aData);

nsString
GetObjectPathFromAddress(const nsAString& aAdapterPath,
                         const nsAString& aDeviceAddress);

nsString
GetAddressFromObjectPath(const nsAString& aObjectPath);

END_BLUETOOTH_NAMESPACE

#endif
