





#include "base/basictypes.h"
#include "BluetoothOppManager.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothServiceUuid.h"
#include "BluetoothUtils.h"
#include "ObexBase.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/RefPtr.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIDOMFile.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsNetUtil.h"

#define TARGET_FOLDER "/sdcard/download/bluetooth/"

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
    if (NS_FAILED(obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false))) {
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
StaticRefPtr<BluetoothOppManager> sInstance;
StaticRefPtr<BluetoothOppManagerObserver> sOppObserver;










static uint32_t sSentFileLength = 0;
static nsString sFileName;
static uint32_t sFileLength = 0;
static nsString sContentType;
static bool sInShutdown = false;
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

class ReadFileTask : public nsRunnable
{
public:
  ReadFileTask(nsIInputStream* aInputStream) : mInputStream(aInputStream)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    



    char buf[255];
    uint32_t numRead;

    
    nsresult rv = mInputStream->Read(buf, sizeof(buf), &numRead);
    if (NS_FAILED(rv)) {
      
      return NS_ERROR_FAILURE;
    }

    if (numRead > 0) {
      if (sSentFileLength + numRead >= sFileLength) {
        sInstance->SendPutRequest((uint8_t*)buf, numRead, true);
      } else {
        sInstance->SendPutRequest((uint8_t*)buf, numRead, false);
      }

      sSentFileLength += numRead;
    }

    return NS_OK;
  };

private:
  nsCOMPtr<nsIInputStream> mInputStream;
};

BluetoothOppManager::BluetoothOppManager() : mConnected(false)
                                           , mConnectionId(1)
                                           , mLastCommand(0)
                                           , mRemoteObexVersion(0)
                                           , mRemoteConnectionFlags(0)
                                           , mRemoteMaxPacketLength(0)
                                           , mAbortFlag(false)
                                           , mPacketLeftLength(0)
                                           , mReceiving(false)
                                           , mPutFinal(false)
                                           , mWaitingForConfirmationFlag(false)
{
  mConnectedDeviceAddress.AssignLiteral("00:00:00:00:00:00");
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

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return false;
  }

  nsString serviceUuidStr =
    NS_ConvertUTF8toUTF16(mozilla::dom::bluetooth::BluetoothServiceUuidStr::ObjectPush);

  nsRefPtr<BluetoothReplyRunnable> runnable = aRunnable;

  nsresult rv = bs->GetSocketViaService(aDeviceObjectPath,
                                        serviceUuidStr,
                                        BluetoothSocketType::RFCOMM,
                                        true,
                                        true,
                                        this,
                                        runnable);

  runnable.forget();
  return NS_FAILED(rv) ? false : true;
}

void
BluetoothOppManager::Disconnect()
{
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

  CloseSocket();

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return false;
  }

  nsresult rv = bs->ListenSocketViaService(BluetoothReservedChannels::OPUSH,
                                           BluetoothSocketType::RFCOMM,
                                           true,
                                           true,
                                           this);
  return NS_FAILED(rv) ? false : true;
}

bool
BluetoothOppManager::SendFile(BlobParent* aActor,
                              BluetoothReplyRunnable* aRunnable)
{
  if (mBlob) {
    
    return false;
  }

  





  mBlob = aActor->GetBlob();

  SendConnectRequest();

  return true;
}

bool
BluetoothOppManager::StopSendingFile(BluetoothReplyRunnable* aRunnable)
{
  if (!mBlob) {
    return false;
  }

  mAbortFlag = true;

  return true;
}

void
BluetoothOppManager::ConfirmReceivingFile(bool aConfirm,
                                          BluetoothReplyRunnable* aRunnable)
{
  if (!mWaitingForConfirmationFlag) {
    NS_WARNING("We are not waiting for a confirmation now.");
    return;
  }

  NS_ASSERTION(mPacketLeftLength == 0,
               "Should not be in the middle of receiving a PUT packet.");

  mWaitingForConfirmationFlag = false;
  ReplyToPut(mPutFinal, aConfirm);

  if (aConfirm) {
    StartFileTransfer(mConnectedDeviceAddress, true,
                      sFileName, sFileLength, sContentType);
  }

  if (mPutFinal || !aConfirm) {
    mReceiving = false;
    FileTransferComplete(mConnectedDeviceAddress, aConfirm, true, sFileName,
                         sSentFileLength, sContentType);
  }
}

void
BluetoothOppManager::AfterOppConnected()
{
  MOZ_ASSERT(NS_IsMainThread());

  mConnected = true;
  mUpdateProgressCounter = 1;
  sSentFileLength = 0;
  mAbortFlag = false;
}

