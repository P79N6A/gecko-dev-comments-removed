





#include "UnixSocket.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/socket.h>

#include "base/eintr_wrapper.h"
#include "base/message_loop.h"

#include "mozilla/Monitor.h"
#include "mozilla/Util.h"
#include "mozilla/FileUtils.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsTArray.h"
#include "nsXULAppAPI.h"

static const size_t MAX_READ_SIZE = 1 << 16;

#undef LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GonkDBus", args);
#else
#define BTDEBUG true
#define LOG(args...) if (BTDEBUG) printf(args);
#endif

static const int SOCKET_RETRY_TIME_MS = 1000;

namespace mozilla {
namespace ipc {

class UnixSocketImpl : public MessageLoopForIO::Watcher
{
public:
  UnixSocketImpl(UnixSocketConsumer* aConsumer, UnixSocketConnector* aConnector,
                 const nsACString& aAddress)
    : mConsumer(aConsumer)
    , mIOLoop(nullptr)
    , mConnector(aConnector)
    , mShuttingDownOnIOThread(false)
    , mAddress(aAddress)
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
    OnFileCanWriteWithoutBlocking(mFd);
  }

  bool isFdValid()
  {
    return mFd > 0;
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

    mReadWatcher.StopWatchingFileDescriptor();
    mWriteWatcher.StopWatchingFileDescriptor();

    mShuttingDownOnIOThread = true;
  }

  void SetUpIO()
  {
    MOZ_ASSERT(!mIOLoop);
    MOZ_ASSERT(mFd >= 0);
    mIOLoop = MessageLoopForIO::current();
    mIOLoop->WatchFileDescriptor(mFd,
                                 true,
                                 MessageLoopForIO::WATCH_READ,
                                 &mReadWatcher,
                                 this);
  }

  


  void Connect();

  


  void Listen();

  


  void Accept();

  




  bool SetNonblockFlags();

  void GetSocketAddr(nsAString& aAddrStr)
  {
    if (!mConnector)
    {
      NS_WARNING("No connector to get socket address from!");
      aAddrStr.Truncate();
      return;
    }
    mConnector->GetSocketAddr(mAddr, aAddrStr);
  }

  




  RefPtr<UnixSocketConsumer> mConsumer;

private:
  





  virtual void OnFileCanReadWithoutBlocking(int aFd);

  





  virtual void OnFileCanWriteWithoutBlocking(int aFd);

  


  MessageLoopForIO* mIOLoop;

  


  typedef nsTArray<UnixSocketRawData* > UnixSocketRawDataQueue;
  UnixSocketRawDataQueue mOutgoingQ;

  


  MessageLoopForIO::FileDescriptorWatcher mReadWatcher;

  


  MessageLoopForIO::FileDescriptorWatcher mWriteWatcher;

  



  ScopedClose mFd;

  


  nsAutoPtr<UnixSocketConnector> mConnector;

  


  bool mShuttingDownOnIOThread;

  


  nsCString mAddress;

  


  socklen_t mAddrSize;

  


  sockaddr mAddr;
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
    if(mImpl->IsShutdownOnMainThread()) {
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

class SocketAcceptTask : public CancelableTask {
  virtual void Run();

  UnixSocketImpl* mImpl;
public:
  SocketAcceptTask(UnixSocketImpl* aImpl) : mImpl(aImpl) { }

  virtual void Cancel()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    mImpl = nullptr;
  }
};

void SocketAcceptTask::Run()
{
  MOZ_ASSERT(!NS_IsMainThread());

  if (mImpl) {
    mImpl->Accept();
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
UnixSocketImpl::Accept()
{
  MOZ_ASSERT(!NS_IsMainThread());

  if (!mConnector) {
    NS_WARNING("No connector object available!");
    return;
  }

  
  
  mConnector->CreateAddr(true, mAddrSize, &mAddr, nullptr);

  if(mFd.get() < 0)
  {
    mFd = mConnector->Create();
    if (mFd.get() < 0) {
      return;
    }

    if (!SetNonblockFlags()) {
      return;
    }

    if (bind(mFd.get(), &mAddr, mAddrSize)) {
#ifdef DEBUG
      LOG("...bind(%d) gave errno %d", mFd.get(), errno);
#endif
      return;
    }

    if (listen(mFd.get(), 1)) {
#ifdef DEBUG
      LOG("...listen(%d) gave errno %d", mFd.get(), errno);
#endif
      return;
    }

  }

  SetUpIO();
}

void
UnixSocketImpl::Connect()
{
  MOZ_ASSERT(!NS_IsMainThread());

  if (!mConnector) {
    NS_WARNING("No connector object available!");
    return;
  }

  if(mFd.get() < 0)
  {
    mFd = mConnector->Create();
    if (mFd.get() < 0) {
      return;
    }
  }

  int ret;

  mConnector->CreateAddr(false, mAddrSize, &mAddr, mAddress.get());

  ret = connect(mFd.get(), &mAddr, mAddrSize);

  if (ret) {
#if DEBUG
    LOG("Socket connect errno=%d\n", errno);
#endif
    mFd.reset(-1);
    nsRefPtr<OnSocketEventTask> t =
      new OnSocketEventTask(this, OnSocketEventTask::CONNECT_ERROR);
    NS_DispatchToMainThread(t);
    return;
  }

  if (!mConnector->SetUp(mFd)) {
    NS_WARNING("Could not set up socket!");
    return;
  }

  nsRefPtr<OnSocketEventTask> t =
    new OnSocketEventTask(this, OnSocketEventTask::CONNECT_SUCCESS);
  NS_DispatchToMainThread(t);

  SetUpIO();
}

bool
UnixSocketImpl::SetNonblockFlags()
{
  
  int n = 1;
  setsockopt(mFd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));

  
  int flags = fcntl(mFd, F_GETFD);
  if (-1 == flags) {
    return false;
  }

  flags |= FD_CLOEXEC;
  if (-1 == fcntl(mFd, F_SETFD, flags)) {
    return false;
  }

  return true;
}

UnixSocketConsumer::UnixSocketConsumer() : mImpl(nullptr)
                                         , mConnectionStatus(SOCKET_DISCONNECTED)
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
  UnixSocketRawData* d = new UnixSocketRawData(aStr.Length());
  memcpy(d->mData, aStr.BeginReading(), aStr.Length());
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

  
  
  
  mImpl->ShutdownOnMainThread();

  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new ShutdownSocketTask(mImpl));

