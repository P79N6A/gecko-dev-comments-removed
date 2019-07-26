





#ifndef mozilla_dom_bluetooth_bluetoothoppmanager_h__
#define mozilla_dom_bluetooth_bluetoothoppmanager_h__

#include "BluetoothCommon.h"
#include "mozilla/dom/ipc/Blob.h"
#include "mozilla/ipc/UnixSocket.h"
#include "nsIDOMFile.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;

class BluetoothOppManager : public mozilla::ipc::UnixSocketConsumer
{
public:
  




  static const int DEFAULT_OPP_CHANNEL = 10;
  static const int MAX_PACKET_LENGTH = 0xFFFE;

  ~BluetoothOppManager();
  static BluetoothOppManager* Get();
  void ReceiveSocketData(mozilla::ipc::UnixSocketRawData* aMessage);

  










  bool Connect(const nsAString& aDeviceObjectPath,
               BluetoothReplyRunnable* aRunnable);
  void Disconnect();

  bool SendFile(BlobParent* aBlob,
                BluetoothReplyRunnable* aRunnable);

  bool StopSendingFile(BluetoothReplyRunnable* aRunnable);

  
  void SendConnectRequest();
  void SendPutHeaderRequest(const nsAString& aFileName, int aFileSize);
  void SendPutRequest(uint8_t* aFileBody, int aFileBodyLength,
                      bool aFinal);
  void SendDisconnectRequest();

private:
  BluetoothOppManager();

  bool mConnected;
  int mConnectionId;
  int mLastCommand;
  uint8_t mRemoteObexVersion;
  uint8_t mRemoteConnectionFlags;
  int mRemoteMaxPacketLength;

  nsCOMPtr<nsIDOMBlob> mBlob;
};

END_BLUETOOTH_NAMESPACE

#endif
