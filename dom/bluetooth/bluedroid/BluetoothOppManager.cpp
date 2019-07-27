





#include "base/basictypes.h"
#include "BluetoothOppManager.h"

#include "BluetoothService.h"
#include "BluetoothSocket.h"
#include "BluetoothUtils.h"
#include "BluetoothUuid.h"
#include "ObexBase.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/ipc/BlobParent.h"
#include "mozilla/dom/File.h"
#include "mozilla/RefPtr.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "nsAutoPtr.h"
#include "nsCExternalHandlerService.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsIMIMEService.h"
#include "nsIOutputStream.h"
#include "nsIVolumeService.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"

#define TARGET_SUBDIR "Download/Bluetooth/"

USING_BLUETOOTH_NAMESPACE
using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::ipc;

namespace {
  
  static const uint32_t kUpdateProgressBase = 50 * 1024;

  



  static const uint32_t kPutRequestHeaderSize = 6;

  




  static const uint32_t kPutRequestAppendHeaderSize = 5;

  
  static const BluetoothUuid kObexObjectPush = {
    {
      0x00, 0x00, 0x11, 0x05, 0x00, 0x00, 0x10, 0x00,
      0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB
    }
  };

  StaticRefPtr<BluetoothOppManager> sBluetoothOppManager;
  static bool sInShutdown = false;
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothOppManager::SendFileBatch final
{
public:
  SendFileBatch(const nsAString& aDeviceAddress, Blob* aBlob)
    : mDeviceAddress(aDeviceAddress)
  {
    mBlobs.AppendElement(aBlob);
  }

  nsString mDeviceAddress;
  nsTArray<nsRefPtr<Blob>> mBlobs;
};

NS_IMETHODIMP
BluetoothOppManager::Observe(nsISupports* aSubject,
                             const char* aTopic,
                             const char16_t* aData)
{
  MOZ_ASSERT(sBluetoothOppManager);

  
  if (!strcmp(aTopic, NS_VOLUME_STATE_CHANGED)) {
    HandleVolumeStateChanged(aSubject);
    return NS_OK;
  }

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    HandleShutdown();
    return NS_OK;
  }

  MOZ_ASSERT(false, "BluetoothOppManager got unexpected topic!");
  return NS_ERROR_UNEXPECTED;
}

class BluetoothOppManager::SendSocketDataTask final : public nsRunnable
{
public:
  SendSocketDataTask(uint8_t* aStream, uint32_t aSize)
    : mStream(aStream)
    , mSize(aSize)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    sBluetoothOppManager->SendPutRequest(mStream, mSize);

    return NS_OK;
  }

private:
  nsAutoArrayPtr<uint8_t> mStream;
  uint32_t mSize;
};

class BluetoothOppManager::ReadFileTask final : public nsRunnable
{
public:
  ReadFileTask(nsIInputStream* aInputStream,
               uint32_t aRemoteMaxPacketSize) : mInputStream(aInputStream)
  {
    MOZ_ASSERT(NS_IsMainThread());

    mAvailablePacketSize = aRemoteMaxPacketSize - kPutRequestHeaderSize;
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    uint32_t numRead;
    nsAutoArrayPtr<char> buf(new char[mAvailablePacketSize]);

    
    nsresult rv = mInputStream->Read(buf, mAvailablePacketSize, &numRead);
    if (NS_FAILED(rv)) {
      
      BT_WARNING("Failed to read from input stream");
      return NS_ERROR_FAILURE;
    }

    if (numRead > 0) {
      sBluetoothOppManager->CheckPutFinal(numRead);

      nsRefPtr<SendSocketDataTask> task =
        new SendSocketDataTask((uint8_t*)buf.forget(), numRead);
      if (NS_FAILED(NS_DispatchToMainThread(task))) {
        BT_WARNING("Failed to dispatch to main thread!");
        return NS_ERROR_FAILURE;
      }
    }

    return NS_OK;
  };

private:
  nsCOMPtr<nsIInputStream> mInputStream;
  uint32_t mAvailablePacketSize;
};

class BluetoothOppManager::CloseSocketTask final : public Task
{
public:
  CloseSocketTask(BluetoothSocket* aSocket) : mSocket(aSocket)
  {
    MOZ_ASSERT(aSocket);
  }

  void Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());
    mSocket->CloseSocket();
  }

private:
  nsRefPtr<BluetoothSocket> mSocket;
};

BluetoothOppManager::BluetoothOppManager() : mConnected(false)
                                           , mRemoteObexVersion(0)
                                           , mRemoteConnectionFlags(0)
                                           , mRemoteMaxPacketLength(0)
                                           , mLastCommand(0)
                                           , mPacketLength(0)
                                           , mPutPacketReceivedLength(0)
                                           , mBodySegmentLength(0)
                                           , mAbortFlag(false)
                                           , mNewFileFlag(false)
                                           , mPutFinalFlag(false)
                                           , mSendTransferCompleteFlag(false)
                                           , mSuccessFlag(false)
                                           , mIsServer(true)
                                           , mWaitingForConfirmationFlag(false)
                                           , mFileLength(0)
                                           , mSentFileLength(0)
                                           , mWaitingToSendPutFinal(false)
                                           , mCurrentBlobIndex(-1)
{
  mDeviceAddress.AssignLiteral(BLUETOOTH_ADDRESS_NONE);
}