  mImpl = nullptr;

  NotifyDisconnect();
}

void
UnixSocketImpl::OnFileCanReadWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  SocketConnectionStatus status = mConsumer->GetConnectionStatus();
  if (status == SOCKET_CONNECTED) {
    
    while (true) {
      uint8_t data[MAX_READ_SIZE];
      ssize_t ret = read(aFd, data, MAX_READ_SIZE);
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

        
        
        mReadWatcher.StopWatchingFileDescriptor();
        mWriteWatcher.StopWatchingFileDescriptor();
        nsRefPtr<RequestClosingSocketTask> t = new RequestClosingSocketTask(this);
        NS_DispatchToMainThread(t);
        return;
      }

      UnixSocketRawData* incoming = new UnixSocketRawData(ret);
      memcpy(incoming->mData, data, ret);
      nsRefPtr<SocketReceiveTask> t = new SocketReceiveTask(this, incoming);
      NS_DispatchToMainThread(t);

      
      
      if (ret < ssize_t(MAX_READ_SIZE)) {
        return;
      }
    }

    MOZ_NOT_REACHED("We returned early");
  }

  if (status == SOCKET_LISTENING) {
    int client_fd = accept(mFd.get(), &mAddr, &mAddrSize);

    if (client_fd < 0) {
      return;
    }

    if (!mConnector->SetUp(client_fd)) {
      NS_WARNING("Could not set up socket!");
      return;
    }

    mReadWatcher.StopWatchingFileDescriptor();
    mWriteWatcher.StopWatchingFileDescriptor();

    mFd.reset(client_fd);
    mIOLoop = nullptr;

    nsRefPtr<OnSocketEventTask> t =
      new OnSocketEventTask(this, OnSocketEventTask::CONNECT_SUCCESS);
    NS_DispatchToMainThread(t);

    SetUpIO();
  }
}

void
UnixSocketImpl::OnFileCanWriteWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  
  
  
  
  
  
  MOZ_ASSERT(aFd >= 0);
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
      written = write (aFd, toWrite + data->mCurrentWriteOffset,
                       write_amount);
      if (written > 0) {
        data->mCurrentWriteOffset += written;
      }
      if (written != write_amount) {
        break;
      }
    }

    if (data->mCurrentWriteOffset != data->mSize) {
      MessageLoopForIO::current()->WatchFileDescriptor(
        aFd,
        false,
        MessageLoopForIO::WATCH_WRITE,
        &mWriteWatcher,
        this);
      return;
    }
    mOutgoingQ.RemoveElementAt(0);
    delete data;
  }
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
  OnConnectSuccess();
}

void
UnixSocketConsumer::NotifyError()
{
  MOZ_ASSERT(NS_IsMainThread());
  mConnectionStatus = SOCKET_DISCONNECTED;
  OnConnectError();
}

void
UnixSocketConsumer::NotifyDisconnect()
{
  MOZ_ASSERT(NS_IsMainThread());
  mConnectionStatus = SOCKET_DISCONNECTED;
  OnDisconnect();
}

bool
UnixSocketConsumer::ConnectSocket(UnixSocketConnector* aConnector,
                                  const char* aAddress,
                                  int aDelayMs)
{
  MOZ_ASSERT(aConnector);
  MOZ_ASSERT(NS_IsMainThread());
  if (mImpl) {
    NS_WARNING("Socket already connecting/connected!");
    return false;
  }
  nsCString addr(aAddress);
  mImpl = new UnixSocketImpl(this, aConnector, addr);
  MessageLoop* ioLoop = XRE_GetIOMessageLoop();
  mConnectionStatus = SOCKET_CONNECTING;
  if (aDelayMs > 0) {
    ioLoop->PostDelayedTask(FROM_HERE, new SocketConnectTask(mImpl), aDelayMs);
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
  if (mImpl) {
    NS_WARNING("Socket already connecting/connected!");
    return false;
  }
  mImpl = new UnixSocketImpl(this, aConnector, EmptyCString());
  mConnectionStatus = SOCKET_LISTENING;
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new SocketAcceptTask(mImpl));
  return true;
}

} 
} 
