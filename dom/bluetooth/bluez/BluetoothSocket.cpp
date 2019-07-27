





#include "BluetoothSocket.h"
#include <fcntl.h>
#include "BluetoothSocketObserver.h"
#include "BluetoothUnixSocketConnector.h"
#include "mozilla/RefPtr.h"
#include "nsXULAppAPI.h"

using namespace mozilla::ipc;

BEGIN_BLUETOOTH_NAMESPACE

static const size_t MAX_READ_SIZE = 1 << 16;





class BluetoothSocket::BluetoothSocketIO final
  : public UnixSocketWatcher
  , public DataSocketIO
{
public:
  BluetoothSocketIO(MessageLoop* aConsumerLoop,
                    MessageLoop* aIOLoop,
                    BluetoothSocket* aConsumer,
                    UnixSocketConnector* aConnector);
  ~BluetoothSocketIO();

  void GetSocketAddr(nsAString& aAddrStr) const;

  BluetoothSocket* GetBluetoothSocket();
  DataSocket* GetDataSocket();

  
  

  void SetDelayedConnectTask(CancelableTask* aTask);
  void ClearDelayedConnectTask();
  void CancelDelayedConnectTask();

  
  

  


  void Listen();

  


  void Connect();

  void Send(UnixSocketIOBuffer* aBuffer);

  
  

  void OnConnected() override;
  void OnError(const char* aFunction, int aErrno) override;
  void OnListening() override;
  void OnSocketCanAcceptWithoutBlocking() override;
  void OnSocketCanReceiveWithoutBlocking() override;
  void OnSocketCanSendWithoutBlocking() override;

  
  

  nsresult QueryReceiveBuffer(UnixSocketIOBuffer** aBuffer);
  void ConsumeBuffer();
  void DiscardBuffer();

  
  

  SocketBase* GetSocketBase() override;

  bool IsShutdownOnConsumerThread() const override;
  bool IsShutdownOnIOThread() const override;

  void ShutdownOnConsumerThread() override;
  void ShutdownOnIOThread() override;

private:
  class ReceiveTask;

  void FireSocketError();

  




  RefPtr<BluetoothSocket> mConsumer;

  


  nsAutoPtr<UnixSocketConnector> mConnector;

  


  bool mShuttingDownOnIOThread;

  


  socklen_t mAddressLength;

  


  struct sockaddr_storage mAddress;

  



  CancelableTask* mDelayedConnectTask;

  


  nsAutoPtr<UnixSocketRawData> mBuffer;
};

BluetoothSocket::BluetoothSocketIO::BluetoothSocketIO(
  MessageLoop* aConsumerLoop,
  MessageLoop* aIOLoop,
  BluetoothSocket* aConsumer,
  UnixSocketConnector* aConnector)
  : UnixSocketWatcher(aIOLoop)
  , DataSocketIO(aConsumerLoop)
  , mConsumer(aConsumer)
  , mConnector(aConnector)
  , mShuttingDownOnIOThread(false)
  , mAddressLength(0)
  , mDelayedConnectTask(nullptr)
{
  MOZ_ASSERT(mConsumer);
  MOZ_ASSERT(mConnector);
}

BluetoothSocket::BluetoothSocketIO::~BluetoothSocketIO()
{
  MOZ_ASSERT(IsConsumerThread());
  MOZ_ASSERT(IsShutdownOnConsumerThread());
}

void
BluetoothSocket::BluetoothSocketIO::GetSocketAddr(nsAString& aAddrStr) const
{
  if (!mConnector) {
    NS_WARNING("No connector to get socket address from!");
    aAddrStr.Truncate();
    return;
  }

  nsCString addressString;
  nsresult rv = mConnector->ConvertAddressToString(
    *reinterpret_cast<const struct sockaddr*>(&mAddress), mAddressLength,
    addressString);
  if (NS_FAILED(rv)) {
    aAddrStr.Truncate();
    return;
  }

  aAddrStr.Assign(NS_ConvertUTF8toUTF16(addressString));
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
  MOZ_ASSERT(IsConsumerThread());

  mDelayedConnectTask = aTask;
}

void
BluetoothSocket::BluetoothSocketIO::ClearDelayedConnectTask()
{
  MOZ_ASSERT(IsConsumerThread());

  mDelayedConnectTask = nullptr;
}

void
BluetoothSocket::BluetoothSocketIO::CancelDelayedConnectTask()
{
  MOZ_ASSERT(IsConsumerThread());

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

  if (!IsOpen()) {
    mAddressLength = sizeof(mAddress);

    int fd;
    nsresult rv = mConnector->CreateListenSocket(
      reinterpret_cast<struct sockaddr*>(&mAddress), &mAddressLength, fd);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      FireSocketError();
      return;
    }
    SetFd(fd);

    
    rv = UnixSocketWatcher::Listen(
      reinterpret_cast<struct sockaddr*>(&mAddress), mAddressLength);
    NS_WARN_IF(NS_FAILED(rv));
  }
}