BluetoothOppManager::~BluetoothOppManager()
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE_VOID(obs);
  if (NS_FAILED(obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID))) {
    BT_WARNING("Failed to remove shutdown observer!");
  }

  if (NS_FAILED(obs->RemoveObserver(this, NS_VOLUME_STATE_CHANGED))) {
    BT_WARNING("Failed to remove volume observer!");
  }
}

bool
BluetoothOppManager::Init()
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE(obs, false);
  if (NS_FAILED(obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false))) {
    BT_WARNING("Failed to add shutdown observer!");
    return false;
  }

  if (NS_FAILED(obs->AddObserver(this, NS_VOLUME_STATE_CHANGED, false))) {
    BT_WARNING("Failed to add ns volume observer!");
    return false;
  }

  








  return true;
}


BluetoothOppManager*
BluetoothOppManager::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (sBluetoothOppManager) {
    return sBluetoothOppManager;
  }

  
  NS_ENSURE_FALSE(sInShutdown, nullptr);

  
  BluetoothOppManager *manager = new BluetoothOppManager();
  NS_ENSURE_TRUE(manager->Init(), nullptr);

  sBluetoothOppManager = manager;
  return sBluetoothOppManager;
}

void
BluetoothOppManager::ConnectInternal(const nsAString& aDeviceAddress)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (mServerSocket) {
    mServerSocket->CloseSocket();
    mServerSocket = nullptr;
  }

  mIsServer = false;

  BluetoothService* bs = BluetoothService::Get();
  if (!bs || sInShutdown || mSocket) {
    OnSocketConnectError(mSocket);
    return;
  }

  mSocket =
    new BluetoothSocket(this, BluetoothSocketType::RFCOMM, false, true);
  mSocket->ConnectSocket(aDeviceAddress, kObexObjectPush, -1);
}

void
BluetoothOppManager::HandleShutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  sInShutdown = true;
  Disconnect(nullptr);
  sBluetoothOppManager = nullptr;
}

void
BluetoothOppManager::HandleVolumeStateChanged(nsISupports* aSubject)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  

  if (!mConnected) {
    return;
  }

  
  nsCOMPtr<nsIVolume> vol = do_QueryInterface(aSubject);
  if (!vol) {
    return;
  }
  int32_t state;
  vol->GetState(&state);
  if (nsIVolume::STATE_MOUNTED != state) {
    
    
    
    
    
    
    
    BT_LOGR("Volume state is not STATE_MOUNTED. Abort any ongoing OPP connection.");
    Disconnect(nullptr);
  }
}

bool
BluetoothOppManager::Listen()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mSocket) {
    BT_WARNING("mSocket exists. Failed to listen.");
    return false;
  }

  




  if (mServerSocket) {
    mServerSocket->CloseSocket();
    mServerSocket = nullptr;
  }

  mServerSocket =
    new BluetoothSocket(this, BluetoothSocketType::RFCOMM, false, true);

  if (!mServerSocket->ListenSocket(NS_LITERAL_STRING("OBEX Object Push"),
                                   kObexObjectPush,
                                   BluetoothReservedChannels::CHANNEL_OPUSH)) {
    BT_WARNING("[OPP] Can't listen on RFCOMM socket!");
    mServerSocket = nullptr;
    return false;
  }

  mIsServer = true;

  return true;
}

void
BluetoothOppManager::StartSendingNextFile()
{
  MOZ_ASSERT(NS_IsMainThread());

  MOZ_ASSERT(!IsConnected());
  MOZ_ASSERT(!mBatches.IsEmpty());
  MOZ_ASSERT((int)mBatches[0].mBlobs.Length() > mCurrentBlobIndex + 1);

  mBlob = mBatches[0].mBlobs[++mCurrentBlobIndex];

  
  
  ExtractBlobHeaders();
  StartFileTransfer();

  if (mCurrentBlobIndex == 0) {
    
    
    
    SendConnectRequest();
  } else {
    SendPutHeaderRequest(mFileName, mFileLength);
    AfterFirstPut();
  }
}

bool
BluetoothOppManager::SendFile(const nsAString& aDeviceAddress,
                              BlobParent* aActor)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<BlobImpl> impl = aActor->GetBlobImpl();
  nsRefPtr<Blob> blob = Blob::Create(nullptr, impl);

  return SendFile(aDeviceAddress, blob.get());
}

bool
BluetoothOppManager::SendFile(const nsAString& aDeviceAddress,
                              Blob* aBlob)
{
  MOZ_ASSERT(NS_IsMainThread());

  AppendBlobToSend(aDeviceAddress, aBlob);
  if (!mSocket) {
    ProcessNextBatch();
  }

  return true;
}

void
BluetoothOppManager::AppendBlobToSend(const nsAString& aDeviceAddress,
                                      Blob* aBlob)
{
  MOZ_ASSERT(NS_IsMainThread());

  int indexTail = mBatches.Length() - 1;

  




  if (mBatches.IsEmpty() ||
      aDeviceAddress != mBatches[indexTail].mDeviceAddress) {
    SendFileBatch batch(aDeviceAddress, aBlob);
    mBatches.AppendElement(batch);
  } else {
    mBatches[indexTail].mBlobs.AppendElement(aBlob);
  }
}

