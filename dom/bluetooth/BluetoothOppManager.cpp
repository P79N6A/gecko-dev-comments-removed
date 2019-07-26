





#include "base/basictypes.h"
#include "BluetoothOppManager.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothUtils.h"
#include "BluetoothUuid.h"
#include "ObexBase.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/RefPtr.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "nsAutoPtr.h"
#include "nsCExternalHandlerService.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIDOMFile.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsIMIMEService.h"
#include "nsIOutputStream.h"
#include "nsNetUtil.h"

#define TARGET_FOLDER "/sdcard/downloads/bluetooth/"

USING_BLUETOOTH_NAMESPACE
using namespace mozilla;
using namespace mozilla::ipc;

class BluetoothOppManagerObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  BluetoothOppManagerObserver()
  {
  }

  bool Init()
  {
    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    if (!obs || NS_FAILED(obs->AddObserver(this,
                                           NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                           false))) {
      NS_WARNING("Failed to add shutdown observer!");
      return false;
    }

    return true;
  }

  bool Shutdown()
  {
    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    if (!obs ||
        (NS_FAILED(obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID)))) {
      NS_WARNING("Can't unregister observers, or already unregistered!");
      return false;
    }
    return true;
  }

  ~BluetoothOppManagerObserver()
  {
    Shutdown();
  }
};

namespace {

static const uint32_t kUpdateProgressBase = 50 * 1024;





static const uint32_t kPutRequestHeaderSize = 6;

StaticRefPtr<BluetoothOppManager> sInstance;
StaticRefPtr<BluetoothOppManagerObserver> sOppObserver;










static uint32_t sSentFileLength = 0;
static nsString sFileName;
static uint32_t sFileLength = 0;
static nsString sContentType;
static bool sInShutdown = false;
static bool sWaitingToSendPutFinal = false;
}

NS_IMETHODIMP
BluetoothOppManagerObserver::Observe(nsISupports* aSubject,
                                     const char* aTopic,
                                     const PRUnichar* aData)
{
  MOZ_ASSERT(sInstance);

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    return sInstance->HandleShutdown();
  }

  MOZ_ASSERT(false, "BluetoothOppManager got unexpected topic!");
  return NS_ERROR_UNEXPECTED;
}

class SendSocketDataTask : public nsRunnable
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

    sInstance->SendPutRequest(mStream, mSize);
    sSentFileLength += mSize;

    return NS_OK;
  }

private:
  nsAutoArrayPtr<uint8_t> mStream;
  uint32_t mSize;
};

class ReadFileTask : public nsRunnable
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
    char* buf = new char[mAvailablePacketSize];

    
    nsresult rv = mInputStream->Read(buf, mAvailablePacketSize, &numRead);
    if (NS_FAILED(rv)) {
      
      NS_WARNING("Failed to read from input stream");
      return NS_ERROR_FAILURE;
    }

    if (numRead > 0) {
      if (sSentFileLength + numRead >= sFileLength) {
        sWaitingToSendPutFinal = true;
      }

      nsRefPtr<SendSocketDataTask> task =
        new SendSocketDataTask((uint8_t*)buf, numRead);
      if (NS_FAILED(NS_DispatchToMainThread(task))) {
        NS_WARNING("Failed to dispatch to main thread!");
        return NS_ERROR_FAILURE;
      }
    }

    return NS_OK;
  };

private:
  nsCOMPtr<nsIInputStream> mInputStream;
  uint32_t mAvailablePacketSize;
};

class CloseSocketTask : public Task
{
public:
  void Run() MOZ_OVERRIDE
  {
    if (!sInstance) {
      NS_WARNING("BluetoothOppManager no longer exists, cannot close socket!");
      return;
    }

    if (sInstance->GetConnectionStatus() ==
          SocketConnectionStatus::SOCKET_CONNECTED) {
      sInstance->CloseSocket();
    }
  }
};

