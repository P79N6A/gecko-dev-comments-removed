





#include "ListenSocket.h"
#include <fcntl.h>
#include "ConnectionOrientedSocket.h"
#include "DataSocket.h"
#include "mozilla/RefPtr.h"
#include "nsXULAppAPI.h"
#include "UnixSocketConnector.h"

namespace mozilla {
namespace ipc {





class ListenSocketIO final : public UnixSocketWatcher
                           , protected SocketIOBase
{
public:
  class ListenTask;

  ListenSocketIO(MessageLoop* mIOLoop,
                 ListenSocket* aListenSocket,
                 UnixSocketConnector* aConnector,
                 const nsACString& aAddress);
  ~ListenSocketIO();

  void        GetSocketAddr(nsAString& aAddrStr) const;
  DataSocket* GetDataSocket();
  SocketBase* GetSocketBase() override;

  
  

  bool IsShutdownOnMainThread() const override;
  void ShutdownOnMainThread();

  bool IsShutdownOnIOThread() const;
  void ShutdownOnIOThread();

  
  

  


  void Listen(ConnectionOrientedSocketIO* aCOSocketIO);

  
  

  void OnAccepted(int aFd, const sockaddr_any* aAddr,
                  socklen_t aAddrLen) override;
  void OnConnected() override;
  void OnError(const char* aFunction, int aErrno) override;
  void OnListening() override;

private:
  void FireSocketError();

  
  static bool SetSocketFlags(int aFd);

  




  RefPtr<ListenSocket> mListenSocket;

  


  nsAutoPtr<UnixSocketConnector> mConnector;

  


  bool mShuttingDownOnIOThread;

  


  nsCString mAddress;

  


  socklen_t mAddrSize;

  


  sockaddr_any mAddr;

  ConnectionOrientedSocketIO* mCOSocketIO;
};

ListenSocketIO::ListenSocketIO(MessageLoop* mIOLoop,
                               ListenSocket* aListenSocket,
                               UnixSocketConnector* aConnector,
                               const nsACString& aAddress)
  : UnixSocketWatcher(mIOLoop)
  , SocketIOBase()
  , mListenSocket(aListenSocket)
  , mConnector(aConnector)
  , mShuttingDownOnIOThread(false)
  , mAddress(aAddress)
  , mCOSocketIO(nullptr)
{
  MOZ_ASSERT(mListenSocket);
  MOZ_ASSERT(mConnector);
}

ListenSocketIO::~ListenSocketIO()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(IsShutdownOnMainThread());
}

void
ListenSocketIO::GetSocketAddr(nsAString& aAddrStr) const
{
  if (!mConnector) {
    NS_WARNING("No connector to get socket address from!");
    aAddrStr.Truncate();
    return;
  }
  mConnector->GetSocketAddr(mAddr, aAddrStr);
}

DataSocket*
ListenSocketIO::GetDataSocket()
{
  MOZ_CRASH("Listen sockets cannot transfer data");

  return nullptr;
}

SocketBase*
ListenSocketIO::GetSocketBase()
{
  return mListenSocket.get();
}

bool
ListenSocketIO::IsShutdownOnMainThread() const
{
  MOZ_ASSERT(NS_IsMainThread());

  return mListenSocket == nullptr;
}

void
ListenSocketIO::ShutdownOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!IsShutdownOnMainThread());

  mListenSocket = nullptr;
}

bool
ListenSocketIO::IsShutdownOnIOThread() const
{
  return mShuttingDownOnIOThread;
}

void
ListenSocketIO::ShutdownOnIOThread()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  Close(); 
  mShuttingDownOnIOThread = true;
}

void
ListenSocketIO::Listen(ConnectionOrientedSocketIO* aCOSocketIO)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(mConnector);
  MOZ_ASSERT(aCOSocketIO);

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

  mCOSocketIO = aCOSocketIO;

  
  
  
  if (!mConnector->CreateAddr(true, mAddrSize, mAddr, nullptr)) {
    NS_WARNING("Cannot create socket address!");
    FireSocketError();
    return;
  }

  
  nsresult rv = UnixSocketWatcher::Listen(
    reinterpret_cast<struct sockaddr*>(&mAddr), mAddrSize);
  NS_WARN_IF(NS_FAILED(rv));
}