void
BluetoothOppManager::AfterOppDisconnected()
{
  MOZ_ASSERT(NS_IsMainThread());

  mConnected = false;
  mReceiving = false;
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

  mConnectedDeviceAddress.AssignLiteral("00:00:00:00:00:00");
}


void
BluetoothOppManager::ReceiveSocketData(UnixSocketRawData* aMessage)
{
  uint8_t opCode;
  int packetLength;
  int receivedLength = aMessage->mSize;

  if (mPacketLeftLength > 0) {
    opCode = mPutFinal ? ObexRequestCode::PutFinal : ObexRequestCode::Put;
    packetLength = mPacketLeftLength;
  } else {
    opCode = aMessage->mData[0];
    packetLength = (((int)aMessage->mData[1]) << 8) | aMessage->mData[2];
  }

  if (mLastCommand == ObexRequestCode::Connect) {
    if (opCode == ObexResponseCode::Success) {
      AfterOppConnected();

      
      mRemoteObexVersion = aMessage->mData[3];
      mRemoteConnectionFlags = aMessage->mData[4];
      mRemoteMaxPacketLength =
        (((int)(aMessage->mData[5]) << 8) | aMessage->mData[6]);

      if (mBlob) {
        



        nsresult rv;
        nsCOMPtr<nsIDOMFile> file = do_QueryInterface(mBlob);
        if (file) {
          rv = file->GetName(sFileName);
        }

        if (!file || sFileName.IsEmpty()) {
          sFileName.AssignLiteral("Unknown");
        }

        rv = mBlob->GetType(sContentType);
        if (NS_FAILED(rv)) {
          NS_WARNING("Can't get content type");
          return;
        }

        uint64_t fileLength;
        rv = mBlob->GetSize(&fileLength);
        if (NS_FAILED(rv)) {
          NS_WARNING("Can't get file size");
          return;
        }

        
        
        
        
        
        if (fileLength <= UINT32_MAX) {
          NS_WARNING("The file size is too large for now");
          SendDisconnectRequest();
          return;
        }

        sFileLength = fileLength;

        if (NS_FAILED(NS_NewThread(getter_AddRefs(mReadFileThread)))) {
          NS_WARNING("Can't create thread");
          SendDisconnectRequest();
          return;
        }

        sInstance->SendPutHeaderRequest(sFileName, sFileLength);
        StartFileTransfer(mConnectedDeviceAddress, false,
                          sFileName, sFileLength, sContentType);
      }
    }
  } else if (mLastCommand == ObexRequestCode::Disconnect) {
    if (opCode != ObexResponseCode::Success) {
      
      NS_WARNING("[OPP] Disconnect failed");
    }

    AfterOppDisconnected();
  } else if (mLastCommand == ObexRequestCode::Put) {
    if (opCode != ObexResponseCode::Continue) {
      
      NS_WARNING("[OPP] Put failed");
      return;
    }

    if (mAbortFlag || mReadFileThread) {
      SendAbortRequest();
      return;
    }

    if (kUpdateProgressBase * mUpdateProgressCounter < sSentFileLength) {
      UpdateProgress(mConnectedDeviceAddress, false,
                     sSentFileLength, sFileLength);
      mUpdateProgressCounter = sSentFileLength / kUpdateProgressBase + 1;
    }

    if (mInputStream) {
      nsresult rv = mBlob->GetInternalStream(getter_AddRefs(mInputStream));
      if (NS_FAILED(rv)) {
        NS_WARNING("Can't get internal stream of blob");
        return;
      }
    }

    nsRefPtr<ReadFileTask> task = new ReadFileTask(mInputStream);
    if (NS_FAILED(mReadFileThread->Dispatch(task, NS_DISPATCH_NORMAL))) {
      NS_WARNING("Cannot dispatch ring task!");
    }
  } else if (mLastCommand == ObexRequestCode::PutFinal) {
    if (opCode != ObexResponseCode::Success) {
      
      NS_WARNING("[OPP] PutFinal failed");
      return;
    }

    FileTransferComplete(mConnectedDeviceAddress, true, false, sFileName,
                         sSentFileLength, sContentType);
    SendDisconnectRequest();
  } else if (mLastCommand == ObexRequestCode::Abort) {
    if (opCode != ObexResponseCode::Success) {
      NS_WARNING("[OPP] Abort failed");
    }

    FileTransferComplete(mConnectedDeviceAddress, false, false, sFileName,
                         sSentFileLength, sContentType);
    SendDisconnectRequest();
  } else {
    
    ObexHeaderSet pktHeaders(opCode);

    if (opCode == ObexRequestCode::Connect) {
      
      
      
      ParseHeadersAndFindBody(&aMessage->mData[7],
                              receivedLength - 7,
                              &pktHeaders);
      ReplyToConnect();
      AfterOppConnected();
    } else if (opCode == ObexRequestCode::Disconnect) {
      
      
      ParseHeadersAndFindBody(&aMessage->mData[3],
                              receivedLength - 3,
                              &pktHeaders);
      ReplyToDisconnect();
      AfterOppDisconnected();
    } else if (opCode == ObexRequestCode::Put ||
               opCode == ObexRequestCode::PutFinal) {
      
      
      int headerStartIndex = 3;

      if (!mReceiving) {
        nsString path;
        path.AssignLiteral(TARGET_FOLDER);

        MOZ_ASSERT(mPacketLeftLength == 0);
        ParseHeadersAndFindBody(&aMessage->mData[headerStartIndex],
                                receivedLength - headerStartIndex,
                                &pktHeaders);

        pktHeaders.GetName(sFileName);
        pktHeaders.GetContentType(sContentType);
        pktHeaders.GetLength(&sFileLength);

        path += sFileName;

        nsCOMPtr<nsIFile> f;
        nsresult rv = NS_NewLocalFile(path, false, getter_AddRefs(f));
        if (NS_FAILED(rv)) {
          NS_WARNING("Couldn't new a local file");
        }

        rv = f->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 00644);
        if (NS_FAILED(rv)) {
          NS_WARNING("Couldn't create the file");
        }

        NS_NewLocalFileOutputStream(getter_AddRefs(mOutputStream), f);
        if (!mOutputStream) {
          NS_WARNING("Couldn't new an output stream");
        }

        mReceiving = true;
        mWaitingForConfirmationFlag = true;
      }

      





      mPutFinal = (opCode == ObexRequestCode::PutFinal);

      uint32_t wrote = 0;
      if (mPacketLeftLength == 0) {
        NS_ASSERTION(packetLength >= receivedLength,
                     "Invalid packet length");
        mPacketLeftLength = packetLength - receivedLength;

        int headerBodyOffset =
          ParseHeadersAndFindBody(&aMessage->mData[headerStartIndex],
                                  receivedLength - headerStartIndex,
                                  &pktHeaders);

        if (headerBodyOffset != -1) {
          






          int fileBodyIndex = headerStartIndex + headerBodyOffset + 3;

          mOutputStream->Write((char*)&aMessage->mData[fileBodyIndex],
                               receivedLength - fileBodyIndex, &wrote);
          NS_ASSERTION(receivedLength - fileBodyIndex == wrote,
                       "Writing to the file failed");
        }
      } else {
        NS_ASSERTION(mPacketLeftLength >= receivedLength,
                     "Invalid packet length");
        mPacketLeftLength -= receivedLength;

        mOutputStream->Write((char*)&aMessage->mData[0], receivedLength, &wrote);
        NS_ASSERTION(receivedLength == wrote, "Writing to the file failed");
      }

      sSentFileLength += wrote;
      if (sSentFileLength > kUpdateProgressBase * mUpdateProgressCounter &&
          !mWaitingForConfirmationFlag) {
        UpdateProgress(mConnectedDeviceAddress, true,
                       sSentFileLength, sFileLength);
        mUpdateProgressCounter = sSentFileLength / kUpdateProgressBase + 1;
      }

      if (mPacketLeftLength == 0) {
        if (mWaitingForConfirmationFlag) {
          ReceivingFileConfirmation(mConnectedDeviceAddress, sFileName,
                                    sFileLength, sContentType);
        } else {
          ReplyToPut(mPutFinal, true);

          if (mPutFinal) {
            mReceiving = false;
            FileTransferComplete(mConnectedDeviceAddress, true, true, sFileName,
                                 sSentFileLength, sContentType);
          }
        }
      }
    }
  }
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
  req[6] = BluetoothOppManager::MAX_PACKET_LENGTH;

  index += AppendHeaderConnectionId(&req[index], mConnectionId);
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

  const PRUnichar* fileNamePtr = aFileName.BeginReading();
  uint32_t len = aFileName.Length();
  uint8_t* fileName = new uint8_t[(len + 1) * 2];
  for (int i = 0; i < len; i++) {
    fileName[i * 2] = (uint8_t)(fileNamePtr[i] >> 8);
    fileName[i * 2 + 1] = (uint8_t)fileNamePtr[i];
  }

  fileName[len * 2] = 0x00;
  fileName[len * 2 + 1] = 0x00;

  int index = 3;
  index += AppendHeaderConnectionId(&req[index], mConnectionId);
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
                                    int aFileBodyLength,
                                    bool aFinal)
{
  int index = 3;
  int packetLeftSpace = mRemoteMaxPacketLength - index - 3;

  if (!mConnected) return;
  if (aFileBodyLength > packetLeftSpace) {
    NS_WARNING("Not allowed such a small MaxPacketLength value");
    return;
  }

  
  
  uint8_t* req = new uint8_t[mRemoteMaxPacketLength];

  index += AppendHeaderBody(&req[index], aFileBody, aFileBodyLength);

  if (aFinal) {
    SetObexPacketInfo(req, ObexRequestCode::PutFinal, index);
    mLastCommand = ObexRequestCode::PutFinal;
  } else {
    SetObexPacketInfo(req, ObexRequestCode::Put, index);
    mLastCommand = ObexRequestCode::Put;
  }

  UnixSocketRawData* s = new UnixSocketRawData(index);
  memcpy(s->mData, req, s->mSize);
  SendSocketData(s);

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
  req[6] = BluetoothOppManager::MAX_PACKET_LENGTH;

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
      SetObexPacketInfo(req, ObexResponseCode::Unauthorized & (~FINAL_BIT), index);
    }
  }

  UnixSocketRawData* s = new UnixSocketRawData(index);
  memcpy(s->mData, req, s->mSize);
  SendSocketData(s);
}

