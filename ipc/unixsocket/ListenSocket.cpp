





#include "ListenSocket.h"
#include <fcntl.h>
#include "ConnectionOrientedSocket.h"
#include "DataSocket.h"
#include "ListenSocketConsumer.h"
#include "mozilla/RefPtr.h"
#include "nsISupportsImpl.h" 
#include "nsXULAppAPI.h"
#include "UnixSocketConnector.h"

namespace mozilla {
namespace ipc {





class ListenSocketIO final
  : public UnixSocketWatcher
  , public SocketIOBase
{
public:
  class ListenTask;

  ListenSocketIO(MessageLoop* aConsumerLoop,
                 MessageLoop* aIOLoop,
                 ListenSocket* aListenSocket,
                 UnixSocketConnector* aConnector);
  ~ListenSocketIO();

  UnixSocketConnector* GetConnector() const;

  
  

  


  void Listen(ConnectionOrientedSocketIO* aCOSocketIO);

  
  

  void OnConnected() override;
  void OnError(const char* aFunction, int aErrno) override;
  void OnListening() override;
  void OnSocketCanAcceptWithoutBlocking() override;

  
  

  SocketBase* GetSocketBase() override;

  bool IsShutdownOnConsumerThread() const override;
  bool IsShutdownOnIOThread() const override;

  void ShutdownOnConsumerThread() override;
  void ShutdownOnIOThread() override;

private:
  void FireSocketError();

  




  ListenSocket* mListenSocket;

  


  nsAutoPtr<UnixSocketConnector> mConnector;

  


  bool mShuttingDownOnIOThread;

  


  socklen_t mAddressLength;

  


  struct sockaddr_storage mAddress;

  ConnectionOrientedSocketIO* mCOSocketIO;
};

ListenSocketIO::ListenSocketIO(MessageLoop* aConsumerLoop,
                               MessageLoop* aIOLoop,
                               ListenSocket* aListenSocket,
                               UnixSocketConnector* aConnector)
  : UnixSocketWatcher(aIOLoop)
  , SocketIOBase(aConsumerLoop)
  , mListenSocket(aListenSocket)
  , mConnector(aConnector)
  , mShuttingDownOnIOThread(false)
  , mAddressLength(0)
  , mCOSocketIO(nullptr)
{
  MOZ_ASSERT(mListenSocket);
  MOZ_ASSERT(mConnector);

  MOZ_COUNT_CTOR_INHERITED(ListenSocketIO, SocketIOBase);
}

ListenSocketIO::~ListenSocketIO()
{
  MOZ_ASSERT(IsConsumerThread());
  MOZ_ASSERT(IsShutdownOnConsumerThread());

  MOZ_COUNT_DTOR_INHERITED(ListenSocketIO, SocketIOBase);
}

UnixSocketConnector*
ListenSocketIO::GetConnector() const
{
  return mConnector;
}

void
ListenSocketIO::Listen(ConnectionOrientedSocketIO* aCOSocketIO)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(mConnector);
  MOZ_ASSERT(aCOSocketIO);

  struct sockaddr* address = reinterpret_cast<struct sockaddr*>(&mAddress);
  mAddressLength = sizeof(mAddress);

  if (!IsOpen()) {
    int fd;
    nsresult rv = mConnector->CreateListenSocket(address, &mAddressLength,
                                                 fd);
    if (NS_FAILED(rv)) {
      FireSocketError();
      return;
    }
    SetFd(fd);
  }

  mCOSocketIO = aCOSocketIO;

  
  nsresult rv = UnixSocketWatcher::Listen(address, mAddressLength);
  NS_WARN_IF(NS_FAILED(rv));
}

void
ListenSocketIO::OnConnected()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  NS_NOTREACHED("Invalid call to |ListenSocketIO::OnConnected|");
}

void
ListenSocketIO::OnListening()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_LISTENING);

  AddWatchers(READ_WATCHER, true);

  
  GetConsumerThread()->PostTask(
    FROM_HERE, new SocketEventTask(this, SocketEventTask::CONNECT_SUCCESS));
}

void
ListenSocketIO::OnError(const char* aFunction, int aErrno)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  UnixFdWatcher::OnError(aFunction, aErrno);
  FireSocketError();
}

void
ListenSocketIO::FireSocketError()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  
  Close();

  
  GetConsumerThread()->PostTask(
    FROM_HERE, new SocketEventTask(this, SocketEventTask::CONNECT_ERROR));
}

void
ListenSocketIO::OnSocketCanAcceptWithoutBlocking()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_LISTENING);
  MOZ_ASSERT(mCOSocketIO);

  RemoveWatchers(READ_WATCHER|WRITE_WATCHER);

  struct sockaddr_storage storage;
  socklen_t addressLength = sizeof(storage);

  int fd;
  nsresult rv = mConnector->AcceptStreamSocket(
    GetFd(),
    reinterpret_cast<struct sockaddr*>(&storage), &addressLength,
    fd);
  if (NS_FAILED(rv)) {
    FireSocketError();
    return;
  }

  mCOSocketIO->Accept(fd,
                      reinterpret_cast<struct sockaddr*>(&storage),
                      addressLength);
}