BluetoothOppManager::BluetoothOppManager() : mConnected(false)
                                           , mRemoteObexVersion(0)
                                           , mRemoteConnectionFlags(0)
                                           , mRemoteMaxPacketLength(0)
                                           , mLastCommand(0)
                                           , mPacketLeftLength(0)
                                           , mBodySegmentLength(0)
                                           , mReceivedDataBufferOffset(0)
                                           , mAbortFlag(false)
                                           , mNewFileFlag(false)
                                           , mPutFinalFlag(false)
                                           , mSendTransferCompleteFlag(false)
                                           , mSuccessFlag(false)
                                           , mWaitingForConfirmationFlag(false)
{
  mConnectedDeviceAddress.AssignLiteral("00:00:00:00:00:00");
  mSocketStatus = GetConnectionStatus();
}

BluetoothOppManager::~BluetoothOppManager()
{
}


BluetoothOppManager*
BluetoothOppManager::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (sInstance == nullptr) {
    sInstance = new BluetoothOppManager();
  }

  return sInstance;
}

bool
BluetoothOppManager::Connect(const nsAString& aDeviceObjectPath,
                             BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (GetConnectionStatus() == SocketConnectionStatus::SOCKET_CONNECTED ||
      GetConnectionStatus() == SocketConnectionStatus::SOCKET_CONNECTING) {
    NS_WARNING("BluetoothOppManager has been already connected");
    return false;
  }

  CloseSocket();

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return false;
  }

  nsString uuid;
  BluetoothUuidHelper::GetString(BluetoothServiceClass::OBJECT_PUSH, uuid);

  mRunnable = aRunnable;

  nsresult rv = bs->GetSocketViaService(aDeviceObjectPath,
                                        uuid,
                                        BluetoothSocketType::RFCOMM,
                                        true,
                                        true,
                                        this,
                                        mRunnable);

  return NS_FAILED(rv) ? false : true;
}

void
BluetoothOppManager::Disconnect()
{
  if (GetConnectionStatus() == SocketConnectionStatus::SOCKET_DISCONNECTED) {
    NS_WARNING("BluetoothOppManager has been disconnected!");
    return;
  }

  CloseSocket();
}

nsresult
BluetoothOppManager::HandleShutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  sInShutdown = true;
  CloseSocket();
  sInstance = nullptr;
  return NS_OK;
}

bool
BluetoothOppManager::Listen()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (GetConnectionStatus() == SocketConnectionStatus::SOCKET_LISTENING) {
    NS_WARNING("BluetoothOppManager has been already listening");
    return true;
  }

  CloseSocket();

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return false;
  }

  nsresult rv =
    bs->ListenSocketViaService(BluetoothReservedChannels::CHANNEL_OPUSH,
                               BluetoothSocketType::RFCOMM,
                               true,
                               true,
                               this);
  mSocketStatus = GetConnectionStatus();

  return NS_FAILED(rv) ? false : true;
}

bool
BluetoothOppManager::SendFile(BlobParent* aActor)
{
  if (mBlob) {
    
    return false;
  }

  






  mBlob = aActor->GetBlob();

  sFileName.Truncate();

  nsCOMPtr<nsIDOMFile> file = do_QueryInterface(mBlob);
  if (file) {
    file->GetName(sFileName);
  }

  





  if (sFileName.IsEmpty()) {
    sFileName.AssignLiteral("Unknown");
  }

  int32_t offset = sFileName.RFindChar('/');
  if (offset != kNotFound) {
    sFileName = Substring(sFileName, offset + 1);
  }

  offset = sFileName.RFindChar('.');
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
        sFileName.AppendLiteral(".");
        AppendUTF8toUTF16(extension, sFileName);
      }
    }
  }

  SendConnectRequest();
  mTransferMode = false;
  StartFileTransfer();

  return true;
}

bool
BluetoothOppManager::StopSendingFile()
{
  mAbortFlag = true;

  return true;
}

