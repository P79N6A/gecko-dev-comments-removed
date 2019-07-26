





#ifndef mozilla_dom_bluetooth_bluetoothscomanager_h__
#define mozilla_dom_bluetooth_bluetoothscomanager_h__

#include "BluetoothCommon.h"
#include "mozilla/ipc/UnixSocket.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;

class BluetoothScoManager : public mozilla::ipc::UnixSocketConsumer
{
public:
  ~BluetoothScoManager();

  static BluetoothScoManager* Get();
  void ReceiveSocketData(mozilla::ipc::UnixSocketRawData* aMessage);

  bool Connect(const nsAString& aDeviceObjectPath);
  void Disconnect();
  void SetConnected(bool aConnected);
  bool GetConnected();

private:
  BluetoothScoManager();
  void CreateScoSocket(const nsAString& aDevicePath);
  bool mConnected;
};

END_BLUETOOTH_NAMESPACE

#endif
