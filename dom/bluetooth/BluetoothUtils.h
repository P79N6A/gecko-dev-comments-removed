





#ifndef mozilla_dom_bluetooth_bluetoothutils_h__
#define mozilla_dom_bluetooth_bluetoothutils_h__

#include "BluetoothCommon.h"

BEGIN_BLUETOOTH_NAMESPACE




















nsresult RegisterBluetoothEventHandler(const nsCString& aNodeName,
                                       BluetoothEventObserver *aMsgHandler);











nsresult UnregisterBluetoothEventHandler(const nsCString& aNodeName,
                                         BluetoothEventObserver *aMsgHandler);







nsresult GetDefaultAdapterPathInternal(nsCString& aAdapterPath);








nsresult StartBluetoothConnection();








nsresult StopBluetoothConnection();

END_BLUETOOTH_NAMESPACE

#endif
