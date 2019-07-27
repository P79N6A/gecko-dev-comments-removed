





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





ConnectionOrientedSocket::~ConnectionOrientedSocket()
{ }

}
}
