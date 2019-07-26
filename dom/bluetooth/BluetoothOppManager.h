





#ifndef mozilla_dom_bluetooth_bluetoothoppmanager_h__
#define mozilla_dom_bluetooth_bluetoothoppmanager_h__

#include "BluetoothCommon.h"
#include "mozilla/ipc/UnixSocket.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;
class BlobParent;

class BluetoothOppManager : public mozilla::ipc::UnixSocketConsumer
{
public:
  




  static const int DEFAULT_OPP_CHANNEL = 10;

  ~BluetoothOppManager();
  static BluetoothOppManager* Get();
  void ReceiveSocketData(mozilla::ipc::UnixSocketRawData* aMessage);

  










  bool Connect(const nsAString& aDeviceObjectPath,
               BluetoothReplyRunnable* aRunnable);
  void Disconnect();

  bool SendFile(BlobParent* aBlob,
                BluetoothReplyRunnable* aRunnable);

  bool StopSendingFile(BluetoothReplyRunnable* aRunnable);

private:
  BluetoothOppManager();
};

END_BLUETOOTH_NAMESPACE

#endif
