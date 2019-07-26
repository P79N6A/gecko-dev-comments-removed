





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
    , mFd(-1)
    , mConnector(aConnector)
    , mCurrentTaskIsCanceled(false)
    , mTask(nullptr)
    , mAddress(aAddress)
    , mLock("UnixSocketImpl.mLock")
  {
  }

  ~UnixSocketImpl()
  {
    StopTask();
    mReadWatcher.StopWatchingFileDescriptor();
    mWriteWatcher.StopWatchingFileDescriptor();
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

  void CancelTask()
  {
    MutexAutoLock lock(mLock);
    mCurrentTaskIsCanceled = true;
  }
  
  bool IsCanceled()
  {
    MutexAutoLock lock(mLock);
    return mCurrentTaskIsCanceled;
  }

  void UnsetTask()
  {
    mTask = nullptr;
  }

  void EnqueueTask(int aDelayMs, CancelableTask* aTask)
  {
    MessageLoopForIO* ioLoop = MessageLoopForIO::current();
    if (!ioLoop) {
      NS_WARNING("No IOLoop to attach to, cancelling self!");
      return;
    }
    if (mTask) {
      return;
    }
    if (IsCanceled()) {
      return;
    }
    mTask = aTask;
    if (aDelayMs) {
      ioLoop->PostDelayedTask(FROM_HERE, mTask, aDelayMs);
    } else {
      ioLoop->PostTask(FROM_HERE, mTask);
    }
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

  


  void Close();

  


  void StopTask()
  {
    if (mTask) {
      mTask->Cancel();
      mTask = nullptr;
    }
    MutexAutoLock lock(mLock);
    mCurrentTaskIsCanceled = true;
  }

  




  bool SetNonblockFlags();

  void GetSocketAddr(nsAString& aAddrStr)
  {
    if (!mConnector)
    {
      NS_WARNING("No connector to get socket address from!");
      aAddrStr = nsString();
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

  


  nsAutoPtr<UnixSocketRawData> mIncoming;

  


  MessageLoopForIO::FileDescriptorWatcher mReadWatcher;

  


  MessageLoopForIO::FileDescriptorWatcher mWriteWatcher;

  



  ScopedClose mFd;

  


  nsAutoPtr<UnixSocketConnector> mConnector;

  


  bool mCurrentTaskIsCanceled;

  



  CancelableTask* mTask;

  


  nsCString mAddress;

  


  socklen_t mAddrSize;

  


  sockaddr mAddr;

  


  mozilla::Mutex mLock;
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
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (!mImpl->mConsumer) {
      NS_WARNING("CloseSocket has already been called! (mConsumer is null)");
      
      
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
    if(!mImpl->mConsumer) {
      NS_WARNING("mConsumer is null, aborting receive!");
      
      
      return NS_OK;
    }
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
    mImpl->QueueWriteData(mData);
  }

private:
  nsRefPtr<UnixSocketConsumer> mConsumer;
  UnixSocketImpl* mImpl;
  UnixSocketRawData* mData;
};

class SocketCloseTask : public Task
{
public:
  SocketCloseTask(UnixSocketImpl* aImpl)
    : mImpl(aImpl)
  {
    MOZ_ASSERT(aImpl);
  }

  void Run()
  {
    NS_ENSURE_TRUE_VOID(mImpl);

    mImpl->UnsetTask();
    mImpl->Close();
  }

private:
  UnixSocketImpl* mImpl;
};

class StartImplReadingTask : public Task
{
public:
  StartImplReadingTask(UnixSocketImpl* aImpl)
    : mImpl(aImpl)
  {
  }

  void
  Run()
  {
    mImpl->SetUpIO();
  }
private:
  UnixSocketImpl* mImpl;
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

    if(!mImpl->mConsumer) {
      NS_WARNING("CloseSocket has already been called! (mConsumer is null)");
      
      
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

  bool mCanceled;
  UnixSocketImpl* mImpl;
public:
  virtual void Cancel() { mCanceled = true; }
  SocketAcceptTask(UnixSocketImpl* aImpl) : mCanceled(false), mImpl(aImpl) { }
};

void SocketAcceptTask::Run() {
  mImpl->UnsetTask();
  if (mCanceled) {
    return;
  }
  mImpl->Accept();
}

class SocketConnectTask : public CancelableTask {
  virtual void Run();

  bool mCanceled;
  UnixSocketImpl* mImpl;
public:
  SocketConnectTask(UnixSocketImpl* aImpl) : mCanceled(false), mImpl(aImpl) { }
  virtual void Cancel() { mCanceled = true; }  
};

void SocketConnectTask::Run() {
  mImpl->UnsetTask();
  if (mCanceled) {
    return;
  }
  mImpl->Connect();
}

void
UnixSocketImpl::Close()
{
  mReadWatcher.StopWatchingFileDescriptor();
  mWriteWatcher.StopWatchingFileDescriptor();

  nsRefPtr<nsIRunnable> t(new DeleteInstanceRunnable<UnixSocketImpl>(this));
  NS_ENSURE_TRUE_VOID(t);
  nsresult rv = NS_DispatchToMainThread(t);
  NS_ENSURE_SUCCESS_VOID(rv);
}

void
UnixSocketImpl::Accept()
{

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

  
  
  
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new StartImplReadingTask(this));
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
  nsCString str(aStr);
  UnixSocketRawData* d = new UnixSocketRawData(aStr.Length());
  memcpy(d->mData, str.get(), aStr.Length());
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
  UnixSocketImpl* impl = mImpl;
  
  mImpl = nullptr;
  
  impl->mConsumer.forget();
  impl->CancelTask();

  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, new SocketCloseTask(impl));

  NotifyDisconnect();
}

void
UnixSocketImpl::OnFileCanReadWithoutBlocking(int aFd)
{
  enum SocketConnectionStatus status = mConsumer->GetConnectionStatus();

  if (status == SOCKET_CONNECTED) {

    
    
    
    
    
    
    
    
    while (true) {
      if (!mIncoming) {
        uint8_t data[MAX_READ_SIZE];
        ssize_t ret = read(aFd, data, MAX_READ_SIZE);
        if (ret < 0) {
          if (ret == -1) {
            if (errno == EINTR) {
              continue; 
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK) {
              return; 
            }
            
          }
#ifdef DEBUG
          NS_WARNING("Cannot read from network");
#endif
          
          
          mReadWatcher.StopWatchingFileDescriptor();
          mWriteWatcher.StopWatchingFileDescriptor();
          nsRefPtr<RequestClosingSocketTask> t = new RequestClosingSocketTask(this);
          NS_DispatchToMainThread(t);
          return;
        }
        if (ret) {
          mIncoming = new UnixSocketRawData(ret);
          memcpy(mIncoming->mData, data, ret);
          nsRefPtr<SocketReceiveTask> t =
            new SocketReceiveTask(this, mIncoming.forget());
          NS_DispatchToMainThread(t);
        }
        if (ret < ssize_t(MAX_READ_SIZE)) {
          return;
        }
      }
    }
  } else if (status == SOCKET_LISTENING) {

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

    
    
    
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     new StartImplReadingTask(this));
  }
}

void
UnixSocketImpl::OnFileCanWriteWithoutBlocking(int aFd)
{
  
  
  
  
  
  
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
  if (!mImpl || mConnectionStatus != SOCKET_CONNECTED) {
    NS_WARNING("No socket currently open!");
    aAddrStr = nsString();
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
  nsCString addr;
  addr.Assign(aAddress);
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
  nsCString addr;
  mImpl = new UnixSocketImpl(this, aConnector, addr);
  mConnectionStatus = SOCKET_LISTENING;
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new SocketAcceptTask(mImpl));
  return true;
}

void
UnixSocketConsumer::CancelSocketTask()
{
  mConnectionStatus = SOCKET_DISCONNECTED;
  if(!mImpl) {
    NS_WARNING("No socket implementation to cancel task on!");
    return;
  }
  mImpl->StopTask();
}

} 
} 