void
BluetoothOppManager::DiscardBlobsToSend()
{
  MOZ_ASSERT(NS_IsMainThread());

  MOZ_ASSERT(!mBatches.IsEmpty());
  MOZ_ASSERT(!mIsServer);

  int length = (int) mBatches[0].mBlobs.Length();
  while (length > mCurrentBlobIndex + 1) {
    mBlob = mBatches[0].mBlobs[++mCurrentBlobIndex];

    BT_LOGR("idx %d", mCurrentBlobIndex);
    ExtractBlobHeaders();
    StartFileTransfer();
    FileTransferComplete();
  }
}

bool
BluetoothOppManager::ProcessNextBatch()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  if (mCurrentBlobIndex >= 0) {
    ClearQueue();
    mBatches.RemoveElementAt(0);
    BT_LOGR("REMOVE. %d remaining", mBatches.Length());
  }

  
  if (!mBatches.IsEmpty()) {
    ConnectInternal(mBatches[0].mDeviceAddress);
    return true;
  }

  
  return false;
}

void
BluetoothOppManager::ClearQueue()
{
  MOZ_ASSERT(NS_IsMainThread());

  MOZ_ASSERT(!mIsServer);
  MOZ_ASSERT(!mBatches.IsEmpty());
  MOZ_ASSERT(!mBatches[0].mBlobs.IsEmpty());

  mCurrentBlobIndex = -1;
  mBlob = nullptr;
  mBatches[0].mBlobs.Clear();
}

bool
BluetoothOppManager::StopSendingFile()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mIsServer) {
    mAbortFlag = true;
  } else {
    Disconnect(nullptr);
  }

  return true;
}

bool
BluetoothOppManager::ConfirmReceivingFile(bool aConfirm)
{
  NS_ENSURE_TRUE(mConnected, false);
  NS_ENSURE_TRUE(mWaitingForConfirmationFlag, false);

  mWaitingForConfirmationFlag = false;

  
  bool success = false;
  if (aConfirm) {
    StartFileTransfer();
    if (CreateFile()) {
      success = WriteToFile(mBodySegment.get(), mBodySegmentLength);
    }
  }

  if (success && mPutFinalFlag) {
    mSuccessFlag = true;
    RestoreReceivedFileAndNotify();
    FileTransferComplete();
  }

  ReplyToPut(mPutFinalFlag, success);

  return true;
}

void
BluetoothOppManager::AfterFirstPut()
{
  mUpdateProgressCounter = 1;
  mPutFinalFlag = false;
  mPutPacketReceivedLength = 0;
  mSentFileLength = 0;
  mWaitingToSendPutFinal = false;
  mSuccessFlag = false;
  mBodySegmentLength = 0;
}

void
BluetoothOppManager::AfterOppConnected()
{
  MOZ_ASSERT(NS_IsMainThread());

  mConnected = true;
  mAbortFlag = false;
  mWaitingForConfirmationFlag = true;
  AfterFirstPut();
  
  
  
  if (!AcquireSdcardMountLock()) {
    
    
    BT_WARNING("BluetoothOPPManager couldn't get a mount lock!");
    Disconnect(nullptr);
  }
}

void
BluetoothOppManager::AfterOppDisconnected()
{
  MOZ_ASSERT(NS_IsMainThread());

  mConnected = false;
  mLastCommand = 0;
  mPutPacketReceivedLength = 0;
  mDsFile = nullptr;
  mDummyDsFile = nullptr;

  
  
  

  if (mInputStream) {
    mInputStream->Close();
    mInputStream = nullptr;
  }

  if (mOutputStream) {
    mOutputStream->Close();
    mOutputStream = nullptr;
  }

  if (mReadFileThread) {
    mReadFileThread->Shutdown();
    mReadFileThread = nullptr;
  }

  
  if (mMountLock) {
    
    mMountLock = nullptr;
  }
}

void
BluetoothOppManager::RestoreReceivedFileAndNotify()
{
  
  if (mDummyDsFile && mDummyDsFile->mFile) {
    mDummyDsFile->mFile->Remove(false);
    mDummyDsFile = nullptr;
  }

  
  
  
  
  
  if (mDsFile && mDsFile->mFile) {
    nsString path;
    path.AssignLiteral(TARGET_SUBDIR);
    path.Append(mFileName);

    mDsFile->SetPath(path);
    mDsFile->mFile->RenameTo(nullptr, mFileName);
  }

  
  NotifyAboutFileChange();
}

void
BluetoothOppManager::DeleteReceivedFile()
{
  if (mOutputStream) {
    mOutputStream->Close();
    mOutputStream = nullptr;
  }

  if (mDsFile && mDsFile->mFile) {
    mDsFile->mFile->Remove(false);
    mDsFile = nullptr;
  }

  
  if (mDummyDsFile && mDummyDsFile->mFile) {
    mDummyDsFile->mFile->Remove(false);
    mDummyDsFile = nullptr;
  }
}

