





#include "BluetoothDaemonConnection.h"
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include "mozilla/ipc/BluetoothDaemonConnectionConsumer.h"
#include "mozilla/ipc/DataSocket.h"
#include "nsTArray.h"
#include "nsXULAppAPI.h"

#ifdef CHROMIUM_LOG
#undef CHROMIUM_LOG
#endif

#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define CHROMIUM_LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "I/O", args);
#else
#include <stdio.h>
#define IODEBUG true
#define CHROMIUM_LOG(args...) if (IODEBUG) printf(args);
#endif

namespace mozilla {
namespace ipc {




static const char sBluetoothdSocketName[] = "bluez_hal_socket";





DaemonSocketPDU::DaemonSocketPDU(uint8_t aService, uint8_t aOpcode,
                                       uint16_t aPayloadSize)
  : mConsumer(nullptr)
  , mUserData(nullptr)
{
  
  size_t availableSpace = HEADER_SIZE + aPayloadSize;
  ResetBuffer(new uint8_t[availableSpace], 0, 0, availableSpace);

  
  uint8_t* data = Append(HEADER_SIZE);
  MOZ_ASSERT(data);

  
  data[OFF_SERVICE] = aService;
  data[OFF_OPCODE] = aOpcode;
  memcpy(data + OFF_LENGTH, &aPayloadSize, sizeof(aPayloadSize));
}

DaemonSocketPDU::DaemonSocketPDU(size_t aPayloadSize)
  : mConsumer(nullptr)
  , mUserData(nullptr)
{
  size_t availableSpace = HEADER_SIZE + aPayloadSize;
  ResetBuffer(new uint8_t[availableSpace], 0, 0, availableSpace);
}

DaemonSocketPDU::~DaemonSocketPDU()
{
  nsAutoArrayPtr<uint8_t> data(GetBuffer());
  ResetBuffer(nullptr, 0, 0, 0);
}

void
DaemonSocketPDU::GetHeader(uint8_t& aService, uint8_t& aOpcode,
                              uint16_t& aPayloadSize)
{
  memcpy(&aService, GetData(OFF_SERVICE), sizeof(aService));
  memcpy(&aOpcode, GetData(OFF_OPCODE), sizeof(aOpcode));
  memcpy(&aPayloadSize, GetData(OFF_LENGTH), sizeof(aPayloadSize));
}

ssize_t
DaemonSocketPDU::Send(int aFd)
{
  struct iovec iv;
  memset(&iv, 0, sizeof(iv));
  iv.iov_base = GetData(GetLeadingSpace());
  iv.iov_len = GetSize();

  struct msghdr msg;
  memset(&msg, 0, sizeof(msg));
  msg.msg_iov = &iv;
  msg.msg_iovlen = 1;
  msg.msg_control = nullptr;
  msg.msg_controllen = 0;

  ssize_t res = TEMP_FAILURE_RETRY(sendmsg(aFd, &msg, 0));
  if (res < 0) {
    MOZ_ASSERT(errno != EBADF); 
    OnError("sendmsg", errno);
    return -1;
  }

  Consume(res);

  if (mConsumer) {
    
    
    mConsumer->StoreUserData(*this);
  }

  return res;
}

#define CMSGHDR_CONTAINS_FD(_cmsghdr) \
    ( ((_cmsghdr)->cmsg_level == SOL_SOCKET) && \
      ((_cmsghdr)->cmsg_type == SCM_RIGHTS) )

ssize_t
DaemonSocketPDU::Receive(int aFd)
{
  struct iovec iv;
  memset(&iv, 0, sizeof(iv));
  iv.iov_base = GetData(0);
  iv.iov_len = GetAvailableSpace();

  uint8_t cmsgbuf[CMSG_SPACE(sizeof(int))];

  struct msghdr msg;
  memset(&msg, 0, sizeof(msg));
  msg.msg_iov = &iv;
  msg.msg_iovlen = 1;
  msg.msg_control = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);

  ssize_t res = TEMP_FAILURE_RETRY(recvmsg(aFd, &msg, MSG_NOSIGNAL));
  if (res < 0) {
    MOZ_ASSERT(errno != EBADF); 
    OnError("recvmsg", errno);
    return -1;
  }
  if (msg.msg_flags & (MSG_CTRUNC | MSG_OOB | MSG_ERRQUEUE)) {
    return -1;
  }

  SetRange(0, res);

  struct cmsghdr *chdr = CMSG_FIRSTHDR(&msg);

  for (; chdr; chdr = CMSG_NXTHDR(&msg, chdr)) {
    if (NS_WARN_IF(!CMSGHDR_CONTAINS_FD(chdr))) {
      continue;
    }
    
    
    mReceivedFd = *(static_cast<int*>(CMSG_DATA(chdr)));
  }

  return res;
}

int
DaemonSocketPDU::AcquireFd()
{
  return mReceivedFd.forget();
}

nsresult
DaemonSocketPDU::UpdateHeader()
{
  size_t len = GetPayloadSize();
  if (len >= MAX_PAYLOAD_LENGTH) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  uint16_t len16 = static_cast<uint16_t>(len);

  memcpy(GetData(OFF_LENGTH), &len16, sizeof(len16));

  return NS_OK;
}

size_t
DaemonSocketPDU::GetPayloadSize() const
{
  MOZ_ASSERT(GetSize() >= HEADER_SIZE);

  return GetSize() - HEADER_SIZE;
}

void
DaemonSocketPDU::OnError(const char* aFunction, int aErrno)
{
  CHROMIUM_LOG("%s failed with error %d (%s)",
               aFunction, aErrno, strerror(aErrno));
}





DaemonSocketIOConsumer::DaemonSocketIOConsumer()
{ }

DaemonSocketIOConsumer::~DaemonSocketIOConsumer()
{ }





class DaemonSocketIO final : public ConnectionOrientedSocketIO
{
public:
  DaemonSocketIO(MessageLoop* aConsumerLoop,
                 MessageLoop* aIOLoop,
                 int aFd, ConnectionStatus aConnectionStatus,
                 UnixSocketConnector* aConnector,
                 BluetoothDaemonConnection* aConnection,
                 DaemonSocketIOConsumer* aConsumer);

  
  

