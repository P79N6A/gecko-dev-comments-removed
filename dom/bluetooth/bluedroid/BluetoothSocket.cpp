





#include "BluetoothSocket.h"

#include <fcntl.h>
#include <sys/socket.h>

#include "base/message_loop.h"
#include "BluetoothSocketObserver.h"
#include "BluetoothInterface.h"
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
                                               , protected SocketIOBase
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
    , SocketIOBase(MAX_READ_SIZE)
    , mConsumer(aConsumer)
    , mShuttingDownOnIOThread(false)
    , mConnectionStatus(SOCKET_IS_DISCONNECTED)
  { }

  ~DroidSocketImpl()
  {
    MOZ_ASSERT(NS_IsMainThread());
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

  SocketConsumerBase* GetConsumer()
  {
    return mConsumer.get();
  }

  




  RefPtr<BluetoothSocket> mConsumer;

private:
  





  virtual void OnFileCanReadWithoutBlocking(int aFd);

  





  virtual void OnFileCanWriteWithoutBlocking(int aFd);

  void OnSocketCanReceiveWithoutBlocking(int aFd);
  void OnSocketCanAcceptWithoutBlocking(int aFd);
  void OnSocketCanSendWithoutBlocking(int aFd);
  void OnSocketCanConnectWithoutBlocking(int aFd);

  


  bool mShuttingDownOnIOThread;

  ConnectionStatus mConnectionStatus;
};

class SocketConnectTask MOZ_FINAL : public SocketIOTask<DroidSocketImpl>
{
public:
  SocketConnectTask(DroidSocketImpl* aDroidSocketImpl, int aFd)
  : SocketIOTask<DroidSocketImpl>(aDroidSocketImpl)
  , mFd(aFd)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!IsCanceled());

    GetIO()->Connect(mFd);
  }

private:
  int mFd;
};

class SocketListenTask MOZ_FINAL : public SocketIOTask<DroidSocketImpl>
{
public:
  SocketListenTask(DroidSocketImpl* aDroidSocketImpl, int aFd)
  : SocketIOTask<DroidSocketImpl>(aDroidSocketImpl)
  , mFd(aFd)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());

    if (!IsCanceled()) {
      GetIO()->Listen(mFd);
    }
  }

private:
  int mFd;
};

class SocketConnectClientFdTask MOZ_FINAL
: public SocketIOTask<DroidSocketImpl>
{
  SocketConnectClientFdTask(DroidSocketImpl* aImpl)
  : SocketIOTask<DroidSocketImpl>(aImpl)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());

    GetIO()->ConnectClientFd();
  }
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

  nsRefPtr<nsRunnable> r =
    new SocketIOEventRunnable<DroidSocketImpl>(
      this, SocketIOEventRunnable<DroidSocketImpl>::CONNECT_SUCCESS);
  NS_DispatchToMainThread(r);

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
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

  ssize_t res = ReceiveData(aFd, this);
  if (res < 0) {
    
    RemoveWatchers(READ_WATCHER|WRITE_WATCHER);
  } else if (!res) {
    
    RemoveWatchers(READ_WATCHER);
  }
}

class AcceptTask MOZ_FINAL : public SocketIOTask<DroidSocketImpl>
{
public:
  AcceptTask(DroidSocketImpl* aDroidSocketImpl, int aFd)
  : SocketIOTask<DroidSocketImpl>(aDroidSocketImpl)
  , mFd(aFd)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!IsCanceled());

    GetIO()->Accept(mFd);
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

  nsresult rv = SendPendingData(aFd, this);
  if (NS_FAILED(rv)) {
    return;
  }

  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
DroidSocketImpl::OnSocketCanConnectWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  



  mConnectionStatus = SOCKET_IS_CONNECTED;

  nsRefPtr<nsRunnable> r =
    new SocketIOEventRunnable<DroidSocketImpl>(
      this, SocketIOEventRunnable<DroidSocketImpl>::CONNECT_SUCCESS);
  NS_DispatchToMainThread(r);

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
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
BluetoothSocket::ConnectSocket(const nsAString& aDeviceAddress, int aChannel)
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
BluetoothSocket::ListenSocket(int aChannel)
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

void
BluetoothSocket::CloseSocket()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mImpl) {
    return;
  }

  
  
  
  mImpl->ShutdownOnMainThread();
  XRE_GetIOMessageLoop()->PostTask(
    FROM_HERE, new SocketIOShutdownTask<DroidSocketImpl>(mImpl));

  mImpl = nullptr;

  NotifyDisconnect();
}

bool
BluetoothSocket::SendSocketData(UnixSocketRawData* aData)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_TRUE(mImpl, false);

  MOZ_ASSERT(!mImpl->IsShutdownOnMainThread());

  XRE_GetIOMessageLoop()->PostTask(
    FROM_HERE, new SocketIOSendTask<DroidSocketImpl>(mImpl, aData));

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