void
BluetoothSocket::BluetoothSocketIO::Connect()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(mConnector);

  if (!IsOpen()) {
    mAddressLength = sizeof(mAddress);

    int fd;
    nsresult rv = mConnector->CreateStreamSocket(
      reinterpret_cast<struct sockaddr*>(&mAddress), &mAddressLength, fd);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      FireSocketError();
      return;
    }
    SetFd(fd);
  }

  
  nsresult rv = UnixSocketWatcher::Connect(
    reinterpret_cast<struct sockaddr*>(&mAddress), mAddressLength);
  NS_WARN_IF(NS_FAILED(rv));
}

void
BluetoothSocket::BluetoothSocketIO::Send(UnixSocketIOBuffer* aBuffer)
{
  EnqueueData(aBuffer);
  AddWatchers(WRITE_WATCHER, false);
}

void
BluetoothSocket::BluetoothSocketIO::OnConnected()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_CONNECTED);

  GetConsumerThread()->PostTask(
    FROM_HERE, new SocketEventTask(this, SocketEventTask::CONNECT_SUCCESS));

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
BluetoothSocket::BluetoothSocketIO::OnSocketCanAcceptWithoutBlocking()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_LISTENING);

  RemoveWatchers(READ_WATCHER|WRITE_WATCHER);

  mAddressLength = sizeof(mAddress);

  int fd;
  nsresult rv = mConnector->AcceptStreamSocket(
    GetFd(),
    reinterpret_cast<struct sockaddr*>(&mAddress), &mAddressLength, fd);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    FireSocketError();
    return;
  }

  Close();
  SetSocket(fd, SOCKET_IS_CONNECTED);

  GetConsumerThread()->PostTask(
    FROM_HERE, new SocketEventTask(this, SocketEventTask::CONNECT_SUCCESS));

  AddWatchers(READ_WATCHER, true);
  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
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

  
  GetConsumerThread()->PostTask(
    FROM_HERE, new SocketEventTask(this, SocketEventTask::CONNECT_ERROR));
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