void
BluetoothOppManager::FileTransferComplete(const nsString& aDeviceAddress,
                                          bool aSuccess,
                                          bool aReceived,
                                          const nsString& aFileName,
                                          uint32_t aFileLength,
                                          const nsString& aContentType)
{
  nsString type, name;
  BluetoothValue v;
  InfallibleTArray<BluetoothNamedValue> parameters;
  type.AssignLiteral("bluetooth-opp-transfer-complete");

  name.AssignLiteral("address");
  v = aDeviceAddress;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("success");
  v = aSuccess;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("received");
  v = aReceived;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileName");
  v = aFileName;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileLength");
  v = aFileLength;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("contentType");
  v = aContentType;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  if (!BroadcastSystemMessage(type, parameters)) {
    NS_WARNING("Failed to broadcast [bluetooth-opp-transfer-complete]");
    return;
  }
}

void
BluetoothOppManager::StartFileTransfer(const nsString& aDeviceAddress,
                                       bool aReceived,
                                       const nsString& aFileName,
                                       uint32_t aFileLength,
                                       const nsString& aContentType)
{
  nsString type, name;
  BluetoothValue v;
  InfallibleTArray<BluetoothNamedValue> parameters;
  type.AssignLiteral("bluetooth-opp-transfer-start");

  name.AssignLiteral("address");
  v = aDeviceAddress;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("received");
  v = aReceived;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileName");
  v = aFileName;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileLength");
  v = aFileLength;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("contentType");
  v = aContentType;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  if (!BroadcastSystemMessage(type, parameters)) {
    NS_WARNING("Failed to broadcast [bluetooth-opp-transfer-start]");
    return;
  }
}