bool
BluetoothOppManager::ConfirmReceivingFile(bool aConfirm)
{
  if (!mConnected) return false;

  if (!mWaitingForConfirmationFlag) {
    NS_WARNING("We are not waiting for a confirmation now.");
    return false;
  }
  mWaitingForConfirmationFlag = false;

  NS_ASSERTION(mPacketLeftLength == 0,
               "Should not be in the middle of receiving a PUT packet.");

  
  bool success = false;
  if (aConfirm) {
    StartFileTransfer();
    if (CreateFile()) {
      success = WriteToFile(mBodySegment.get(), mBodySegmentLength);
    }
  }

  if (success && mPutFinalFlag) {
    mSuccessFlag = true;
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
  mReceivedDataBufferOffset = 0;
  mSendTransferCompleteFlag = false;
  sSentFileLength = 0;
  sWaitingToSendPutFinal = false;
  mSuccessFlag = false;
}

void
BluetoothOppManager::AfterOppConnected()
{
  MOZ_ASSERT(NS_IsMainThread());

  mConnected = true;
  mAbortFlag = false;
  mWaitingForConfirmationFlag = true;
  AfterFirstPut();
}

void
BluetoothOppManager::AfterOppDisconnected()
{
  MOZ_ASSERT(NS_IsMainThread());

  mConnected = false;
  mLastCommand = 0;
  mBlob = nullptr;

  
  
  

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
}

void
BluetoothOppManager::DeleteReceivedFile()
{
  nsString path;
  path.AssignLiteral(TARGET_FOLDER);

  nsCOMPtr<nsIFile> f;
  nsresult rv = NS_NewLocalFile(path + sFileName, false, getter_AddRefs(f));
  if (NS_FAILED(rv)) {
    NS_WARNING("Couldn't find received file, nothing to delete.");
    return;
  }

  if (mOutputStream) {
    mOutputStream->Close();
    mOutputStream = nullptr;
  }

  f->Remove(false);
}

bool
BluetoothOppManager::CreateFile()
{
  nsString path;
  path.AssignLiteral(TARGET_FOLDER);

  MOZ_ASSERT(mPacketLeftLength == 0);

  nsCOMPtr<nsIFile> f;
  nsresult rv;
  rv = NS_NewLocalFile(path + sFileName, false, getter_AddRefs(f));
  if (NS_FAILED(rv)) {
    NS_WARNING("Couldn't new a local file");
    return false;
  }

  rv = f->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 00644);
  if (NS_FAILED(rv)) {
    NS_WARNING("Couldn't create the file");
    return false;
  }

  




  f->GetLeafName(sFileName);

  mDsFile = nullptr;

  nsCOMPtr<nsIMIMEService> mimeSvc = do_GetService(NS_MIMESERVICE_CONTRACTID);
  if (mimeSvc) {
    nsCString mimeType;
    nsresult rv = mimeSvc->GetTypeFromFile(f, mimeType);

    if (NS_SUCCEEDED(rv)) {
      if (StringBeginsWith(mimeType, NS_LITERAL_CSTRING("image/"))) {
        mDsFile = new DeviceStorageFile(NS_LITERAL_STRING("pictures"), f);
      } else if (StringBeginsWith(mimeType, NS_LITERAL_CSTRING("video/"))) {
        mDsFile = new DeviceStorageFile(NS_LITERAL_STRING("movies"), f);
      } else if (StringBeginsWith(mimeType, NS_LITERAL_CSTRING("audio/"))) {
        mDsFile = new DeviceStorageFile(NS_LITERAL_STRING("music"), f);
      } else {
        NS_WARNING("Couldn't recognize the mimetype of received file.");
      }
    }
  }

  NS_NewLocalFileOutputStream(getter_AddRefs(mOutputStream), f);
  if (!mOutputStream) {
    NS_WARNING("Couldn't new an output stream");
    return false;
  }

  return true;
}

bool
BluetoothOppManager::WriteToFile(const uint8_t* aData, int aDataLength)
{
  if (!mOutputStream) {
    NS_WARNING("No available output stream");
    return false;
  }

  uint32_t wrote = 0;
  mOutputStream->Write((const char*)aData, aDataLength, &wrote);
  if (aDataLength != wrote) {
    NS_WARNING("Writing to the file failed");
    return false;
  }

  return true;
}


