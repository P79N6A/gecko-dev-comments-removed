





#ifndef mozilla_dom_bluetooth_bluetoothoppmanager_h__
#define mozilla_dom_bluetooth_bluetoothoppmanager_h__

#include "BluetoothCommon.h"
#include "mozilla/dom/ipc/Blob.h"
#include "mozilla/ipc/UnixSocket.h"

class nsIOutputStream;
class nsIInputStream;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;

class BluetoothOppManager : public mozilla::ipc::UnixSocketConsumer
{
public:
  




  static const int DEFAULT_OPP_CHANNEL = 10;
  static const int MAX_PACKET_LENGTH = 0xFFFE;

  ~BluetoothOppManager();
  static BluetoothOppManager* Get();
  void ReceiveSocketData(mozilla::ipc::UnixSocketRawData* aMessage)
    MOZ_OVERRIDE;

  










  bool Connect(const nsAString& aDeviceObjectPath,
               BluetoothReplyRunnable* aRunnable);
  void Disconnect();
  bool Listen();

  bool SendFile(BlobParent* aBlob);
  bool StopSendingFile();
  bool ConfirmReceivingFile(bool aConfirm);

  void SendConnectRequest();
  void SendPutHeaderRequest(const nsAString& aFileName, int aFileSize);
  void SendPutRequest(uint8_t* aFileBody, int aFileBodyLength,
                      bool aFinal);
  void SendDisconnectRequest();
  void SendAbortRequest();

  nsresult HandleShutdown();
private:
  BluetoothOppManager();
  void StartFileTransfer(const nsString& aDeviceAddress,
                         bool aReceived,
                         const nsString& aFileName,
                         uint32_t aFileLength,
                         const nsString& aContentType);
  void FileTransferComplete(const nsString& aDeviceAddress,
                            bool aSuccess,
                            bool aReceived,
                            const nsString& aFileName,
                            uint32_t aFileLength,
                            const nsString& aContentType);
  void UpdateProgress(const nsString& aDeviceAddress,
                      bool aReceived,
                      uint32_t aProcessedLength,
                      uint32_t aFileLength);
  void ReceivingFileConfirmation(const nsString& aAddress,
                                 const nsString& aFileName,
                                 uint32_t aFileLength,
                                 const nsString& aContentType);
  bool CreateFile();
  bool WriteToFile(const uint8_t* aData, int aDataLength);
  void DeleteReceivedFile();
  void ReplyToConnect();
  void ReplyToDisconnect();
  void ReplyToPut(bool aFinal, bool aContinue);
  void AfterOppConnected();
  void AfterOppDisconnected();
  virtual void OnConnectSuccess() MOZ_OVERRIDE;
  virtual void OnConnectError() MOZ_OVERRIDE;
  virtual void OnDisconnect() MOZ_OVERRIDE;

  bool mConnected;
  int mConnectionId;
  int mLastCommand;
  uint8_t mRemoteObexVersion;
  uint8_t mRemoteConnectionFlags;
  int mRemoteMaxPacketLength;
  bool mAbortFlag;
  int mPacketLeftLength;
  int mBodySegmentLength;
  int mReceivedDataBufferOffset;
  nsString mConnectedDeviceAddress;
  bool mPutFinal;
  bool mWaitingForConfirmationFlag;
  int mUpdateProgressCounter;
  enum mozilla::ipc::SocketConnectionStatus mSocketStatus;

  nsAutoPtr<uint8_t> mBodySegment;
  nsAutoPtr<uint8_t> mReceivedDataBuffer;

  nsCOMPtr<nsIDOMBlob> mBlob;
  nsCOMPtr<nsIThread> mReadFileThread;
  nsCOMPtr<nsIOutputStream> mOutputStream;
  nsCOMPtr<nsIInputStream> mInputStream;
};

END_BLUETOOTH_NAMESPACE

#endif
