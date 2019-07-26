





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
  void ReceiveSocketData(nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage)
    MOZ_OVERRIDE;

  bool Connect(const nsAString& aDeviceObjectPath);
  void Disconnect();
  bool Listen();

private:
  friend class BluetoothScoManagerObserver;
  BluetoothScoManager();
  bool Init();
  void Cleanup();
  nsresult HandleShutdown();
  void NotifyAudioManager(const nsAString& aAddress);
  virtual void OnConnectSuccess() MOZ_OVERRIDE;
  virtual void OnConnectError() MOZ_OVERRIDE;
  virtual void OnDisconnect() MOZ_OVERRIDE;

  int mSocketStatus;
};

END_BLUETOOTH_NAMESPACE

#endif
