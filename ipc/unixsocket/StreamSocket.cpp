





#include "StreamSocket.h"
#include <fcntl.h>
#include "mozilla/RefPtr.h"
#include "nsXULAppAPI.h"
#include "UnixSocketConnector.h"

static const size_t MAX_READ_SIZE = 1 << 16;

namespace mozilla {
namespace ipc {





class StreamSocketIO final : public UnixSocketWatcher
                           , protected SocketIOBase
                           , public ConnectionOrientedSocketIO
{
public:
  class ConnectTask;
  class DelayedConnectTask;

  StreamSocketIO(MessageLoop* mIOLoop,
                 StreamSocket* aStreamSocket,
                 UnixSocketConnector* aConnector,
                 const nsACString& aAddress);
  StreamSocketIO(MessageLoop* mIOLoop, int aFd,
                 ConnectionStatus aConnectionStatus,
                 StreamSocket* aStreamSocket,
                 UnixSocketConnector* aConnector,
                 const nsACString& aAddress);
  ~StreamSocketIO();

  void                GetSocketAddr(nsAString& aAddrStr) const;
  SocketConsumerBase* GetConsumer();
  SocketBase*         GetSocketBase();

  
  

  nsresult Accept(int aFd,
                  const union sockaddr_any* aAddr, socklen_t aAddrLen);

  
  

  bool IsShutdownOnMainThread() const;
  void ShutdownOnMainThread();

  bool IsShutdownOnIOThread() const;
  void ShutdownOnIOThread();

  
  

  void SetDelayedConnectTask(CancelableTask* aTask);
  void ClearDelayedConnectTask();
  void CancelDelayedConnectTask();

  
  

  


  void Connect();

  void Send(UnixSocketIOBuffer* aBuffer);

  
  

  void OnAccepted(int aFd, const sockaddr_any* aAddr,
                  socklen_t aAddrLen) override;
  void OnConnected() override;
  void OnError(const char* aFunction, int aErrno) override;
  void OnListening() override;
  void OnSocketCanReceiveWithoutBlocking() override;
  void OnSocketCanSendWithoutBlocking() override;

private:
  void FireSocketError();

  
  static bool SetSocketFlags(int aFd);

  




  RefPtr<StreamSocket> mStreamSocket;

  


  nsAutoPtr<UnixSocketConnector> mConnector;

  


  bool mShuttingDownOnIOThread;

  


  nsCString mAddress;

  


  socklen_t mAddrSize;

  


  sockaddr_any mAddr;

  


  CancelableTask* mDelayedConnectTask;
};

StreamSocketIO::StreamSocketIO(MessageLoop* mIOLoop,
                               StreamSocket* aStreamSocket,
                               UnixSocketConnector* aConnector,
                               const nsACString& aAddress)
: UnixSocketWatcher(mIOLoop)
, SocketIOBase(MAX_READ_SIZE)
, mStreamSocket(aStreamSocket)
, mConnector(aConnector)
, mShuttingDownOnIOThread(false)
, mAddress(aAddress)
, mDelayedConnectTask(nullptr)
{
  MOZ_ASSERT(mStreamSocket);
  MOZ_ASSERT(mConnector);
}

StreamSocketIO::StreamSocketIO(MessageLoop* mIOLoop, int aFd,
                               ConnectionStatus aConnectionStatus,
                               StreamSocket* aStreamSocket,
                               UnixSocketConnector* aConnector,
                               const nsACString& aAddress)
: UnixSocketWatcher(mIOLoop, aFd, aConnectionStatus)
, SocketIOBase(MAX_READ_SIZE)
, mStreamSocket(aStreamSocket)
, mConnector(aConnector)
, mShuttingDownOnIOThread(false)
, mAddress(aAddress)
, mDelayedConnectTask(nullptr)
{
  MOZ_ASSERT(mStreamSocket);
  MOZ_ASSERT(mConnector);
}

StreamSocketIO::~StreamSocketIO()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(IsShutdownOnMainThread());
}

void
StreamSocketIO::GetSocketAddr(nsAString& aAddrStr) const
{
  if (!mConnector) {
    NS_WARNING("No connector to get socket address from!");
    aAddrStr.Truncate();
    return;
  }
  mConnector->GetSocketAddr(mAddr, aAddrStr);
}

SocketConsumerBase*
StreamSocketIO::GetConsumer()
{
  return mStreamSocket.get();
}

SocketBase*
StreamSocketIO::GetSocketBase()
{
  return GetConsumer();
}

bool
StreamSocketIO::IsShutdownOnMainThread() const
{
  MOZ_ASSERT(NS_IsMainThread());

  return mStreamSocket == nullptr;
}

void
StreamSocketIO::ShutdownOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!IsShutdownOnMainThread());

  mStreamSocket = nullptr;
}

bool
StreamSocketIO::IsShutdownOnIOThread() const
{
  return mShuttingDownOnIOThread;
}

