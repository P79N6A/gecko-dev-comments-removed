





#include "ConnectionOrientedSocket.h"
#include "UnixSocketConnector.h"

namespace mozilla {
namespace ipc {





ConnectionOrientedSocketIO::ConnectionOrientedSocketIO(
  MessageLoop* aConsumerLoop,
  MessageLoop* aIOLoop,
  int aFd, ConnectionStatus aConnectionStatus,
  UnixSocketConnector* aConnector)
  : DataSocketIO(aConsumerLoop)
  , UnixSocketWatcher(aIOLoop, aFd, aConnectionStatus)
  , mConnector(aConnector)
  , mPeerAddressLength(0)
{
  MOZ_ASSERT(mConnector);
}

ConnectionOrientedSocketIO::ConnectionOrientedSocketIO(
  MessageLoop* aConsumerLoop,
  MessageLoop* aIOLoop,
  UnixSocketConnector* aConnector)
  : DataSocketIO(aConsumerLoop)
  , UnixSocketWatcher(aIOLoop)
  , mConnector(aConnector)
  , mPeerAddressLength(0)
{
  MOZ_ASSERT(mConnector);
}

ConnectionOrientedSocketIO::~ConnectionOrientedSocketIO()
{ }

nsresult
ConnectionOrientedSocketIO::Accept(int aFd,
                                   const struct sockaddr* aPeerAddress,
                                   socklen_t aPeerAddressLength)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_CONNECTING);

  SetSocket(aFd, SOCKET_IS_CONNECTED);

  
  mPeerAddressLength = aPeerAddressLength;
  memcpy(&mPeerAddress, aPeerAddress, mPeerAddressLength);

  
  OnConnected();

  return NS_OK;
}

nsresult
ConnectionOrientedSocketIO::Connect()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(!IsOpen());

  struct sockaddr* peerAddress =
    reinterpret_cast<struct sockaddr*>(&mPeerAddress);
  mPeerAddressLength = sizeof(mPeerAddress);

  int fd;
  nsresult rv = mConnector->CreateStreamSocket(peerAddress,
                                               &mPeerAddressLength,
                                               fd);
  if (NS_FAILED(rv)) {
    
    GetConsumerThread()->PostTask(
      FROM_HERE, new SocketEventTask(this, SocketEventTask::CONNECT_ERROR));
    return NS_ERROR_FAILURE;
  }

  SetFd(fd);

  
  rv = UnixSocketWatcher::Connect(peerAddress, mPeerAddressLength);

  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

void
ConnectionOrientedSocketIO::Send(UnixSocketIOBuffer* aBuffer)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  EnqueueData(aBuffer);
  AddWatchers(WRITE_WATCHER, false);
}



void
ConnectionOrientedSocketIO::OnSocketCanReceiveWithoutBlocking()
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
ConnectionOrientedSocketIO::OnSocketCanSendWithoutBlocking()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(GetConnectionStatus() == SOCKET_IS_CONNECTED); 
  MOZ_ASSERT(!IsShutdownOnIOThread());

  nsresult rv = SendPendingData(GetFd());
  if (NS_FAILED(rv)) {
    return;
  }

  if (HasPendingData()) {
    AddWatchers(WRITE_WATCHER, false);
  }
}

void
ConnectionOrientedSocketIO::OnConnected()
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
ConnectionOrientedSocketIO::OnListening()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  NS_NOTREACHED("Invalid call to |ConnectionOrientedSocketIO::OnListening|");
}

void
ConnectionOrientedSocketIO::OnError(const char* aFunction, int aErrno)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  UnixFdWatcher::OnError(aFunction, aErrno);

  
  Close();

  
  GetConsumerThread()->PostTask(
    FROM_HERE, new SocketEventTask(this, SocketEventTask::CONNECT_ERROR));
}





ConnectionOrientedSocket::~ConnectionOrientedSocket()
{ }

}
}
