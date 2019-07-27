





#ifndef mozilla_dom_bluetooth_bluetoothutils_h
#define mozilla_dom_bluetooth_bluetoothutils_h

#include "BluetoothCommon.h"
#include "js/TypeDecls.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothNamedValue;
class BluetoothReplyRunnable;
class BluetoothValue;











void
UuidToString(const BluetoothUuid& aUuid, nsAString& aString);











void
ReversedUuidToString(const BluetoothUuid& aUuid, nsAString& aString);







void
StringToUuid(const char* aString, BluetoothUuid& aUuid);






void
GenerateUuid(nsAString &aUuidString);












void
GeneratePathFromGattId(const BluetoothGattId& aId,
                       nsAString& aPath,
                       nsAString& aUuidStr);







void
GeneratePathFromGattId(const BluetoothGattId& aId,
                       nsAString& aPath);












void
RegisterBluetoothSignalHandler(const nsAString& aPath,
                               BluetoothSignalObserver* aHandler);








void
UnregisterBluetoothSignalHandler(const nsAString& aPath,
                                 BluetoothSignalObserver* aHandler);





bool
BroadcastSystemMessage(const nsAString& aType,
                       const BluetoothValue& aData);

bool
BroadcastSystemMessage(const nsAString& aType,
                       const InfallibleTArray<BluetoothNamedValue>& aData);










void
DispatchReplySuccess(BluetoothReplyRunnable* aRunnable);







void
DispatchReplySuccess(BluetoothReplyRunnable* aRunnable,
                     const BluetoothValue& aValue);














void
DispatchReplyError(BluetoothReplyRunnable* aRunnable,
                   const nsAString& aErrorStr);












void
DispatchReplyError(BluetoothReplyRunnable* aRunnable,
                   const enum BluetoothStatus aStatus);

void
DispatchStatusChangedEvent(const nsAString& aType,
                           const nsAString& aDeviceAddress,
                           bool aStatus);






bool
IsMainProcess();

END_BLUETOOTH_NAMESPACE

#endif