void
BluetoothOppManager::ExtractPacketHeaders(const ObexHeaderSet& aHeader)
{
  if (aHeader.Has(ObexHeaderId::Name)) {
    aHeader.GetName(sFileName);
  }

  if (aHeader.Has(ObexHeaderId::Type)) {
    aHeader.GetContentType(sContentType);
  }

  if (aHeader.Has(ObexHeaderId::Length)) {
    aHeader.GetLength(&sFileLength);
  }

  if (aHeader.Has(ObexHeaderId::Body) ||
      aHeader.Has(ObexHeaderId::EndOfBody)) {
    uint8_t* bodyPtr;
    aHeader.GetBody(&bodyPtr);
    mBodySegment = bodyPtr;

    aHeader.GetBodyLength(&mBodySegmentLength);
  }
}

bool
BluetoothOppManager::ExtractBlobHeaders()
{
  nsresult rv = mBlob->GetType(sContentType);
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't get content type");
    SendDisconnectRequest();
    return false;
  }

  uint64_t fileLength;
  rv = mBlob->GetSize(&fileLength);
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't get file size");
    SendDisconnectRequest();
    return false;
  }

  
  
  
  
  
  if (fileLength > (uint64_t)UINT32_MAX) {
    NS_WARNING("The file size is too large for now");
    SendDisconnectRequest();
    return false;
  }

  sFileLength = fileLength;
  rv = NS_NewThread(getter_AddRefs(mReadFileThread));
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't create thread");
    SendDisconnectRequest();
    return false;
  }

  return true;
}

bool
BluetoothOppManager::IsReservedChar(PRUnichar c)
{
  return (c < 0x0020 ||
          c == PRUnichar('?') || c == PRUnichar('|') || c == PRUnichar('<') ||
          c == PRUnichar('>') || c == PRUnichar('"') || c == PRUnichar(':') ||
          c == PRUnichar('/') || c == PRUnichar('*') || c == PRUnichar('\\'));
}

void
BluetoothOppManager::ValidateFileName()
{
  int length = sFileName.Length();

  for (int i = 0; i < length; ++i) {
    
    if (IsReservedChar(sFileName.CharAt(i))) {
      sFileName.Replace(i, 1, PRUnichar('_'));
    }
  }
}