SocketBase*
ListenSocketIO::GetSocketBase()
{
  return mListenSocket;
}

bool
ListenSocketIO::IsShutdownOnConsumerThread() const
{
  MOZ_ASSERT(IsConsumerThread());

  return mListenSocket == nullptr;
}

bool
ListenSocketIO::IsShutdownOnIOThread() const
{
  return mShuttingDownOnIOThread;
}

void
ListenSocketIO::ShutdownOnConsumerThread()
{
  MOZ_ASSERT(IsConsumerThread());
  MOZ_ASSERT(!IsShutdownOnConsumerThread());

  mListenSocket = nullptr;
}

void
ListenSocketIO::ShutdownOnIOThread()
{
  MOZ_ASSERT(!IsConsumerThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  Close(); 
  mShuttingDownOnIOThread = true;
}





class ListenSocketIO::ListenTask final : public SocketIOTask<ListenSocketIO>
{
public:
  ListenTask(ListenSocketIO* aIO, ConnectionOrientedSocketIO* aCOSocketIO)
    : SocketIOTask<ListenSocketIO>(aIO)
    , mCOSocketIO(aCOSocketIO)
  {
    MOZ_ASSERT(mCOSocketIO);

    MOZ_COUNT_CTOR(ListenTask);
  }

  ~ListenTask()
  {
    MOZ_COUNT_DTOR(ListenTask);
  }

  void Run() override
  {
    MOZ_ASSERT(!GetIO()->IsConsumerThread());

    if (!IsCanceled()) {
      GetIO()->Listen(mCOSocketIO);
    }
  }

private:
  ConnectionOrientedSocketIO* mCOSocketIO;
};





ListenSocket::ListenSocket(ListenSocketConsumer* aConsumer, int aIndex)
  : mIO(nullptr)
  , mConsumer(aConsumer)
  , mIndex(aIndex)
{
  MOZ_ASSERT(mConsumer);

  MOZ_COUNT_CTOR_INHERITED(ListenSocket, SocketBase);
}

ListenSocket::~ListenSocket()
{
  MOZ_ASSERT(!mIO);

  MOZ_COUNT_DTOR_INHERITED(ListenSocket, SocketBase);
}

nsresult
ListenSocket::Listen(UnixSocketConnector* aConnector,
                     MessageLoop* aConsumerLoop,
                     MessageLoop* aIOLoop,
                     ConnectionOrientedSocket* aCOSocket)
{
  MOZ_ASSERT(!mIO);

  mIO = new ListenSocketIO(aConsumerLoop, aIOLoop, this, aConnector);

  
  nsresult rv = Listen(aCOSocket);
  if (NS_FAILED(rv)) {
    delete mIO;
    mIO = nullptr;
    return rv;
  }

  return NS_OK;
}

nsresult
ListenSocket::Listen(UnixSocketConnector* aConnector,
                     ConnectionOrientedSocket* aCOSocket)
{
  return Listen(aConnector, MessageLoop::current(), XRE_GetIOMessageLoop(),
                aCOSocket);
}

nsresult
ListenSocket::Listen(ConnectionOrientedSocket* aCOSocket)
{
  MOZ_ASSERT(aCOSocket);
  MOZ_ASSERT(mIO);

  
  

  nsAutoPtr<UnixSocketConnector> connector;
  nsresult rv = mIO->GetConnector()->Duplicate(*connector.StartAssignment());
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsAutoPtr<ConnectionOrientedSocketIO> io;
  rv = aCOSocket->PrepareAccept(connector,
                                mIO->GetConsumerThread(), mIO->GetIOLoop(),
                                *io.StartAssignment());
  if (NS_FAILED(rv)) {
    return rv;
  }
  connector.forget(); 

  

  SetConnectionStatus(SOCKET_LISTENING);

  mIO->GetIOLoop()->PostTask(
    FROM_HERE, new ListenSocketIO::ListenTask(mIO, io.forget()));

  return NS_OK;
}



void
ListenSocket::Close()
{
  if (!mIO) {
    return;
  }

  MOZ_ASSERT(mIO->IsConsumerThread());

  
  
  
  mIO->ShutdownOnConsumerThread();
  mIO->GetIOLoop()->PostTask(FROM_HERE, new SocketIOShutdownTask(mIO));
  mIO = nullptr;

  NotifyDisconnect();
}

void
ListenSocket::OnConnectSuccess()
{
  mConsumer->OnConnectSuccess(mIndex);
}

void
ListenSocket::OnConnectError()
{
  mConsumer->OnConnectError(mIndex);
}

void
ListenSocket::OnDisconnect()
{
  mConsumer->OnDisconnect(mIndex);
}

} 
} 
