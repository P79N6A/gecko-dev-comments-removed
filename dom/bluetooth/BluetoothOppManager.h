





#ifndef mozilla_dom_bluetooth_bluetoothoppmanager_h__
#define mozilla_dom_bluetooth_bluetoothoppmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothProfileManagerBase.h"
#include "BluetoothSocketObserver.h"
#include "DeviceStorage.h"
#include "mozilla/dom/ipc/Blob.h"
#include "mozilla/ipc/UnixSocket.h"
#include "nsCOMArray.h"

class nsIOutputStream;
class nsIInputStream;
class nsIVolumeMountLock;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;
class BluetoothSocket;
class ObexHeaderSet;

class BluetoothOppManager : public BluetoothSocketObserver
                          , public BluetoothProfileManagerBase
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  




  static const int DEFAULT_OPP_CHANNEL = 10;
  static const int MAX_PACKET_LENGTH = 0xFFFE;

  ~BluetoothOppManager();
  static BluetoothOppManager* Get();
  void ClientDataHandler(mozilla::ipc::UnixSocketRawData* aMessage);
  void ServerDataHandler(mozilla::ipc::UnixSocketRawData* aMessage);

  










  void Connect(const nsAString& aDeviceAddress,
               BluetoothReplyRunnable* aRunnable);
  void Disconnect();
  bool Listen();

  bool SendFile(const nsAString& aDeviceAddress, BlobParent* aBlob);
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

  
  
  bool IsTransferring();

  
  void ReceiveSocketData(
    BluetoothSocket* aSocket,
    nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage) MOZ_OVERRIDE;

  virtual void OnConnectSuccess(BluetoothSocket* aSocket) MOZ_OVERRIDE;
  virtual void OnConnectError(BluetoothSocket* aSocket) MOZ_OVERRIDE;
  virtual void OnDisconnect(BluetoothSocket* aSocket) MOZ_OVERRIDE;
  virtual void OnGetServiceChannel(const nsAString& aDeviceAddress,
                                   const nsAString& aServiceUuid,
                                   int aChannel) MOZ_OVERRIDE;
  virtual void OnUpdateSdpRecords(const nsAString& aDeviceAddress) MOZ_OVERRIDE;
  virtual void GetAddress(nsAString& aDeviceAddress) MOZ_OVERRIDE;

private:
  BluetoothOppManager();
  bool Init();
  void HandleShutdown();

  void StartFileTransfer();
  void StartSendingNextFile();
  void FileTransferComplete();
  void UpdateProgress();
  void ReceivingFileConfirmation();
  bool CreateFile();
  bool WriteToFile(const uint8_t* aData, int aDataLength);
  void DeleteReceivedFile();
  void ReplyToConnect();
  void ReplyToDisconnectOrAbort();
  void ReplyToPut(bool aFinal, bool aContinue);
  void AfterOppConnected();
  void AfterFirstPut();
  void AfterOppDisconnected();
  void ValidateFileName();
  bool IsReservedChar(PRUnichar c);
  void ClearQueue();
  void RetrieveSentFileName();
  void NotifyAboutFileChange();
  bool AcquireSdcardMountLock();
  



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

  



  bool mNeedsUpdatingSdpRecords;

  


  bool mAbortFlag;

  


  bool mNewFileFlag;

  


  bool mPutFinalFlag;

  


  bool mSendTransferCompleteFlag;

  


  bool mSuccessFlag;

  



  bool mIsServer;

  



  bool mWaitingForConfirmationFlag;

  nsAutoArrayPtr<uint8_t> mBodySegment;
  nsAutoArrayPtr<uint8_t> mReceivedDataBuffer;

  int mCurrentBlobIndex;
  nsCOMPtr<nsIDOMBlob> mBlob;
  nsCOMArray<nsIDOMBlob> mBlobs;

  




  nsCOMPtr<nsIThread> mReadFileThread;
  nsCOMPtr<nsIOutputStream> mOutputStream;
  nsCOMPtr<nsIInputStream> mInputStream;
  nsCOMPtr<nsIVolumeMountLock> mMountLock;
  nsRefPtr<BluetoothReplyRunnable> mRunnable;
  nsRefPtr<DeviceStorageFile> mDsFile;

  
  
  
  
  nsRefPtr<BluetoothSocket> mSocket;

  
  
  
  nsRefPtr<BluetoothSocket> mRfcommSocket;
  nsRefPtr<BluetoothSocket> mL2capSocket;
};

END_BLUETOOTH_NAMESPACE

#endif