void
BluetoothOppManager::ServerDataHandler(UnixSocketRawData* aMessage)
{
  uint8_t opCode;
  int packetLength;
  int receivedLength = aMessage->mSize;

  if (mPacketLeftLength > 0) {
    opCode = mPutFinalFlag ? ObexRequestCode::PutFinal : ObexRequestCode::Put;
    packetLength = mPacketLeftLength;
  } else {
    opCode = aMessage->mData[0];
    packetLength = (((int)aMessage->mData[1]) << 8) | aMessage->mData[2];

    
    
    if (mPutFinalFlag &&
        (opCode == ObexRequestCode::Put || opCode == ObexRequestCode::PutFinal)) {
      mNewFileFlag = true;
      AfterFirstPut();
    }
  }

  ObexHeaderSet pktHeaders(opCode);
  if (opCode == ObexRequestCode::Connect) {
    
    
    
    ParseHeaders(&aMessage->mData[7],
                 receivedLength - 7,
                 &pktHeaders);
    ReplyToConnect();
    AfterOppConnected();
    mTransferMode = true;
  } else if (opCode == ObexRequestCode::Disconnect ||
             opCode == ObexRequestCode::Abort) {
    
    
    
    ParseHeaders(&aMessage->mData[3],
                receivedLength - 3,
                &pktHeaders);
    ReplyToDisconnect();
    AfterOppDisconnected();
    if (opCode == ObexRequestCode::Abort) {
      DeleteReceivedFile();
    }
    FileTransferComplete();
  } else if (opCode == ObexRequestCode::Put ||
             opCode == ObexRequestCode::PutFinal) {
    int headerStartIndex = 0;

    
    if (mReceivedDataBufferOffset == 0) {
      
      
      headerStartIndex = 3;
      
      
      mReceivedDataBuffer = new uint8_t[packetLength];
      mPacketLeftLength = packetLength;

      





      mPutFinalFlag = (opCode == ObexRequestCode::PutFinal);
    }

    memcpy(mReceivedDataBuffer.get() + mReceivedDataBufferOffset,
           &aMessage->mData[headerStartIndex],
           receivedLength - headerStartIndex);

    mPacketLeftLength -= receivedLength;
    mReceivedDataBufferOffset += receivedLength - headerStartIndex;
    if (mPacketLeftLength) {
      return;
    }

    
    ParseHeaders(mReceivedDataBuffer.get(),
                 mReceivedDataBufferOffset,
                 &pktHeaders);
    ExtractPacketHeaders(pktHeaders);
    ValidateFileName();

    mReceivedDataBufferOffset = 0;

    
    if (mAbortFlag) {
      ReplyToPut(mPutFinalFlag, !mAbortFlag);
      sSentFileLength += mBodySegmentLength;
      DeleteReceivedFile();
      FileTransferComplete();
      return;
    }

    
    if (mWaitingForConfirmationFlag) {
      ReceivingFileConfirmation();
      sSentFileLength += mBodySegmentLength;
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

    
    sSentFileLength += mBodySegmentLength;
    if (sSentFileLength > kUpdateProgressBase * mUpdateProgressCounter) {
      UpdateProgress();
      mUpdateProgressCounter = sSentFileLength / kUpdateProgressBase + 1;
    }

    
    if (mPutFinalFlag) {
      mSuccessFlag = true;
      FileTransferComplete();
    }
  } else {
    NS_WARNING("Unhandled ObexRequestCode");
  }
}

void
BluetoothOppManager::ClientDataHandler(UnixSocketRawData* aMessage)
{
  uint8_t opCode;
  int packetLength;

  if (mPacketLeftLength > 0) {
    opCode = mPutFinalFlag ? ObexRequestCode::PutFinal : ObexRequestCode::Put;
    packetLength = mPacketLeftLength;
  } else {
    opCode = aMessage->mData[0];
    packetLength = (((int)aMessage->mData[1]) << 8) | aMessage->mData[2];
  }

  
  
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
    NS_WARNING(str.get());
    FileTransferComplete();
    return;
  }

  if (mLastCommand == ObexRequestCode::PutFinal) {
    mSuccessFlag = true;
    FileTransferComplete();
    SendDisconnectRequest();
  } else if (mLastCommand == ObexRequestCode::Abort) {
    SendDisconnectRequest();
    FileTransferComplete();
  } else if (mLastCommand == ObexRequestCode::Disconnect) {
    AfterOppDisconnected();
    
    
    
    MessageLoop::current()->
      PostDelayedTask(FROM_HERE, new CloseSocketTask(), 1000);
  } else if (mLastCommand == ObexRequestCode::Connect) {
    MOZ_ASSERT(!sFileName.IsEmpty());
    MOZ_ASSERT(mBlob);

    AfterOppConnected();

    
    mRemoteObexVersion = aMessage->mData[3];
    mRemoteConnectionFlags = aMessage->mData[4];
    mRemoteMaxPacketLength =
      (((int)(aMessage->mData[5]) << 8) | aMessage->mData[6]);

    



    if (ExtractBlobHeaders()) {
      sInstance->SendPutHeaderRequest(sFileName, sFileLength);
    }
  } else if (mLastCommand == ObexRequestCode::Put) {

    
    if (sWaitingToSendPutFinal) {
      SendPutFinalRequest();
      return;
    }

    if (mAbortFlag) {
      SendAbortRequest();
      return;
    }

    if (kUpdateProgressBase * mUpdateProgressCounter < sSentFileLength) {
      UpdateProgress();
      mUpdateProgressCounter = sSentFileLength / kUpdateProgressBase + 1;
    }

    nsresult rv;
    if (!mInputStream) {
      rv = mBlob->GetInternalStream(getter_AddRefs(mInputStream));
      if (NS_FAILED(rv)) {
        NS_WARNING("Can't get internal stream of blob");
        SendDisconnectRequest();
        return;
      }
    }

    nsRefPtr<ReadFileTask> task = new ReadFileTask(mInputStream,
                                                   mRemoteMaxPacketLength);
    rv = mReadFileThread->Dispatch(task, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      NS_WARNING("Cannot dispatch read file task!");
      SendDisconnectRequest();
    }
  } else {
    NS_WARNING("Unhandled ObexRequestCode");
  }
}


