





#include "ConnectionOrientedSocket.h"

namespace mozilla {
namespace ipc {





ConnectionOrientedSocketIO::ConnectionOrientedSocketIO(
  nsIThread* aConsumerThread,
  MessageLoop* aIOLoop,
  int aFd,
  ConnectionStatus aConnectionStatus)
  : DataSocketIO(aConsumerThread)
  , UnixSocketWatcher(aIOLoop, aFd, aConnectionStatus)
{ }

ConnectionOrientedSocketIO::ConnectionOrientedSocketIO(
  nsIThread* aConsumerThread,
  MessageLoop* aIOLoop)
  : DataSocketIO(aConsumerThread)
  , UnixSocketWatcher(aIOLoop)
{ }

ConnectionOrientedSocketIO::~ConnectionOrientedSocketIO()
{ }

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

  GetConsumerThread()->Dispatch(
    new SocketIOEventRunnable(this, SocketIOEventRunnable::CONNECT_SUCCESS),
    NS_DISPATCH_NORMAL);

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

  
  GetConsumerThread()->Dispatch(
    new SocketIOEventRunnable(this, SocketIOEventRunnable::CONNECT_ERROR),
    NS_DISPATCH_NORMAL);
}





ConnectionOrientedSocket::~ConnectionOrientedSocket()
{ }

}
}
