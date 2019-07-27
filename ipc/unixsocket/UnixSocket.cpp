





#include "UnixSocket.h"
#include "nsTArray.h"
#include "nsXULAppAPI.h"
#include <fcntl.h>
#include "mozilla/unused.h"

static const size_t MAX_READ_SIZE = 1 << 16;

namespace mozilla {
namespace ipc {





class UnixSocketConsumerIO MOZ_FINAL : public UnixSocketWatcher
                                     , protected SocketIOBase
{
public:
  UnixSocketConsumerIO(MessageLoop* mIOLoop,
                       UnixSocketConsumer* aConsumer,
                       UnixSocketConnector* aConnector,
                       const nsACString& aAddress);
  ~UnixSocketConsumerIO();

  void                GetSocketAddr(nsAString& aAddrStr) const;
  SocketConsumerBase* GetConsumer();

  
  

  bool IsShutdownOnMainThread() const;
  void ShutdownOnMainThread();

  bool IsShutdownOnIOThread() const;
  void ShutdownOnIOThread();

  
  

  void SetDelayedConnectTask(CancelableTask* aTask);
  void ClearDelayedConnectTask();
  void CancelDelayedConnectTask();

  
  

  


  void Listen();

  


  void Connect();

  void Send(UnixSocketRawData* aData);

  
  

  void OnAccepted(int aFd, const sockaddr_any* aAddr,
                  socklen_t aAddrLen) MOZ_OVERRIDE;
  void OnConnected() MOZ_OVERRIDE;
  void OnError(const char* aFunction, int aErrno) MOZ_OVERRIDE;
  void OnListening() MOZ_OVERRIDE;
  void OnSocketCanReceiveWithoutBlocking() MOZ_OVERRIDE;
  void OnSocketCanSendWithoutBlocking() MOZ_OVERRIDE;

private:
  void FireSocketError();

  
  static bool SetSocketFlags(int aFd);

  




  RefPtr<UnixSocketConsumer> mConsumer;

  


  nsAutoPtr<UnixSocketConnector> mConnector;

  


  bool mShuttingDownOnIOThread;

  


  nsCString mAddress;

  


  socklen_t mAddrSize;

  


  sockaddr_any mAddr;

  


  CancelableTask* mDelayedConnectTask;
};

UnixSocketConsumerIO::UnixSocketConsumerIO(MessageLoop* mIOLoop,
                                           UnixSocketConsumer* aConsumer,
                                           UnixSocketConnector* aConnector,
                                           const nsACString& aAddress)
  : UnixSocketWatcher(mIOLoop)
  , SocketIOBase(MAX_READ_SIZE)
  , mConsumer(aConsumer)
  , mConnector(aConnector)
  , mShuttingDownOnIOThread(false)
  , mAddress(aAddress)
  , mDelayedConnectTask(nullptr)
{
  MOZ_ASSERT(mConsumer);
  MOZ_ASSERT(mConnector);
}

UnixSocketConsumerIO::~UnixSocketConsumerIO()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(IsShutdownOnMainThread());
}

void
UnixSocketConsumerIO::GetSocketAddr(nsAString& aAddrStr) const
{
  if (!mConnector) {
    NS_WARNING("No connector to get socket address from!");
    aAddrStr.Truncate();
    return;
  }
  mConnector->GetSocketAddr(mAddr, aAddrStr);
}

SocketConsumerBase*
UnixSocketConsumerIO::GetConsumer()
{
  return mConsumer.get();
}

bool
UnixSocketConsumerIO::IsShutdownOnMainThread() const
{
  MOZ_ASSERT(NS_IsMainThread());

  return mConsumer == nullptr;
}

void
UnixSocketConsumerIO::ShutdownOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!IsShutdownOnMainThread());

  mConsumer = nullptr;
}

bool
UnixSocketConsumerIO::IsShutdownOnIOThread() const
{
  return mShuttingDownOnIOThread;
}

void
UnixSocketConsumerIO::ShutdownOnIOThread()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  Close(); 
  mShuttingDownOnIOThread = true;
}

void
UnixSocketConsumerIO::SetDelayedConnectTask(CancelableTask* aTask)
{
  MOZ_ASSERT(NS_IsMainThread());

  mDelayedConnectTask = aTask;
}

void
UnixSocketConsumerIO::ClearDelayedConnectTask()
{
  MOZ_ASSERT(NS_IsMainThread());

  mDelayedConnectTask = nullptr;
}

void
UnixSocketConsumerIO::CancelDelayedConnectTask()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mDelayedConnectTask) {
    return;
  }

  mDelayedConnectTask->Cancel();
  ClearDelayedConnectTask();
}

void
UnixSocketConsumerIO::Listen()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(mConnector);

  
  
  if (!mConnector->CreateAddr(true, mAddrSize, mAddr, nullptr)) {
    NS_WARNING("Cannot create socket address!");
    FireSocketError();
    return;
  }

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

    
    nsresult rv = UnixSocketWatcher::Listen(
      reinterpret_cast<struct sockaddr*>(&mAddr), mAddrSize);
    NS_WARN_IF(NS_FAILED(rv));
  }
}

void
UnixSocketConsumerIO::Connect()
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
UnixSocketConsumerIO::Send(UnixSocketRawData* aData)
{
  EnqueueData(aData);
  AddWatchers(WRITE_WATCHER, false);
}

