





#include <fcntl.h>
#include "UnixSocketWatcher.h"

namespace mozilla {
namespace ipc {

UnixSocketWatcher::~UnixSocketWatcher()
{
}

void UnixSocketWatcher::Close()
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  mConnectionStatus = SOCKET_IS_DISCONNECTED;
  UnixFdWatcher::Close();
}

nsresult
UnixSocketWatcher::Connect(const struct sockaddr* aAddr, socklen_t aAddrLen)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(IsOpen());
  MOZ_ASSERT(aAddr || !aAddrLen);

  if (TEMP_FAILURE_RETRY(connect(GetFd(), aAddr, aAddrLen) < 0)) {
    if (errno == EINPROGRESS) {
      mConnectionStatus = SOCKET_IS_CONNECTING;
      
      AddWatchers(WRITE_WATCHER, false);
      return NS_OK;
    }
    OnError("connect", errno);
    return NS_ERROR_FAILURE;
  }

  mConnectionStatus = SOCKET_IS_CONNECTED;
  OnConnected();

  return NS_OK;
}

nsresult
UnixSocketWatcher::Listen(const struct sockaddr* aAddr, socklen_t aAddrLen)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(IsOpen());
  MOZ_ASSERT(aAddr || !aAddrLen);

  if (mConnectionStatus == SOCKET_IS_DISCONNECTED) {
    
    if (bind(GetFd(), aAddr, aAddrLen) < 0) {
      OnError("bind", errno);
      return NS_ERROR_FAILURE;
    }
    if (listen(GetFd(), 1) < 0) {
      OnError("listen", errno);
      return NS_ERROR_FAILURE;
    }
  }
  mConnectionStatus = SOCKET_IS_LISTENING;
  OnListening();

  return NS_OK;
}

UnixSocketWatcher::UnixSocketWatcher(MessageLoop* aIOLoop)
: UnixFdWatcher(aIOLoop)
, mConnectionStatus(SOCKET_IS_DISCONNECTED)
{
}

UnixSocketWatcher::UnixSocketWatcher(MessageLoop* aIOLoop, int aFd,
                                     ConnectionStatus aConnectionStatus)
: UnixFdWatcher(aIOLoop, aFd)
, mConnectionStatus(aConnectionStatus)
{
}

void
UnixSocketWatcher::SetSocket(int aFd, ConnectionStatus aConnectionStatus)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());

  SetFd(aFd);
  mConnectionStatus = aConnectionStatus;
}

void
UnixSocketWatcher::OnFileCanReadWithoutBlocking(int aFd)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(aFd == GetFd());

  if (mConnectionStatus == SOCKET_IS_CONNECTED) {
    OnSocketCanReceiveWithoutBlocking();
  } else if (mConnectionStatus == SOCKET_IS_LISTENING) {
    OnSocketCanAcceptWithoutBlocking();
  } else {
    NS_NOTREACHED("invalid connection state for reading");
  }
}

void
UnixSocketWatcher::OnFileCanWriteWithoutBlocking(int aFd)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(aFd == GetFd());

  if (mConnectionStatus == SOCKET_IS_CONNECTED) {
    OnSocketCanSendWithoutBlocking();
  } else if (mConnectionStatus == SOCKET_IS_CONNECTING) {
    RemoveWatchers(WRITE_WATCHER);
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(GetFd(), SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
      OnError("getsockopt", errno);
    } else if (error) {
      OnError("connect", error);
    } else {
      mConnectionStatus = SOCKET_IS_CONNECTED;
      OnConnected();
    }
  } else {
    NS_NOTREACHED("invalid connection state for writing");
  }
}

}
}
