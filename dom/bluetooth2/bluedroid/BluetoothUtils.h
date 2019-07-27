





#ifndef mozilla_dom_bluetooth_bluetoothutils_h__
#define mozilla_dom_bluetooth_bluetoothutils_h__

#include "BluetoothCommon.h"
#include "js/TypeDecls.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothNamedValue;
class BluetoothValue;
class BluetoothReplyRunnable;

void
UuidToString(const BluetoothUuid& aUuid, nsAString& aString);

bool
SetJsObject(JSContext* aContext,
            const BluetoothValue& aValue,
            JS::Handle<JSObject*> aObj);

bool
BroadcastSystemMessage(const nsAString& aType,
                       const BluetoothValue& aData);
















void
DispatchBluetoothReply(BluetoothReplyRunnable* aRunnable,
                       const BluetoothValue& aValue,
                       const nsAString& aErrorStr);



















void
DispatchBluetoothReply(BluetoothReplyRunnable* aRunnable,
                       const BluetoothValue& aValue,
                       const enum BluetoothStatus aStatusCode);

void
DispatchStatusChangedEvent(const nsAString& aType,
                           const nsAString& aDeviceAddress,
                           bool aStatus);


END_BLUETOOTH_NAMESPACE

#endif