class BluetoothSocket::BluetoothSocketIO::ReceiveTask final
  : public SocketTask<BluetoothSocketIO>
{
public:
  ReceiveTask(BluetoothSocketIO* aIO, UnixSocketBuffer* aBuffer)
    : SocketTask<BluetoothSocketIO>(aIO)
    , mBuffer(aBuffer)
  { }

  void Run() override
  {
    BluetoothSocketIO* io = SocketTask<BluetoothSocketIO>::GetIO();

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
BluetoothSocket::BluetoothSocketIO::ConsumeBuffer()
{
  GetConsumerThread()->PostTask(FROM_HERE,
                                new ReceiveTask(this, mBuffer.forget()));
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
BluetoothSocket::BluetoothSocketIO::IsShutdownOnConsumerThread() const
{
  MOZ_ASSERT(IsConsumerThread());

  return mConsumer == nullptr;
}

void
BluetoothSocket::BluetoothSocketIO::ShutdownOnConsumerThread()
{
  MOZ_ASSERT(IsConsumerThread());
  MOZ_ASSERT(!IsShutdownOnConsumerThread());

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
  MOZ_ASSERT(!IsConsumerThread());
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
    MOZ_ASSERT(!GetIO()->IsConsumerThread());

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
    MOZ_ASSERT(!GetIO()->IsConsumerThread());
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
    MOZ_ASSERT(GetIO()->IsConsumerThread());

    if (IsCanceled()) {
      return;
    }

    BluetoothSocketIO* io = GetIO();
    if (io->IsShutdownOnConsumerThread()) {
      return;
    }

    io->ClearDelayedConnectTask();
    io->GetIOLoop()->PostTask(FROM_HERE, new ConnectTask(io));
  }
};





BluetoothSocket::BluetoothSocket(BluetoothSocketObserver* aObserver)
  : mObserver(aObserver)
  , mIO(nullptr)
{
  MOZ_ASSERT(aObserver);
}

BluetoothSocket::~BluetoothSocket()
{
  MOZ_ASSERT(!mIO);
}

nsresult
BluetoothSocket::Connect(const nsAString& aDeviceAddress,
                         const BluetoothUuid& aServiceUuid,
                         BluetoothSocketType aType,
                         int aChannel,
                         bool aAuth, bool aEncrypt)
{
  MOZ_ASSERT(!aDeviceAddress.IsEmpty());

  nsAutoPtr<BluetoothUnixSocketConnector> connector(
    new BluetoothUnixSocketConnector(NS_ConvertUTF16toUTF8(aDeviceAddress),
                                     aType, aChannel, aAuth, aEncrypt));

  nsresult rv = Connect(connector);
  if (NS_FAILED(rv)) {
    nsAutoString addr;
    GetAddress(addr);
    BT_LOGD("%s failed. Current connected device address: %s",
           __FUNCTION__, NS_ConvertUTF16toUTF8(addr).get());
    return rv;
  }
  connector.forget();

  return NS_OK;
}

nsresult
BluetoothSocket::Listen(const nsAString& aServiceName,
                        const BluetoothUuid& aServiceUuid,
                        BluetoothSocketType aType,
                        int aChannel,
                        bool aAuth, bool aEncrypt)
{
  nsAutoPtr<BluetoothUnixSocketConnector> connector(
    new BluetoothUnixSocketConnector(NS_LITERAL_CSTRING(BLUETOOTH_ADDRESS_NONE),
                                     aType, aChannel, aAuth, aEncrypt));

  nsresult rv = Listen(connector);
  if (NS_FAILED(rv)) {
    nsAutoString addr;
    GetAddress(addr);
    BT_LOGD("%s failed. Current connected device address: %s",
           __FUNCTION__, NS_ConvertUTF16toUTF8(addr).get());
    return rv;
  }
  connector.forget();

  return NS_OK;
}

void
BluetoothSocket::ReceiveSocketData(nsAutoPtr<UnixSocketBuffer>& aBuffer)
{
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

nsresult
BluetoothSocket::Connect(BluetoothUnixSocketConnector* aConnector,
                         int aDelayMs,
                         MessageLoop* aConsumerLoop, MessageLoop* aIOLoop)
{
  MOZ_ASSERT(aConnector);
  MOZ_ASSERT(aConsumerLoop);
  MOZ_ASSERT(aIOLoop);
  MOZ_ASSERT(!mIO);

  mIO = new BluetoothSocketIO(aConsumerLoop, aIOLoop, this, aConnector);
  SetConnectionStatus(SOCKET_CONNECTING);

  if (aDelayMs > 0) {
    DelayedConnectTask* connectTask = new DelayedConnectTask(mIO);
    mIO->SetDelayedConnectTask(connectTask);
    MessageLoop::current()->PostDelayedTask(FROM_HERE, connectTask, aDelayMs);
  } else {
    aIOLoop->PostTask(FROM_HERE, new ConnectTask(mIO));
  }

  return NS_OK;
}

nsresult
BluetoothSocket::Connect(BluetoothUnixSocketConnector* aConnector,
                         int aDelayMs)
{
  return Connect(aConnector, aDelayMs, MessageLoop::current(),
                 XRE_GetIOMessageLoop());
}

nsresult
BluetoothSocket::Listen(BluetoothUnixSocketConnector* aConnector,
                        MessageLoop* aConsumerLoop, MessageLoop* aIOLoop)
{
  MOZ_ASSERT(aConnector);
  MOZ_ASSERT(aConsumerLoop);
  MOZ_ASSERT(aIOLoop);
  MOZ_ASSERT(!mIO);

  mIO = new BluetoothSocketIO(aConsumerLoop, aIOLoop, this, aConnector);
  SetConnectionStatus(SOCKET_LISTENING);

  aIOLoop->PostTask(FROM_HERE, new ListenTask(mIO));

  return NS_OK;
}

nsresult
BluetoothSocket::Listen(BluetoothUnixSocketConnector* aConnector)
{
  return Listen(aConnector, MessageLoop::current(), XRE_GetIOMessageLoop());
}

void
BluetoothSocket::GetAddress(nsAString& aAddrStr)
{
  aAddrStr.Truncate();
  if (!mIO || GetConnectionStatus() != SOCKET_CONNECTED) {
    NS_WARNING("No socket currently open!");
    return;
  }
  mIO->GetSocketAddr(aAddrStr);
}



void
BluetoothSocket::SendSocketData(UnixSocketIOBuffer* aBuffer)
{
  MOZ_ASSERT(mIO);
  MOZ_ASSERT(mIO->IsConsumerThread());
  MOZ_ASSERT(!mIO->IsShutdownOnConsumerThread());

  mIO->GetIOLoop()->PostTask(
    FROM_HERE,
    new SocketIOSendTask<BluetoothSocketIO, UnixSocketIOBuffer>(mIO, aBuffer));
}



void
BluetoothSocket::Close()
{
  if (!mIO) {
    return;
  }

  MOZ_ASSERT(mIO->IsConsumerThread());

  mIO->CancelDelayedConnectTask();

  
  
  
  mIO->ShutdownOnConsumerThread();
  mIO->GetIOLoop()->PostTask(FROM_HERE, new SocketIOShutdownTask(mIO));
  mIO = nullptr;

  NotifyDisconnect();
}

void
BluetoothSocket::OnConnectSuccess()
{
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketConnectSuccess(this);
}

void
BluetoothSocket::OnConnectError()
{
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketConnectError(this);
}

void
BluetoothSocket::OnDisconnect()
{
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketDisconnect(this);
}

END_BLUETOOTH_NAMESPACE
