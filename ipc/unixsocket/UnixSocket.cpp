





#include "UnixSocket.h"
#include "nsTArray.h"
#include "nsXULAppAPI.h"
#include <fcntl.h>

static const size_t MAX_READ_SIZE = 1 << 16;

namespace mozilla {
namespace ipc {





class UnixSocketImpl : public UnixSocketWatcher
                     , protected SocketIOBase
{
public:
  UnixSocketImpl(MessageLoop* mIOLoop,
                 UnixSocketConsumer* aConsumer, UnixSocketConnector* aConnector,
                 const nsACString& aAddress)
    : UnixSocketWatcher(mIOLoop)
    , SocketIOBase(MAX_READ_SIZE)
    , mConsumer(aConsumer)
    , mConnector(aConnector)
    , mShuttingDownOnIOThread(false)
    , mAddress(aAddress)
    , mDelayedConnectTask(nullptr)
  {
  }

  ~UnixSocketImpl()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(IsShutdownOnMainThread());
  }

  void Send(UnixSocketRawData* aData)
  {
    EnqueueData(aData);
    AddWatchers(WRITE_WATCHER, false);
  }

  bool IsShutdownOnMainThread()
  {
    MOZ_ASSERT(NS_IsMainThread());
    return mConsumer == nullptr;
  }

  void ShutdownOnMainThread()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(!IsShutdownOnMainThread());
    mConsumer = nullptr;
  }

  bool IsShutdownOnIOThread()
  {
    return mShuttingDownOnIOThread;
  }

  void ShutdownOnIOThread()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!mShuttingDownOnIOThread);

    Close(); 
    mShuttingDownOnIOThread = true;
  }

  void SetDelayedConnectTask(CancelableTask* aTask)
  {
    MOZ_ASSERT(NS_IsMainThread());
    mDelayedConnectTask = aTask;
  }

  void ClearDelayedConnectTask()
  {
    MOZ_ASSERT(NS_IsMainThread());
    mDelayedConnectTask = nullptr;
  }

  void CancelDelayedConnectTask()
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (!mDelayedConnectTask) {
      return;
    }
    mDelayedConnectTask->Cancel();
    ClearDelayedConnectTask();
  }

  


  void Connect();

  


  void Listen();

  void GetSocketAddr(nsAString& aAddrStr)
  {
    if (!mConnector) {
      NS_WARNING("No connector to get socket address from!");
      aAddrStr.Truncate();
      return;
    }
    mConnector->GetSocketAddr(mAddr, aAddrStr);
  }

  




  RefPtr<UnixSocketConsumer> mConsumer;

  void OnAccepted(int aFd, const sockaddr_any* aAddr,
                  socklen_t aAddrLen) MOZ_OVERRIDE;
  void OnConnected() MOZ_OVERRIDE;
  void OnError(const char* aFunction, int aErrno) MOZ_OVERRIDE;
  void OnListening() MOZ_OVERRIDE;
  void OnSocketCanReceiveWithoutBlocking() MOZ_OVERRIDE;
  void OnSocketCanSendWithoutBlocking() MOZ_OVERRIDE;

  SocketConsumerBase* GetConsumer()
  {
    return mConsumer.get();
  }

private:
  
  static bool SetSocketFlags(int aFd);

  void FireSocketError();

  


  nsAutoPtr<UnixSocketConnector> mConnector;

  


  bool mShuttingDownOnIOThread;

  


  nsCString mAddress;

  


  socklen_t mAddrSize;

  


  sockaddr_any mAddr;

  


  CancelableTask* mDelayedConnectTask;
};

class UnixSocketImplTask : public CancelableTask
{
public:
  UnixSocketImpl* GetImpl() const
  {
    return mImpl;
  }
  void Cancel() MOZ_OVERRIDE
  {
    mImpl = nullptr;
  }
  bool IsCanceled() const
  {
    return !mImpl;
  }
protected:
  UnixSocketImplTask(UnixSocketImpl* aImpl)
  : mImpl(aImpl)
  {
    MOZ_ASSERT(mImpl);
  }
private:
  UnixSocketImpl* mImpl;
};

class SocketSendTask : public UnixSocketImplTask
{
public:
  SocketSendTask(UnixSocketImpl* aImpl,
                 UnixSocketConsumer* aConsumer,
                 UnixSocketRawData* aData)
  : UnixSocketImplTask(aImpl)
  , mConsumer(aConsumer)
  , mData(aData)
  {
    MOZ_ASSERT(aConsumer);
    MOZ_ASSERT(aData);
  }
  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!IsCanceled());

    UnixSocketImpl* impl = GetImpl();
    MOZ_ASSERT(!impl->IsShutdownOnIOThread());

    impl->Send(mData);
  }
private:
  nsRefPtr<UnixSocketConsumer> mConsumer;
  UnixSocketRawData* mData;
};

class SocketListenTask : public UnixSocketImplTask
{
public:
  SocketListenTask(UnixSocketImpl* aImpl)
  : UnixSocketImplTask(aImpl)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    if (!IsCanceled()) {
      GetImpl()->Listen();
    }
  }
};

class SocketConnectTask : public UnixSocketImplTask
{
public:
  SocketConnectTask(UnixSocketImpl* aImpl)
  : UnixSocketImplTask(aImpl)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!IsCanceled());
    GetImpl()->Connect();
  }
};

class SocketDelayedConnectTask : public UnixSocketImplTask
{
public:
  SocketDelayedConnectTask(UnixSocketImpl* aImpl)
  : UnixSocketImplTask(aImpl)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (IsCanceled()) {
      return;
    }
    UnixSocketImpl* impl = GetImpl();
    if (impl->IsShutdownOnMainThread()) {
      return;
    }
    impl->ClearDelayedConnectTask();
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE, new SocketConnectTask(impl));
  }
};