void
StreamSocketIO::ShutdownOnIOThread()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  Close(); 
  mShuttingDownOnIOThread = true;
}

void
StreamSocketIO::SetDelayedConnectTask(CancelableTask* aTask)
{
  MOZ_ASSERT(NS_IsMainThread());

  mDelayedConnectTask = aTask;
}

void
StreamSocketIO::ClearDelayedConnectTask()
{
  MOZ_ASSERT(NS_IsMainThread());

  mDelayedConnectTask = nullptr;
}

void
StreamSocketIO::CancelDelayedConnectTask()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mDelayedConnectTask) {
    return;
  }

  mDelayedConnectTask->Cancel();
  ClearDelayedConnectTask();
}

nsresult
StreamSocketIO::Accept(int aFd,
                       const union sockaddr_any* aAddr, socklen_t aAddrLen)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_CONNECTING);

  

  if (!mConnector->SetUp(aFd)) {
    NS_WARNING("Could not set up socket!");
    return NS_ERROR_FAILURE;
  }

  if (!SetSocketFlags(aFd)) {
    return NS_ERROR_FAILURE;
  }
  SetSocket(aFd, SOCKET_IS_CONNECTED);

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }

  

  memcpy(&mAddr, aAddr, aAddrLen);
  mAddrSize = aAddrLen;

  

  nsRefPtr<nsRunnable> r =
    new SocketIOEventRunnable<StreamSocketIO>(
      this, SocketIOEventRunnable<StreamSocketIO>::CONNECT_SUCCESS);
  NS_DispatchToMainThread(r);

  return NS_OK;
}

void
StreamSocketIO::Connect()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(mConnector);

  if (!IsOpen()) {
    int fd = mConnector->Create();
    if (fd < 0) {
      NS_WARNING("Cannot create socket fd!");
      FireSocketError();
      return;
    }
    if (!SetSocketFlags(fd)) {
      NS_WARNING("Cannot set socket flags!");
      FireSocketError();
      return;
    }
    SetFd(fd);
  }

  if (!mConnector->CreateAddr(false, mAddrSize, mAddr, mAddress.get())) {
    NS_WARNING("Cannot create socket address!");
    FireSocketError();
    return;
  }

  
  nsresult rv = UnixSocketWatcher::Connect(
    reinterpret_cast<struct sockaddr*>(&mAddr), mAddrSize);
  NS_WARN_IF(NS_FAILED(rv));
}

void
StreamSocketIO::Send(UnixSocketIOBuffer* aData)
{
  EnqueueData(aData);
  AddWatchers(WRITE_WATCHER, false);
}

void
StreamSocketIO::OnAccepted(int aFd,
                           const sockaddr_any* aAddr,
                           socklen_t aAddrLen)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_LISTENING);
  MOZ_ASSERT(aAddr);
  MOZ_ASSERT(aAddrLen <= static_cast<socklen_t>(sizeof(mAddr)));

  memcpy (&mAddr, aAddr, aAddrLen);
  mAddrSize = aAddrLen;

  if (!mConnector->SetUp(aFd)) {
    NS_WARNING("Could not set up socket!");
    return;
  }

  RemoveWatchers(READ_WATCHER|WRITE_WATCHER);
  Close();
  if (!SetSocketFlags(aFd)) {
    return;
  }
  SetSocket(aFd, SOCKET_IS_CONNECTED);

  nsRefPtr<nsRunnable> r =
    new SocketIOEventRunnable<StreamSocketIO>(
      this, SocketIOEventRunnable<StreamSocketIO>::CONNECT_SUCCESS);
  NS_DispatchToMainThread(r);

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
StreamSocketIO::OnConnected()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_CONNECTED);

  if (!SetSocketFlags(GetFd())) {
    NS_WARNING("Cannot set socket flags!");
    FireSocketError();
    return;
  }

  if (!mConnector->SetUp(GetFd())) {
    NS_WARNING("Could not set up socket!");
    FireSocketError();
    return;
  }

  nsRefPtr<nsRunnable> r =
    new SocketIOEventRunnable<StreamSocketIO>(
      this, SocketIOEventRunnable<StreamSocketIO>::CONNECT_SUCCESS);
  NS_DispatchToMainThread(r);

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
StreamSocketIO::OnListening()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  NS_NOTREACHED("Invalid call to |StreamSocketIO::OnListening|");
}

void
StreamSocketIO::OnError(const char* aFunction, int aErrno)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  UnixFdWatcher::OnError(aFunction, aErrno);
  FireSocketError();
}

void
StreamSocketIO::OnSocketCanReceiveWithoutBlocking()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_CONNECTED); 

  ssize_t res = ReceiveData(GetFd(), this);
  if (res < 0) {
    
    RemoveWatchers(READ_WATCHER|WRITE_WATCHER);
  } else if (!res) {
    
    RemoveWatchers(READ_WATCHER);
  }
}

void
StreamSocketIO::OnSocketCanSendWithoutBlocking()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_CONNECTED); 

  nsresult rv = SendPendingData(GetFd(), this);
  if (NS_FAILED(rv)) {
    return;
  }

  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
