





#include "StreamSocket.h"
#include <fcntl.h>
#include "mozilla/RefPtr.h"
#include "nsXULAppAPI.h"
#include "StreamSocketConsumer.h"
#include "UnixSocketConnector.h"

static const size_t MAX_READ_SIZE = 1 << 16;

namespace mozilla {
namespace ipc {





class StreamSocketIO final : public ConnectionOrientedSocketIO
{
public:
  class ConnectTask;
  class DelayedConnectTask;
  class ReceiveRunnable;

  StreamSocketIO(nsIThread* aConsumerThread,
                 MessageLoop* mIOLoop,
                 StreamSocket* aStreamSocket,
                 UnixSocketConnector* aConnector);
  StreamSocketIO(nsIThread* aConsumerThread,
                 MessageLoop* mIOLoop, int aFd,
                 ConnectionStatus aConnectionStatus,
                 StreamSocket* aStreamSocket,
                 UnixSocketConnector* aConnector);
  ~StreamSocketIO();

  StreamSocket* GetStreamSocket();
  DataSocket* GetDataSocket();

  
  

  void SetDelayedConnectTask(CancelableTask* aTask);
  void ClearDelayedConnectTask();
  void CancelDelayedConnectTask();

  
  

  nsresult QueryReceiveBuffer(UnixSocketIOBuffer** aBuffer) override;
  void ConsumeBuffer() override;
  void DiscardBuffer() override;

  
  

  SocketBase* GetSocketBase() override;

  bool IsShutdownOnConsumerThread() const override;
  bool IsShutdownOnIOThread() const override;

  void ShutdownOnConsumerThread() override;
  void ShutdownOnIOThread() override;

private:
  




  RefPtr<StreamSocket> mStreamSocket;

  


  bool mShuttingDownOnIOThread;

  



  CancelableTask* mDelayedConnectTask;

  


