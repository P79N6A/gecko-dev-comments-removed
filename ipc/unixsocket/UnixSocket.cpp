





#include "UnixSocket.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/socket.h>

#include "base/eintr_wrapper.h"
#include "base/message_loop.h"

#include "mozilla/ipc/UnixSocketWatcher.h"
#include "mozilla/Monitor.h"
#include "mozilla/FileUtils.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsXULAppAPI.h"

static const size_t MAX_READ_SIZE = 1 << 16;

#undef CHROMIUM_LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define CHROMIUM_LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "I/O", args);
#else
#define IODEBUG true
#define CHROMIUM_LOG(args...) if (IODEBUG) printf(args);
#endif

namespace mozilla {
namespace ipc {

class UnixSocketImpl : public UnixSocketWatcher
{
public:
  UnixSocketImpl(MessageLoop* mIOLoop,
                 UnixSocketConsumer* aConsumer, UnixSocketConnector* aConnector,
                 const nsACString& aAddress)
    : UnixSocketWatcher(mIOLoop)
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

  void QueueWriteData(UnixSocketRawData* aData)
  {
    mOutgoingQ.AppendElement(aData);
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

    RemoveWatchers(READ_WATCHER|WRITE_WATCHER);

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

  void OnAccepted(int aFd) MOZ_OVERRIDE;
  void OnConnected() MOZ_OVERRIDE;
  void OnError(const char* aFunction, int aErrno) MOZ_OVERRIDE;
  void OnListening() MOZ_OVERRIDE;
  void OnSocketCanReceiveWithoutBlocking() MOZ_OVERRIDE;
  void OnSocketCanSendWithoutBlocking() MOZ_OVERRIDE;

private:
  
  static bool SetSocketFlags(int aFd);

  void FireSocketError();

  


  typedef nsTArray<UnixSocketRawData* > UnixSocketRawDataQueue;
  UnixSocketRawDataQueue mOutgoingQ;

  


  nsAutoPtr<UnixSocketConnector> mConnector;

  


  bool mShuttingDownOnIOThread;

  


  nsCString mAddress;

  


  socklen_t mAddrSize;

  


  sockaddr_any mAddr;

  


  CancelableTask* mDelayedConnectTask;
};

template<class T>
class DeleteInstanceRunnable : public nsRunnable
{
public:
  DeleteInstanceRunnable(T* aInstance)
  : mInstance(aInstance)
  { }

  NS_IMETHOD Run()
  {
    delete mInstance;

    return NS_OK;
  }

private:
  T* mInstance;
};

class OnSocketEventTask : public nsRunnable
{
public:
  enum SocketEvent {
    CONNECT_SUCCESS,
    CONNECT_ERROR,
    DISCONNECT
  };

  OnSocketEventTask(UnixSocketImpl* aImpl, SocketEvent e) :
    mImpl(aImpl),
    mEvent(e)
  {
    MOZ_ASSERT(aImpl);
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (mImpl->IsShutdownOnMainThread()) {
      NS_WARNING("CloseSocket has already been called!");
      
      
      return NS_OK;
    }
    if (mEvent == CONNECT_SUCCESS) {
      mImpl->mConsumer->NotifySuccess();
    } else if (mEvent == CONNECT_ERROR) {
      mImpl->mConsumer->NotifyError();
    } else if (mEvent == DISCONNECT) {
      mImpl->mConsumer->NotifyDisconnect();
    }
    return NS_OK;
  }
private:
  UnixSocketImpl* mImpl;
  SocketEvent mEvent;
};

class SocketReceiveTask : public nsRunnable
{
public:
  SocketReceiveTask(UnixSocketImpl* aImpl, UnixSocketRawData* aData) :
    mImpl(aImpl),
    mRawData(aData)
  {
    MOZ_ASSERT(aImpl);
    MOZ_ASSERT(aData);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (mImpl->IsShutdownOnMainThread()) {
      NS_WARNING("mConsumer is null, aborting receive!");
      
      
      return NS_OK;
    }

    MOZ_ASSERT(mImpl->mConsumer);
    mImpl->mConsumer->ReceiveSocketData(mRawData);
    return NS_OK;
  }
private:
  UnixSocketImpl* mImpl;
  nsAutoPtr<UnixSocketRawData> mRawData;
};

class SocketSendTask : public Task
{
public:
  SocketSendTask(UnixSocketConsumer* aConsumer, UnixSocketImpl* aImpl,
                 UnixSocketRawData* aData)
    : mConsumer(aConsumer),
      mImpl(aImpl),
      mData(aData)
  {
    MOZ_ASSERT(aConsumer);
    MOZ_ASSERT(aImpl);
    MOZ_ASSERT(aData);
  }

