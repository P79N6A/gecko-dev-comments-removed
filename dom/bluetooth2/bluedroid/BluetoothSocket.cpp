





#include "BluetoothSocket.h"

#include <fcntl.h>
#include <sys/socket.h>

#include "base/message_loop.h"
#include "BluetoothInterface.h"
#include "BluetoothSocketObserver.h"
#include "BluetoothUtils.h"
#include "mozilla/FileUtils.h"
#include "mozilla/RefPtr.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

using namespace mozilla::ipc;
USING_BLUETOOTH_NAMESPACE

static const size_t MAX_READ_SIZE = 1 << 16;
static const uint8_t UUID_OBEX_OBJECT_PUSH[] = {
  0x00, 0x00, 0x11, 0x05, 0x00, 0x00, 0x10, 0x00,
  0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB
};
static BluetoothSocketInterface* sBluetoothSocketInterface;


static bool
EnsureBluetoothSocketHalLoad()
{
  if (sBluetoothSocketInterface) {
    return true;
  }

  BluetoothInterface* btInf = BluetoothInterface::GetInstance();
  NS_ENSURE_TRUE(btInf, false);

  sBluetoothSocketInterface = btInf->GetBluetoothSocketInterface();
  NS_ENSURE_TRUE(sBluetoothSocketInterface, false);

  return true;
}

class mozilla::dom::bluetooth::DroidSocketImpl : public ipc::UnixFdWatcher
{
public:
  



















  enum ConnectionStatus {
    SOCKET_IS_DISCONNECTED = 0,
    SOCKET_IS_LISTENING,
    SOCKET_IS_CONNECTING,
    SOCKET_IS_CONNECTED
  };

  DroidSocketImpl(MessageLoop* aIOLoop, BluetoothSocket* aConsumer)
    : ipc::UnixFdWatcher(aIOLoop)
    , mConsumer(aConsumer)
    , mShuttingDownOnIOThread(false)
    , mConnectionStatus(SOCKET_IS_DISCONNECTED)
  { }

  ~DroidSocketImpl()
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  void QueueWriteData(UnixSocketRawData* aData)
  {
    mOutgoingQ.AppendElement(aData);
    OnFileCanWriteWithoutBlocking(GetFd());
  }

  bool IsShutdownOnMainThread()
  {
    MOZ_ASSERT(NS_IsMainThread());
    return mConsumer == nullptr;
  }

  bool IsShutdownOnIOThread()
  {
    return mShuttingDownOnIOThread;
  }

  void ShutdownOnMainThread()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(!IsShutdownOnMainThread());
    mConsumer = nullptr;
  }

  void ShutdownOnIOThread()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!mShuttingDownOnIOThread);

    Close(); 
    mShuttingDownOnIOThread = true;
  }

  void Connect(int aFd);
  void Listen(int aFd);
  void Accept(int aFd);

  void ConnectClientFd()
  {
    
    RemoveWatchers(READ_WATCHER);

    mConnectionStatus = SOCKET_IS_CONNECTED;

    
    AddWatchers(READ_WATCHER, true);
    AddWatchers(WRITE_WATCHER, false);
  }

  




  RefPtr<BluetoothSocket> mConsumer;

private:
  





  virtual void OnFileCanReadWithoutBlocking(int aFd);

  





  virtual void OnFileCanWriteWithoutBlocking(int aFd);

  void OnSocketCanReceiveWithoutBlocking(int aFd);
  void OnSocketCanAcceptWithoutBlocking(int aFd);
  void OnSocketCanSendWithoutBlocking(int aFd);
  void OnSocketCanConnectWithoutBlocking(int aFd);

  


  typedef nsTArray<UnixSocketRawData* > UnixSocketRawDataQueue;
  UnixSocketRawDataQueue mOutgoingQ;

  


  bool mShuttingDownOnIOThread;

  ConnectionStatus mConnectionStatus;
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

class OnSocketEventRunnable : public SocketIORunnable<DroidSocketImpl>
{
public:
  enum SocketEvent {
    CONNECT_SUCCESS,
    CONNECT_ERROR,
    DISCONNECT
  };

  OnSocketEventRunnable(DroidSocketImpl* aImpl, SocketEvent e)
  : SocketIORunnable<DroidSocketImpl>(aImpl)
  , mEvent(e)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    DroidSocketImpl* impl = GetIO();

    if (impl->IsShutdownOnMainThread()) {
      NS_WARNING("CloseSocket has already been called!");
      
      
      return NS_OK;
    }
    if (mEvent == CONNECT_SUCCESS) {
      impl->mConsumer->NotifySuccess();
    } else if (mEvent == CONNECT_ERROR) {
      impl->mConsumer->NotifyError();
    } else if (mEvent == DISCONNECT) {
      impl->mConsumer->NotifyDisconnect();
    }
    return NS_OK;
  }

