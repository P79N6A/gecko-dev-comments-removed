





#ifndef mozilla_dom_bluetooth_bluetoothoppmanager_h__
#define mozilla_dom_bluetooth_bluetoothoppmanager_h__

#include "BluetoothCommon.h"
#include "mozilla/dom/ipc/Blob.h"
#include "mozilla/ipc/UnixSocket.h"
#include "DeviceStorage.h"

class nsIOutputStream;
class nsIInputStream;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;
class ObexHeaderSet;

class BluetoothOppManager : public mozilla::ipc::UnixSocketConsumer
{
public:
  




  static const int DEFAULT_OPP_CHANNEL = 10;
  static const int MAX_PACKET_LENGTH = 0xFFFE;

  ~BluetoothOppManager();
  static BluetoothOppManager* Get();
  void ReceiveSocketData(nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage)
    MOZ_OVERRIDE;
  void ClientDataHandler(mozilla::ipc::UnixSocketRawData* aMessage);
  void ServerDataHandler(mozilla::ipc::UnixSocketRawData* aMessage);

  










  bool Connect(const nsAString& aDeviceObjectPath,
               BluetoothReplyRunnable* aRunnable);
  void Disconnect();
  bool Listen();

  bool SendFile(BlobParent* aBlob);
  bool StopSendingFile();
  bool ConfirmReceivingFile(bool aConfirm);

  void SendConnectRequest();
  void SendPutHeaderRequest(const nsAString& aFileName, int aFileSize);
  void SendPutRequest(uint8_t* aFileBody, int aFileBodyLength);
  void SendPutFinalRequest();
  void SendDisconnectRequest();
  void SendAbortRequest();

  void ExtractPacketHeaders(const ObexHeaderSet& aHeader);
  bool ExtractBlobHeaders();
  nsresult HandleShutdown();

  
  
  bool IsTransferring();
private:
  BluetoothOppManager();
  void StartFileTransfer();
  void FileTransferComplete();
  void UpdateProgress();
  void ReceivingFileConfirmation();
  bool CreateFile();
  bool WriteToFile(const uint8_t* aData, int aDataLength);
  void DeleteReceivedFile();
  void ReplyToConnect();
  void ReplyToDisconnect();
  void ReplyToPut(bool aFinal, bool aContinue);
  void AfterOppConnected();
  void AfterFirstPut();
  void AfterOppDisconnected();
  void ValidateFileName();
  bool IsReservedChar(PRUnichar c);

  virtual void OnConnectSuccess() MOZ_OVERRIDE;
  virtual void OnConnectError() MOZ_OVERRIDE;
  virtual void OnDisconnect() MOZ_OVERRIDE;

  


  enum mozilla::ipc::SocketConnectionStatus mSocketStatus;

  



  bool mConnected;
  nsString mConnectedDeviceAddress;

  


  uint8_t mRemoteObexVersion;
  uint8_t mRemoteConnectionFlags;
  int mRemoteMaxPacketLength;

  





  int mLastCommand;

  int mPacketLeftLength;
  int mBodySegmentLength;
  int mReceivedDataBufferOffset;
  int mUpdateProgressCounter;

  


  bool mAbortFlag;

  


  bool mNewFileFlag;

  


  bool mPutFinalFlag;

  


  bool mSendTransferCompleteFlag;

  


  bool mSuccessFlag;

  



  bool mTransferMode;

  



  bool mWaitingForConfirmationFlag;

  nsAutoArrayPtr<uint8_t> mBodySegment;
  nsAutoArrayPtr<uint8_t> mReceivedDataBuffer;

  nsCOMPtr<nsIDOMBlob> mBlob;
  nsCOMPtr<nsIThread> mReadFileThread;
  nsCOMPtr<nsIOutputStream> mOutputStream;
  nsCOMPtr<nsIInputStream> mInputStream;

  nsRefPtr<BluetoothReplyRunnable> mRunnable;
  nsRefPtr<DeviceStorageFile> mDsFile;
};

END_BLUETOOTH_NAMESPACE

#endif