StreamSocketIO::FireSocketError()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  
  Close();

  
  nsRefPtr<nsRunnable> r =
    new SocketIOEventRunnable<StreamSocketIO>(
      this, SocketIOEventRunnable<StreamSocketIO>::CONNECT_ERROR);

  NS_DispatchToMainThread(r);
}

bool
StreamSocketIO::SetSocketFlags(int aFd)
{
  static const int reuseaddr = 1;

  
  int res = setsockopt(aFd, SOL_SOCKET, SO_REUSEADDR,
                       &reuseaddr, sizeof(reuseaddr));
  if (res < 0) {
    return false;
  }

  
  int flags = TEMP_FAILURE_RETRY(fcntl(aFd, F_GETFD));
  if (-1 == flags) {
    return false;
  }
  flags |= FD_CLOEXEC;
  if (-1 == TEMP_FAILURE_RETRY(fcntl(aFd, F_SETFD, flags))) {
    return false;
  }

  
  flags = TEMP_FAILURE_RETRY(fcntl(aFd, F_GETFL));
  if (-1 == flags) {
    return false;
  }
  flags |= O_NONBLOCK;
  if (-1 == TEMP_FAILURE_RETRY(fcntl(aFd, F_SETFL, flags))) {
    return false;
  }

  return true;
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
    MOZ_ASSERT(!NS_IsMainThread());
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
    MOZ_ASSERT(NS_IsMainThread());

    if (IsCanceled()) {
      return;
    }

    StreamSocketIO* io = GetIO();
    if (io->IsShutdownOnMainThread()) {
      return;
    }

    io->ClearDelayedConnectTask();
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE, new ConnectTask(io));
  }
};





StreamSocket::StreamSocket()
: mIO(nullptr)
{ }

StreamSocket::~StreamSocket()
{
  MOZ_ASSERT(!mIO);
}

void
StreamSocket::SendSocketData(UnixSocketIOBuffer* aBuffer)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mIO);

  MOZ_ASSERT(!mIO->IsShutdownOnMainThread());
  XRE_GetIOMessageLoop()->PostTask(
    FROM_HERE,
    new SocketIOSendTask<StreamSocketIO, UnixSocketIOBuffer>(mIO, aBuffer));
}

bool
StreamSocket::SendSocketData(const nsACString& aStr)
{
  if (aStr.Length() > MAX_READ_SIZE) {
    return false;
  }

  SendSocketData(new UnixSocketRawData(aStr.BeginReading(), aStr.Length()));

  return true;
}

void
StreamSocket::Close()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mIO) {
    return;
  }

  mIO->CancelDelayedConnectTask();

  
  
  
  mIO->ShutdownOnMainThread();

  XRE_GetIOMessageLoop()->PostTask(
    FROM_HERE, new SocketIOShutdownTask<StreamSocketIO>(mIO));

  mIO = nullptr;

  NotifyDisconnect();
}

void
StreamSocket::GetSocketAddr(nsAString& aAddrStr)
{
  aAddrStr.Truncate();
  if (!mIO || GetConnectionStatus() != SOCKET_CONNECTED) {
    NS_WARNING("No socket currently open!");
    return;
  }
  mIO->GetSocketAddr(aAddrStr);
}

bool
StreamSocket::Connect(UnixSocketConnector* aConnector,
                      const char* aAddress,
                      int aDelayMs)
{
  MOZ_ASSERT(aConnector);
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<UnixSocketConnector> connector(aConnector);

  if (mIO) {
    NS_WARNING("Socket already connecting/connected!");
    return false;
  }

  nsCString addr(aAddress);
  MessageLoop* ioLoop = XRE_GetIOMessageLoop();
  mIO = new StreamSocketIO(ioLoop, this, connector.forget(), addr);
  SetConnectionStatus(SOCKET_CONNECTING);
  if (aDelayMs > 0) {
    StreamSocketIO::DelayedConnectTask* connectTask =
      new StreamSocketIO::DelayedConnectTask(mIO);
    mIO->SetDelayedConnectTask(connectTask);
    MessageLoop::current()->PostDelayedTask(FROM_HERE, connectTask, aDelayMs);
  } else {
    ioLoop->PostTask(FROM_HERE, new StreamSocketIO::ConnectTask(mIO));
  }

  return true;
}

ConnectionOrientedSocketIO*
StreamSocket::PrepareAccept(UnixSocketConnector* aConnector)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mIO);
  MOZ_ASSERT(aConnector);

  nsAutoPtr<UnixSocketConnector> connector(aConnector);

  SetConnectionStatus(SOCKET_CONNECTING);

  mIO = new StreamSocketIO(XRE_GetIOMessageLoop(),
                           -1, UnixSocketWatcher::SOCKET_IS_CONNECTING,
                           this, connector.forget(), EmptyCString());
  return mIO;
}

} 
} 