private:
  SocketEvent mEvent;
};

class RequestClosingSocketTask : public SocketIORunnable<DroidSocketImpl>
{
public:
  RequestClosingSocketTask(DroidSocketImpl* aImpl)
  : SocketIORunnable<DroidSocketImpl>(aImpl)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    DroidSocketImpl* impl = GetIO();

    if (impl->IsShutdownOnMainThread()) {
      NS_WARNING("CloseSocket has already been called!");
      
      
      return NS_OK;
    }

    
    
    impl->mConsumer->CloseDroidSocket();
    return NS_OK;
  }
};

class ShutdownSocketTask : public Task {
  virtual void Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    
    
    
    
    
    mImpl->ShutdownOnIOThread();

    nsRefPtr<nsIRunnable> t(new DeleteInstanceRunnable<
                                  mozilla::dom::bluetooth::DroidSocketImpl>(mImpl));
    nsresult rv = NS_DispatchToMainThread(t);
    NS_ENSURE_SUCCESS_VOID(rv);
  }

  DroidSocketImpl* mImpl;

public:
  ShutdownSocketTask(DroidSocketImpl* aImpl) : mImpl(aImpl) { }
};

class SocketReceiveTask : public SocketIORunnable<DroidSocketImpl>
{
public:
  SocketReceiveTask(DroidSocketImpl* aImpl, UnixSocketRawData* aData)
  : SocketIORunnable<DroidSocketImpl>(aImpl)
  , mRawData(aData)
  {
    MOZ_ASSERT(aData);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    DroidSocketImpl* impl = GetIO();

    if (impl->IsShutdownOnMainThread()) {
      NS_WARNING("mConsumer is null, aborting receive!");
      
      
      return NS_OK;
    }

    MOZ_ASSERT(impl->mConsumer);
    impl->mConsumer->ReceiveSocketData(mRawData);
    return NS_OK;
  }

private:
  nsAutoPtr<UnixSocketRawData> mRawData;
};

class SocketSendTask : public Task
{
public:
  SocketSendTask(BluetoothSocket* aConsumer, DroidSocketImpl* aImpl,
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
  nsRefPtr<BluetoothSocket> mConsumer;
  DroidSocketImpl* mImpl;
  UnixSocketRawData* mData;
};

class DroidSocketImplTask : public CancelableTask
{
public:
  DroidSocketImpl* GetDroidSocketImpl() const
  {
    return mDroidSocketImpl;
  }
  void Cancel() MOZ_OVERRIDE
  {
    mDroidSocketImpl = nullptr;
  }
  bool IsCanceled() const
  {
    return !mDroidSocketImpl;
  }
protected:
  DroidSocketImplTask(DroidSocketImpl* aDroidSocketImpl)
  : mDroidSocketImpl(aDroidSocketImpl)
  {
    MOZ_ASSERT(mDroidSocketImpl);
  }
private:
  DroidSocketImpl* mDroidSocketImpl;
};

class SocketConnectTask : public DroidSocketImplTask
{
public:
  SocketConnectTask(DroidSocketImpl* aDroidSocketImpl, int aFd)
  : DroidSocketImplTask(aDroidSocketImpl)
  , mFd(aFd)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!IsCanceled());
    GetDroidSocketImpl()->Connect(mFd);
  }

private:
  int mFd;
};

class SocketListenTask : public DroidSocketImplTask
{
public:
  SocketListenTask(DroidSocketImpl* aDroidSocketImpl, int aFd)
  : DroidSocketImplTask(aDroidSocketImpl)
  , mFd(aFd)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    if (!IsCanceled()) {
      GetDroidSocketImpl()->Listen(mFd);
    }
  }

private:
  int mFd;
};

class SocketConnectClientFdTask : public Task
{
  virtual void Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    mImpl->ConnectClientFd();
  }

  DroidSocketImpl* mImpl;
public:
  SocketConnectClientFdTask(DroidSocketImpl* aImpl) : mImpl(aImpl) { }
};

void
DroidSocketImpl::Connect(int aFd)
{
  MOZ_ASSERT(aFd >= 0);

  int flags = TEMP_FAILURE_RETRY(fcntl(aFd, F_GETFL));
  NS_ENSURE_TRUE_VOID(flags >= 0);

  if (!(flags & O_NONBLOCK)) {
    int res = TEMP_FAILURE_RETRY(fcntl(aFd, F_SETFL, flags | O_NONBLOCK));
    NS_ENSURE_TRUE_VOID(!res);
  }

  SetFd(aFd);
  mConnectionStatus = SOCKET_IS_CONNECTING;

  AddWatchers(WRITE_WATCHER, false);
}

