





#ifndef mozilla_ipc_connectionorientedsocket_h
#define mozilla_ipc_connectionorientedsocket_h

#include <sys/socket.h>
#include "DataSocket.h"

namespace mozilla {
namespace ipc {

union sockaddr_any;







class ConnectionOrientedSocketIO : public DataSocketIO
{
public:
  virtual nsresult Accept(int aFd,
                          const union sockaddr_any* aAddr,
                          socklen_t aAddrLen) = 0;

protected:
  virtual ~ConnectionOrientedSocketIO();
};

class ConnectionOrientedSocket : public DataSocket
{
public:
  virtual ConnectionOrientedSocketIO* GetIO() = 0;

protected:
  virtual ~ConnectionOrientedSocket();
};

}
}

#endif
