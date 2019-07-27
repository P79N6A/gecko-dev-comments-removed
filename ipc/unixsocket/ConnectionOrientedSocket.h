





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

  nsresult Accept(int aFd,
                  const struct sockaddr* aAddress,
                  socklen_t aAddressLength);

  nsresult Connect();

  void Send(UnixSocketIOBuffer* aBuffer);

  
  

  void OnSocketCanReceiveWithoutBlocking() final;
  void OnSocketCanSendWithoutBlocking() final;

  void OnListening() final;
  void OnConnected() final;
  void OnError(const char* aFunction, int aErrno) final;

protected:
  








  ConnectionOrientedSocketIO(MessageLoop* aConsumerLoop,
                             MessageLoop* aIOLoop,
                             int aFd, ConnectionStatus aConnectionStatus,
                             UnixSocketConnector* aConnector);

  






  ConnectionOrientedSocketIO(MessageLoop* aConsumerLoop,
                             MessageLoop* aIOLoop,
                             UnixSocketConnector* aConnector);

private:
  


  nsAutoPtr<UnixSocketConnector> mConnector;

  


  socklen_t mPeerAddressLength;

  


  struct sockaddr_storage mPeerAddress;
};

class ConnectionOrientedSocket : public DataSocket
{
public:
  










  virtual nsresult PrepareAccept(UnixSocketConnector* aConnector,
                                 MessageLoop* aConsumerLoop,
                                 MessageLoop* aIOLoop,
                                 ConnectionOrientedSocketIO*& aIO) = 0;

protected:
  virtual ~ConnectionOrientedSocket();
};

}
}

#endif
