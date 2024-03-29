





#ifndef mozilla_dom_bluetooth_bluetoothoppmanager_h__
#define mozilla_dom_bluetooth_bluetoothoppmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothProfileManagerBase.h"
#include "BluetoothSocketObserver.h"
#include "DeviceStorage.h"
#include "mozilla/ipc/SocketBase.h"
#include "nsCOMArray.h"

class nsIOutputStream;
class nsIInputStream;
class nsIVolumeMountLock;

namespace mozilla {
namespace dom {
class Blob;
class BlobParent;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSocket;
class ObexHeaderSet;
class SendFileBatch;

class BluetoothOppManager : public BluetoothSocketObserver
                          , public BluetoothProfileManagerBase
{
public:
  BT_DECL_PROFILE_MGR_BASE
  BT_DECL_SOCKET_OBSERVER
  virtual void GetName(nsACString& aName)
  {
    aName.AssignLiteral("OPP");
  }

  static const int MAX_PACKET_LENGTH = 0xFFFE;

  static BluetoothOppManager* Get();
  void ClientDataHandler(mozilla::ipc::UnixSocketBuffer* aMessage);
  void ServerDataHandler(mozilla::ipc::UnixSocketBuffer* aMessage);

  bool Listen();

  bool SendFile(const nsAString& aDeviceAddress, BlobParent* aActor);
  bool SendFile(const nsAString& aDeviceAddress, Blob* aBlob);
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

protected:
  virtual ~BluetoothOppManager();

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
  void AppendBlobToSend(const nsAString& aDeviceAddress, Blob* aBlob);
  void DiscardBlobsToSend();
  bool ProcessNextBatch();
  void ConnectInternal(const nsAString& aDeviceAddress);

  








  bool ComposePacket(uint8_t aOpCode,
                     mozilla::ipc::UnixSocketBuffer* aMessage);

  



  bool mConnected;
  nsString mConnectedDeviceAddress;

  


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
  nsRefPtr<Blob> mBlob;
  nsTArray<SendFileBatch> mBatches;

  



  nsCOMPtr<nsIThread> mReadFileThread;
  nsCOMPtr<nsIOutputStream> mOutputStream;
  nsCOMPtr<nsIInputStream> mInputStream;
  nsCOMPtr<nsIVolumeMountLock> mMountLock;
  nsRefPtr<DeviceStorageFile> mDsFile;
  nsRefPtr<DeviceStorageFile> mDummyDsFile;

  
  
  
  
  nsRefPtr<BluetoothSocket> mSocket;

  
  
  
  nsRefPtr<BluetoothSocket> mRfcommSocket;
  nsRefPtr<BluetoothSocket> mL2capSocket;

  BluetoothSocketType mSocketType;

  
  
  mozilla::TimeStamp mLastServiceChannelCheck;
};

END_BLUETOOTH_NAMESPACE

#endif
