





#ifndef mozilla_dom_bluetooth_bluetoothscomanager_h__
#define mozilla_dom_bluetooth_bluetoothscomanager_h__

#include "BluetoothCommon.h"
#include "mozilla/ipc/UnixSocket.h"
#include "nsIObserver.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;
class BluetoothScoManagerObserver;

class BluetoothScoManager : public mozilla::ipc::UnixSocketConsumer
{
public:
  ~BluetoothScoManager();

  static BluetoothScoManager* Get();
  void ReceiveSocketData(mozilla::ipc::UnixSocketRawData* aMessage)
    MOZ_OVERRIDE;

  bool Connect(const nsAString& aDeviceObjectPath);
  void Disconnect();
  void SetConnected(bool aConnected);
  bool GetConnected();

private:
  friend class BluetoothScoManagerObserver;
  BluetoothScoManager();
  bool Init();
  void Cleanup();
  nsresult HandleShutdown();
  void CreateScoSocket(const nsAString& aDevicePath);
  virtual void OnConnectSuccess() MOZ_OVERRIDE;
  virtual void OnConnectError() MOZ_OVERRIDE;
  bool mConnected;
};

END_BLUETOOTH_NAMESPACE

#endif