bool
BluetoothOppManager::CreateFile()
{
  MOZ_ASSERT(mPutPacketReceivedLength == mPacketLength);

  
  
  
  
  nsString path;
  path.AssignLiteral(TARGET_SUBDIR);
  path.Append(mFileName);

  
  
  
  mDummyDsFile =
    DeviceStorageFile::CreateUnique(path, nsIFile::NORMAL_FILE_TYPE, 0644);
  NS_ENSURE_TRUE(mDummyDsFile, false);

  
  
  
  mDummyDsFile->mFile->GetLeafName(mFileName);

  BT_LOGR("mFileName: %s", NS_ConvertUTF16toUTF8(mFileName).get());

  
  path.Truncate();
  path.AssignLiteral(TARGET_SUBDIR);
  path.Append(mFileName);
  path.AppendLiteral(".part");

  mDsFile =
    DeviceStorageFile::CreateUnique(path, nsIFile::NORMAL_FILE_TYPE, 0644);
  NS_ENSURE_TRUE(mDsFile, false);

  NS_NewLocalFileOutputStream(getter_AddRefs(mOutputStream), mDsFile->mFile);
  NS_ENSURE_TRUE(mOutputStream, false);

  return true;
}

bool
BluetoothOppManager::WriteToFile(const uint8_t* aData, int aDataLength)
{
  NS_ENSURE_TRUE(mOutputStream, false);

  uint32_t wrote = 0;
  mOutputStream->Write((const char*)aData, aDataLength, &wrote);
  NS_ENSURE_TRUE(aDataLength == (int) wrote, false);

  return true;
}


void
BluetoothOppManager::ExtractPacketHeaders(const ObexHeaderSet& aHeader)
{
  if (aHeader.Has(ObexHeaderId::Name)) {
    aHeader.GetName(mFileName);
  }

  if (aHeader.Has(ObexHeaderId::Type)) {
    aHeader.GetContentType(mContentType);
  }

  if (aHeader.Has(ObexHeaderId::Length)) {
    aHeader.GetLength(&mFileLength);
  }

  if (aHeader.Has(ObexHeaderId::Body) ||
      aHeader.Has(ObexHeaderId::EndOfBody)) {
    uint8_t* bodyPtr;
    aHeader.GetBody(&bodyPtr, &mBodySegmentLength);
    mBodySegment = bodyPtr;
  }
}

bool
BluetoothOppManager::ExtractBlobHeaders()
{
  RetrieveSentFileName();
  mBlob->GetType(mContentType);

  ErrorResult rv;
  uint64_t fileLength = mBlob->GetSize(rv);
  if (NS_WARN_IF(rv.Failed())) {
    SendDisconnectRequest();
    rv.SuppressException();
    return false;
  }

  
  
  
  
  
  if (fileLength > (uint64_t)UINT32_MAX) {
    BT_WARNING("The file size is too large for now");
    SendDisconnectRequest();
    return false;
  }

  mFileLength = fileLength;
  rv = NS_NewThread(getter_AddRefs(mReadFileThread));
  if (NS_WARN_IF(rv.Failed())) {
    SendDisconnectRequest();
    rv.SuppressException();
    return false;
  }

  return true;
}

void
BluetoothOppManager::RetrieveSentFileName()
{
  mFileName.Truncate();

  nsRefPtr<File> file = static_cast<Blob*>(mBlob.get())->ToFile();
  if (file) {
    file->GetName(mFileName);
  }

  





  if (mFileName.IsEmpty()) {
    mFileName.AssignLiteral("Unknown");
  }

  int32_t offset = mFileName.RFindChar('/');
  if (offset != kNotFound) {
    mFileName = Substring(mFileName, offset + 1);
  }

  offset = mFileName.RFindChar('.');
  if (offset == kNotFound) {
    nsCOMPtr<nsIMIMEService> mimeSvc = do_GetService(NS_MIMESERVICE_CONTRACTID);

    if (mimeSvc) {
      nsString mimeType;
      mBlob->GetType(mimeType);

      nsCString extension;
      nsresult rv =
        mimeSvc->GetPrimaryExtension(NS_LossyConvertUTF16toASCII(mimeType),
                                     EmptyCString(),
                                     extension);
      if (NS_SUCCEEDED(rv)) {
        mFileName.Append('.');
        AppendUTF8toUTF16(extension, mFileName);
      }
    }
  }
}

bool
BluetoothOppManager::IsReservedChar(char16_t c)
{
  return (c < 0x0020 ||
          c == char16_t('?') || c == char16_t('|') || c == char16_t('<') ||
          c == char16_t('>') || c == char16_t('"') || c == char16_t(':') ||
          c == char16_t('/') || c == char16_t('*') || c == char16_t('\\'));
}

void
BluetoothOppManager::ValidateFileName()
{
  int length = mFileName.Length();

  for (int i = 0; i < length; ++i) {
    
    if (IsReservedChar(mFileName.CharAt(i))) {
      mFileName.Replace(i, 1, char16_t('_'));
    }
  }
}

