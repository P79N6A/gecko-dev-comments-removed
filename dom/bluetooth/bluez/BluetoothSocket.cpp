





#include "BluetoothSocket.h"
#include <fcntl.h>
#include "BluetoothSocketObserver.h"
#include "BluetoothUnixSocketConnector.h"
#include "mozilla/unused.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

using namespace mozilla::ipc;

BEGIN_BLUETOOTH_NAMESPACE

static const size_t MAX_READ_SIZE = 1 << 16;





class BluetoothSocket::BluetoothSocketIO final
  : public UnixSocketWatcher
  , public DataSocketIO
{
public:
  BluetoothSocketIO(MessageLoop* mIOLoop,
                    BluetoothSocket* aConsumer,
                    UnixSocketConnector* aConnector,
                    const nsACString& aAddress);
  ~BluetoothSocketIO();

  void        GetSocketAddr(nsAString& aAddrStr) const;

  BluetoothSocket* GetBluetoothSocket();
  DataSocket* GetDataSocket();

  
  

  void SetDelayedConnectTask(CancelableTask* aTask);
  void ClearDelayedConnectTask();
  void CancelDelayedConnectTask();

  
  

  


  void Listen();

  


  void Connect();

  void Send(UnixSocketIOBuffer* aBuffer);

  
  

  void OnAccepted(int aFd, const sockaddr_any* aAddr,
                  socklen_t aAddrLen) override;
  void OnConnected() override;
  void OnError(const char* aFunction, int aErrno) override;
  void OnListening() override;
  void OnSocketCanReceiveWithoutBlocking() override;
  void OnSocketCanSendWithoutBlocking() override;

  
  

  nsresult QueryReceiveBuffer(UnixSocketIOBuffer** aBuffer);
  void ConsumeBuffer();
  void DiscardBuffer();

  
  

  SocketBase* GetSocketBase() override;

  bool IsShutdownOnMainThread() const override;
  bool IsShutdownOnIOThread() const override;

  void ShutdownOnMainThread() override;
  void ShutdownOnIOThread() override;

private:
  class ReceiveRunnable;

  void FireSocketError();

  
  static bool SetSocketFlags(int aFd);

  




  RefPtr<BluetoothSocket> mConsumer;

  


  nsAutoPtr<UnixSocketConnector> mConnector;

  


  bool mShuttingDownOnIOThread;

  


  nsCString mAddress;

  


  socklen_t mAddrSize;

  


  sockaddr_any mAddr;

  


  CancelableTask* mDelayedConnectTask;

  


  nsAutoPtr<UnixSocketRawData> mBuffer;
};

BluetoothSocket::BluetoothSocketIO::BluetoothSocketIO(
  MessageLoop* mIOLoop,
  BluetoothSocket* aConsumer,
  UnixSocketConnector* aConnector,
  const nsACString& aAddress)
  : UnixSocketWatcher(mIOLoop)
  , mConsumer(aConsumer)
  , mConnector(aConnector)
  , mShuttingDownOnIOThread(false)
  , mAddress(aAddress)
  , mDelayedConnectTask(nullptr)
{
  MOZ_ASSERT(mConsumer);
  MOZ_ASSERT(mConnector);
}

BluetoothSocket::BluetoothSocketIO::~BluetoothSocketIO()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(IsShutdownOnMainThread());
}

void
BluetoothSocket::BluetoothSocketIO::GetSocketAddr(nsAString& aAddrStr) const
{
  if (!mConnector) {
    NS_WARNING("No connector to get socket address from!");
    aAddrStr.Truncate();
    return;
  }
  mConnector->GetSocketAddr(mAddr, aAddrStr);
}

BluetoothSocket*
BluetoothSocket::BluetoothSocketIO::GetBluetoothSocket()
{
  return mConsumer.get();
}

DataSocket*
BluetoothSocket::BluetoothSocketIO::GetDataSocket()
{
  return GetBluetoothSocket();
}

void
BluetoothSocket::BluetoothSocketIO::SetDelayedConnectTask(CancelableTask* aTask)
{
  MOZ_ASSERT(NS_IsMainThread());

  mDelayedConnectTask = aTask;
}

void
BluetoothSocket::BluetoothSocketIO::ClearDelayedConnectTask()
{
  MOZ_ASSERT(NS_IsMainThread());

  mDelayedConnectTask = nullptr;
}

void
BluetoothSocket::BluetoothSocketIO::CancelDelayedConnectTask()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mDelayedConnectTask) {
    return;
  }

  mDelayedConnectTask->Cancel();
  ClearDelayedConnectTask();
}

void
BluetoothSocket::BluetoothSocketIO::Listen()
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
BluetoothSocket::BluetoothSocketIO::Connect()
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
BluetoothSocket::BluetoothSocketIO::Send(UnixSocketIOBuffer* aBuffer)
{
  EnqueueData(aBuffer);
  AddWatchers(WRITE_WATCHER, false);
}