void
BluetoothOppManager::ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage)
{
  if (mLastCommand) {
    ClientDataHandler(aMessage);
    return;
  }

  ServerDataHandler(aMessage);
}

void
BluetoothOppManager::SendConnectRequest()
{
  if (mConnected) return;

  
  
  
  uint8_t req[255];
  int index = 7;

  req[3] = 0x10; 
  req[4] = 0x00; 
  req[5] = BluetoothOppManager::MAX_PACKET_LENGTH >> 8;
  req[6] = (uint8_t)BluetoothOppManager::MAX_PACKET_LENGTH;

  SetObexPacketInfo(req, ObexRequestCode::Connect, index);
  mLastCommand = ObexRequestCode::Connect;

  UnixSocketRawData* s = new UnixSocketRawData(index);
  memcpy(s->mData, req, s->mSize);
  SendSocketData(s);
}

void
BluetoothOppManager::SendPutHeaderRequest(const nsAString& aFileName,
                                          int aFileSize)
{
  uint8_t* req = new uint8_t[mRemoteMaxPacketLength];

  int len = aFileName.Length();
  uint8_t* fileName = new uint8_t[(len + 1) * 2];
  const PRUnichar* fileNamePtr = aFileName.BeginReading();

  for (int i = 0; i < len; i++) {
    fileName[i * 2] = (uint8_t)(fileNamePtr[i] >> 8);
    fileName[i * 2 + 1] = (uint8_t)fileNamePtr[i];
  }

  fileName[len * 2] = 0x00;
  fileName[len * 2 + 1] = 0x00;

  int index = 3;
  index += AppendHeaderName(&req[index], (char*)fileName, (len + 1) * 2);
  index += AppendHeaderLength(&req[index], aFileSize);

  SetObexPacketInfo(req, ObexRequestCode::Put, index);
  mLastCommand = ObexRequestCode::Put;

  UnixSocketRawData* s = new UnixSocketRawData(index);
  memcpy(s->mData, req, s->mSize);
  SendSocketData(s);

  delete [] fileName;
  delete [] req;
}

void
BluetoothOppManager::SendPutRequest(uint8_t* aFileBody,
                                    int aFileBodyLength)
{
  int packetLeftSpace = mRemoteMaxPacketLength - kPutRequestHeaderSize;

  if (!mConnected) return;
  if (aFileBodyLength > packetLeftSpace) {
    NS_WARNING("Not allowed such a small MaxPacketLength value");
    return;
  }

  
  
  uint8_t* req = new uint8_t[mRemoteMaxPacketLength];

  int index = 3;
  index += AppendHeaderBody(&req[index], aFileBody, aFileBodyLength);

  SetObexPacketInfo(req, ObexRequestCode::Put, index);
  mLastCommand = ObexRequestCode::Put;

  UnixSocketRawData* s = new UnixSocketRawData(index);
  memcpy(s->mData, req, s->mSize);
  SendSocketData(s);

  delete [] req;
}