void
ListenSocketIO::OnAccepted(int aFd,
                           const sockaddr_any* aAddr,
                           socklen_t aAddrLen)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_LISTENING);
  MOZ_ASSERT(mCOSocketIO);

  RemoveWatchers(READ_WATCHER|WRITE_WATCHER);

  mCOSocketIO->Accept(aFd, aAddr, aAddrLen);
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

  if (!mConnector->SetUpListenSocket(GetFd())) {
    NS_WARNING("Could not set up listen socket!");
    FireSocketError();
    return;
  }

  AddWatchers(READ_WATCHER, true);

  
  NS_DispatchToMainThread(
    new SocketIOEventRunnable(this, SocketIOEventRunnable::CONNECT_SUCCESS));
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

  
  NS_DispatchToMainThread(
    new SocketIOEventRunnable(this, SocketIOEventRunnable::CONNECT_ERROR));
}

bool
ListenSocketIO::SetSocketFlags(int aFd)
{
  static const int reuseaddr = 1;

  
  int res = setsockopt(aFd, SOL_SOCKET, SO_REUSEADDR,
                       &reuseaddr, sizeof(reuseaddr));
  if (res < 0) {
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





class ListenSocketIO::ListenTask final
  : public SocketIOTask<ListenSocketIO>
{
public:
  ListenTask(ListenSocketIO* aIO, ConnectionOrientedSocketIO* aCOSocketIO)
  : SocketIOTask<ListenSocketIO>(aIO)
  , mCOSocketIO(aCOSocketIO)
  {
    MOZ_ASSERT(mCOSocketIO);
  }

  void Run() override
  {
    MOZ_ASSERT(!NS_IsMainThread());

    if (!IsCanceled()) {
      GetIO()->Listen(mCOSocketIO);
    }
  }

private:
  ConnectionOrientedSocketIO* mCOSocketIO;
};





ListenSocket::ListenSocket()
: mIO(nullptr)
{ }

ListenSocket::~ListenSocket()
{
  MOZ_ASSERT(!mIO);
}

void
ListenSocket::Close()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mIO) {
    return;
  }

  
  
  
  mIO->ShutdownOnMainThread();

  XRE_GetIOMessageLoop()->PostTask(
    FROM_HERE, new SocketIOShutdownTask<ListenSocketIO>(mIO));

  mIO = nullptr;

  NotifyDisconnect();
}

bool
ListenSocket::Listen(UnixSocketConnector* aConnector,
                     ConnectionOrientedSocket* aCOSocket)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aConnector);
  MOZ_ASSERT(aCOSocket);

  nsAutoPtr<UnixSocketConnector> connector(aConnector);

  if (mIO) {
    NS_WARNING("Socket already connecting/connected!");
    return false;
  }

  mIO = new ListenSocketIO(
    XRE_GetIOMessageLoop(), this, connector.forget(), EmptyCString());

  
  return Listen(aCOSocket);
}

bool
ListenSocket::Listen(ConnectionOrientedSocket* aCOSocket)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mIO);
  MOZ_ASSERT(aCOSocket);

  SetConnectionStatus(SOCKET_LISTENING);

  XRE_GetIOMessageLoop()->PostTask(
    FROM_HERE, new ListenSocketIO::ListenTask(mIO, aCOSocket->GetIO()));

  return true;
}

void
ListenSocket::GetSocketAddr(nsAString& aAddrStr)
{
  aAddrStr.Truncate();
  if (!mIO || GetConnectionStatus() != SOCKET_CONNECTED) {
    NS_WARNING("No socket currently open!");
    return;
  }
  mIO->GetSocketAddr(aAddrStr);
}

} 
} 