void
BluetoothSocket::BluetoothSocketIO::OnAccepted(
  int aFd, const sockaddr_any* aAddr, socklen_t aAddrLen)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_LISTENING);
  MOZ_ASSERT(aAddr);
  MOZ_ASSERT(aAddrLen > 0 && (size_t)aAddrLen <= sizeof(mAddr));

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

  NS_DispatchToMainThread(
    new SocketIOEventRunnable(this, SocketIOEventRunnable::CONNECT_SUCCESS));

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
BluetoothSocket::BluetoothSocketIO::OnConnected()
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

  NS_DispatchToMainThread(
    new SocketIOEventRunnable(this, SocketIOEventRunnable::CONNECT_SUCCESS));

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
BluetoothSocket::BluetoothSocketIO::OnListening()
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
BluetoothSocket::BluetoothSocketIO::OnError(const char* aFunction, int aErrno)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  UnixFdWatcher::OnError(aFunction, aErrno);
  FireSocketError();
}

void
BluetoothSocket::BluetoothSocketIO::OnSocketCanReceiveWithoutBlocking()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_CONNECTED); 

  ssize_t res = ReceiveData(GetFd());
  if (res < 0) {
    
    RemoveWatchers(READ_WATCHER|WRITE_WATCHER);
  } else if (!res) {
    
    RemoveWatchers(READ_WATCHER);
  }
}

void
BluetoothSocket::BluetoothSocketIO::OnSocketCanSendWithoutBlocking()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_CONNECTED); 

  nsresult rv = SendPendingData(GetFd());
  if (NS_FAILED(rv)) {
    return;
  }

  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
BluetoothSocket::BluetoothSocketIO::FireSocketError()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  
  Close();

  
  NS_DispatchToMainThread(
    new SocketIOEventRunnable(this, SocketIOEventRunnable::CONNECT_ERROR));

}

bool
BluetoothSocket::BluetoothSocketIO::SetSocketFlags(int aFd)
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



nsresult
BluetoothSocket::BluetoothSocketIO::QueryReceiveBuffer(
  UnixSocketIOBuffer** aBuffer)
{
  MOZ_ASSERT(aBuffer);

  if (!mBuffer) {
    mBuffer = new UnixSocketRawData(MAX_READ_SIZE);
  }
  *aBuffer = mBuffer.get();

  return NS_OK;
}





class BluetoothSocket::BluetoothSocketIO::ReceiveRunnable final
  : public SocketIORunnable<BluetoothSocketIO>
{
public:
  ReceiveRunnable(BluetoothSocketIO* aIO, UnixSocketBuffer* aBuffer)
    : SocketIORunnable<BluetoothSocketIO>(aIO)
    , mBuffer(aBuffer)
  { }

  NS_IMETHOD Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    BluetoothSocketIO* io = SocketIORunnable<BluetoothSocketIO>::GetIO();

    if (NS_WARN_IF(io->IsShutdownOnMainThread())) {
      
      
      return NS_OK;
    }

    BluetoothSocket* bluetoothSocket = io->GetBluetoothSocket();
    MOZ_ASSERT(bluetoothSocket);

    bluetoothSocket->ReceiveSocketData(mBuffer);

    return NS_OK;
  }

private:
  nsAutoPtr<UnixSocketBuffer> mBuffer;
};

void
BluetoothSocket::BluetoothSocketIO::ConsumeBuffer()
{
  NS_DispatchToMainThread(new ReceiveRunnable(this, mBuffer.forget()));
}

void
BluetoothSocket::BluetoothSocketIO::DiscardBuffer()
{
  
}



SocketBase*
BluetoothSocket::BluetoothSocketIO::GetSocketBase()
{
  return GetDataSocket();
}

bool
BluetoothSocket::BluetoothSocketIO::IsShutdownOnMainThread() const
{
  MOZ_ASSERT(NS_IsMainThread());

  return mConsumer == nullptr;
}

void
BluetoothSocket::BluetoothSocketIO::ShutdownOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!IsShutdownOnMainThread());

  mConsumer = nullptr;
}

bool
BluetoothSocket::BluetoothSocketIO::IsShutdownOnIOThread() const
{
  return mShuttingDownOnIOThread;
}

void
BluetoothSocket::BluetoothSocketIO::ShutdownOnIOThread()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  Close(); 
  mShuttingDownOnIOThread = true;
}






class BluetoothSocket::ListenTask final
  : public SocketIOTask<BluetoothSocketIO>
{
public:
  ListenTask(BluetoothSocketIO* aIO)
    : SocketIOTask<BluetoothSocketIO>(aIO)
  { }

  void Run() override
  {
    MOZ_ASSERT(!NS_IsMainThread());

    if (!IsCanceled()) {
      GetIO()->Listen();
    }
  }
};

