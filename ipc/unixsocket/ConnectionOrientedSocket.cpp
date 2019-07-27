





#include "ConnectionOrientedSocket.h"

namespace mozilla {
namespace ipc {





ConnectionOrientedSocketIO::ConnectionOrientedSocketIO(
  nsIThread* aConsumerThread)
  : DataSocketIO(aConsumerThread)
{ }

ConnectionOrientedSocketIO::~ConnectionOrientedSocketIO()
{ }





ConnectionOrientedSocket::~ConnectionOrientedSocket()
{ }

}
}