void
UnixSocketConsumerIO::OnAccepted(int aFd,
                                 const sockaddr_any* aAddr,
                                 socklen_t aAddrLen)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_LISTENING);
  MOZ_ASSERT(aAddr);
  MOZ_ASSERT(aAddrLen <= sizeof(mAddr));

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
    new SocketIOEventRunnable<UnixSocketConsumerIO>(
      this, SocketIOEventRunnable<UnixSocketConsumerIO>::CONNECT_SUCCESS);
  NS_DispatchToMainThread(r);

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
UnixSocketConsumerIO::OnConnected()
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
    new SocketIOEventRunnable<UnixSocketConsumerIO>(
      this, SocketIOEventRunnable<UnixSocketConsumerIO>::CONNECT_SUCCESS);
  NS_DispatchToMainThread(r);

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
UnixSocketConsumerIO::OnListening()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_LISTENING);

  if (!mConnector->SetUpListenSocket(GetFd())) {
    NS_WARNING("Could not set up listen socket!");
    FireSocketError();
    return;
  }

  AddWatchers(READ_WATCHER, true);
}

void
UnixSocketConsumerIO::OnError(const char* aFunction, int aErrno)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  UnixFdWatcher::OnError(aFunction, aErrno);
  FireSocketError();
}

void
UnixSocketConsumerIO::OnSocketCanReceiveWithoutBlocking()
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
UnixSocketConsumerIO::OnSocketCanSendWithoutBlocking()
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
UnixSocketConsumerIO::FireSocketError()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  
  Close();

  
  nsRefPtr<nsRunnable> r =
    new SocketIOEventRunnable<UnixSocketConsumerIO>(
      this, SocketIOEventRunnable<UnixSocketConsumerIO>::CONNECT_ERROR);

  NS_DispatchToMainThread(r);
}

bool
UnixSocketConsumerIO::SetSocketFlags(int aFd)
{
  
  int n = 1;
  if (setsockopt(aFd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n)) < 0) {
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





class ListenTask MOZ_FINAL : public SocketIOTask<UnixSocketConsumerIO>
{
public:
  ListenTask(UnixSocketConsumerIO* aIO)
  : SocketIOTask<UnixSocketConsumerIO>(aIO)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());

    if (!IsCanceled()) {
      GetIO()->Listen();
    }
  }
};

class ConnectTask MOZ_FINAL : public SocketIOTask<UnixSocketConsumerIO>
{
public:
  ConnectTask(UnixSocketConsumerIO* aIO)
  : SocketIOTask<UnixSocketConsumerIO>(aIO)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!IsCanceled());

    GetIO()->Connect();
  }
};

class DelayedConnectTask MOZ_FINAL : public SocketIOTask<UnixSocketConsumerIO>
{
public:
  DelayedConnectTask(UnixSocketConsumerIO* aIO)
  : SocketIOTask<UnixSocketConsumerIO>(aIO)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (IsCanceled()) {
      return;
    }

    UnixSocketConsumerIO* io = GetIO();
    if (io->IsShutdownOnMainThread()) {
      return;
    }

    io->ClearDelayedConnectTask();
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE, new ConnectTask(io));
  }
};





UnixSocketConsumer::UnixSocketConsumer()
: mIO(nullptr)
{ }

UnixSocketConsumer::~UnixSocketConsumer()
{
  MOZ_ASSERT(!mIO);
}

bool
UnixSocketConsumer::SendSocketData(UnixSocketRawData* aData)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mIO) {
    return false;
  }

  MOZ_ASSERT(!mIO->IsShutdownOnMainThread());
  XRE_GetIOMessageLoop()->PostTask(
    FROM_HERE, new SocketIOSendTask<UnixSocketConsumerIO>(mIO, aData));

  return true;
}

bool
UnixSocketConsumer::SendSocketData(const nsACString& aStr)
{
  if (aStr.Length() > MAX_READ_SIZE) {
    return false;
  }

  nsAutoPtr<UnixSocketRawData> data(
    new UnixSocketRawData(aStr.BeginReading(), aStr.Length()));

  if (!SendSocketData(data)) {
    return false;
  }

  unused << data.forget();

  return true;
}

void
UnixSocketConsumer::CloseSocket()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mIO) {
    return;
  }

  mIO->CancelDelayedConnectTask();

  
  
  
  mIO->ShutdownOnMainThread();

  XRE_GetIOMessageLoop()->PostTask(
    FROM_HERE, new SocketIOShutdownTask<UnixSocketConsumerIO>(mIO));

  mIO = nullptr;

  NotifyDisconnect();
}

void
UnixSocketConsumer::GetSocketAddr(nsAString& aAddrStr)
{
  aAddrStr.Truncate();
  if (!mIO || GetConnectionStatus() != SOCKET_CONNECTED) {
    NS_WARNING("No socket currently open!");
    return;
  }
  mIO->GetSocketAddr(aAddrStr);
}

bool
UnixSocketConsumer::ConnectSocket(UnixSocketConnector* aConnector,
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
  mIO = new UnixSocketConsumerIO(ioLoop, this, connector.forget(), addr);
  SetConnectionStatus(SOCKET_CONNECTING);
  if (aDelayMs > 0) {
    DelayedConnectTask* connectTask = new DelayedConnectTask(mIO);
    mIO->SetDelayedConnectTask(connectTask);
    MessageLoop::current()->PostDelayedTask(FROM_HERE, connectTask, aDelayMs);
  } else {
    ioLoop->PostTask(FROM_HERE, new ConnectTask(mIO));
  }
  return true;
}

bool
UnixSocketConsumer::ListenSocket(UnixSocketConnector* aConnector)
{
  MOZ_ASSERT(aConnector);
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<UnixSocketConnector> connector(aConnector);

  if (mIO) {
    NS_WARNING("Socket already connecting/connected!");
    return false;
  }

  mIO = new UnixSocketConsumerIO(
    XRE_GetIOMessageLoop(), this, connector.forget(), EmptyCString());
  SetConnectionStatus(SOCKET_LISTENING);
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, new ListenTask(mIO));
  return true;
}

} 
} 