  nsresult QueryReceiveBuffer(UnixSocketIOBuffer** aBuffer) override;
  void ConsumeBuffer() override;
  void DiscardBuffer() override;

  
  

  SocketBase* GetSocketBase() override;

  bool IsShutdownOnConsumerThread() const override;
  bool IsShutdownOnIOThread() const override;

  void ShutdownOnConsumerThread() override;
  void ShutdownOnIOThread() override;

private:
  BluetoothDaemonConnection* mConnection;
  DaemonSocketIOConsumer* mConsumer;
  nsAutoPtr<DaemonSocketPDU> mPDU;
  bool mShuttingDownOnIOThread;
};

DaemonSocketIO::DaemonSocketIO(
  MessageLoop* aConsumerLoop,
  MessageLoop* aIOLoop,
  int aFd,
  ConnectionStatus aConnectionStatus,
  UnixSocketConnector* aConnector,
  BluetoothDaemonConnection* aConnection,
  DaemonSocketIOConsumer* aConsumer)
  : ConnectionOrientedSocketIO(aConsumerLoop,
                               aIOLoop,
                               aFd,
                               aConnectionStatus,
                               aConnector)
  , mConnection(aConnection)
  , mConsumer(aConsumer)
  , mShuttingDownOnIOThread(false)
{
  MOZ_ASSERT(mConnection);
  MOZ_ASSERT(mConsumer);
}



nsresult
DaemonSocketIO::QueryReceiveBuffer(UnixSocketIOBuffer** aBuffer)
{
  MOZ_ASSERT(aBuffer);

  if (!mPDU) {
    
    mPDU = new DaemonSocketPDU(DaemonSocketPDU::MAX_PAYLOAD_LENGTH);
  }
  *aBuffer = mPDU.get();

  return NS_OK;
}

void
DaemonSocketIO::ConsumeBuffer()
{
  MOZ_ASSERT(mConsumer);

  mConsumer->Handle(*mPDU);
}

void
DaemonSocketIO::DiscardBuffer()
{
  
}



SocketBase*
DaemonSocketIO::GetSocketBase()
{
  return mConnection;
}

bool
DaemonSocketIO::IsShutdownOnConsumerThread() const
{
  MOZ_ASSERT(IsConsumerThread());

  return mConnection == nullptr;
}

bool
DaemonSocketIO::IsShutdownOnIOThread() const
{
  return mShuttingDownOnIOThread;
}

void
DaemonSocketIO::ShutdownOnConsumerThread()
{
  MOZ_ASSERT(IsConsumerThread());
  MOZ_ASSERT(!IsShutdownOnConsumerThread());

  mConnection = nullptr;
}

void
DaemonSocketIO::ShutdownOnIOThread()
{
  MOZ_ASSERT(!IsConsumerThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  Close(); 
  mShuttingDownOnIOThread = true;
}





BluetoothDaemonConnection::BluetoothDaemonConnection(
  DaemonSocketIOConsumer* aIOConsumer,
  DaemonSocketConsumer* aConsumer,
  int aIndex)
  : mIO(nullptr)
  , mIOConsumer(aIOConsumer)
  , mConsumer(aConsumer)
  , mIndex(aIndex)
{
  MOZ_ASSERT(mConsumer);
}

BluetoothDaemonConnection::~BluetoothDaemonConnection()
{ }



nsresult
BluetoothDaemonConnection::PrepareAccept(UnixSocketConnector* aConnector,
                                         MessageLoop* aConsumerLoop,
                                         MessageLoop* aIOLoop,
                                         ConnectionOrientedSocketIO*& aIO)
{
  MOZ_ASSERT(!mIO);

  SetConnectionStatus(SOCKET_CONNECTING);

  mIO = new DaemonSocketIO(
    aConsumerLoop, aIOLoop, -1, UnixSocketWatcher::SOCKET_IS_CONNECTING,
    aConnector, this, mIOConsumer);
  aIO = mIO;

  return NS_OK;
}



void
BluetoothDaemonConnection::SendSocketData(UnixSocketIOBuffer* aBuffer)
{
  MOZ_ASSERT(mIO);
  MOZ_ASSERT(mIO->IsConsumerThread());

  mIO->GetIOLoop()->PostTask(
    FROM_HERE,
    new SocketIOSendTask<DaemonSocketIO, UnixSocketIOBuffer>(mIO, aBuffer));
}



void
BluetoothDaemonConnection::Close()
{
  if (!mIO) {
    CHROMIUM_LOG("Bluetooth daemon already disconnected!");
    return;
  }

  MOZ_ASSERT(mIO->IsConsumerThread());

  mIO->ShutdownOnConsumerThread();
  mIO->GetIOLoop()->PostTask(FROM_HERE, new SocketIOShutdownTask(mIO));
  mIO = nullptr;

  NotifyDisconnect();
}

void
BluetoothDaemonConnection::OnConnectSuccess()
{
  mConsumer->OnConnectSuccess(mIndex);
}

void
BluetoothDaemonConnection::OnConnectError()
{
  mConsumer->OnConnectError(mIndex);
}

void
BluetoothDaemonConnection::OnDisconnect()
{
  mConsumer->OnDisconnect(mIndex);
}

}
}
