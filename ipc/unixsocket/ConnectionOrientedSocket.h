





#ifndef mozilla_ipc_connectionorientedsocket_h
#define mozilla_ipc_connectionorientedsocket_h

#include <sys/socket.h>
#include "DataSocket.h"

namespace mozilla {
namespace ipc {

class UnixSocketConnector;







class ConnectionOrientedSocketIO : public DataSocketIO
{
public:
  virtual ~ConnectionOrientedSocketIO();

  virtual nsresult Accept(int aFd,
                          const struct sockaddr* aAddress,
                          socklen_t aAddressLength) = 0;
};

class ConnectionOrientedSocket : public DataSocket
{
public:
  








  virtual nsresult PrepareAccept(UnixSocketConnector* aConnector,
                                 ConnectionOrientedSocketIO*& aIO) = 0;

protected:
  virtual ~ConnectionOrientedSocket();
};

}
}

#endif