class ShutdownSocketTask : public UnixSocketImplTask
{
public:
  ShutdownSocketTask(UnixSocketImpl* aImpl)
  : UnixSocketImplTask(aImpl)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!IsCanceled());

    UnixSocketImpl* impl = GetImpl();

    
    
    
    
    
    impl->ShutdownOnIOThread();

    nsRefPtr<nsIRunnable> r =
      new SocketIODeleteInstanceRunnable<UnixSocketImpl>(impl);
    nsresult rv = NS_DispatchToMainThread(r);
    NS_ENSURE_SUCCESS_VOID(rv);
  }
};

void
UnixSocketImpl::FireSocketError()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  
  Close();

  
  nsRefPtr<nsRunnable> r =
    new SocketIOEventRunnable<UnixSocketImpl>(
      this, SocketIOEventRunnable<UnixSocketImpl>::CONNECT_ERROR);

  NS_DispatchToMainThread(r);
}

void
UnixSocketImpl::Listen()
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
UnixSocketImpl::Connect()
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

bool
UnixSocketImpl::SetSocketFlags(int aFd)
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

void
UnixSocketImpl::OnAccepted(int aFd,
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
    new SocketIOEventRunnable<UnixSocketImpl>(
      this, SocketIOEventRunnable<UnixSocketImpl>::CONNECT_SUCCESS);
  NS_DispatchToMainThread(r);

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
UnixSocketImpl::OnConnected()
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
    new SocketIOEventRunnable<UnixSocketImpl>(
      this, SocketIOEventRunnable<UnixSocketImpl>::CONNECT_SUCCESS);
  NS_DispatchToMainThread(r);

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
UnixSocketImpl::OnListening()
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
UnixSocketImpl::OnError(const char* aFunction, int aErrno)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  UnixFdWatcher::OnError(aFunction, aErrno);
  FireSocketError();
}

void
UnixSocketImpl::OnSocketCanReceiveWithoutBlocking()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_CONNECTED); 

  nsresult rv = ReceiveData(GetFd(), this);
  if (NS_FAILED(rv)) {
    RemoveWatchers(READ_WATCHER|WRITE_WATCHER);
    return;
  }
}

void
UnixSocketImpl::OnSocketCanSendWithoutBlocking()
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





UnixSocketConsumer::UnixSocketConsumer()
: mImpl(nullptr)
{ }

UnixSocketConsumer::~UnixSocketConsumer()
{
  MOZ_ASSERT(!mImpl);
}

bool
UnixSocketConsumer::SendSocketData(UnixSocketRawData* aData)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mImpl) {
    return false;
  }

  MOZ_ASSERT(!mImpl->IsShutdownOnMainThread());
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new SocketSendTask(mImpl, this, aData));
  return true;
}

bool
UnixSocketConsumer::SendSocketData(const nsACString& aStr)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mImpl) {
    return false;
  }
  if (aStr.Length() > MAX_READ_SIZE) {
    return false;
  }

  MOZ_ASSERT(!mImpl->IsShutdownOnMainThread());
  UnixSocketRawData* d = new UnixSocketRawData(aStr.BeginReading(),
                                               aStr.Length());
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new SocketSendTask(mImpl, this, d));
  return true;
}

void
UnixSocketConsumer::CloseSocket()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mImpl) {
    return;
  }

  mImpl->CancelDelayedConnectTask();

  
  
  
  mImpl->ShutdownOnMainThread();

  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new ShutdownSocketTask(mImpl));

  mImpl = nullptr;

  NotifyDisconnect();
}

void
UnixSocketConsumer::GetSocketAddr(nsAString& aAddrStr)
{
  aAddrStr.Truncate();
  if (!mImpl || GetConnectionStatus() != SOCKET_CONNECTED) {
    NS_WARNING("No socket currently open!");
    return;
  }
  mImpl->GetSocketAddr(aAddrStr);
}

bool
UnixSocketConsumer::ConnectSocket(UnixSocketConnector* aConnector,
                                  const char* aAddress,
                                  int aDelayMs)
{
  MOZ_ASSERT(aConnector);
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<UnixSocketConnector> connector(aConnector);

  if (mImpl) {
    NS_WARNING("Socket already connecting/connected!");
    return false;
  }

  nsCString addr(aAddress);
  MessageLoop* ioLoop = XRE_GetIOMessageLoop();
  mImpl = new UnixSocketImpl(ioLoop, this, connector.forget(), addr);
  SetConnectionStatus(SOCKET_CONNECTING);
  if (aDelayMs > 0) {
    SocketDelayedConnectTask* connectTask = new SocketDelayedConnectTask(mImpl);
    mImpl->SetDelayedConnectTask(connectTask);
    MessageLoop::current()->PostDelayedTask(FROM_HERE, connectTask, aDelayMs);
  } else {
    ioLoop->PostTask(FROM_HERE, new SocketConnectTask(mImpl));
  }
  return true;
}

bool
UnixSocketConsumer::ListenSocket(UnixSocketConnector* aConnector)
{
  MOZ_ASSERT(aConnector);
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<UnixSocketConnector> connector(aConnector);

  if (mImpl) {
    NS_WARNING("Socket already connecting/connected!");
    return false;
  }

  mImpl = new UnixSocketImpl(XRE_GetIOMessageLoop(), this, connector.forget(),
                             EmptyCString());
  SetConnectionStatus(SOCKET_LISTENING);
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new SocketListenTask(mImpl));
  return true;
}

} 
} 