void
DroidSocketImpl::Listen(int aFd)
{
  MOZ_ASSERT(aFd >= 0);

  int flags = TEMP_FAILURE_RETRY(fcntl(aFd, F_GETFL));
  NS_ENSURE_TRUE_VOID(flags >= 0);

  if (!(flags & O_NONBLOCK)) {
    int res = TEMP_FAILURE_RETRY(fcntl(aFd, F_SETFL, flags | O_NONBLOCK));
    NS_ENSURE_TRUE_VOID(!res);
  }

  SetFd(aFd);
  mConnectionStatus = SOCKET_IS_LISTENING;

  AddWatchers(READ_WATCHER, true);
}

void
DroidSocketImpl::Accept(int aFd)
{
  Close();

  int flags = TEMP_FAILURE_RETRY(fcntl(aFd, F_GETFL));
  NS_ENSURE_TRUE_VOID(flags >= 0);

  if (!(flags & O_NONBLOCK)) {
    int res = TEMP_FAILURE_RETRY(fcntl(aFd, F_SETFL, flags | O_NONBLOCK));
    NS_ENSURE_TRUE_VOID(!res);
  }

  SetFd(aFd);
  mConnectionStatus = SOCKET_IS_CONNECTED;

  nsRefPtr<OnSocketEventRunnable> r =
    new OnSocketEventRunnable(this, OnSocketEventRunnable::CONNECT_SUCCESS);
  NS_DispatchToMainThread(r);

  AddWatchers(READ_WATCHER, true);
  if (!mOutgoingQ.IsEmpty()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
DroidSocketImpl::OnFileCanReadWithoutBlocking(int aFd)
{
  if (mConnectionStatus == SOCKET_IS_CONNECTED) {
    OnSocketCanReceiveWithoutBlocking(aFd);
  } else if (mConnectionStatus == SOCKET_IS_LISTENING) {
    OnSocketCanAcceptWithoutBlocking(aFd);
  } else {
    NS_NOTREACHED("invalid connection state for reading");
  }
}

void
DroidSocketImpl::OnSocketCanReceiveWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  
  while (true) {
    nsAutoPtr<UnixSocketRawData> incoming(new UnixSocketRawData(MAX_READ_SIZE));

    ssize_t ret = read(aFd, incoming->mData, incoming->mSize);

    if (ret <= 0) {
      if (ret == -1) {
        if (errno == EINTR) {
          continue; 
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          return; 
        }

        BT_WARNING("Cannot read from network");
        
      }

      
      
      RemoveWatchers(READ_WATCHER | WRITE_WATCHER);
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

  MOZ_CRASH("We returned early");
}

class AcceptTask MOZ_FINAL : public DroidSocketImplTask
{
public:
  AcceptTask(DroidSocketImpl* aDroidSocketImpl, int aFd)
  : DroidSocketImplTask(aDroidSocketImpl)
  , mFd(aFd)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!IsCanceled());

    GetDroidSocketImpl()->Accept(mFd);
  }

private:
  int mFd;
};

class AcceptResultHandler MOZ_FINAL : public BluetoothSocketResultHandler
{
public:
  AcceptResultHandler(DroidSocketImpl* aImpl)
  : mImpl(aImpl)
  {
    MOZ_ASSERT(mImpl);
  }

  void Accept(int aFd, const nsAString& aBdAddress,
              int aConnectionStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mImpl->IsShutdownOnMainThread()) {
      BT_LOGD("mConsumer is null, aborting receive!");
      return;
    }

    mImpl->mConsumer->SetAddress(aBdAddress);
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE, new AcceptTask(mImpl, aFd));
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    BT_LOGR("BluetoothSocketInterface::Accept failed: %d", (int)aStatus);
  }

private:
  DroidSocketImpl* mImpl;
};

class AcceptRunnable MOZ_FINAL : public SocketIORunnable<DroidSocketImpl>
{
public:
  AcceptRunnable(DroidSocketImpl* aImpl, int aFd)
  : SocketIORunnable<DroidSocketImpl>(aImpl)
  , mFd(aFd)
  { }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(sBluetoothSocketInterface);

    sBluetoothSocketInterface->Accept(mFd, new AcceptResultHandler(GetIO()));

    return NS_OK;
  }

private:
  int mFd;
};

void
DroidSocketImpl::OnSocketCanAcceptWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  



  RemoveWatchers(READ_WATCHER);
  nsRefPtr<AcceptRunnable> t = new AcceptRunnable(this, aFd);
  NS_DispatchToMainThread(t);
}

