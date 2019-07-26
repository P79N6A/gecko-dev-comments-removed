





#ifndef mozilla_dom_bluetooth_bluetoothutils_h__
#define mozilla_dom_bluetooth_bluetoothutils_h__

#include "BluetoothCommon.h"

class JSContext;
class JSObject;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothDevice;
class BluetoothNamedValue;

bool
SetJsObject(JSContext* aContext,
            JSObject* aObj,
            const InfallibleTArray<BluetoothNamedValue>& aData);

nsString
GetObjectPathFromAddress(const nsAString& aAdapterPath,
                         const nsAString& aDeviceAddress);

nsString
GetAddressFromObjectPath(const nsAString& aObjectPath);

bool
BroadcastSystemMessage(const nsAString& aType,
                       const InfallibleTArray<BluetoothNamedValue>& aData);

END_BLUETOOTH_NAMESPACE

#endif
