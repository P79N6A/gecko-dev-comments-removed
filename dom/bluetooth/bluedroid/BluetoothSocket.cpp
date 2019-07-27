





#include "BluetoothSocket.h"
#include <fcntl.h>
#include <sys/socket.h>
#include "BluetoothSocketObserver.h"
#include "BluetoothInterface.h"
#include "BluetoothUtils.h"
#include "mozilla/ipc/UnixSocketWatcher.h"
#include "mozilla/FileUtils.h"
#include "mozilla/RefPtr.h"
#include "nsXULAppAPI.h"

using namespace mozilla::ipc;
USING_BLUETOOTH_NAMESPACE

static const size_t MAX_READ_SIZE = 1 << 16;
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

class mozilla::dom::bluetooth::DroidSocketImpl
  : public ipc::UnixFdWatcher
  , public DataSocketIO
{
public:
  



















  enum ConnectionStatus {
    SOCKET_IS_DISCONNECTED = 0,
    SOCKET_IS_LISTENING,
    SOCKET_IS_CONNECTING,
    SOCKET_IS_CONNECTED
  };

  DroidSocketImpl(MessageLoop* aConsumerLoop,
                  MessageLoop* aIOLoop,
                  BluetoothSocket* aConsumer)
    : ipc::UnixFdWatcher(aIOLoop)
    , DataSocketIO(aConsumerLoop)
    , mConsumer(aConsumer)
    , mShuttingDownOnIOThread(false)
    , mConnectionStatus(SOCKET_IS_DISCONNECTED)
  { }

  ~DroidSocketImpl()
  {
    MOZ_ASSERT(IsConsumerThread());
  }

  void Send(UnixSocketIOBuffer* aBuffer)
  {
    EnqueueData(aBuffer);
    AddWatchers(WRITE_WATCHER, false);
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

  BluetoothSocket* GetBluetoothSocket()
  {
    return mConsumer.get();
  }

  DataSocket* GetDataSocket()
  {
    return GetBluetoothSocket();
  }

  




  RefPtr<BluetoothSocket> mConsumer;

  
  

  nsresult QueryReceiveBuffer(UnixSocketIOBuffer** aBuffer);
  void ConsumeBuffer();
  void DiscardBuffer();

  
  

  SocketBase* GetSocketBase() override
  {
    return GetDataSocket();
  }

  bool IsShutdownOnConsumerThread() const override
  {
    MOZ_ASSERT(IsConsumerThread());

    return mConsumer == nullptr;
  }

  bool IsShutdownOnIOThread() const override
  {
    return mShuttingDownOnIOThread;
  }

  void ShutdownOnConsumerThread() override
  {
    MOZ_ASSERT(IsConsumerThread());
    MOZ_ASSERT(!IsShutdownOnConsumerThread());

    mConsumer = nullptr;
  }

  void ShutdownOnIOThread() override
  {
    MOZ_ASSERT(!IsConsumerThread());
    MOZ_ASSERT(!mShuttingDownOnIOThread);

    Close(); 
    mShuttingDownOnIOThread = true;
  }

private:
  class ReceiveTask;

  





  virtual void OnFileCanReadWithoutBlocking(int aFd);

  





  virtual void OnFileCanWriteWithoutBlocking(int aFd);

  void OnSocketCanReceiveWithoutBlocking(int aFd);
  void OnSocketCanAcceptWithoutBlocking(int aFd);
  void OnSocketCanSendWithoutBlocking(int aFd);
  void OnSocketCanConnectWithoutBlocking(int aFd);

  


  bool mShuttingDownOnIOThread;

  ConnectionStatus mConnectionStatus;

  


  nsAutoPtr<UnixSocketRawData> mBuffer;
};

class SocketConnectTask final : public SocketIOTask<DroidSocketImpl>
{
public:
  SocketConnectTask(DroidSocketImpl* aDroidSocketImpl, int aFd)
  : SocketIOTask<DroidSocketImpl>(aDroidSocketImpl)
  , mFd(aFd)
  { }

  void Run() override
  {
    MOZ_ASSERT(!GetIO()->IsConsumerThread());
    MOZ_ASSERT(!IsCanceled());

    GetIO()->Connect(mFd);
  }

private:
  int mFd;
};

class SocketListenTask final : public SocketIOTask<DroidSocketImpl>
{
public:
  SocketListenTask(DroidSocketImpl* aDroidSocketImpl, int aFd)
  : SocketIOTask<DroidSocketImpl>(aDroidSocketImpl)
  , mFd(aFd)
  { }

  void Run() override
  {
    MOZ_ASSERT(!GetIO()->IsConsumerThread());

    if (!IsCanceled()) {
      GetIO()->Listen(mFd);
    }
  }

private:
  int mFd;
};

class SocketConnectClientFdTask final
: public SocketIOTask<DroidSocketImpl>
{
  SocketConnectClientFdTask(DroidSocketImpl* aImpl)
  : SocketIOTask<DroidSocketImpl>(aImpl)
  { }

  void Run() override
  {
    MOZ_ASSERT(!GetIO()->IsConsumerThread());

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

  GetConsumerThread()->PostTask(
    FROM_HERE, new SocketEventTask(this, SocketEventTask::CONNECT_SUCCESS));

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
  MOZ_ASSERT(!IsConsumerThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  ssize_t res = ReceiveData(aFd);
  if (res < 0) {
    
    RemoveWatchers(READ_WATCHER|WRITE_WATCHER);
  } else if (!res) {
    
    RemoveWatchers(READ_WATCHER);
  }
}

class AcceptTask final : public SocketIOTask<DroidSocketImpl>
{
public:
  AcceptTask(DroidSocketImpl* aDroidSocketImpl, int aFd)
  : SocketIOTask<DroidSocketImpl>(aDroidSocketImpl)
  , mFd(aFd)
  { }

  void Run() override
  {
    MOZ_ASSERT(!GetIO()->IsConsumerThread());
    MOZ_ASSERT(!IsCanceled());

    GetIO()->Accept(mFd);
  }

private:
  int mFd;
};

class AcceptResultHandler final : public BluetoothSocketResultHandler
{
public:
  AcceptResultHandler(DroidSocketImpl* aImpl)
  : mImpl(aImpl)
  {
    MOZ_ASSERT(mImpl);
  }

  void Accept(int aFd, const nsAString& aBdAddress,
              int aConnectionStatus) override
  {
    MOZ_ASSERT(mImpl->IsConsumerThread());

    mozilla::ScopedClose fd(aFd); 

    if (mImpl->IsShutdownOnConsumerThread()) {
      BT_LOGD("mConsumer is null, aborting receive!");
      return;
    }

    if (aConnectionStatus != 0) {
      mImpl->mConsumer->NotifyError();
      return;
    }

    mImpl->mConsumer->SetAddress(aBdAddress);
    mImpl->GetIOLoop()->PostTask(FROM_HERE,
                                 new AcceptTask(mImpl, fd.forget()));
  }

  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(mImpl->IsConsumerThread());
    BT_LOGR("BluetoothSocketInterface::Accept failed: %d", (int)aStatus);

    if (!mImpl->IsShutdownOnConsumerThread()) {
      
      
      
      
      mImpl->mConsumer->NotifyDisconnect();
    }
  }

private:
  DroidSocketImpl* mImpl;
};

class InvokeAcceptTask final : public SocketTask<DroidSocketImpl>
{
public:
  InvokeAcceptTask(DroidSocketImpl* aImpl, int aFd)
    : SocketTask<DroidSocketImpl>(aImpl)
    , mFd(aFd)
  { }

  void Run() override
  {
    MOZ_ASSERT(GetIO()->IsConsumerThread());
    MOZ_ASSERT(sBluetoothSocketInterface);

    BluetoothSocketResultHandler* res = new AcceptResultHandler(GetIO());
    GetIO()->mConsumer->SetCurrentResultHandler(res);

    sBluetoothSocketInterface->Accept(mFd, res);
  }

private:
  int mFd;
};

void
DroidSocketImpl::OnSocketCanAcceptWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!IsConsumerThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  



  RemoveWatchers(READ_WATCHER);
  GetConsumerThread()->PostTask(FROM_HERE, new InvokeAcceptTask(this, aFd));
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
  MOZ_ASSERT(!IsConsumerThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);
  MOZ_ASSERT(aFd >= 0);

  nsresult rv = SendPendingData(aFd);
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
  MOZ_ASSERT(!IsConsumerThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  



  mConnectionStatus = SOCKET_IS_CONNECTED;

  GetConsumerThread()->PostTask(
    FROM_HERE, new SocketEventTask(this, SocketEventTask::CONNECT_SUCCESS));

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}



nsresult
DroidSocketImpl::QueryReceiveBuffer(
  UnixSocketIOBuffer** aBuffer)
{
  MOZ_ASSERT(aBuffer);

  if (!mBuffer) {
    mBuffer = new UnixSocketRawData(MAX_READ_SIZE);
  }
  *aBuffer = mBuffer.get();

  return NS_OK;
}





class DroidSocketImpl::ReceiveTask final : public SocketTask<DroidSocketImpl>
{
public:
  ReceiveTask(DroidSocketImpl* aIO, UnixSocketBuffer* aBuffer)
    : SocketTask<DroidSocketImpl>(aIO)
    , mBuffer(aBuffer)
  { }

  void Run() override
  {
    DroidSocketImpl* io = SocketTask<DroidSocketImpl>::GetIO();

    MOZ_ASSERT(io->IsConsumerThread());

    if (NS_WARN_IF(io->IsShutdownOnConsumerThread())) {
      
      
      return;
    }

    BluetoothSocket* bluetoothSocket = io->GetBluetoothSocket();
    MOZ_ASSERT(bluetoothSocket);

    bluetoothSocket->ReceiveSocketData(mBuffer);
  }

private:
  nsAutoPtr<UnixSocketBuffer> mBuffer;
};

void
DroidSocketImpl::ConsumeBuffer()
{
  GetConsumerThread()->PostTask(FROM_HERE,
                                new ReceiveTask(this, mBuffer.forget()));
}

void
DroidSocketImpl::DiscardBuffer()
{
  
}





BluetoothSocket::BluetoothSocket(BluetoothSocketObserver* aObserver)
  : mObserver(aObserver)
  , mCurrentRes(nullptr)
  , mImpl(nullptr)
{
  MOZ_ASSERT(aObserver);

  EnsureBluetoothSocketHalLoad();
  mDeviceAddress.AssignLiteral(BLUETOOTH_ADDRESS_NONE);
}

class ConnectSocketResultHandler final : public BluetoothSocketResultHandler
{
public:
  ConnectSocketResultHandler(DroidSocketImpl* aImpl)
  : mImpl(aImpl)
  {
    MOZ_ASSERT(mImpl);
  }

  void Connect(int aFd, const nsAString& aBdAddress,
               int aConnectionStatus) override
  {
    MOZ_ASSERT(mImpl->IsConsumerThread());

    if (mImpl->IsShutdownOnConsumerThread()) {
      BT_LOGD("mConsumer is null, aborting send!");
      return;
    }

    if (aConnectionStatus != 0) {
      mImpl->mConsumer->NotifyError();
      return;
    }

    mImpl->mConsumer->SetAddress(aBdAddress);
    mImpl->GetIOLoop()->PostTask(FROM_HERE,
                                 new SocketConnectTask(mImpl, aFd));
  }

  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(mImpl->IsConsumerThread());
    BT_WARNING("Connect failed: %d", (int)aStatus);

    if (!mImpl->IsShutdownOnConsumerThread()) {
      
      
      
      
      mImpl->mConsumer->NotifyDisconnect();
    }
  }

private:
  DroidSocketImpl* mImpl;
};

nsresult
BluetoothSocket::Connect(const nsAString& aDeviceAddress,
                         const BluetoothUuid& aServiceUuid,
                         BluetoothSocketType aType,
                         int aChannel,
                         bool aAuth, bool aEncrypt,
                         MessageLoop* aConsumerLoop,
                         MessageLoop* aIOLoop)
{
  MOZ_ASSERT(!mImpl);

  SetConnectionStatus(SOCKET_CONNECTING);

  mImpl = new DroidSocketImpl(aConsumerLoop, aIOLoop, this);

  BluetoothSocketResultHandler* res = new ConnectSocketResultHandler(mImpl);
  SetCurrentResultHandler(res);

  sBluetoothSocketInterface->Connect(
    aDeviceAddress, aType,
    aServiceUuid.mUuid, aChannel,
    aEncrypt, aAuth, res);

  return NS_OK;
}

nsresult
BluetoothSocket::Connect(const nsAString& aDeviceAddress,
                         const BluetoothUuid& aServiceUuid,
                         BluetoothSocketType aType,
                         int aChannel,
                         bool aAuth, bool aEncrypt)
{
  return Connect(aDeviceAddress, aServiceUuid, aType, aChannel, aAuth,
                 aEncrypt, MessageLoop::current(), XRE_GetIOMessageLoop());
}

class ListenResultHandler final : public BluetoothSocketResultHandler
{
public:
  ListenResultHandler(DroidSocketImpl* aImpl)
  : mImpl(aImpl)
  {
    MOZ_ASSERT(mImpl);
  }

  void Listen(int aFd) override
  {
    MOZ_ASSERT(mImpl->IsConsumerThread());

    mImpl->GetIOLoop()->PostTask(FROM_HERE, new SocketListenTask(mImpl, aFd));
  }

  void OnError(BluetoothStatus aStatus) override
  {
    MOZ_ASSERT(mImpl->IsConsumerThread());

    BT_WARNING("Listen failed: %d", (int)aStatus);
  }

private:
  DroidSocketImpl* mImpl;
};

nsresult
BluetoothSocket::Listen(const nsAString& aServiceName,
                        const BluetoothUuid& aServiceUuid,
                        BluetoothSocketType aType,
                        int aChannel,
                        bool aAuth, bool aEncrypt,
                        MessageLoop* aConsumerLoop,
                        MessageLoop* aIOLoop)
{
  MOZ_ASSERT(!mImpl);

  SetConnectionStatus(SOCKET_LISTENING);

  mImpl = new DroidSocketImpl(aConsumerLoop, aIOLoop, this);

  BluetoothSocketResultHandler* res = new ListenResultHandler(mImpl);
  SetCurrentResultHandler(res);

  sBluetoothSocketInterface->Listen(
    aType,
    aServiceName, aServiceUuid.mUuid, aChannel,
    aEncrypt, aAuth, res);

  return NS_OK;
}

nsresult
BluetoothSocket::Listen(const nsAString& aServiceName,
                        const BluetoothUuid& aServiceUuid,
                        BluetoothSocketType aType,
                        int aChannel,
                        bool aAuth, bool aEncrypt)
{
  return Listen(aServiceName, aServiceUuid, aType, aChannel, aAuth, aEncrypt,
                MessageLoop::current(), XRE_GetIOMessageLoop());
}

void
BluetoothSocket::ReceiveSocketData(nsAutoPtr<UnixSocketBuffer>& aBuffer)
{
  MOZ_ASSERT(mObserver);

  mObserver->ReceiveSocketData(this, aBuffer);
}



void
BluetoothSocket::SendSocketData(UnixSocketIOBuffer* aBuffer)
{
  MOZ_ASSERT(mImpl);
  MOZ_ASSERT(mImpl->IsConsumerThread());
  MOZ_ASSERT(!mImpl->IsShutdownOnConsumerThread());

  mImpl->GetIOLoop()->PostTask(
    FROM_HERE,
    new SocketIOSendTask<DroidSocketImpl, UnixSocketIOBuffer>(mImpl, aBuffer));
}



void
BluetoothSocket::Close()
{
  MOZ_ASSERT(sBluetoothSocketInterface);

  if (!mImpl) {
    return;
  }

  MOZ_ASSERT(mImpl->IsConsumerThread());

  
  if (mCurrentRes) {
    sBluetoothSocketInterface->Close(mCurrentRes);
  }

  
  
  
  mImpl->ShutdownOnConsumerThread();
  mImpl->GetIOLoop()->PostTask(FROM_HERE, new SocketIOShutdownTask(mImpl));
  mImpl = nullptr;

  NotifyDisconnect();
}

void
BluetoothSocket::OnConnectSuccess()
{
  MOZ_ASSERT(mObserver);

  SetCurrentResultHandler(nullptr);
  mObserver->OnSocketConnectSuccess(this);
}

void
BluetoothSocket::OnConnectError()
{
  MOZ_ASSERT(mObserver);

  SetCurrentResultHandler(nullptr);
  mObserver->OnSocketConnectError(this);
}

void
BluetoothSocket::OnDisconnect()
{
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketDisconnect(this);
}