void
DroidSocketImpl::OnFileCanWriteWithoutBlocking(int aFd)
{
  if (mConnectionStatus == SOCKET_IS_CONNECTED) {
    OnSocketCanSendWithoutBlocking(aFd);
  } else if (mConnectionStatus == SOCKET_IS_CONNECTING) {
    OnSocketCanConnectWithoutBlocking(aFd);
  } else {
    NS_NOTREACHED("invalid connection state for writing");
  }
}

void
DroidSocketImpl::OnSocketCanSendWithoutBlocking(int aFd)
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
      AddWatchers(WRITE_WATCHER, false);
    }
    mOutgoingQ.RemoveElementAt(0);
    delete data;
  }
}

void
DroidSocketImpl::OnSocketCanConnectWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  



  mConnectionStatus = SOCKET_IS_CONNECTED;

  nsRefPtr<OnSocketEventRunnable> r =
    new OnSocketEventRunnable(this, OnSocketEventRunnable::CONNECT_SUCCESS);
  NS_DispatchToMainThread(r);

  AddWatchers(READ_WATCHER, true);
  if (!mOutgoingQ.IsEmpty()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

BluetoothSocket::BluetoothSocket(BluetoothSocketObserver* aObserver,
                                 BluetoothSocketType aType,
                                 bool aAuth,
                                 bool aEncrypt)
  : mObserver(aObserver)
  , mImpl(nullptr)
  , mAuth(aAuth)
  , mEncrypt(aEncrypt)
{
  MOZ_ASSERT(aObserver);

  EnsureBluetoothSocketHalLoad();
  mDeviceAddress.AssignLiteral(BLUETOOTH_ADDRESS_NONE);
}

void
BluetoothSocket::CloseDroidSocket()
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

class ConnectSocketResultHandler MOZ_FINAL : public BluetoothSocketResultHandler
{
public:
  ConnectSocketResultHandler(DroidSocketImpl* aImpl)
  : mImpl(aImpl)
  {
    MOZ_ASSERT(mImpl);
  }

  void Connect(int aFd, const nsAString& aBdAddress,
               int aConnectionStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (!mImpl->IsShutdownOnMainThread()) {
      mImpl->mConsumer->SetAddress(aBdAddress);
    }
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     new SocketConnectTask(mImpl, aFd));
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    BT_WARNING("Connect failed: %d", (int)aStatus);
  }

private:
  DroidSocketImpl* mImpl;
};

bool
BluetoothSocket::Connect(const nsAString& aDeviceAddress, int aChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_FALSE(mImpl, false);

  SetConnectionStatus(SOCKET_CONNECTING);

  mImpl = new DroidSocketImpl(XRE_GetIOMessageLoop(), this);

  
  sBluetoothSocketInterface->Connect(
    aDeviceAddress,
    BluetoothSocketType::RFCOMM,
    UUID_OBEX_OBJECT_PUSH,
    aChannel, mEncrypt, mAuth,
    new ConnectSocketResultHandler(mImpl));

  return true;
}

class ListenResultHandler MOZ_FINAL : public BluetoothSocketResultHandler
{
public:
  ListenResultHandler(DroidSocketImpl* aImpl)
  : mImpl(aImpl)
  {
    MOZ_ASSERT(mImpl);
  }

  void Listen(int aFd) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     new SocketListenTask(mImpl, aFd));
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    BT_WARNING("Listen failed: %d", (int)aStatus);
  }

private:
  DroidSocketImpl* mImpl;
};

bool
BluetoothSocket::Listen(int aChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_FALSE(mImpl, false);

  SetConnectionStatus(SOCKET_LISTENING);

  mImpl = new DroidSocketImpl(XRE_GetIOMessageLoop(), this);

  sBluetoothSocketInterface->Listen(
    BluetoothSocketType::RFCOMM,
    NS_LITERAL_STRING("OBEX Object Push"),
    UUID_OBEX_OBJECT_PUSH,
    aChannel, mEncrypt, mAuth,
    new ListenResultHandler(mImpl));

  return true;
}

bool
BluetoothSocket::SendDroidSocketData(UnixSocketRawData* aData)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_TRUE(mImpl, false);

  MOZ_ASSERT(!mImpl->IsShutdownOnMainThread());
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new SocketSendTask(this, mImpl, aData));
  return true;
}

void
BluetoothSocket::ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);
  mObserver->ReceiveSocketData(this, aMessage);
}

void
BluetoothSocket::OnConnectSuccess()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketConnectSuccess(this);
}

void
BluetoothSocket::OnConnectError()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketConnectError(this);
}

void
BluetoothSocket::OnDisconnect()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketDisconnect(this);
}