bool
BluetoothOppManager::ComposePacket(uint8_t aOpCode, UnixSocketBuffer* aMessage)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aMessage);

  const uint8_t* data = aMessage->GetData();
  int frameHeaderLength = 0;

  
  if (mPutPacketReceivedLength == 0) {
    
    
    frameHeaderLength = 3;

    mPacketLength = ((static_cast<int>(data[1]) << 8) | data[2]) -
                    frameHeaderLength;

    





    mReceivedDataBuffer = new uint8_t[mPacketLength];
    mPutFinalFlag = (aOpCode == ObexRequestCode::PutFinal);
  }

  int dataLength = aMessage->GetSize() - frameHeaderLength;

  
  if (dataLength < 0 ||
      mPutPacketReceivedLength + dataLength > mPacketLength) {
    BT_LOGR("Received packet size is unreasonable");

    ReplyToPut(mPutFinalFlag, false);
    DeleteReceivedFile();
    FileTransferComplete();

    return false;
  }

  memcpy(mReceivedDataBuffer.get() + mPutPacketReceivedLength,
         &data[frameHeaderLength], dataLength);

  mPutPacketReceivedLength += dataLength;

  return (mPutPacketReceivedLength == mPacketLength);
}

void
BluetoothOppManager::ServerDataHandler(UnixSocketBuffer* aMessage)
{
  MOZ_ASSERT(NS_IsMainThread());

  uint8_t opCode;
  int receivedLength = aMessage->GetSize();
  const uint8_t* data = aMessage->GetData();

  if (mPutPacketReceivedLength > 0) {
    opCode = mPutFinalFlag ? ObexRequestCode::PutFinal : ObexRequestCode::Put;
  } else {
    opCode = data[0];

    
    
    if (mPutFinalFlag &&
        (opCode == ObexRequestCode::Put ||
         opCode == ObexRequestCode::PutFinal)) {
      mNewFileFlag = true;
      AfterFirstPut();
    }
  }

  ObexHeaderSet pktHeaders(opCode);
  if (opCode == ObexRequestCode::Connect) {
    
    
    
    if (!ParseHeaders(&data[7], receivedLength - 7, &pktHeaders)) {
      ReplyError(ObexResponseCode::BadRequest);
      return;
    }

    ReplyToConnect();
    AfterOppConnected();
  } else if (opCode == ObexRequestCode::Abort) {
    
    
    if (!ParseHeaders(&data[3], receivedLength - 3, &pktHeaders)) {
      ReplyError(ObexResponseCode::BadRequest);
      return;
    }

    ReplyToDisconnectOrAbort();
    DeleteReceivedFile();
  } else if (opCode == ObexRequestCode::Disconnect) {
    
    
    if (!ParseHeaders(&data[3], receivedLength - 3, &pktHeaders)) {
      ReplyError(ObexResponseCode::BadRequest);
      return;
    }

    ReplyToDisconnectOrAbort();
    AfterOppDisconnected();
    FileTransferComplete();
  } else if (opCode == ObexRequestCode::Put ||
             opCode == ObexRequestCode::PutFinal) {
    if (!ComposePacket(opCode, aMessage)) {
      return;
    }

    
    ParseHeaders(mReceivedDataBuffer.get(),
                 mPutPacketReceivedLength, &pktHeaders);
    ExtractPacketHeaders(pktHeaders);
    ValidateFileName();

    
    if (mAbortFlag) {
      ReplyToPut(mPutFinalFlag, false);
      mSentFileLength += mBodySegmentLength;
      DeleteReceivedFile();
      FileTransferComplete();
      return;
    }

    
    if (mWaitingForConfirmationFlag) {
      ReceivingFileConfirmation();
      mSentFileLength += mBodySegmentLength;
      return;
    }

    
    
    if (mNewFileFlag) {
      StartFileTransfer();
      if (!CreateFile()) {
        ReplyToPut(mPutFinalFlag, false);
        return;
      }
      mNewFileFlag = false;
    }

    if (!WriteToFile(mBodySegment.get(), mBodySegmentLength)) {
      ReplyToPut(mPutFinalFlag, false);
      return;
    }

    ReplyToPut(mPutFinalFlag, true);

    
    mSentFileLength += mBodySegmentLength;
    if (mSentFileLength > kUpdateProgressBase * mUpdateProgressCounter) {
      UpdateProgress();
      mUpdateProgressCounter = mSentFileLength / kUpdateProgressBase + 1;
    }

    
    if (mPutFinalFlag) {
      mSuccessFlag = true;
      RestoreReceivedFileAndNotify();
      FileTransferComplete();
    }
  } else if (opCode == ObexRequestCode::Get ||
             opCode == ObexRequestCode::GetFinal ||
             opCode == ObexRequestCode::SetPath) {
    ReplyError(ObexResponseCode::BadRequest);
    BT_WARNING("Unsupported ObexRequestCode");
  } else {
    ReplyError(ObexResponseCode::NotImplemented);
    BT_WARNING("Unrecognized ObexRequestCode");
  }
}

