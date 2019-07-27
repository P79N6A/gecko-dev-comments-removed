





#include "DaemonSocket.h"
#include "mozilla/ipc/DaemonSocketConsumer.h"
#include "mozilla/ipc/DaemonSocketPDU.h"
#include "nsISupportsImpl.h" 

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





class DaemonSocketIO final : public ConnectionOrientedSocketIO
{
public:
  DaemonSocketIO(MessageLoop* aConsumerLoop,
                 MessageLoop* aIOLoop,
                 int aFd, ConnectionStatus aConnectionStatus,
                 UnixSocketConnector* aConnector,
                 DaemonSocket* aConnection,
                 DaemonSocketIOConsumer* aConsumer);

  ~DaemonSocketIO();

  
  

  nsresult QueryReceiveBuffer(UnixSocketIOBuffer** aBuffer) override;
  void ConsumeBuffer() override;
  void DiscardBuffer() override;

  
  

  SocketBase* GetSocketBase() override;

  bool IsShutdownOnConsumerThread() const override;
  bool IsShutdownOnIOThread() const override;

  void ShutdownOnConsumerThread() override;
  void ShutdownOnIOThread() override;

private:
  DaemonSocket* mConnection;
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
  DaemonSocket* aConnection,
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

  MOZ_COUNT_CTOR_INHERITED(DaemonSocketIO, ConnectionOrientedSocketIO);
}

DaemonSocketIO::~DaemonSocketIO()
{
  MOZ_COUNT_DTOR_INHERITED(DaemonSocketIO, ConnectionOrientedSocketIO);
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





DaemonSocket::DaemonSocket(
  DaemonSocketIOConsumer* aIOConsumer,
  DaemonSocketConsumer* aConsumer,
  int aIndex)
  : mIO(nullptr)
  , mIOConsumer(aIOConsumer)
  , mConsumer(aConsumer)
  , mIndex(aIndex)
{
  MOZ_ASSERT(mConsumer);

  MOZ_COUNT_CTOR_INHERITED(DaemonSocket, ConnectionOrientedSocket);
}

DaemonSocket::~DaemonSocket()
{
  MOZ_COUNT_DTOR_INHERITED(DaemonSocket, ConnectionOrientedSocket);
}



nsresult
DaemonSocket::PrepareAccept(UnixSocketConnector* aConnector,
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
DaemonSocket::SendSocketData(UnixSocketIOBuffer* aBuffer)
{
  MOZ_ASSERT(mIO);
  MOZ_ASSERT(mIO->IsConsumerThread());

  mIO->GetIOLoop()->PostTask(
    FROM_HERE,
    new SocketIOSendTask<DaemonSocketIO, UnixSocketIOBuffer>(mIO, aBuffer));
}



void
DaemonSocket::Close()
{
  if (!mIO) {
    CHROMIUM_LOG("HAL daemon already disconnected!");
    return;
  }

  MOZ_ASSERT(mIO->IsConsumerThread());

  mIO->ShutdownOnConsumerThread();
  mIO->GetIOLoop()->PostTask(FROM_HERE, new SocketIOShutdownTask(mIO));
  mIO = nullptr;

  NotifyDisconnect();
}

void
DaemonSocket::OnConnectSuccess()
{
  mConsumer->OnConnectSuccess(mIndex);
}

void
DaemonSocket::OnConnectError()
{
  mConsumer->OnConnectError(mIndex);
}

void
DaemonSocket::OnDisconnect()
{
  mConsumer->OnDisconnect(mIndex);
}

}
}