  void
  Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!mImpl->IsShutdownOnIOThread());

    mImpl->QueueWriteData(mData);
  }

private:
  nsRefPtr<UnixSocketConsumer> mConsumer;
  UnixSocketImpl* mImpl;
  UnixSocketRawData* mData;
};

class RequestClosingSocketTask : public nsRunnable
{
public:
  RequestClosingSocketTask(UnixSocketImpl* aImpl) : mImpl(aImpl)
  {
    MOZ_ASSERT(aImpl);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mImpl->IsShutdownOnMainThread()) {
      NS_WARNING("CloseSocket has already been called!");
      
      
      return NS_OK;
    }

    
    
    mImpl->mConsumer->CloseSocket();
    return NS_OK;
  }
private:
  UnixSocketImpl* mImpl;
};

class SocketListenTask : public CancelableTask
{
  virtual void Run();

  UnixSocketImpl* mImpl;
public:
  SocketListenTask(UnixSocketImpl* aImpl) : mImpl(aImpl) { }

  virtual void Cancel()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    mImpl = nullptr;
  }
};

void SocketListenTask::Run()
{
  MOZ_ASSERT(!NS_IsMainThread());

  if (mImpl) {
    mImpl->Listen();
  }
}

class SocketConnectTask : public Task {
  virtual void Run();

  UnixSocketImpl* mImpl;
public:
  SocketConnectTask(UnixSocketImpl* aImpl) : mImpl(aImpl) { }
};

void SocketConnectTask::Run()
{
  MOZ_ASSERT(!NS_IsMainThread());
  mImpl->Connect();
}

class SocketDelayedConnectTask : public CancelableTask {
  virtual void Run();

  UnixSocketImpl* mImpl;
public:
  SocketDelayedConnectTask(UnixSocketImpl* aImpl) : mImpl(aImpl) { }

  virtual void Cancel()
  {
    MOZ_ASSERT(NS_IsMainThread());
    mImpl = nullptr;
  }
};

void SocketDelayedConnectTask::Run()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mImpl || mImpl->IsShutdownOnMainThread()) {
    return;
  }
  mImpl->ClearDelayedConnectTask();
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, new SocketConnectTask(mImpl));
}

class ShutdownSocketTask : public Task {
  virtual void Run();

  UnixSocketImpl* mImpl;

public:
  ShutdownSocketTask(UnixSocketImpl* aImpl) : mImpl(aImpl) { }
};

void ShutdownSocketTask::Run()
{
  MOZ_ASSERT(!NS_IsMainThread());

  
  
  
  
  
  mImpl->ShutdownOnIOThread();

  nsRefPtr<nsIRunnable> t(new DeleteInstanceRunnable<UnixSocketImpl>(mImpl));
  nsresult rv = NS_DispatchToMainThread(t);
  NS_ENSURE_SUCCESS_VOID(rv);
}