  nsAutoPtr<UnixSocketRawData> mBuffer;
};

StreamSocketIO::StreamSocketIO(nsIThread* aConsumerThread,
                               MessageLoop* aIOLoop,
                               StreamSocket* aStreamSocket,
                               UnixSocketConnector* aConnector)
  : ConnectionOrientedSocketIO(aConsumerThread, aIOLoop, aConnector)
  , mStreamSocket(aStreamSocket)
  , mShuttingDownOnIOThread(false)
  , mDelayedConnectTask(nullptr)
{
  MOZ_ASSERT(mStreamSocket);
}

StreamSocketIO::StreamSocketIO(nsIThread* aConsumerThread,
                               MessageLoop* aIOLoop,
                               int aFd, ConnectionStatus aConnectionStatus,
                               StreamSocket* aStreamSocket,
                               UnixSocketConnector* aConnector)
  : ConnectionOrientedSocketIO(aConsumerThread,
                               aIOLoop,
                               aFd,
                               aConnectionStatus,
                               aConnector)
  , mStreamSocket(aStreamSocket)
  , mShuttingDownOnIOThread(false)
  , mDelayedConnectTask(nullptr)
{
  MOZ_ASSERT(mStreamSocket);
}

StreamSocketIO::~StreamSocketIO()
{
  MOZ_ASSERT(IsConsumerThread());
  MOZ_ASSERT(IsShutdownOnConsumerThread());
}

StreamSocket*
StreamSocketIO::GetStreamSocket()
{
  return mStreamSocket.get();
}

DataSocket*
StreamSocketIO::GetDataSocket()
{
  return mStreamSocket.get();
}

void
StreamSocketIO::SetDelayedConnectTask(CancelableTask* aTask)
{
  MOZ_ASSERT(IsConsumerThread());

  mDelayedConnectTask = aTask;
}

void
StreamSocketIO::ClearDelayedConnectTask()
{
  MOZ_ASSERT(IsConsumerThread());

  mDelayedConnectTask = nullptr;
}

void
StreamSocketIO::CancelDelayedConnectTask()
{
  MOZ_ASSERT(IsConsumerThread());

  if (!mDelayedConnectTask) {
    return;
  }

  mDelayedConnectTask->Cancel();
  ClearDelayedConnectTask();
}



nsresult
StreamSocketIO::QueryReceiveBuffer(UnixSocketIOBuffer** aBuffer)
{
  MOZ_ASSERT(aBuffer);

  if (!mBuffer) {
    mBuffer = new UnixSocketRawData(MAX_READ_SIZE);
  }
  *aBuffer = mBuffer.get();

  return NS_OK;
}





class StreamSocketIO::ReceiveRunnable final
  : public SocketIORunnable<StreamSocketIO>
{
public:
  ReceiveRunnable(StreamSocketIO* aIO, UnixSocketBuffer* aBuffer)
    : SocketIORunnable<StreamSocketIO>(aIO)
    , mBuffer(aBuffer)
  { }

  NS_IMETHOD Run() override
  {
    StreamSocketIO* io = SocketIORunnable<StreamSocketIO>::GetIO();

    MOZ_ASSERT(io->IsConsumerThread());

    if (NS_WARN_IF(io->IsShutdownOnConsumerThread())) {
      
      
      return NS_OK;
    }

    StreamSocket* streamSocket = io->GetStreamSocket();
    MOZ_ASSERT(streamSocket);

    streamSocket->ReceiveSocketData(mBuffer);

    return NS_OK;
  }

private:
  nsAutoPtr<UnixSocketBuffer> mBuffer;
};

void
StreamSocketIO::ConsumeBuffer()
{
  GetConsumerThread()->Dispatch(new ReceiveRunnable(this, mBuffer.forget()),
                                NS_DISPATCH_NORMAL);
}

void
StreamSocketIO::DiscardBuffer()
{
  
}



SocketBase*
StreamSocketIO::GetSocketBase()
{
  return GetDataSocket();
}

bool
StreamSocketIO::IsShutdownOnConsumerThread() const
{
  MOZ_ASSERT(IsConsumerThread());

  return mStreamSocket == nullptr;
}

bool
StreamSocketIO::IsShutdownOnIOThread() const
{
  return mShuttingDownOnIOThread;
}

void
StreamSocketIO::ShutdownOnConsumerThread()
{
  MOZ_ASSERT(IsConsumerThread());
  MOZ_ASSERT(!IsShutdownOnConsumerThread());

  mStreamSocket = nullptr;
}

void
StreamSocketIO::ShutdownOnIOThread()
{
  MOZ_ASSERT(!IsConsumerThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  Close(); 
  mShuttingDownOnIOThread = true;
}





class StreamSocketIO::ConnectTask final
  : public SocketIOTask<StreamSocketIO>
{
public:
  ConnectTask(StreamSocketIO* aIO)
  : SocketIOTask<StreamSocketIO>(aIO)
  { }

  void Run() override
  {
    MOZ_ASSERT(!GetIO()->IsConsumerThread());
    MOZ_ASSERT(!IsCanceled());

    GetIO()->Connect();
  }
};

class StreamSocketIO::DelayedConnectTask final
  : public SocketIOTask<StreamSocketIO>
{
public:
  DelayedConnectTask(StreamSocketIO* aIO)
    : SocketIOTask<StreamSocketIO>(aIO)
  { }

  void Run() override
  {
    MOZ_ASSERT(GetIO()->IsConsumerThread());

    if (IsCanceled()) {
      return;
    }

    StreamSocketIO* io = GetIO();
    if (io->IsShutdownOnConsumerThread()) {
      return;
    }

    io->ClearDelayedConnectTask();
    io->GetIOLoop()->PostTask(FROM_HERE, new ConnectTask(io));
  }
};





StreamSocket::StreamSocket(StreamSocketConsumer* aConsumer, int aIndex)
  : mIO(nullptr)
  , mConsumer(aConsumer)
  , mIndex(aIndex)
{
  MOZ_ASSERT(mConsumer);
}

StreamSocket::~StreamSocket()
{
  MOZ_ASSERT(!mIO);
}

void
StreamSocket::ReceiveSocketData(nsAutoPtr<UnixSocketBuffer>& aBuffer)
{
  mConsumer->ReceiveSocketData(mIndex, aBuffer);
}

nsresult
StreamSocket::Connect(UnixSocketConnector* aConnector, int aDelayMs,
                      nsIThread* aConsumerThread, MessageLoop* aIOLoop)
{
  MOZ_ASSERT(!mIO);

  mIO = new StreamSocketIO(aConsumerThread, aIOLoop, this, aConnector);
  SetConnectionStatus(SOCKET_CONNECTING);

  if (aDelayMs > 0) {
    StreamSocketIO::DelayedConnectTask* connectTask =
      new StreamSocketIO::DelayedConnectTask(mIO);
    mIO->SetDelayedConnectTask(connectTask);
    MessageLoop::current()->PostDelayedTask(FROM_HERE, connectTask, aDelayMs);
  } else {
    aIOLoop->PostTask(FROM_HERE, new StreamSocketIO::ConnectTask(mIO));
  }

  return NS_OK;
}

nsresult
StreamSocket::Connect(UnixSocketConnector* aConnector, int aDelayMs)
{
  nsIThread* consumerThread = nullptr;
  nsresult rv = NS_GetCurrentThread(&consumerThread);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return Connect(aConnector, aDelayMs, consumerThread, XRE_GetIOMessageLoop());
}



nsresult
StreamSocket::PrepareAccept(UnixSocketConnector* aConnector,
                            nsIThread* aConsumerThread,
                            MessageLoop* aIOLoop,
                            ConnectionOrientedSocketIO*& aIO)
{
  MOZ_ASSERT(!mIO);
  MOZ_ASSERT(aConnector);

  SetConnectionStatus(SOCKET_CONNECTING);

  mIO = new StreamSocketIO(aConsumerThread, aIOLoop,
                           -1, UnixSocketWatcher::SOCKET_IS_CONNECTING,
                           this, aConnector);
  aIO = mIO;

  return NS_OK;
}



void
StreamSocket::SendSocketData(UnixSocketIOBuffer* aBuffer)
{
  MOZ_ASSERT(mIO);
  MOZ_ASSERT(mIO->IsConsumerThread());
  MOZ_ASSERT(!mIO->IsShutdownOnConsumerThread());

  mIO->GetIOLoop()->PostTask(
    FROM_HERE,
    new SocketIOSendTask<StreamSocketIO, UnixSocketIOBuffer>(mIO, aBuffer));
}



void
StreamSocket::Close()
{
  MOZ_ASSERT(mIO);
  MOZ_ASSERT(mIO->IsConsumerThread());

  mIO->CancelDelayedConnectTask();

  
  
  
  mIO->ShutdownOnConsumerThread();
  mIO->GetIOLoop()->PostTask(FROM_HERE, new SocketIOShutdownTask(mIO));
  mIO = nullptr;

  NotifyDisconnect();
}

void
StreamSocket::OnConnectSuccess()
{
  mConsumer->OnConnectSuccess(mIndex);
}

void
StreamSocket::OnConnectError()
{
  mConsumer->OnConnectError(mIndex);
}

void
StreamSocket::OnDisconnect()
{
  mConsumer->OnDisconnect(mIndex);
}

} 
} 