void
BluetoothOppManager::ClientDataHandler(UnixSocketBuffer* aMessage)
{
  MOZ_ASSERT(NS_IsMainThread());

  const uint8_t* data = aMessage->GetData();
  uint8_t opCode = data[0];

  
  
  uint8_t expectedOpCode = ObexResponseCode::Success;
  if (mLastCommand == ObexRequestCode::Put) {
    expectedOpCode = ObexResponseCode::Continue;
  }

  if (opCode != expectedOpCode) {
    if (mLastCommand == ObexRequestCode::Put ||
        mLastCommand == ObexRequestCode::Abort ||
        mLastCommand == ObexRequestCode::PutFinal) {
      SendDisconnectRequest();
    }
    nsAutoCString str;
    str += "[OPP] 0x";
    str.AppendInt(mLastCommand, 16);
    str += " failed";
    BT_WARNING(str.get());
    FileTransferComplete();
    return;
  }

  if (mLastCommand == ObexRequestCode::PutFinal) {
    mSuccessFlag = true;
    FileTransferComplete();

    if (mInputStream) {
      mInputStream->Close();
      mInputStream = nullptr;
    }

    if (mCurrentBlobIndex + 1 == (int) mBatches[0].mBlobs.Length()) {
      SendDisconnectRequest();
    } else {
      StartSendingNextFile();
    }
  } else if (mLastCommand == ObexRequestCode::Abort) {
    SendDisconnectRequest();
    FileTransferComplete();
  } else if (mLastCommand == ObexRequestCode::Disconnect) {
    AfterOppDisconnected();
    
    
    
    if (mSocket) {
      MessageLoop::current()->
        PostDelayedTask(FROM_HERE, new CloseSocketTask(mSocket), 1000);
    }
  } else if (mLastCommand == ObexRequestCode::Connect) {
    MOZ_ASSERT(!mFileName.IsEmpty());
    MOZ_ASSERT(mBlob);

    AfterOppConnected();

    
    mRemoteObexVersion = data[3];
    mRemoteConnectionFlags = data[4];
    mRemoteMaxPacketLength = ((static_cast<int>(data[5]) << 8) | data[6]);

    
    int fileNameByteLen = (mFileName.Length() + 1) * 2;
    int headerLen = kPutRequestHeaderSize + kPutRequestAppendHeaderSize;
    if (fileNameByteLen > mRemoteMaxPacketLength - headerLen) {
      BT_WARNING("The length of file name is aberrant.");
      SendDisconnectRequest();
      return;
    }

    SendPutHeaderRequest(mFileName, mFileLength);
  } else if (mLastCommand == ObexRequestCode::Put) {
    if (mWaitingToSendPutFinal) {
      SendPutFinalRequest();
      return;
    }

    if (kUpdateProgressBase * mUpdateProgressCounter < mSentFileLength) {
      UpdateProgress();
      mUpdateProgressCounter = mSentFileLength / kUpdateProgressBase + 1;
    }

    ErrorResult rv;
    if (!mInputStream) {
      mBlob->GetInternalStream(getter_AddRefs(mInputStream), rv);
      if (NS_WARN_IF(rv.Failed())) {
        SendDisconnectRequest();
        rv.SuppressException();
        return;
      }
    }

    nsRefPtr<ReadFileTask> task = new ReadFileTask(mInputStream,
                                                   mRemoteMaxPacketLength);
    rv = mReadFileThread->Dispatch(task, NS_DISPATCH_NORMAL);
    if (NS_WARN_IF(rv.Failed())) {
      SendDisconnectRequest();
      rv.SuppressException();
      return;
    }
  } else {
    BT_WARNING("Unhandled ObexRequestCode");
  }
}


void
BluetoothOppManager::ReceiveSocketData(BluetoothSocket* aSocket,
                                       nsAutoPtr<UnixSocketBuffer>& aBuffer)
{
  if (mIsServer) {
    ServerDataHandler(aBuffer);
  } else {
    ClientDataHandler(aBuffer);
  }
}

void
BluetoothOppManager::SendConnectRequest()
{
  if (mConnected) {
    return;
  }

  
  
  
  uint8_t req[255];
  int index = 7;

  req[3] = 0x10; 
  req[4] = 0x00; 
  req[5] = BluetoothOppManager::MAX_PACKET_LENGTH >> 8;
  req[6] = (uint8_t)BluetoothOppManager::MAX_PACKET_LENGTH;

  SendObexData(req, ObexRequestCode::Connect, index);
}

void
BluetoothOppManager::SendPutHeaderRequest(const nsAString& aFileName,
                                          int aFileSize)
{
  if (!mConnected) {
    return;
  }

  uint8_t* req = new uint8_t[mRemoteMaxPacketLength];

  int len = aFileName.Length();
  uint8_t* fileName = new uint8_t[(len + 1) * 2];
  const char16_t* fileNamePtr = aFileName.BeginReading();

  for (int i = 0; i < len; i++) {
    fileName[i * 2] = (uint8_t)(fileNamePtr[i] >> 8);
    fileName[i * 2 + 1] = (uint8_t)fileNamePtr[i];
  }

  fileName[len * 2] = 0x00;
  fileName[len * 2 + 1] = 0x00;

  int index = 3;
  index += AppendHeaderName(&req[index], mRemoteMaxPacketLength - index,
                            fileName, (len + 1) * 2);
  index += AppendHeaderLength(&req[index], aFileSize);

  
  uint8_t opcode = (aFileSize > 0) ? ObexRequestCode::Put
                                   : ObexRequestCode::PutFinal;
  SendObexData(req, opcode, index);

  delete [] fileName;
  delete [] req;
}