void
UnixSocketImpl::FireSocketError()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  
  Close();

  
  nsRefPtr<OnSocketEventTask> t =
    new OnSocketEventTask(this, OnSocketEventTask::CONNECT_ERROR);
  NS_DispatchToMainThread(t);
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
UnixSocketImpl::OnAccepted(int aFd)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_LISTENING);

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

  nsRefPtr<OnSocketEventTask> t =
    new OnSocketEventTask(this, OnSocketEventTask::CONNECT_SUCCESS);
  NS_DispatchToMainThread(t);

  AddWatchers(READ_WATCHER, true);
  if (!mOutgoingQ.IsEmpty()) {
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

  nsRefPtr<OnSocketEventTask> t =
    new OnSocketEventTask(this, OnSocketEventTask::CONNECT_SUCCESS);
  NS_DispatchToMainThread(t);

  AddWatchers(READ_WATCHER, true);
  if (!mOutgoingQ.IsEmpty()) {
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

  
  while (true) {
    nsAutoPtr<UnixSocketRawData> incoming(new UnixSocketRawData(MAX_READ_SIZE));

    ssize_t ret = read(GetFd(), incoming->mData, incoming->mSize);
    if (ret <= 0) {
      if (ret == -1) {
        if (errno == EINTR) {
          continue; 
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          return; 
        }

#ifdef DEBUG
        NS_WARNING("Cannot read from network");
#endif
        
      }

      
      
      RemoveWatchers(READ_WATCHER|WRITE_WATCHER);
      nsRefPtr<RequestClosingSocketTask> t = new RequestClosingSocketTask(this);
      NS_DispatchToMainThread(t);
      return;
    }

    incoming->mSize = ret;
    nsRefPtr<SocketReceiveTask> t =
      new SocketReceiveTask(this, incoming.forget());
    NS_DispatchToMainThread(t);

    
    
    if (ret < ssize_t(MAX_READ_SIZE)) {
      return;
    }
  }
}

void
UnixSocketImpl::OnSocketCanSendWithoutBlocking()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_CONNECTED);

  
  
  
  
  
  
  while (true) {
    UnixSocketRawData* data;
    if (mOutgoingQ.IsEmpty()) {
      return;
    }
    data = mOutgoingQ.ElementAt(0);
    const uint8_t *toWrite;
    toWrite = data->mData;

    while (data->mCurrentWriteOffset < data->mSize) {
      ssize_t write_amount = data->mSize - data->mCurrentWriteOffset;
      ssize_t written;
      written = write (GetFd(), toWrite + data->mCurrentWriteOffset,
                         write_amount);
      if (written > 0) {
        data->mCurrentWriteOffset += written;
      }
      if (written != write_amount) {
        break;
      }
    }

    if (data->mCurrentWriteOffset != data->mSize) {
      AddWatchers(WRITE_WATCHER, false);
      return;
    }
    mOutgoingQ.RemoveElementAt(0);
    delete data;
  }
}

UnixSocketConsumer::UnixSocketConsumer() : mImpl(nullptr)
                                         , mConnectionStatus(SOCKET_DISCONNECTED)
                                         , mConnectTimestamp(0)
                                         , mConnectDelayMs(0)
{
}

UnixSocketConsumer::~UnixSocketConsumer()
{
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
                                   new SocketSendTask(this, mImpl, aData));
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
                                   new SocketSendTask(this, mImpl, d));
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
  if (!mImpl || mConnectionStatus != SOCKET_CONNECTED) {
    NS_WARNING("No socket currently open!");
    return;
  }
  mImpl->GetSocketAddr(aAddrStr);
}

void
UnixSocketConsumer::NotifySuccess()
{
  MOZ_ASSERT(NS_IsMainThread());
  mConnectionStatus = SOCKET_CONNECTED;
  mConnectTimestamp = PR_IntervalNow();
  OnConnectSuccess();
}

void
UnixSocketConsumer::NotifyError()
{
  MOZ_ASSERT(NS_IsMainThread());
  mConnectionStatus = SOCKET_DISCONNECTED;
  mConnectDelayMs = CalculateConnectDelayMs();
  OnConnectError();
}

void
UnixSocketConsumer::NotifyDisconnect()
{
  MOZ_ASSERT(NS_IsMainThread());
  mConnectionStatus = SOCKET_DISCONNECTED;
  mConnectDelayMs = CalculateConnectDelayMs();
  OnDisconnect();
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
  mConnectionStatus = SOCKET_CONNECTING;
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
  mConnectionStatus = SOCKET_LISTENING;
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new SocketListenTask(mImpl));
  return true;
}

uint32_t
UnixSocketConsumer::CalculateConnectDelayMs() const
{
  MOZ_ASSERT(NS_IsMainThread());

  uint32_t connectDelayMs = mConnectDelayMs;

  if ((PR_IntervalNow()-mConnectTimestamp) > connectDelayMs) {
    
    connectDelayMs = 0;
  } else if (!connectDelayMs) {
    
    connectDelayMs = 1<<10;
  } else if (connectDelayMs < (1<<16)) {
    
    connectDelayMs <<= 1;
  }
  return connectDelayMs;
}

} 
} 
