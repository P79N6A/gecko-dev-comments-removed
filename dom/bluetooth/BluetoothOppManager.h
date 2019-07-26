





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

class BluetoothSocket;
class ObexHeaderSet;
class SendFileBatch;

class BluetoothOppManager : public BluetoothSocketObserver
                          , public BluetoothProfileManagerBase
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  BT_DECL_PROFILE_MGR_BASE
  virtual void GetName(nsACString& aName)
  {
    aName.AssignLiteral("OPP");
  }

  




  static const int DEFAULT_OPP_CHANNEL = 10;
  static const int MAX_PACKET_LENGTH = 0xFFFE;

  ~BluetoothOppManager();
  static BluetoothOppManager* Get();
  void ClientDataHandler(mozilla::ipc::UnixSocketRawData* aMessage);
  void ServerDataHandler(mozilla::ipc::UnixSocketRawData* aMessage);

  bool Listen();

  bool SendFile(const nsAString& aDeviceAddress, BlobParent* aActor);
  bool StopSendingFile();
  bool ConfirmReceivingFile(bool aConfirm);

  void SendConnectRequest();
  void SendPutHeaderRequest(const nsAString& aFileName, int aFileSize);
  void SendPutRequest(uint8_t* aFileBody, int aFileBodyLength);
  void SendPutFinalRequest();
  void SendDisconnectRequest();

  void ExtractPacketHeaders(const ObexHeaderSet& aHeader);
  bool ExtractBlobHeaders();
  void CheckPutFinal(uint32_t aNumRead);

  
  void ReceiveSocketData(
    BluetoothSocket* aSocket,
    nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage) MOZ_OVERRIDE;
  virtual void OnSocketConnectSuccess(BluetoothSocket* aSocket) MOZ_OVERRIDE;
  virtual void OnSocketConnectError(BluetoothSocket* aSocket) MOZ_OVERRIDE;
  virtual void OnSocketDisconnect(BluetoothSocket* aSocket) MOZ_OVERRIDE;

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
  void ReplyError(uint8_t aError);
  void AfterOppConnected();
  void AfterFirstPut();
  void AfterOppDisconnected();
  void ValidateFileName();
  bool IsReservedChar(PRUnichar c);
  void ClearQueue();
  void RetrieveSentFileName();
  void NotifyAboutFileChange();
  bool AcquireSdcardMountLock();
  void SendObexData(uint8_t* aData, uint8_t aOpcode, int aSize);
  void AppendBlobToSend(const nsAString& aDeviceAddress, BlobParent* aActor);
  void DiscardBlobsToSend();
  bool ProcessNextBatch();
  void ConnectInternal(const nsAString& aDeviceAddress);

  



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

  nsString mFileName;
  nsString mContentType;
  uint32_t mFileLength;
  uint32_t mSentFileLength;
  bool mWaitingToSendPutFinal;

  nsAutoArrayPtr<uint8_t> mBodySegment;
  nsAutoArrayPtr<uint8_t> mReceivedDataBuffer;

  int mCurrentBlobIndex;
  nsCOMPtr<nsIDOMBlob> mBlob;
  nsTArray<SendFileBatch> mBatches;

  



  nsCOMPtr<nsIThread> mReadFileThread;
  nsCOMPtr<nsIOutputStream> mOutputStream;
  nsCOMPtr<nsIInputStream> mInputStream;
  nsCOMPtr<nsIVolumeMountLock> mMountLock;
  nsRefPtr<DeviceStorageFile> mDsFile;

  
  
  
  
  nsRefPtr<BluetoothSocket> mSocket;

  
  
  
  nsRefPtr<BluetoothSocket> mRfcommSocket;
  nsRefPtr<BluetoothSocket> mL2capSocket;
};

END_BLUETOOTH_NAMESPACE

#endif
