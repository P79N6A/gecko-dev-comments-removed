





#ifndef mozilla_ipc_connectionorientedsocket_h
#define mozilla_ipc_connectionorientedsocket_h

#include <sys/socket.h>
#include "DataSocket.h"
#include "mozilla/ipc/UnixSocketWatcher.h"

class MessageLoop;

namespace mozilla {
namespace ipc {

class UnixSocketConnector;







class ConnectionOrientedSocketIO
  : public DataSocketIO
  , public UnixSocketWatcher
{
public:
  virtual ~ConnectionOrientedSocketIO();

  virtual nsresult Accept(int aFd,
                          const struct sockaddr* aAddress,
                          socklen_t aAddressLength) = 0;

  void Send(UnixSocketIOBuffer* aBuffer);

  
  

  void OnSocketCanReceiveWithoutBlocking() final;
  void OnSocketCanSendWithoutBlocking() final;

  void OnListening() final;
  void OnConnected() final;
  void OnError(const char* aFunction, int aErrno) final;

protected:
  







  ConnectionOrientedSocketIO(nsIThread* aConsumerThread,
                             MessageLoop* aIOLoop,
                             int aFd, ConnectionStatus aConnectionStatus);

  





  ConnectionOrientedSocketIO(nsIThread* aConsumerThread,
                             MessageLoop* aIOLoop);
};

class ConnectionOrientedSocket : public DataSocket
{
public:
  










  virtual nsresult PrepareAccept(UnixSocketConnector* aConnector,
                                 nsIThread* aConsumerThread,
                                 MessageLoop* aIOLoop,
                                 ConnectionOrientedSocketIO*& aIO) = 0;

protected:
  virtual ~ConnectionOrientedSocket();
};

}
}

#endif