class BluetoothSocket::ConnectTask final
  : public SocketIOTask<BluetoothSocketIO>
{
public:
  ConnectTask(BluetoothSocketIO* aIO)
    : SocketIOTask<BluetoothSocketIO>(aIO)
  { }

  void Run() override
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!IsCanceled());

    GetIO()->Connect();
  }
};

class BluetoothSocket::DelayedConnectTask final
  : public SocketIOTask<BluetoothSocketIO>
{
public:
  DelayedConnectTask(BluetoothSocketIO* aIO)
    : SocketIOTask<BluetoothSocketIO>(aIO)
  { }

  void Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (IsCanceled()) {
      return;
    }

    BluetoothSocketIO* io = GetIO();
    if (io->IsShutdownOnMainThread()) {
      return;
    }

    io->ClearDelayedConnectTask();
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE, new ConnectTask(io));
  }
};





BluetoothSocket::BluetoothSocket(BluetoothSocketObserver* aObserver,
                                 BluetoothSocketType aType,
                                 bool aAuth,
                                 bool aEncrypt)
  : mObserver(aObserver)
  , mType(aType)
  , mAuth(aAuth)
  , mEncrypt(aEncrypt)
  , mIO(nullptr)
{
  MOZ_ASSERT(aObserver);
}

BluetoothSocket::~BluetoothSocket()
{
  MOZ_ASSERT(!mIO);
}

bool
BluetoothSocket::Connect(const nsAString& aDeviceAddress,
                         const BluetoothUuid& aServiceUuid,
                         int aChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!aDeviceAddress.IsEmpty());

  nsAutoPtr<BluetoothUnixSocketConnector> c(
    new BluetoothUnixSocketConnector(mType, aChannel, mAuth, mEncrypt));

  if (!ConnectSocket(c.forget(),
                     NS_ConvertUTF16toUTF8(aDeviceAddress).BeginReading())) {
    nsAutoString addr;
    GetAddress(addr);
    BT_LOGD("%s failed. Current connected device address: %s",
           __FUNCTION__, NS_ConvertUTF16toUTF8(addr).get());
    return false;
  }

  return true;
}

bool
BluetoothSocket::Listen(const nsAString& aServiceName,
                        const BluetoothUuid& aServiceUuid,
                        int aChannel)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothUnixSocketConnector> c(
    new BluetoothUnixSocketConnector(mType, aChannel, mAuth, mEncrypt));

  if (!ListenSocket(c.forget())) {
    nsAutoString addr;
    GetAddress(addr);
    BT_LOGD("%s failed. Current connected device address: %s",
           __FUNCTION__, NS_ConvertUTF16toUTF8(addr).get());
    return false;
  }

  return true;
}

void
BluetoothSocket::ReceiveSocketData(nsAutoPtr<UnixSocketBuffer>& aBuffer)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);

  mObserver->ReceiveSocketData(this, aBuffer);
}

bool
BluetoothSocket::SendSocketData(const nsACString& aStr)
{
  if (aStr.Length() > MAX_READ_SIZE) {
    return false;
  }

  SendSocketData(new UnixSocketRawData(aStr.BeginReading(), aStr.Length()));

  return true;
}

void
BluetoothSocket::GetSocketAddr(nsAString& aAddrStr)
{
  aAddrStr.Truncate();
  if (!mIO || GetConnectionStatus() != SOCKET_CONNECTED) {
    NS_WARNING("No socket currently open!");
    return;
  }
  mIO->GetSocketAddr(aAddrStr);
}

bool
BluetoothSocket::ConnectSocket(BluetoothUnixSocketConnector* aConnector,
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
  mIO = new BluetoothSocketIO(ioLoop, this, connector.forget(), addr);
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
BluetoothSocket::ListenSocket(BluetoothUnixSocketConnector* aConnector)
{
  MOZ_ASSERT(aConnector);
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<UnixSocketConnector> connector(aConnector);

  if (mIO) {
    NS_WARNING("Socket already connecting/connected!");
    return false;
  }

  mIO = new BluetoothSocketIO(
    XRE_GetIOMessageLoop(), this, connector.forget(), EmptyCString());
  SetConnectionStatus(SOCKET_LISTENING);
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, new ListenTask(mIO));
  return true;
}



void
BluetoothSocket::SendSocketData(UnixSocketIOBuffer* aBuffer)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mIO);
  MOZ_ASSERT(!mIO->IsShutdownOnMainThread());

  XRE_GetIOMessageLoop()->PostTask(
    FROM_HERE,
    new SocketIOSendTask<BluetoothSocketIO, UnixSocketIOBuffer>(mIO, aBuffer));
}



void
BluetoothSocket::CloseSocket()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mIO) {
    return;
  }

  mIO->CancelDelayedConnectTask();

  
  
  
  mIO->ShutdownOnMainThread();

  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, new SocketIOShutdownTask(mIO));

  mIO = nullptr;

  NotifyDisconnect();
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

END_BLUETOOTH_NAMESPACE