void
BluetoothOppManager::SendPutFinalRequest()
{
  if (!mConnected) return;

  






  
  int index = 3;
  uint8_t* req = new uint8_t[mRemoteMaxPacketLength];
  index += AppendHeaderEndOfBody(&req[index]);
  SetObexPacketInfo(req, ObexRequestCode::PutFinal, index);
  mLastCommand = ObexRequestCode::PutFinal;

  UnixSocketRawData* s = new UnixSocketRawData(index);
  memcpy(s->mData, req, s->mSize);
  SendSocketData(s);

  sWaitingToSendPutFinal = false;

  delete [] req;
}

void
BluetoothOppManager::SendDisconnectRequest()
{
  
  
  uint8_t req[255];
  int index = 3;

  SetObexPacketInfo(req, ObexRequestCode::Disconnect, index);
  mLastCommand = ObexRequestCode::Disconnect;

  UnixSocketRawData* s = new UnixSocketRawData(index);
  memcpy(s->mData, req, s->mSize);
  SendSocketData(s);
}

void
BluetoothOppManager::SendAbortRequest()
{
  
  
  uint8_t req[255];
  int index = 3;

  SetObexPacketInfo(req, ObexRequestCode::Abort, index);
  mLastCommand = ObexRequestCode::Abort;

  UnixSocketRawData* s = new UnixSocketRawData(index);
  memcpy(s->mData, req, s->mSize);
  SendSocketData(s);
}

bool
BluetoothOppManager::IsTransferring()
{
  return (mConnected && !mSendTransferCompleteFlag);
}

void
BluetoothOppManager::ReplyToConnect()
{
  if (mConnected) return;
  mConnected = true;

  
  
  
  uint8_t req[255];
  int index = 7;

  req[3] = 0x10; 
  req[4] = 0x00; 
  req[5] = BluetoothOppManager::MAX_PACKET_LENGTH >> 8;
  req[6] = (uint8_t)BluetoothOppManager::MAX_PACKET_LENGTH;

  SetObexPacketInfo(req, ObexResponseCode::Success, index);

  UnixSocketRawData* s = new UnixSocketRawData(index);
  memcpy(s->mData, req, s->mSize);
  SendSocketData(s);
}

void
BluetoothOppManager::ReplyToDisconnect()
{
  if (!mConnected) return;
  mConnected = false;

  
  
  uint8_t req[255];
  int index = 3;

  SetObexPacketInfo(req, ObexResponseCode::Success, index);

  UnixSocketRawData* s = new UnixSocketRawData(index);
  memcpy(s->mData, req, s->mSize);
  SendSocketData(s);
}

void
BluetoothOppManager::ReplyToPut(bool aFinal, bool aContinue)
{
  if (!mConnected) return;

  
  
  uint8_t req[255];
  int index = 3;

  if (aContinue) {
    if (aFinal) {
      SetObexPacketInfo(req, ObexResponseCode::Success, index);
    } else {
      SetObexPacketInfo(req, ObexResponseCode::Continue, index);
    }
  } else {
    if (aFinal) {
      SetObexPacketInfo(req, ObexResponseCode::Unauthorized, index);
    } else {
      SetObexPacketInfo(req,
                        ObexResponseCode::Unauthorized & (~FINAL_BIT),
                        index);
    }
  }

  UnixSocketRawData* s = new UnixSocketRawData(index);
  memcpy(s->mData, req, s->mSize);
  SendSocketData(s);
}

void
BluetoothOppManager::FileTransferComplete()
{
  if (mSendTransferCompleteFlag) {
    return;
  }

  nsString type, name;
  BluetoothValue v;
  InfallibleTArray<BluetoothNamedValue> parameters;
  type.AssignLiteral("bluetooth-opp-transfer-complete");

  name.AssignLiteral("address");
  v = mConnectedDeviceAddress;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("success");
  v = mSuccessFlag;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("received");
  v = mTransferMode;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileName");
  v = sFileName;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileLength");
  v = sSentFileLength;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("contentType");
  v = sContentType;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  if (!BroadcastSystemMessage(type, parameters)) {
    NS_WARNING("Failed to broadcast [bluetooth-opp-transfer-complete]");
    return;
  }

  mSendTransferCompleteFlag = true;
}