void
BluetoothOppManager::UpdateProgress(const nsString& aDeviceAddress,
                                    bool aReceived,
                                    uint32_t aProcessedLength,
                                    uint32_t aFileLength)
{
  nsString type, name;
  BluetoothValue v;
  InfallibleTArray<BluetoothNamedValue> parameters;
  type.AssignLiteral("bluetooth-opp-update-progress");

  name.AssignLiteral("address");
  v = aDeviceAddress;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("received");
  v = aReceived;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("processedLength");
  v = aProcessedLength;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileLength");
  v = aFileLength;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  if (!BroadcastSystemMessage(type, parameters)) {
    NS_WARNING("Failed to broadcast [bluetooth-opp-update-progress]");
    return;
  }
}

void
BluetoothOppManager::ReceivingFileConfirmation(const nsString& aAddress,
                                               const nsString& aFileName,
                                               uint32_t aFileLength,
                                               const nsString& aContentType)
{
  nsString type, name;
  BluetoothValue v;
  InfallibleTArray<BluetoothNamedValue> parameters;
  type.AssignLiteral("bluetooth-opp-receiving-file-confirmation");

  name.AssignLiteral("address");
  v = aAddress;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileName");
  v = aFileName;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("fileLength");
  v = aFileLength;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("contentType");
  v = aContentType;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  if (!BroadcastSystemMessage(type, parameters)) {
    NS_WARNING("Failed to broadcast [bluetooth-opp-receiving-file-confirmation]");
    return;
  }
}

void
BluetoothOppManager::OnConnectSuccess()
{
  
  
  GetSocketAddr(mConnectedDeviceAddress);
}

void
BluetoothOppManager::OnConnectError()
{
}

void
BluetoothOppManager::OnDisconnect()
{
  
  
  
  AfterOppDisconnected();
}
