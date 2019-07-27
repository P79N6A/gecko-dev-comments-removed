





#ifndef mozilla_dom_bluetooth_bluetoothoppmanager_h__
#define mozilla_dom_bluetooth_bluetoothoppmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothProfileManagerBase.h"
#include "BluetoothSocketObserver.h"
#include "DeviceStorage.h"
#include "mozilla/ipc/SocketBase.h"
#include "nsCOMArray.h"

class nsIDOMBlob;
class nsIOutputStream;
class nsIInputStream;
class nsIVolumeMountLock;

namespace mozilla {
namespace dom {
class BlobParent;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSocket;
class ObexHeaderSet;

class BluetoothOppManager : public BluetoothSocketObserver
                          , public BluetoothProfileManagerBase
{
  class CloseSocketTask;
  class ReadFileTask;
  class SendFileBatch;
  class SendSocketDataTask;

public:
  BT_DECL_PROFILE_MGR_BASE
  virtual void GetName(nsACString& aName)
  {
    aName.AssignLiteral("OPP");
  }

  static const int MAX_PACKET_LENGTH = 0xFFFE;

  static BluetoothOppManager* Get();
  void ClientDataHandler(mozilla::ipc::UnixSocketRawData* aMessage);
  void ServerDataHandler(mozilla::ipc::UnixSocketRawData* aMessage);

  bool Listen();

  bool SendFile(const nsAString& aDeviceAddress, BlobParent* aActor);
  bool SendFile(const nsAString& aDeviceAddress, nsIDOMBlob* aBlob);
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
    nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage) override;
  virtual void OnSocketConnectSuccess(BluetoothSocket* aSocket) override;
  virtual void OnSocketConnectError(BluetoothSocket* aSocket) override;
  virtual void OnSocketDisconnect(BluetoothSocket* aSocket) override;

protected:
  virtual ~BluetoothOppManager();

private:
  BluetoothOppManager();
  bool Init();
  void HandleShutdown();

#ifdef MOZ_B2G_BT_API_V2
  
#else
  void HandleVolumeStateChanged(nsISupports* aSubject);
#endif

  void StartFileTransfer();
  void StartSendingNextFile();
  void FileTransferComplete();
  void UpdateProgress();
  void ReceivingFileConfirmation();
  bool CreateFile();
  bool WriteToFile(const uint8_t* aData, int aDataLength);
  void RestoreReceivedFileAndNotify();
  void DeleteReceivedFile();
  void ReplyToConnect();
  void ReplyToDisconnectOrAbort();
  void ReplyToPut(bool aFinal, bool aContinue);
  void ReplyError(uint8_t aError);
  void AfterOppConnected();
  void AfterFirstPut();
  void AfterOppDisconnected();
  void ValidateFileName();
  bool IsReservedChar(char16_t c);
  void ClearQueue();
  void RetrieveSentFileName();
  void NotifyAboutFileChange();
  bool AcquireSdcardMountLock();
  void SendObexData(uint8_t* aData, uint8_t aOpcode, int aSize);
  void AppendBlobToSend(const nsAString& aDeviceAddress, nsIDOMBlob* aBlob);
  void DiscardBlobsToSend();
  bool ProcessNextBatch();
  void ConnectInternal(const nsAString& aDeviceAddress);

  








  bool ComposePacket(uint8_t aOpCode,
                     mozilla::ipc::UnixSocketRawData* aMessage);

  



  bool mConnected;
  nsString mDeviceAddress;

  


  uint8_t mRemoteObexVersion;
  uint8_t mRemoteConnectionFlags;
  int mRemoteMaxPacketLength;

  





  int mLastCommand;

  int mPacketLength;
  int mPutPacketReceivedLength;
  int mBodySegmentLength;
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
  nsRefPtr<DeviceStorageFile> mDummyDsFile;

  
  
  
  nsRefPtr<BluetoothSocket> mSocket;

  
  
  
  nsRefPtr<BluetoothSocket> mServerSocket;
};

END_BLUETOOTH_NAMESPACE

#endif