void
BluetoothOppManager::StartFileTransfer()
{
  nsString type, name;
  BluetoothValue v;
  InfallibleTArray<BluetoothNamedValue> parameters;
  type.AssignLiteral("bluetooth-opp-transfer-start");

  name.AssignLiteral("address");
  v = mConnectedDeviceAddress;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("received");
  v = mTransferMode;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileName");
  v = sFileName;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileLength");
  v = sFileLength;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("contentType");
  v = sContentType;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  if (!BroadcastSystemMessage(type, parameters)) {
    NS_WARNING("Failed to broadcast [bluetooth-opp-transfer-start]");
    return;
  }
}

void
BluetoothOppManager::UpdateProgress()
{
  nsString type, name;
  BluetoothValue v;
  InfallibleTArray<BluetoothNamedValue> parameters;
  type.AssignLiteral("bluetooth-opp-update-progress");

  name.AssignLiteral("address");
  v = mConnectedDeviceAddress;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("received");
  v = mTransferMode;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("processedLength");
  v = sSentFileLength;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileLength");
  v = sFileLength;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  if (!BroadcastSystemMessage(type, parameters)) {
    NS_WARNING("Failed to broadcast [bluetooth-opp-update-progress]");
    return;
  }
}

void
BluetoothOppManager::ReceivingFileConfirmation()
{
  nsString type, name;
  BluetoothValue v;
  InfallibleTArray<BluetoothNamedValue> parameters;
  type.AssignLiteral("bluetooth-opp-receiving-file-confirmation");

  name.AssignLiteral("address");
  v = mConnectedDeviceAddress;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileName");
  v = sFileName;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileLength");
  v = sFileLength;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("contentType");
  v = sContentType;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  if (!BroadcastSystemMessage(type, parameters)) {
    NS_WARNING("Failed to send [bluetooth-opp-receiving-file-confirmation]");
    return;
  }
}

void
BluetoothOppManager::OnConnectSuccess()
{
  if (mRunnable) {
    BluetoothReply* reply = new BluetoothReply(BluetoothReplySuccess(true));
    mRunnable->SetReply(reply);
    if (NS_FAILED(NS_DispatchToMainThread(mRunnable))) {
      NS_WARNING("Failed to dispatch to main thread!");
    }
    mRunnable.forget();
  }

  
  
  GetSocketAddr(mConnectedDeviceAddress);

  mSocketStatus = GetConnectionStatus();
}

void
BluetoothOppManager::OnConnectError()
{
  if (mRunnable) {
    nsString errorStr;
    errorStr.AssignLiteral("Failed to connect with a bluetooth opp manager!");
    BluetoothReply* reply = new BluetoothReply(BluetoothReplyError(errorStr));
    mRunnable->SetReply(reply);
    if (NS_FAILED(NS_DispatchToMainThread(mRunnable))) {
      NS_WARNING("Failed to dispatch to main thread!");
    }
    mRunnable.forget();
  }

  CloseSocket();
  mSocketStatus = GetConnectionStatus();
  Listen();
}

void
BluetoothOppManager::OnDisconnect()
{
  






  if (mSocketStatus == SocketConnectionStatus::SOCKET_CONNECTED) {
    if (mTransferMode) {
      if (!mSuccessFlag) {
        DeleteReceivedFile();
      } else if (mDsFile) {
        nsString data;
        CopyASCIItoUTF16("modified", data);

        nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
        if (obs) {
          obs->NotifyObservers(mDsFile, "file-watcher-notify", data.get());
        }
      }
    }

    if (!mSuccessFlag) {
      FileTransferComplete();
    }

    Listen();
  } else if (mSocketStatus == SocketConnectionStatus::SOCKET_CONNECTING) {
    NS_WARNING("BluetoothOppManager got unexpected socket status!");
  }

  AfterOppDisconnected();
  mConnectedDeviceAddress.AssignLiteral("00:00:00:00:00:00");
  mSuccessFlag = false;
}