void
BluetoothOppManager::SendPutRequest(uint8_t* aFileBody,
                                    int aFileBodyLength)
{
  if (!mConnected) {
    return;
  }

  int packetLeftSpace = mRemoteMaxPacketLength - kPutRequestHeaderSize;
  if (aFileBodyLength > packetLeftSpace) {
    BT_WARNING("Not allowed such a small MaxPacketLength value");
    return;
  }

  
  
  uint8_t* req = new uint8_t[mRemoteMaxPacketLength];

  int index = 3;
  index += AppendHeaderBody(&req[index], mRemoteMaxPacketLength - index,
                            aFileBody, aFileBodyLength);

  SendObexData(req, ObexRequestCode::Put, index);
  delete [] req;

  mSentFileLength += aFileBodyLength;
}

void
BluetoothOppManager::SendPutFinalRequest()
{
  if (!mConnected) {
    return;
  }

  






  
  int index = 3;
  uint8_t* req = new uint8_t[mRemoteMaxPacketLength];
  index += AppendHeaderEndOfBody(&req[index]);

  SendObexData(req, ObexRequestCode::PutFinal, index);
  delete [] req;

  mWaitingToSendPutFinal = false;
}

void
BluetoothOppManager::SendDisconnectRequest()
{
  if (!mConnected) {
    return;
  }

  
  
  uint8_t req[255];
  int index = 3;

  SendObexData(req, ObexRequestCode::Disconnect, index);
}

void
BluetoothOppManager::CheckPutFinal(uint32_t aNumRead)
{
  if (mSentFileLength + aNumRead >= mFileLength) {
    mWaitingToSendPutFinal = true;
  }
}

bool
BluetoothOppManager::IsConnected()
{
  return mConnected;
}

void
BluetoothOppManager::GetAddress(nsAString& aDeviceAddress)
{
  return mSocket->GetAddress(aDeviceAddress);
}

void
BluetoothOppManager::ReplyToConnect()
{
  if (mConnected) {
    return;
  }

  
  
  
  uint8_t req[255];
  int index = 7;

  req[3] = 0x10; 
  req[4] = 0x00; 
  req[5] = BluetoothOppManager::MAX_PACKET_LENGTH >> 8;
  req[6] = (uint8_t)BluetoothOppManager::MAX_PACKET_LENGTH;

  SendObexData(req, ObexResponseCode::Success, index);
}

void
BluetoothOppManager::ReplyToDisconnectOrAbort()
{
  if (!mConnected) {
    return;
  }

  
  
  
  uint8_t req[255];
  int index = 3;

  SendObexData(req, ObexResponseCode::Success, index);
}

void
BluetoothOppManager::ReplyToPut(bool aFinal, bool aContinue)
{
  if (!mConnected) {
    return;
  }

  
  
  mPutPacketReceivedLength = 0;

  
  
  uint8_t req[255];
  int index = 3;
  uint8_t opcode;

  if (aContinue) {
    opcode = (aFinal)? ObexResponseCode::Success :
                       ObexResponseCode::Continue;
  } else {
    opcode = (aFinal)? ObexResponseCode::Unauthorized :
                       ObexResponseCode::Unauthorized & (~FINAL_BIT);
  }

  SendObexData(req, opcode, index);
}

void
BluetoothOppManager::ReplyError(uint8_t aError)
{
  BT_LOGR("error: %d", aError);

  
  
  uint8_t req[255];
  int index = 3;

  SendObexData(req, aError, index);
}

void
BluetoothOppManager::SendObexData(uint8_t* aData, uint8_t aOpcode, int aSize)
{
  if (!mIsServer) {
    mLastCommand = aOpcode;
  }

  SetObexPacketInfo(aData, aOpcode, aSize);
  mSocket->SendSocketData(new UnixSocketRawData(aData, aSize));
}

void
BluetoothOppManager::FileTransferComplete()
{
  if (mSendTransferCompleteFlag) {
    return;
  }

  NS_NAMED_LITERAL_STRING(type, "bluetooth-opp-transfer-complete");
  InfallibleTArray<BluetoothNamedValue> parameters;

  BT_APPEND_NAMED_VALUE(parameters, "address", mDeviceAddress);
  BT_APPEND_NAMED_VALUE(parameters, "success", mSuccessFlag);
  BT_APPEND_NAMED_VALUE(parameters, "received", mIsServer);
  BT_APPEND_NAMED_VALUE(parameters, "fileName", mFileName);
  BT_APPEND_NAMED_VALUE(parameters, "fileLength", mSentFileLength);
  BT_APPEND_NAMED_VALUE(parameters, "contentType", mContentType);

  BT_ENSURE_TRUE_VOID_BROADCAST_SYSMSG(type, parameters);

  mSendTransferCompleteFlag = true;
}

void
BluetoothOppManager::StartFileTransfer()
{
  NS_NAMED_LITERAL_STRING(type, "bluetooth-opp-transfer-start");
  InfallibleTArray<BluetoothNamedValue> parameters;

  BT_APPEND_NAMED_VALUE(parameters, "address", mDeviceAddress);
  BT_APPEND_NAMED_VALUE(parameters, "received", mIsServer);
  BT_APPEND_NAMED_VALUE(parameters, "fileName", mFileName);
  BT_APPEND_NAMED_VALUE(parameters, "fileLength", mFileLength);
  BT_APPEND_NAMED_VALUE(parameters, "contentType", mContentType);

  BT_ENSURE_TRUE_VOID_BROADCAST_SYSMSG(type, parameters);

  mSendTransferCompleteFlag = false;
}

void
BluetoothOppManager::UpdateProgress()
{
  NS_NAMED_LITERAL_STRING(type, "bluetooth-opp-update-progress");
  InfallibleTArray<BluetoothNamedValue> parameters;

  BT_APPEND_NAMED_VALUE(parameters, "address", mDeviceAddress);
  BT_APPEND_NAMED_VALUE(parameters, "received", mIsServer);
  BT_APPEND_NAMED_VALUE(parameters, "processedLength", mSentFileLength);
  BT_APPEND_NAMED_VALUE(parameters, "fileLength", mFileLength);

  BT_ENSURE_TRUE_VOID_BROADCAST_SYSMSG(type, parameters);
}

void
BluetoothOppManager::ReceivingFileConfirmation()
{
  NS_NAMED_LITERAL_STRING(type, "bluetooth-opp-receiving-file-confirmation");
  InfallibleTArray<BluetoothNamedValue> parameters;

  BT_APPEND_NAMED_VALUE(parameters, "address", mDeviceAddress);
  BT_APPEND_NAMED_VALUE(parameters, "fileName", mFileName);
  BT_APPEND_NAMED_VALUE(parameters, "fileLength", mFileLength);
  BT_APPEND_NAMED_VALUE(parameters, "contentType", mContentType);

  BT_ENSURE_TRUE_VOID_BROADCAST_SYSMSG(type, parameters);
}

void
BluetoothOppManager::NotifyAboutFileChange()
{
  NS_NAMED_LITERAL_STRING(data, "modified");

  nsCOMPtr<nsIObserverService> obs =
    mozilla::services::GetObserverService();
  NS_ENSURE_TRUE_VOID(obs);

  obs->NotifyObservers(mDsFile, "file-watcher-notify", data.get());
}

void
BluetoothOppManager::OnSocketConnectSuccess(BluetoothSocket* aSocket)
{
  BT_LOGR("[%s]", (mIsServer)? "server" : "client");
  MOZ_ASSERT(aSocket);

  





  if (aSocket == mServerSocket) {
    MOZ_ASSERT(!mSocket);
    mServerSocket.swap(mSocket);
  }

  
  
  mSocket->GetAddress(mDeviceAddress);

  
  if (!mIsServer) {
    StartSendingNextFile();
  }
}

void
BluetoothOppManager::OnSocketConnectError(BluetoothSocket* aSocket)
{
  BT_LOGR("[%s]", (mIsServer)? "server" : "client");

  mServerSocket = nullptr;
  mSocket = nullptr;

  if (!mIsServer) {
    
    DiscardBlobsToSend();
  }

  
  if (!ProcessNextBatch()) {
    Listen();
  }
}

void
BluetoothOppManager::OnSocketDisconnect(BluetoothSocket* aSocket)
{
  MOZ_ASSERT(aSocket);
  if (aSocket != mSocket) {
    
    return;
  }
  BT_LOGR("[%s]", (mIsServer) ? "server" : "client");

  






  if (!mSuccessFlag) {
    if (mIsServer) {
      DeleteReceivedFile();
    }

    FileTransferComplete();
    if (!mIsServer) {
      
      DiscardBlobsToSend();
    }
  }

  AfterOppDisconnected();
  mDeviceAddress.AssignLiteral(BLUETOOTH_ADDRESS_NONE);
  mSuccessFlag = false;

  mSocket = nullptr;
  
  if (!ProcessNextBatch()) {
    Listen();
  }
}

void
BluetoothOppManager::Disconnect(BluetoothProfileController* aController)
{
  if (mSocket) {
    mSocket->CloseSocket();
  } else {
    BT_WARNING("%s: No ongoing file transfer to stop", __FUNCTION__);
  }
}

NS_IMPL_ISUPPORTS(BluetoothOppManager, nsIObserver)

bool
BluetoothOppManager::AcquireSdcardMountLock()
{
  nsCOMPtr<nsIVolumeService> volumeSrv =
    do_GetService(NS_VOLUMESERVICE_CONTRACTID);
  NS_ENSURE_TRUE(volumeSrv, false);
  nsresult rv;
  rv = volumeSrv->CreateMountLock(NS_LITERAL_STRING("sdcard"),
                                  getter_AddRefs(mMountLock));
  NS_ENSURE_SUCCESS(rv, false);
  return true;
}

void
BluetoothOppManager::OnGetServiceChannel(const nsAString& aDeviceAddress,
                                         const nsAString& aServiceUuid,
                                         int aChannel)
{
  MOZ_ASSERT(false);
}

void
BluetoothOppManager::OnUpdateSdpRecords(const nsAString& aDeviceAddress)
{
  MOZ_ASSERT(false);
}

void
BluetoothOppManager::Connect(const nsAString& aDeviceAddress,
                             BluetoothProfileController* aController)
{
  MOZ_ASSERT(false);
}

void
BluetoothOppManager::OnConnect(const nsAString& aErrorStr)
{
  MOZ_ASSERT(false);
}

void
BluetoothOppManager::OnDisconnect(const nsAString& aErrorStr)
{
  MOZ_ASSERT(false);
}

void
BluetoothOppManager::Reset()
{
  MOZ_ASSERT(false);
}

END_BLUETOOTH_NAMESPACE
